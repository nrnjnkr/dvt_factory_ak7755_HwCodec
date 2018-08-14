/*******************************************************************************
 * am_signal.cpp
 *
 * History:
 *   Jun 15, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include <execinfo.h>
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
  for (ucontext_t *uc = (ucontext_t*)ucontext;
       uc != nullptr;
       uc = uc->uc_link) {
    char **strings       = nullptr;
    void *callstack[128] = {0};
    size_t size          = 0;
    const int max_frames = sizeof(callstack) / sizeof(callstack[0]);
#if defined(__aarch64__)
    void *caller_addr = (void*)uc->uc_mcontext.pc;
#else
    void *caller_addr = (void*)uc->uc_mcontext.arm_pc;
#endif

    ERROR("Caught signal \"%s(%d)\", accessing address(%p) from %p!",
          strsignal(sig_num),
          sig_num,
          info->si_addr,
          caller_addr);

    size = backtrace(callstack, max_frames);
    callstack[1] = caller_addr; /* overwrite sigaction with caller's address */
    if (AM_LIKELY(size < max_frames)) {
      /* Add return address */
#if defined(__aarch64__)
      callstack[size++] = (void*)uc->uc_mcontext.regs[30];
#else
      callstack[size++] = (void*)uc->uc_mcontext.arm_lr;
#endif
    }
    strings = backtrace_symbols(callstack, size);
    if (strings) {
      for (size_t i = 1; i < size; ++ i) {
        Dl_info info;
        int status = 0;
        char *demangled = nullptr;
        if (AM_LIKELY(dladdr(callstack[i], &info))) {
          demangled = abi::__cxa_demangle(info.dli_sname,
                                          nullptr,
                                          0,
                                          &status);
          ERROR("#%-2d [load: 0x%0*zx][func: 0x%0*zx] %s in file %s: %s",
                i,
                sizeof(size_t) * 2,
                (size_t)info.dli_fbase,
                sizeof(size_t) * 2,
#if defined(__aarch64__)
                (size_t)ROUND_DOWN((uintptr_t)info.dli_saddr, 8),
#elif defined(__thumb__)
                (size_t)ROUND_DOWN((uintptr_t)info.dli_saddr, 2),
#else
                (size_t)ROUND_DOWN((uintptr_t)info.dli_saddr, 4),
#endif
                (0 == status) ? demangled : info.dli_sname,
                info.dli_fname,
                strings[i]);
        } else {
          demangled = abi::__cxa_demangle(strings[i], nullptr, 0, &status);
          ERROR("#%-2d %s", i, (0 == status) ? demangled : strings[i]);
        }
        if (demangled) {
          free(demangled);
        }
      }
      free(strings);
    }
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
