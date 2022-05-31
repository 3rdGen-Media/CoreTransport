//
//  ct_byte_utils.h
//  crUtils
//
//  Created by Joe Moulton on 1/8/17.
//  Copyright © 2017 Abstract Embedded. All rights reserved.
//

#ifndef CTENDIAN_H
#define CTENDIAN_H

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)

#	define __WINDOWS__

#endif

#ifndef CT_UTILS_INLINE
#ifdef _WIN32
#define CT_UTILS_INLINE __inline
#else
#define CT_UTILS_INLINE
#endif
#endif

/** compatibility header for endian.h
 * This is a simple compatibility shim to convert
 * BSD/Linux endian macros to the Mac OS X equivalents.
 * It is public domain.
 * */

 //8 bit stubs are here for posterity, such as when using x-macro expansion for serializing with a function macro
#define ct_htobe8(x) x
#define ct_htole8(x) x
#define ct_be8toh(x) x
#define ct_le8toh(x) x

//Platform specific byte conversion functions
#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define ct_htobe16(x) OSSwapHostToBigInt16(x)
#define ct_htole16(x) OSSwapHostToLittleInt16(x)
#define ct_be16toh(x) OSSwapBigToHostInt16(x)
#define ct_le16toh(x) OSSwapLittleToHostInt16(x)

#define ct_htobe32(x) OSSwapHostToBigInt32(x)
#define ct_htole32(x) OSSwapHostToLittleInt32(x)
#define ct_be32toh(x) OSSwapBigToHostInt32(x)
#define ct_le32toh(x) OSSwapLittleToHostInt32(x)

#define ct_htobe64(x) OSSwapHostToBigInt64(x)
#define ct_htole64(x) OSSwapHostToLittleInt64(x)
#define ct_be64toh(x) OSSwapBigToHostInt64(x)
#define ct_le64toh(x) OSSwapLittleToHostInt64(x)

#elif defined(__OpenBSD__)

#	include <sys/endian.h>

#	define ct_be16toh(x) betoh16(x)
#	define ct_le16toh(x) letoh16(x)

#	define ct_be32toh(x) betoh32(x)
#	define ct_le32toh(x) letoh32(x)

#	define ct_be64toh(x) betoh64(x)
#	define ct_le64toh(x) letoh64(x)

#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__DragonFly__)

#	include <sys/endian.h>

#	define ct_be16toh(x) betoh16(x)
#	define ct_le16toh(x) letoh16(x)

#	define ct_be32toh(x) betoh32(x)
#	define ct_le32toh(x) letoh32(x)

#	define ct_be64toh(x) betoh64(x)
#	define ct_le64toh(x) letoh64(x)

#elif defined(__WINDOWS__)

#include <stdlib.h>
//#	include <winsock2.h>
//#	include <sys/param.h>
//#pragma comment(lib, "Ws2_32.lib")
#	if BYTE_ORDER == LITTLE_ENDIAN

/*
#		define ct_htobe16(x) htons(x)
#		define ct_htole16(x) (x)
#		define ct_be16toh(x) ntohs(x)
#		define ct_le16toh(x) (x)

#		define ct_htobe32(x) htonl(x)
#		define ct_htole32(x) (x)
#		define ct_be32toh(x) ntohl(x)
#		define ct_le32toh(x) (x)

#		define ct_htobe64(x) htonll(x)
#		define ct_htole64(x) (x)
#		define ct_be64toh(x) ntohll(x)
#		define ct_le64toh(x) (x)
 */

#		define ct_htobe16(x) _byteswap_ushort(x)
#		define ct_htole16(x) (x)
#		define ct_be16toh(x) _byteswap_ushort(x)
#		define ct_le16toh(x) (x)

#		define ct_htobe32(x) _byteswap_ulong(x)
#		define ct_htole32(x) (x)
#		define ct_be32toh(x) _byteswap_ulong(x)
#		define ct_le32toh(x) (x)

#		define ct_htobe64(x) _byteswap_uint64(x)
#		define ct_htole64(x) (x)
#		define ct_be64toh(x) _byteswap_uint64(x)
#		define ct_le64toh(x) (x)


#	elif BYTE_ORDER == BIG_ENDIAN

		/* that would be xbox 360 */
#		define ct_htobe16(x) (x)
#		define ct_htole16(x) __builtin_bswap16(x)
#		define ct_be16toh(x) (x)
#		define ct_le16toh(x) __builtin_bswap16(x)

#		define ct_htobe32(x) (x)
#		define ct_htole32(x) __builtin_bswap32(x)
#		define ct_be32toh(x) (x)
#		define ct_le32toh(x) __builtin_bswap32(x)

#		define ct_htobe64(x) (x)
#		define ct_htole64(x) __builtin_bswap64(x)
#		define ct_be64toh(x) (x)
#		define ct_le64toh(x) __builtin_bswap64(x)

#	else

#		error byte order not supported

#	endif

#	define __BYTE_ORDER    BYTE_ORDER
#	define __BIG_ENDIAN    BIG_ENDIAN
#	define __LITTLE_ENDIAN LITTLE_ENDIAN
#	define __PDP_ENDIAN    PDP_ENDIAN

#else

//#	error platform not supported

#endif

#include <stdio.h>      /* printf, scanf, NULL */
#include <stdlib.h>


static CT_UTILS_INLINE int ct_is_little_endian()
{

	int n;
	char* charn;

	n = 1;
	charn = (char*)&n;

	// little endian if true
	if (*charn == 1)
	{
		return 1;
	}

	return 0;
}


static CT_UTILS_INLINE void ct_print_data_as_binary(unsigned* data, unsigned int numBitsToPrint)
{
	unsigned
		input = *data,
		n_bits = numBitsToPrint,
		* bits = (unsigned*)malloc(sizeof(unsigned) * n_bits),
		bit = 0;

	for (bit = 0; bit < n_bits; ++bit, input >>= 1)
		bits[bit] = input & 1;;

	for (bit = n_bits; bit--;)
		printf("%u", bits[bit]);

	free(bits);
}

//FYI:  client must free returned memory
static CT_UTILS_INLINE unsigned* ct_convert_to_binary(unsigned* data, unsigned int numBitsToPrint)
{
	unsigned
		input = *data,
		n_bits = numBitsToPrint,
		* bits = (unsigned*)malloc(sizeof(unsigned) * n_bits),
		bit = 0;

	for (bit = 0; bit < n_bits; ++bit, input >>= 1)
		bits[bit] = input & 1;;

	return bits;
}


#endif /* ct_byte_utils_h */
