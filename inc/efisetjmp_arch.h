#ifndef GNU_EFI_SETJMP_ARCH_H
#define GNU_EFI_SETJMP_ARCH_H

#if defined(__x86_64__)
#include "x86_64/efisetjmp_arch.h"
#elif defined(__aarch64__)
#include "aarch64/efisetjmp_arch.h"
#elif defined(__riscv) && __riscv_xlen == 64
#include "riscv64/efisetjmp_arch.h"
#elif defined(__i386__)
#include "ia32/efisetjmp_arch.h"
#elif defined(__ia64__)
#include "ia64/efisetjmp_arch.h"
#elif defined(__loongarch64)
#include "loongarch64/efisetjmp_arch.h"
#elif defined(__mips64__)
#include "mips64el/efisetjmp_arch.h"
#elif defined(__arm__)
#include "arm/efisetjmp_arch.h"
#endif

#endif /* GNU_EFI_SETJMP_ARCH_H */
