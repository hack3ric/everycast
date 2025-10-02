#ifndef EVERYCAST_DEFS_H
#define EVERYCAST_DEFS_H

#define UNUSED(x) (void)(x)
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

#if __BIG_ENDIAN__
#define htonll(x) (x)
#define ntohll(x) (x)
#else
#define htonll(x) (((uint64_t)htonl((x) & 0xffffffff) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x) & 0xffffffff) << 32) | ntohl((x) >> 32))
#endif

#endif  // EVERYCAST_DEFS_H
