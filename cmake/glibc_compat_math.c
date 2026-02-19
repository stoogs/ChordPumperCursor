/*
 * Linker-level glibc symbol version pinning.
 *
 * The compile-time shim (bits/libc-header-start.h) prevents glibc
 * headers from redirecting math functions to newer symbol versions.
 * Object files end up with *unversioned* references (e.g. plain "sqrtf").
 * The linker then resolves these to the newest DEFAULT version in libm
 * (e.g. sqrtf@@GLIBC_2.43), which Bitwig's older glibc cannot provide.
 *
 * This file uses --wrap to intercept those calls and route them through
 * explicit old-version symbols via .symver directives.
 *
 * Link with:
 *   -Wl,--wrap=sqrtf,--wrap=atan2f,--wrap=fmod,--wrap=hypot,--wrap=hypotf
 */

extern float  old_sqrtf(float);
extern float  old_atan2f(float, float);
extern double old_fmod(double, double);
extern double old_hypot(double, double);
extern float  old_hypotf(float, float);

__asm__(".symver old_sqrtf,  sqrtf@GLIBC_2.2.5");
__asm__(".symver old_atan2f, atan2f@GLIBC_2.2.5");
__asm__(".symver old_fmod,   fmod@GLIBC_2.2.5");
__asm__(".symver old_hypot,  hypot@GLIBC_2.2.5");
__asm__(".symver old_hypotf, hypotf@GLIBC_2.2.5");

float  __wrap_sqrtf(float x)            { return old_sqrtf(x); }
float  __wrap_atan2f(float y, float x)  { return old_atan2f(y, x); }
double __wrap_fmod(double x, double y)  { return old_fmod(x, y); }
double __wrap_hypot(double x, double y) { return old_hypot(x, y); }
float  __wrap_hypotf(float x, float y)  { return old_hypotf(x, y); }
