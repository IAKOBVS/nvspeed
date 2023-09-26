#ifndef NVSPEED_MACROS_H
#define NVSPEED_MACROS_H

#if defined(__GNUC__) || defined(__clang__)
#	define INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#	define INLINE __forceinline inline
#else
#	define INLINE inline
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3)) || (defined(__clang__) && __has_builtin(__builtin_expect))
#	define likely(x)   __builtin_expect((x), 1)
#	define unlikely(x) __builtin_expect((x), 0)
#else
#	define likely(x)   (x)
#	define unlikely(x) (x)
#endif

#ifdef __USE_POSIX199309
#	define HAVE_NANOSLEEP 1
#else
#	define HAVE_NANOSLEEP 0
#endif

#endif /* NVSPEED_MACROS_H */
