/*******************************************************************************
 * big_number_asm.h
 *
 * History:
 *  2015/06/25 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __BIG_NUMBER_ASM_H__
#define __BIG_NUMBER_ASM_H__

#ifndef D_DISABLE_ASM

#if defined(__GNUC__)
#if defined(__i386__)

#define MULADDC_INIT                        \
    asm(                                    \
        "movl   %%ebx, %0           \n\t"   \
        "movl   %5, %%esi           \n\t"   \
        "movl   %6, %%edi           \n\t"   \
        "movl   %7, %%ecx           \n\t"   \
        "movl   %8, %%ebx           \n\t"

#define MULADDC_CORE                        \
    "lodsl                      \n\t"   \
    "mull   %%ebx               \n\t"   \
    "addl   %%ecx,   %%eax      \n\t"   \
    "adcl   $0,      %%edx      \n\t"   \
    "addl   (%%edi), %%eax      \n\t"   \
    "adcl   $0,      %%edx      \n\t"   \
    "movl   %%edx,   %%ecx      \n\t"   \
    "stosl                      \n\t"

#if defined(HAVE_SSE2)

#define MULADDC_HUIT                            \
    "movd     %%ecx,     %%mm1      \n\t"   \
    "movd     %%ebx,     %%mm0      \n\t"   \
    "movd     (%%edi),   %%mm3      \n\t"   \
    "paddq    %%mm3,     %%mm1      \n\t"   \
    "movd     (%%esi),   %%mm2      \n\t"   \
    "pmuludq  %%mm0,     %%mm2      \n\t"   \
    "movd     4(%%esi),  %%mm4      \n\t"   \
    "pmuludq  %%mm0,     %%mm4      \n\t"   \
    "movd     8(%%esi),  %%mm6      \n\t"   \
    "pmuludq  %%mm0,     %%mm6      \n\t"   \
    "movd     12(%%esi), %%mm7      \n\t"   \
    "pmuludq  %%mm0,     %%mm7      \n\t"   \
    "paddq    %%mm2,     %%mm1      \n\t"   \
    "movd     4(%%edi),  %%mm3      \n\t"   \
    "paddq    %%mm4,     %%mm3      \n\t"   \
    "movd     8(%%edi),  %%mm5      \n\t"   \
    "paddq    %%mm6,     %%mm5      \n\t"   \
    "movd     12(%%edi), %%mm4      \n\t"   \
    "paddq    %%mm4,     %%mm7      \n\t"   \
    "movd     %%mm1,     (%%edi)    \n\t"   \
    "movd     16(%%esi), %%mm2      \n\t"   \
    "pmuludq  %%mm0,     %%mm2      \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "movd     20(%%esi), %%mm4      \n\t"   \
    "pmuludq  %%mm0,     %%mm4      \n\t"   \
    "paddq    %%mm3,     %%mm1      \n\t"   \
    "movd     24(%%esi), %%mm6      \n\t"   \
    "pmuludq  %%mm0,     %%mm6      \n\t"   \
    "movd     %%mm1,     4(%%edi)   \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "movd     28(%%esi), %%mm3      \n\t"   \
    "pmuludq  %%mm0,     %%mm3      \n\t"   \
    "paddq    %%mm5,     %%mm1      \n\t"   \
    "movd     16(%%edi), %%mm5      \n\t"   \
    "paddq    %%mm5,     %%mm2      \n\t"   \
    "movd     %%mm1,     8(%%edi)   \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "paddq    %%mm7,     %%mm1      \n\t"   \
    "movd     20(%%edi), %%mm5      \n\t"   \
    "paddq    %%mm5,     %%mm4      \n\t"   \
    "movd     %%mm1,     12(%%edi)  \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "paddq    %%mm2,     %%mm1      \n\t"   \
    "movd     24(%%edi), %%mm5      \n\t"   \
    "paddq    %%mm5,     %%mm6      \n\t"   \
    "movd     %%mm1,     16(%%edi)  \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "paddq    %%mm4,     %%mm1      \n\t"   \
    "movd     28(%%edi), %%mm5      \n\t"   \
    "paddq    %%mm5,     %%mm3      \n\t"   \
    "movd     %%mm1,     20(%%edi)  \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "paddq    %%mm6,     %%mm1      \n\t"   \
    "movd     %%mm1,     24(%%edi)  \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "paddq    %%mm3,     %%mm1      \n\t"   \
    "movd     %%mm1,     28(%%edi)  \n\t"   \
    "addl     $32,       %%edi      \n\t"   \
    "addl     $32,       %%esi      \n\t"   \
    "psrlq    $32,       %%mm1      \n\t"   \
    "movd     %%mm1,     %%ecx      \n\t"

#define MULADDC_STOP                    \
    "emms                   \n\t"   \
    "movl   %4, %%ebx       \n\t"   \
    "movl   %%ecx, %1       \n\t"   \
    "movl   %%edi, %2       \n\t"   \
    "movl   %%esi, %3       \n\t"   \
    : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ecx", "edx", "esi", "edi"             \
        );

#else

#define MULADDC_STOP                    \
    "movl   %4, %%ebx       \n\t"   \
    "movl   %%ecx, %1       \n\t"   \
    "movl   %%edi, %2       \n\t"   \
    "movl   %%esi, %3       \n\t"   \
    : "=m" (t), "=m" (c), "=m" (d), "=m" (s)        \
        : "m" (t), "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "eax", "ecx", "edx", "esi", "edi"             \
        );
#endif /* SSE2 */
#endif /* i386 */

#if defined(__amd64__) || defined (__x86_64__)

#define MULADDC_INIT                        \
    asm(                                    \
        "movq   %3, %%rsi           \n\t"   \
        "movq   %4, %%rdi           \n\t"   \
        "movq   %5, %%rcx           \n\t"   \
        "movq   %6, %%rbx           \n\t"   \
        "xorq   %%r8, %%r8          \n\t"

#define MULADDC_CORE                        \
    "movq   (%%rsi), %%rax      \n\t"   \
    "mulq   %%rbx               \n\t"   \
    "addq   $8,      %%rsi      \n\t"   \
    "addq   %%rcx,   %%rax      \n\t"   \
    "movq   %%r8,    %%rcx      \n\t"   \
    "adcq   $0,      %%rdx      \n\t"   \
    "nop                        \n\t"   \
    "addq   %%rax,   (%%rdi)    \n\t"   \
    "adcq   %%rdx,   %%rcx      \n\t"   \
    "addq   $8,      %%rdi      \n\t"

#define MULADDC_STOP                        \
    "movq   %%rcx, %0           \n\t"   \
    "movq   %%rdi, %1           \n\t"   \
    "movq   %%rsi, %2           \n\t"   \
    : "=m" (c), "=m" (d), "=m" (s)                      \
        : "m" (s), "m" (d), "m" (c), "m" (b)                \
        : "rax", "rcx", "rdx", "rbx", "rsi", "rdi", "r8"    \
        );

#endif /* AMD64 */

#if defined(__arm__)

#if defined(__thumb__) && !defined(__thumb2__)

#define MULADDC_INIT                                    \
    asm(                                                \
        "ldr    r0, %3                      \n\t"   \
        "ldr    r1, %4                      \n\t"   \
        "ldr    r2, %5                      \n\t"   \
        "ldr    r3, %6                      \n\t"   \
        "lsr    r10, r3, #16                 \n\t"   \
        "mov    r9, r10                      \n\t"   \
        "lsl    r10, r3, #16                 \n\t"   \
        "lsr    r10, r10, #16                 \n\t"   \
        "mov    r8, r10                      \n\t"

#define MULADDC_CORE                                    \
    "ldmia  r0!, {r6}                   \n\t"   \
    "lsr    r10, r6, #16                 \n\t"   \
    "lsl    r6, r6, #16                 \n\t"   \
    "lsr    r6, r6, #16                 \n\t"   \
    "mov    r4, r8                      \n\t"   \
    "mul    r4, r6                      \n\t"   \
    "mov    r3, r9                      \n\t"   \
    "mul    r6, r3                      \n\t"   \
    "mov    r5, r9                      \n\t"   \
    "mul    r5, r10                      \n\t"   \
    "mov    r3, r8                      \n\t"   \
    "mul    r10, r3                      \n\t"   \
    "lsr    r3, r6, #16                 \n\t"   \
    "add    r5, r5, r3                  \n\t"   \
    "lsr    r3, r10, #16                 \n\t"   \
    "add    r5, r5, r3                  \n\t"   \
    "add    r4, r4, r2                  \n\t"   \
    "mov    r2, #0                      \n\t"   \
    "adc    r5, r2                      \n\t"   \
    "lsl    r3, r6, #16                 \n\t"   \
    "add    r4, r4, r3                  \n\t"   \
    "adc    r5, r2                      \n\t"   \
    "lsl    r3, r10, #16                 \n\t"   \
    "add    r4, r4, r3                  \n\t"   \
    "adc    r5, r2                      \n\t"   \
    "ldr    r3, [r1]                    \n\t"   \
    "add    r4, r4, r3                  \n\t"   \
    "adc    r2, r5                      \n\t"   \
    "stmia  r1!, {r4}                   \n\t"

#define MULADDC_STOP                                    \
    "str    r2, %0                      \n\t"   \
    "str    r1, %1                      \n\t"   \
    "str    r0, %2                      \n\t"   \
    : "=m" (c),  "=m" (d), "=m" (s)        \
        : "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "r0", "r1", "r2", "r3", "r4", "r5",  \
        "r6", "r10", "r8", "r9", "cc"         \
        );

#else

#define MULADDC_INIT                                    \
    asm(                                                \
        "ldr    r0, %3                      \n\t"   \
        "ldr    r1, %4                      \n\t"   \
        "ldr    r2, %5                      \n\t"   \
        "ldr    r3, %6                      \n\t"

#define MULADDC_CORE                                    \
    "ldr    r4, [r0], #4                \n\t"   \
    "mov    r5, #0                      \n\t"   \
    "ldr    r6, [r1]                    \n\t"   \
    "umlal  r2, r5, r3, r4              \n\t"   \
    "adds   r10, r6, r2                  \n\t"   \
    "adc    r2, r5, #0                  \n\t"   \
    "str    r10, [r1], #4                \n\t"

#define MULADDC_STOP                                    \
    "str    r2, %0                      \n\t"   \
    "str    r1, %1                      \n\t"   \
    "str    r0, %2                      \n\t"   \
    : "=m" (c),  "=m" (d), "=m" (s)        \
        : "m" (s), "m" (d), "m" (c), "m" (b)   \
        : "r0", "r1", "r2", "r3", "r4", "r5",  \
        "r6", "r10", "cc"                     \
        );

#endif /* Thumb */

#endif /* ARMv3 */

#endif /* GNUC */

#if (defined(_MSC_VER) && defined(_M_IX86)) || defined(__WATCOMC__)

#define MULADDC_INIT                            \
    __asm   mov     esi, s                      \
    __asm   mov     edi, d                      \
    __asm   mov     ecx, c                      \
    __asm   mov     ebx, b

#define MULADDC_CORE                            \
    __asm   lodsd                               \
    __asm   mul     ebx                         \
    __asm   add     eax, ecx                    \
    __asm   adc     edx, 0                      \
    __asm   add     eax, [edi]                  \
    __asm   adc     edx, 0                      \
    __asm   mov     ecx, edx                    \
    __asm   stosd

#if defined(HAVE_SSE2)

#define EMIT __asm _emit

#define MULADDC_HUIT                            \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC9             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0xC3             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x1F             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x16             \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x7E  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xEE             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x67  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xFC             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x0F             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x56  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD0             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x66  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xE0             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x76  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xF0             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x04  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x5E  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xF4  EMIT 0xD8             \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCD             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xD5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x08  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCF             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xE5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x0C  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCA             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xF5             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x10  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCC             \
    EMIT 0x0F  EMIT 0x6E  EMIT 0x6F  EMIT 0x1C  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xDD             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x14  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCE             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x18  \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0xD4  EMIT 0xCB             \
    EMIT 0x0F  EMIT 0x7E  EMIT 0x4F  EMIT 0x1C  \
    EMIT 0x83  EMIT 0xC7  EMIT 0x20             \
    EMIT 0x83  EMIT 0xC6  EMIT 0x20             \
    EMIT 0x0F  EMIT 0x73  EMIT 0xD1  EMIT 0x20  \
    EMIT 0x0F  EMIT 0x7E  EMIT 0xC9

#define MULADDC_STOP                            \
    EMIT 0x0F  EMIT 0x77                        \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi                      \

#else

#define MULADDC_STOP                            \
    __asm   mov     c, ecx                      \
    __asm   mov     d, edi                      \
    __asm   mov     s, esi                      \

#endif /* SSE2 */
#endif /* MSVC */

#endif

#if !defined(MULADDC_CORE)
#if defined(DHAVE_DOUBLE_LONG_INT)

#define MULADDC_INIT                    \
{                                       \
    TUDBL r;                           \
    TUINT r0, r1;

#define MULADDC_CORE                    \
    r   = *(s++) * (TUDBL) b;          \
    r0  = (TUINT) r;                   \
    r1  = (TUINT)( r >> DBITS_IN_LIMB );         \
    r0 += c;  r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_STOP                    \
}

#else
#define MULADDC_INIT                    \
{                                       \
    TUINT s0, s1, b0, b1;              \
    TUINT r0, r1, rx, ry;              \
    b0 = ( b << DHALF_BITS_IN_LIMB ) >> DHALF_BITS_IN_LIMB;           \
    b1 = ( b >> DHALF_BITS_IN_LIMB );

#define MULADDC_CORE                    \
    s0 = ( *s << DHALF_BITS_IN_LIMB ) >> DHALF_BITS_IN_LIMB;          \
    s1 = ( *s >> DHALF_BITS_IN_LIMB ); s++;            \
    rx = s0 * b1; r0 = s0 * b0;         \
    ry = s1 * b0; r1 = s1 * b1;         \
    r1 += ( rx >> DHALF_BITS_IN_LIMB );                \
    r1 += ( ry >> DHALF_BITS_IN_LIMB );                \
    rx <<= DHALF_BITS_IN_LIMB; ry <<= DHALF_BITS_IN_LIMB;             \
    r0 += rx; r1 += (r0 < rx);          \
    r0 += ry; r1 += (r0 < ry);          \
    r0 +=  c; r1 += (r0 <  c);          \
    r0 += *d; r1 += (r0 < *d);          \
    c = r1; *(d++) = r0;

#define MULADDC_STOP                    \
}

#endif /* C (generic)  */


#endif

#endif

