#ifndef KIJI_HANDY_H_
#define KIJI_HANDY_H_

#include <string.h>
#include <stdlib.h>

// TODO use MVM_panic
#define Renew(v,n,t) \
  v = (t*)realloc(v, sizeof(t)*n); \
  if (!v) { MVM_panic(MVM_exitcode_compunit, "Cannot allocate memory"); }

#define Renewc(v,n,t,c) \
  v = (c*)realloc(v, sizeof(t)*n); \
  if (!v) { MVM_panic(MVM_exitcode_compunit, "Cannot allocate memory"); }

/* void    Newxz(void* ptr, int nitems, type) */
#define Newxz(ptr, nitems, type) \
    ptr=(type*)malloc(sizeof(type)*nitems); \
    if (!ptr) { MVM_panic(MVM_exitcode_compunit, "Cannot allocate memory"); } \
    memset(ptr, 0, sizeof(type)*nitems);

#define Safefree(ptr) free(ptr)

#ifndef KIJI_STATIC_INLINE
# if defined(__GNUC__) || defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#   define KIJI_STATIC_INLINE static inline
# else
#   define KIJI_STATIC_INLINE static
# endif
#endif /* STATIC_INLINE */

#endif /* KIJI_HANDY_H_ */
