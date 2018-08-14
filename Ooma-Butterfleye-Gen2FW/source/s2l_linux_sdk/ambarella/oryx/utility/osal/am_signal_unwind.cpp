/*******************************************************************************
 * am_signal_unwind.cpp
 *
 * History:
 *   Jun 21, 2017 - [ypchang] created file
 *
 * Copyright (c) 2017 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_signal.h"
#include <libunwind.h>
#include <ucontext.h>
#include <cxxabi.h>
#include <dlfcn.h>

static uint32_t signum[] =
{
  SIGILL,  /* Illegal instruction (ANSI).  */
  SIGABRT, /* Abort (ANSI).  */
  SIGBUS,  /* BUS error (4.2 BSD). (unaligned access) */
  SIGFPE,  /* Floating-point exception (ANSI).  */
  SIGSEGV, /* Segmentation violation (ANSI).  */
};

static void critical_error_handler(int sig_num, siginfo_t *info, void *ucontext)
{
  uint32_t count = 0;
  unw_cursor_t cursor;
  unw_context_t context;

  ucontext_t *uc = (ucontext_t*)ucontext;
  int step_ret = 0;
#if defined(__aarch64__)
  void *caller_addr = (void*)uc->uc_mcontext.pc;
  void *arm_sp = (void*)uc->uc_mcontext.sp;
#else
  void *caller_addr = (void*)uc->uc_mcontext.arm_pc;
  void *arm_sp = (void*)uc->uc_mcontext.arm_sp;
#endif

  ERROR("Caught signal \"%s(%d)\", accessing address(%0*p) from %0*p!",
        strsignal(sig_num),
        sig_num,
        sizeof(info->si_addr) * 2,
        info->si_addr,
        sizeof(caller_addr) * 2,
        caller_addr);
  ERROR("Register dump:");
#if defined(__aarch64__)
  for (uint32_t i = 0; i < sizeof(uc->uc_mcontext.regs) / sizeof(__u64); ++ i) {
    ERROR("Reg[%2u]: 0x%0*jx", i, sizeof(__u64) * 2, uc->uc_mcontext.regs[i]);
  }
#else
  const char *reg_names[] = {
      "    trap_no  ",
      " error_code  ",
      "    oldmask  ",
      "     arm_r0  ",
      "     arm_r1  ",
      "     arm_r2  ",
      "     arm_r3  ",
      "     arm_r4  ",
      "     arm_r5  ",
      "     arm_r6  ",
      "     arm_r7  ",
      "     arm_r8  ",
      "     arm_r9  ",
      "     arm_r10 ",
      "     arm_fp  ",
      "     arm_ip  ",
      "     arm_sp  ",
      "     arm_lr  ",
      "     arm_pc  ",
      "     arm_cpsr",
      "fault_address",
  };
  unsigned long *regs = (unsigned long*)&uc->uc_mcontext;
  uint32_t size = sizeof(uc->uc_mcontext) / sizeof(unsigned long);
  for (uint32_t i = 0; i < size; ++ i) {
    ERROR("%s: 0x%08x", reg_names[i], regs[i]);
  }
#endif

  unw_getcontext(&context);
  unw_init_local(&cursor, &context);

  while ((step_ret = unw_step(&cursor)) > 0) {
    unw_word_t ip;
    unw_word_t sp;
    unw_word_t off;
    char symbol[256] = {"<unknown>"};
    char *name = symbol;
    char *demangled = nullptr;

    unw_get_reg(&cursor, UNW_REG_IP, &ip);
    unw_get_reg(&cursor, UNW_REG_SP, &sp);

    if (AM_LIKELY(!unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off))) {
      int status;
      demangled = abi::__cxa_demangle(symbol, nullptr, 0, &status);
      name = (status == 0) ? demangled : symbol;
    }
    if (AM_UNLIKELY(1 == count)) {
      Dl_info dlinfo;
      int status = 0;
      if (AM_LIKELY(dladdr(caller_addr, &dlinfo))) {
        char *func = abi::__cxa_demangle(dlinfo.dli_sname, nullptr, 0, &status);
        ERROR("#%-2d: [Function: 0x%0*zx] sp=0x%0*zx %s !!!! BAD Address %p",
              count,
              sizeof(size_t) * 2,
              (size_t)caller_addr,
              sizeof(size_t) * 2,
              (size_t)arm_sp,
              (status == 0) ? func : dlinfo.dli_sname,
              info->si_addr);
        if (AM_LIKELY(func)) {
          free(func);
        }
        ++ count;
      }
    }
    ERROR("#%-2d: [Function: 0x%0*zx] sp=0x%0*zx %s + 0x%zx%s",
          count,
          sizeof(size_t) * 2,
          static_cast<size_t>(ip),
          sizeof(size_t) * 2,
          static_cast<size_t>(sp),
          name,
          static_cast<size_t>(off),
          (count == 1) ? " <<<<<<<<<< ERROR OCCURRED HERE!!!" : "");
    ++ count;
    if (AM_LIKELY(demangled)) {
      free(demangled);
    }
  }
  if (AM_UNLIKELY(step_ret < 0)) {
    ERROR("unw_step error: %s", unw_strerror(step_ret));
  }
  exit(EXIT_FAILURE);
}

bool register_critical_error_signal_handler()
{
  struct sigaction sigact;
  sigact.sa_sigaction = critical_error_handler;
  sigact.sa_flags = SA_RESTART | SA_SIGINFO;

  bool ret = true;

  for (uint32_t i = 0; i < (sizeof(signum) / sizeof(uint32_t)); ++ i) {
    if (AM_UNLIKELY(sigaction(signum[i], &sigact, nullptr) != 0)) {
      ERROR("Failed to set signal handler for %s(%d)!",
            strsignal(signum[i]),
            signum[i]);
      ret = false;
      break;
    }
  }

  return ret;
}

