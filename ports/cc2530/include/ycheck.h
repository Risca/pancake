/* ycheck.h internal checking header file. */
/* Copyright 2005-2010 IAR Systems AB. */

/* Note that there is no include guard for this header. This is intentional. */

#ifndef _SYSTEM_BUILD
  #pragma system_include
#endif

/* __INTRINSIC
 *
 * Note: Redefined each time ycheck.h is included, i.e. for each
 * system header, to ensure that intrinsic support could be turned off
 * individually for each file.
 */
#ifdef __INTRINSIC
  #undef __INTRINSIC
#endif /* __INTRINSIC */

#ifndef __NO_INTRINSIC
  #define __INTRINSIC __intrinsic
#else
  #define __INTRINSIC
#endif

/* __TAABI_PORTABILITY_INTERNAL_LEVEL
 *
 * Note: Redefined each time ycheck.h is included, i.e. for each
 * system header, to ensure that ABI support could be turned off/on
 * individually for each file.
 *
 * Possible values for this preprocessor symbol:
 *
 * 0 - ABI portability mode is disabled.
 *
 * 1 - ABI portability mode (version 1) is enabled.
 *
 * All other values are reserved for future use.
 */

#ifdef __TAABI_PORTABILITY_INTERNAL_LEVEL
  #undef __TAABI_PORTABILITY_INTERNAL_LEVEL
#endif


#ifdef _TAABI_PORTABILITY_LEVEL

  /* The user can define this in the source code before including
   * a standard header. */
  #define __TAABI_PORTABILITY_INTERNAL_LEVEL _TAABI_PORTABILITY_LEVEL

#elif defined(__TAABI_PORTABILITY_LEVEL)

  /* The user can define this in the source code before including
   * a standard header. */
  #define __TAABI_PORTABILITY_INTERNAL_LEVEL __TAABI_PORTABILITY_LEVEL

#elif defined(__DEFAULT_TAABI_PORTABILITY_LEVEL)

  /* The tool chain defines this if the project has enabled abi
   * portability. */
  #define __TAABI_PORTABILITY_INTERNAL_LEVEL __DEFAULT_TAABI_PORTABILITY_LEVEL

#else
  #define __TAABI_PORTABILITY_INTERNAL_LEVEL 0
#endif

#ifndef _YCHECK
#define _YCHECK

#define __ATTRIBUTES             __INTRINSIC __nounwind
#define __ATTRIBUTES_CAN_THROW   __INTRINSIC 
#define __ATTRIBUTES_NORETURN    __INTRINSIC __noreturn __nounwind
#define __ATTRIBUTES_NORETURN_CAN_THROW __INTRINSIC __noreturn

#endif /* _YCHECK */
