#ifndef EVERYCAST_DEFS_H
#define EVERYCAST_DEFS_H

#include <stdlib.h>

#define UNUSED(x) (void)(x)
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

#if __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) (((uint64_t)htonl((x) & 0xffffffff) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x) & 0xffffffff) << 32) | ntohl((x) >> 32))
#endif

#define RAII(f) __attribute__((__cleanup__(f)))

#define _def_freep(type)                        \
  static inline void freep_##type(type** ptr) { \
    if (*ptr) free(*ptr);                       \
  }

#define _def_freep_struct(type)                        \
  static inline void freep_struct_##type(struct type** ptr) { \
    if (*ptr) free(*ptr);                       \
  }

_def_freep(char);

#endif  // EVERYCAST_DEFS_H
