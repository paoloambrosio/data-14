#ifndef MESSAGE_H_
#define MESSAGE_H_

/*
 * Standard C99 fixed-size integer types
 */

#ifdef _MSC_VER
typedef __int8           int8_t;
typedef __int16          int16_t;
typedef __int32          int32_t;
typedef __int64          int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

/*
 * Data-14 message magic number
 * 
 * It is not defined as 'D-14' to be used to determine the endianness
 * if the message is sent through the internet
 */
#define D14_MAGIC 0x2d443431

/*
typedef struct {
	uint32_t magic;
	uint32_t crc;
} D14NetHead;
*/

typedef struct {
	uint16_t type;
	uint16_t subtype;
} D14MsgHead;

#endif /*MESSAGE_H_*/
