#ifndef MACROS_H
#	define MACROS_H 1

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

#	if defined _POSIX_C_SOURCE && _POSIX_C_SOURCE >= 199309L
#		define HAVE_NANOSLEEP 1
#	else
#		define HAVE_NANOSLEEP 0
#	endif /* Posix 199309L */

#	ifdef __glibc_has_attribute
#		define HAS_ATTRIBUTE(attr) __glibc_has_attribute(attr)
#	elif defined __has_attribute
#		define HAS_ATTRIBUTE(attr) __has_attribute(attr)
#	else
#		define HAS_ATTRIBUTE(attr) 0
#	endif /* has_attribute */

#	ifndef ATTR_INLINE_
#		ifdef __inline
#			define ATTR_INLINE_ __inline
#		elif (defined __cplusplus || (defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L))
#			define ATTR_INLINE_ inline
#		else
#			define ATTR_INLINE_
#		endif
#	endif

#	ifdef __always_inline
#		define ATTR_INLINE __always_inline
#	elif HAS_ATTRIBUTE(__always_inline__)
#		define ATTR_INLINE __attribute__((__always_inline__)) ATTR_INLINE_
#	else
#		define ATTR_INLINE
#	endif

#endif /* MACROS_H */
