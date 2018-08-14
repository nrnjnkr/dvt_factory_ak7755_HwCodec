/**
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __SMCCC_H__
#define __SMCCC_H__

#if defined(CONFIG_AARCH64_TRUSTZONE)
/*
 * SMC Calling Convention definition per ARM_DEN0028B_SMC_Calling_Convention.pdf
 *
 * Terms:
 *    SMCCC: SMC Calling Convention
 *    OEN: Owning Entity Number
 *    SIP: Silicib Partner
 *    SVC: Service
 */

#define SMCCC_TYPE_FAST			(1)
#define SMCCC_TYPE_YIELD		(0)
#define SMCCC_FUNCID_TYPE_SHIFT		(31)
#define SMCCC_FUNCID_TYPE_MASK		(0x1)

#define SMCCC_64			(1)
#define SMCCC_32			(0)
#define SMCCC_FUNCID_CC_SHIFT		(30)
#define SMCCC_FUNCID_CC_MASK		(0x1)

#define SMCCC_OEN_SIP			(2)
#define SMCCC_FUNCID_OEN_SHIFT		(24)
#define SMCCC_FUNCID_OEN_MASK		(0x3f)

#define SMCCC_FUNCID_NUM_SHIFT		(0)
#define SMCCC_FUNCID_NUM_MASK		(0xffff)

#define SMCCC_CALL_VAL(type, smc64, oen, func_num) \
	((((type) & SMCCC_FUNCID_TYPE_MASK) << SMCCC_FUNCID_TYPE_SHIFT) | \
	(((smc64) & SMCCC_FUNCID_CC_MASK) << SMCCC_FUNCID_CC_SHIFT) | \
	(((oen) & SMCCC_FUNCID_OEN_MASK) << SMCCC_FUNCID_OEN_SHIFT) | \
	(((func_num) & SMCCC_FUNCID_NUM_MASK) << SMCCC_FUNCID_NUM_SHIFT))

#define SMC_OK				(0)
#define SMC_UNK				(0xffffffff)


/* Ambarella Private Definition */

#define AMBA_SIP_ACCESS_REG		0x3
#define AMBA_SIP_ACCESS_REG_READ	0x1
#define AMBA_SIP_ACCESS_REG_WRITE	0x2
#define AMBA_SIP_ACCESS_REG_SETBIT	0x3
#define AMBA_SIP_ACCESS_REG_CLRBIT	0x4

#define	SVC_SMCCC_FN(s, f)		((((s) & 0xff) << 8) | ((f) & 0xff))

#define SMCCC_SIP_CALL(s, f)		SMCCC_CALL_VAL(		\
					SMCCC_TYPE_FAST,	\
					SMCCC_64,		\
					SMCCC_OEN_SIP,		\
					SVC_SMCCC_FN(s, f))

struct smccc_result {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
};

extern void __arm_smccc_smc(unsigned long a0, unsigned long a1,
			unsigned long a2, unsigned long a3, unsigned long a4,
			unsigned long a5, unsigned long a6, unsigned long a7,
			struct smccc_result *res);

static inline u32 smc_readl(uintptr_t addr)
{
	struct smccc_result res;
	u32 el, func;

	__asm__ __volatile__("mrs %0, CurrentEL" : "=r" (el));
	if (el == 0xc) { /* EL3 */
		return readl(addr);
	} else {
		func = SMCCC_SIP_CALL(AMBA_SIP_ACCESS_REG, AMBA_SIP_ACCESS_REG_READ);
		__arm_smccc_smc(func, addr, 0, 0, 0, 0, 0, 0, &res);

		return res.a0;
	}
}

static inline void smc_writel(uintptr_t addr, u32 val)
{
	u32 el, func;

	__asm__ __volatile__("mrs %0, CurrentEL" : "=r" (el));
	if (el == 0xc) { /* EL3 */
		writel(addr, val);
	} else{
		func = SMCCC_SIP_CALL(AMBA_SIP_ACCESS_REG, AMBA_SIP_ACCESS_REG_WRITE);
		__arm_smccc_smc(func, addr, val, 0, 0, 0, 0, 0, NULL);
	}
}

static inline void smc_setbitsl(uintptr_t addr, u32 val)
{
	u32 el, func;

	__asm__ __volatile__("mrs %0, CurrentEL" : "=r" (el));
	if (el == 0xc) { /* EL3 */
		setbitsl(addr, val);
	} else {
		func = SMCCC_SIP_CALL(AMBA_SIP_ACCESS_REG, AMBA_SIP_ACCESS_REG_SETBIT);
		__arm_smccc_smc(func, addr, val, 0, 0, 0, 0, 0, NULL);
	}
}

static inline void smc_clrbitsl(uintptr_t addr, u32 val)
{
	u32 el, func;

	__asm__ __volatile__("mrs %0, CurrentEL" : "=r" (el));
	if (el == 0xc) { /* EL3 */
		clrbitsl(addr, val);
	} else {
		func = SMCCC_SIP_CALL(AMBA_SIP_ACCESS_REG, AMBA_SIP_ACCESS_REG_CLRBIT);
		__arm_smccc_smc(func, addr, val, 0, 0, 0, 0, 0, NULL);
	}
}
#else
static inline u32 smc_readl(uintptr_t addr) {return readl(addr);}
static inline void smc_writel(uintptr_t addr, u32 val) {writel(addr, val);}
static inline void smc_setbitsl(uintptr_t addr, u32 val) {setbitsl(addr, val);}
static inline void smc_clrbitsl(uintptr_t addr, u32 val) {clrbitsl(addr, val);}
#endif

#endif /* __SMCC_H__ */
