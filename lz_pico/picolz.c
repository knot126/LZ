#include <stdlib.h>
#include <string.h>

#ifndef byte_t
#define byte_t unsigned char
#endif

#define plz_min(a, b) ((a) < (b) ? (a) : (b))

#define plz_failure ((size_t) -1)

static inline size_t plz_memcmp(byte_t *a, byte_t *b, size_t max_size) {
	/**
	 * Return the number of bytes that are equal at the start of buffers a and b.
	 */
	
	for (size_t i = 0; i < max_size; i++) {
		if (a[i] != b[i]) {
			// RLE style self referencing
			#ifndef PLZ_DISABLE_REPEAT_BEHAVIOUR
			if (i != 0) {
				for (size_t j = i; j < max_size; j++) {
					if (a[j % i] != b[j]) {
						i += j;
						break;
					}
				}
			}
			#endif
			
			return i;
		}
	}
	
	return max_size;
}

struct plz_match {
	size_t offset, len;
};

static inline struct plz_match plz_find_match(byte_t *back, size_t back_size, byte_t *front, size_t front_size) {
	/**
	 * Find a match (by simple linear search)
	 */
	
	struct plz_match m = {0, 0};
	
	for (size_t i = 0; i < back_size; i++) {
		const size_t search_size = plz_min(back_size - i, front_size);
		const size_t match_size = plz_memcmp(&back[i], front, search_size);
		
		if (match_size > m.len) {
			m.len = match_size;
			m.offset = i;
		}
	}
	
	return m;
}

static size_t plz_copy(byte_t *outbuf, size_t outsize, byte_t *inbuf, size_t insize) {
	/**
	 * Copy `insize` bytes from inbuf to outbuf (up to the amount that fits)
	 */
	
	const size_t copied = insize > outsize ? outsize : insize;
	
	memcpy(outbuf, inbuf, copied);
	
	return (insize == copied) ? copied : plz_failure;
}

static size_t plz_writeint(byte_t *outbuf, size_t outsize, unsigned long long num) {
	/**
	 * Write an leb128 encoded integer
	 */
	
	size_t i = 0;
	
	do {
		byte_t b = (num & 0x7f);
		if (num >= 0x80) { b |= 0x80; }
		num >>= 7;
		if (outsize) {
			outbuf[0] = b;
			outbuf++;
			outsize--;
			i++;
		}
		else {
			return plz_failure;
		}
	} while (num);
	
	return i;
}

#define WRITE_INT(I) {\
	status = plz_writeint(outbuf + outpos, maxoutsize - outpos, I);\
	\
	if (status == plz_failure) {\
		return plz_failure;\
	}\
	\
	outpos += status;\
}

#define WRITE_LIT(I) {\
	status = plz_copy(outbuf + outpos, maxoutsize - outpos, inbuf + I - lit_length, lit_length);\
	\
	if (status == plz_failure) {\
		return plz_failure;\
	}\
	\
	outpos += status;\
	lit_length = 0;\
}

size_t plz_compress(byte_t *inbuf, size_t insize, byte_t *outbuf, size_t maxoutsize) {
	/**
	 * Compress the input block `inbuf` to the output buffer `outbuf`.
	 */
	
	size_t lit_length = 0;
	size_t outpos = 0;
	size_t status;
	
	for (size_t i = 0; i < insize;) {
		struct plz_match m = plz_find_match(inbuf, i, inbuf + i, insize - i);
		
		if (m.len >= 3) {
			// Write number of literals
			WRITE_INT(lit_length);
			
			// Write literals themselves
			WRITE_LIT(i);
			
			// Write match length
			WRITE_INT(m.len);
			WRITE_INT(i - m.offset);
			
			i += m.len;
		}
		else {
			lit_length += 1;
			i++;
		}
	}
	
	// Write any remaining literals
	if (lit_length) {
		WRITE_LIT(insize);
	}
	
	return outpos;
}

#undef WRITE_INT
#undef WRITE_LIT

static inline size_t plz_dcopy(byte_t *src, size_t src_size, byte_t *dest, size_t dest_size) {
	/**
	 * Copy `dest_size` bytes to dest, wrapping around if needed.
	 */
	
	size_t i;
	
	for (i = 0; i < dest_size; i++) {
		dest[i] = src[i % src_size];
	}
	
	return i;
}

static inline size_t plz_readint(byte_t *input, size_t input_size, size_t *output) {
	size_t num = 0;
	size_t i = 0;
	
	while (1) {
		byte_t b = input[0];
		num |= (b & 0x7f) << (7 * i);
		
		input++;
		input_size--;
		i++;
		
		if (!(b >> 7)) {
			break;
		}
		else {
			if (!input_size) {
				return plz_failure;
			}
		}
	}
	
	output[0] = num;
	
	return i;
}

#define READ_INT(V) {\
	size_t status = plz_readint(input + i, input_size - i, &V);\
	\
	if (status == plz_failure) {\
		return plz_failure;\
	}\
	\
	i += status;\
}

size_t plz_decompress(byte_t *input, size_t input_size, byte_t *output, size_t output_size) {
	size_t outpos = 0;
	
	for (size_t i = 0; i < input_size;) {
		// Get literal length
		size_t lit_len = 0;
		READ_INT(lit_len);
		
		// Check we have enough input and output
		if ((input_size - i) < lit_len || (output_size - i) < lit_len) {
			return plz_failure;
		}
		
		// Copy the literals
		plz_dcopy(input + i, input_size - i, output + outpos, lit_len);
		i += lit_len;
		outpos += lit_len;
		
		// Check if we're done
		if (input_size == i) {
			break;
		}
		
		// Get match size and offset
		size_t match_size = 0, match_offset = 0;
		READ_INT(match_size);
		READ_INT(match_offset);
		
		// Make sure the offset starts within bounds and the size isn't bigger
		// than the buffer
		
		/// TODO TODO TODO FINISH THIS!!
	}
	
	return outpos;
}

#if defined(PLZ_TEST) && (PLZ_TEST == 2)

#include <stdio.h>

size_t plz_debug_describe_compressed_stream(byte_t *input, size_t input_size) {
	/**
	 * Describe the format of a compressed stream to stdout. Not bounds checked
	 * or doing much validation!
	 */
	
	size_t l;
	
	for (size_t i = 0; i < input_size;) {
		size_t lit_len;
		l = i;
		
		READ_INT(lit_len);
		
		printf("[%zu] %zu literal bytes\n", l, lit_len);
		
		i += lit_len;
		
		if (i == input_size) {
			printf("end\n");
			break;
		}
		
		l = i;
		
		size_t match_len, match_offset;
		
		READ_INT(match_len);
		READ_INT(match_offset);
		
		printf("[%zu] go back %zu and copy %zu bytes\n", l, match_offset, match_len);
		
		if (i == input_size) {
			printf("end\n");
			break;
		}
	}
	
	return 0;
}

#endif

#ifdef PLZ_TEST
#define FILE_UTILS_IMPLEMENTATION
#include "FileUtils.h"

#if PLZ_TEST == 1
int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "Too few arguments.\n");
		return 127;
	}
	
	byte_t *in_data; size_t in_size;
	
	if (!FULoad(argv[1], (void **) &in_data, &in_size)) {
		fprintf(stderr, "Could not open input file.\n");
		return 1;
	}
	
	byte_t *out_data = malloc(in_size);
	
	if (!out_data) {
		fprintf(stderr, "Could not create buffer for storing output data.\n");
		return 1;
	}
	
	size_t out_size = plz_compress(in_data, in_size, out_data, in_size);
	
	if (out_size == plz_failure) {
		fprintf(stderr, "Could not compress the input file, not trying anything else.\n");
		return 1;
	}
	
	fprintf(stderr, "Compressed %zu bytes to %zu bytes.\n", in_size, out_size);
	fprintf(stderr, "Ratio = %0.3f%%\n", 100.0f * (((float) out_size) / ((float) in_size)));
	
	if (!FUSave(argv[2], out_data, out_size)) {
		fprintf(stderr, "Could not write to output file.\n");
		return 1;
	}
	
	return 0;
}
#elif PLZ_TEST == 2
int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "Too few arguments.\n");
		return 127;
	}
	
	byte_t *in_data; size_t in_size;
	
	if (!FULoad(argv[1], (void **) &in_data, &in_size)) {
		fprintf(stderr, "Could not open input file.\n");
		return 1;
	}
	
	size_t status = plz_debug_describe_compressed_stream(in_data, in_size);
	
	if (status == plz_failure) {
		fprintf(stderr, "failed\n");
		return 1;
	}
	
	return 0;
}
#endif

#endif
