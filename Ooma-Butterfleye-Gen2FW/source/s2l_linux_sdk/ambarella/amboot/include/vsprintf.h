/**
 * include/vsprintf.h
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 */
#ifndef __AMBA_VSPRINTF__
#define __AMBA_VSPRINTF__

#define __COL_RED	"\033[0;31m"
#define __COL_GREEN	"\033[0;32m"
#define __COL_YELLOW	"\033[0;33m"
#define __COL_BLUE	"\033[0;34m"
#define __COL_PURPLE	"\033[0;35m"
#define __COL_END__	"\033[0;39m"

# define do_div(n,base) ({				\
	int __base = (base);			\
	int __rem;					\
	(void)(((typeof((n)) *)0) == ((unsigned long long *)0));	\
	if (((n) >> 32) == 0) {			\
		__rem = (int)(n) % __base;		\
		(n) = (int)(n) / __base;		\
	} else						\
		__rem = __div64_32(&(n), __base);	\
	__rem;						\
 })


int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);
#define pr_color(c, fmt, ...)	printf(__COL_##c fmt __COL_END__, ##__VA_ARGS__)


#endif /* __AMBA_VSPRINTF__ */
