#ifndef HAVE_PDT_H
#define HAVE_PDT_H

#include <stdint.h>

#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif

typedef	int       Boolean;
typedef uint8_t   Enumeration8;
typedef uint16_t  Enumeration16;
typedef int8_t    Integer8;
typedef uint8_t   UInteger8;
typedef int16_t   Integer16;
typedef uint16_t  UInteger16;
typedef int32_t   Integer32;
typedef uint32_t  UInteger32;
typedef int64_t   Integer64;
typedef uint64_t  UInteger64; /* ITU-T G.8275.2 */
typedef uint8_t   Octet;

#endif
