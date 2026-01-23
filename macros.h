#ifndef MACROS_H
#define MACROS_H 1

#	ifdef __glibc_has_builtin
#		define HAS_BUILTIN(name) __glibc_has_builtin(name)
#	elif defined __has_builtin
#		define HAS_BUILTIN(name) __has_builtin(name)
#	else
#		define HAS_BUILTIN(name) 0
#	endif /* has_builtin */

#	if defined __glibc_unlikely && defined __glibc_likely
#		define likely(x)   __glibc_likely(x)
#		define unlikely(x) __glibc_unlikely(x)
#	elif ((defined __GNUC__ && (__GNUC__ >= 3)) || defined __clang__) && HAS_BUILTIN(__builtin_expect)
#		define likely(x)   __builtin_expect((x), 1)
#		define unlikely(x) __builtin_expect((x), 0)
#	else
#		define likely(x)   (x)
#		define unlikely(x) (x)
#	endif /* unlikely */

#endif /* MACROS_H */
