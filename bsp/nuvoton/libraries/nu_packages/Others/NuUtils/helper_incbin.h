/*
 * @copyright (C) 2026 Nuvoton Technology Corp. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HELPER_INCBIN_H__
#define __HELPER_INCBIN_H__

#define HELPER_INCBIN_STR2(x) #x
#define HELPER_INCBIN_STR(x) HELPER_INCBIN_STR2(x)
#define INCBIN(name, file) \
    __asm__(".section .rodata\n" \
            ".global incbin_" HELPER_INCBIN_STR(name) "_start\n" \
            ".balign 16\n" \
            "incbin_" HELPER_INCBIN_STR(name) "_start:\n" \
            ".incbin \"" file "\"\n" \
            \
            ".global incbin_" HELPER_INCBIN_STR(name) "_end\n" \
            ".balign 1\n" \
            "incbin_" HELPER_INCBIN_STR(name) "_end:\n" \
            ".byte 0\n" \
    ); \
    extern const __attribute__((aligned(32))) void* incbin_ ## name ## _start; \
    extern const void* incbin_ ## name ## _end;

#endif /* __HELPER_INCBIN_H__ */
