#ifndef KIJI_HANDY_H_
#define KIJI_HANDY_H_

// TODO use MVM_panic
#define Renew(v,n,t) \
  v = (t*)realloc(v, sizeof(t)*n); \
  assert(v);

#define Renewc(v,n,t,c) \
  v = (c*)realloc(v, sizeof(t)*n); \
  assert(v);

#ifndef KIJI_STATIC_INLINE
# if defined(__GNUC__) || defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#   define KIJI_STATIC_INLINE static inline
# else
#   define KIJI_STATIC_INLINE static
# endif
#endif /* STATIC_INLINE */

#endif /* KIJI_HANDY_H_ */
