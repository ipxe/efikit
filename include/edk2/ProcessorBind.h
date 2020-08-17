/*
 * Copyright (C) 2020 Michael Brown <mbrown@fensystems.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 * This is a wrapper header for EDK2's ProcessorBind.h.  The EDK2
 * Base.h header file issues a
 *
 *   #include <ProcessorBind.h>
 *
 * before doing anything else, and relies upon the appropriate file
 * (e.g. MdePkg/Include/X64/ProcessorBind.h) being picked up via the
 * architecture-specific directory in the include path.
 *
 * By omitting the architecture-specific directory from the include
 * path, this gives us an opportunity to hook in our own code, in
 * order to adapt the EDK2 compilation environment to cooperate with
 * normal system headers.
 *
 */

/******************************************************************************
 *
 * Fix (re)definition of various macros in Base.h
 *
 ******************************************************************************
 *
 * The EDK2 Base.h header file unconditionally #defines NULL along
 * with several other macros often used by non-EDK2 headers.
 *
 * This ProcessorBind.h file is #included at the start of Base.h.  We
 * may therefore #undef NULL and other macros here in order to ensure
 * that the subsequent (re)definitions in Base.h will succeed.
 *
 * We must do this outside of our own include guard to allow for the
 * possibility that ProcessorBind.h is #included directly (rather than
 * via Base.h).
 *
 */

#ifdef __BASE_H__
#undef NULL
#undef TRUE
#undef FALSE
#undef ABS
#endif

#ifndef _PROCESSOR_BIND_WRAPPER_H
#define _PROCESSOR_BIND_WRAPPER_H

/******************************************************************************
 *
 * Include architecture-specific ProcessorBind.h
 *
 ******************************************************************************
 *
 * Use compiler-defined macros to select and include the relevant
 * architecture-specific ProcessorBind.h file.
 *
 */

#if defined(__i386__)
#include <Ia32/ProcessorBind.h>
#endif

#if defined(__x86_64__)
#include <X64/ProcessorBind.h>
#endif

#if defined(__arm__)
#include <Arm/ProcessorBind.h>
#endif

#if defined(__aarch64__)
#include <AArch64/ProcessorBind.h>
#endif

#if defined(__riscv)
#include <RiscV64/ProcessorBind.h>
#endif

/******************************************************************************
 *
 * Fix symbol visibility
 *
 ******************************************************************************
 *
 * The EDK2 X64/ProcessorBind.h header file may modify the default
 * symbol visibility.  Undo this change, to allow for compiling in a
 * standard userspace environment.
 *
 * The list of condition checks must exactly match those used in
 * X64/ProcessorBind.h to modify the default symbol visibility.
 *
 */

#if defined(__x86_64__) && defined(__GNUC__) && defined(__pic__) && \
    ! defined(USING_LTO) && ! defined(__APPLE__)
#pragma GCC visibility pop
#endif

/******************************************************************************
 *
 * Use standard variable argument mechanism
 *
 ******************************************************************************
 *
 * The EDK2 Base.h header file defaults to using a non-standard
 * varargs mechanism.  Inhibit this.
 *
 */

#define NO_MSABI_VA_FUNCS

/******************************************************************************
 *
 * Fix compilation as C++
 *
 ******************************************************************************
 *
 * The EDK2 headers may generally be compiled via an extern "C"
 * wrapper, with the notable exception of the _Static_assert() macro
 * which is C-only.
 *
 */
#ifdef __cplusplus
#define _Static_assert static_assert
#endif

#endif /* _PROCESSOR_BIND_WRAPPER_H */
