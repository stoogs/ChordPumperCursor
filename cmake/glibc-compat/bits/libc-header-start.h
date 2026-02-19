/*
 * glibc symbol version compatibility shim.
 *
 * CachyOS ships glibc 2.43 which redirects standard math and string
 * functions (sqrtf, atan2f, sscanf, strtol, …) to newer symbol versions
 * via IEC 60559 / C23 feature macros.  Bitwig Studio loads plugins in a
 * sandboxed runtime with an older glibc that lacks these versions.
 *
 * This header intercepts glibc's internal <bits/libc-header-start.h>
 * (which every standard header includes), lets it run normally, then
 * overrides the feature macros to zero so the compiler emits references
 * to the classic GLIBC_2.2.5 symbols instead.
 *
 * Added via  -isystem ${CMAKE_SOURCE_DIR}/cmake/glibc-compat  which
 * ensures this file is found before /usr/include/bits/.
 */

/* Forward to the real glibc header first. */
#include_next <bits/libc-header-start.h>

/* --- C23 string/scanf redirections (__isoc23_sscanf, __isoc23_strtol etc.) --- */
#undef __GLIBC_USE_ISOC23
#define __GLIBC_USE_ISOC23 0

/* C23_STRTOL is derived from ISOC23 in features.h (include-guarded,
   so our ISOC23 override above doesn't retroactively fix it). */
#undef __GLIBC_USE_C23_STRTOL
#define __GLIBC_USE_C23_STRTOL 0

/* --- IEC 60559 BFP math redirections (sqrtf, atan2f, hypot → GLIBC_2.35/2.43) --- */
#undef __GLIBC_USE_IEC_60559_BFP_EXT
#define __GLIBC_USE_IEC_60559_BFP_EXT 0
#undef __GLIBC_USE_IEC_60559_BFP_EXT_C23
#define __GLIBC_USE_IEC_60559_BFP_EXT_C23 0

/* --- IEC 60559 funcs redirections (fmod → GLIBC_2.38) --- */
#undef __GLIBC_USE_IEC_60559_FUNCS_EXT
#define __GLIBC_USE_IEC_60559_FUNCS_EXT 0
#undef __GLIBC_USE_IEC_60559_FUNCS_EXT_C23
#define __GLIBC_USE_IEC_60559_FUNCS_EXT_C23 0
