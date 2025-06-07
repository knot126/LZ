/**
 * Puff (LZ) part of huff-puff compression
 * 
 * bytestream format:
 *   <ismatch:1><lenlen:2><byte0:5> [<byte1> [<byte2> [<byte3>]]] [<lenlen2:2><byteA:6>... | <data...>]
 *                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *                        first number (length)                    second number (offset) OR literals
 * 
 * <ismatch>:
 *   0: These are literals. The first number encodes the length of the literals.
 *   1: This is a match. The first number encodes the match length, and the
 *      second number encodes the distance.
 * 
 * <lenlen>:
 *   lenlen + 1 is the number of bytes in this variable length, little endian
 *   number, including the first byte which only partially contains a single
 *   number
 * 
 * <byteN>:
 *   Bits of the first/second number. The second number is only present in a
 *   match.
 * 
 * <data>:
 *   Literal data.
 */

/**
 * In memory buffer with length
 */
typedef struct {
	size_t size;
	uint8_t *data;
} Buffer;

/**
 * Status codes which should be returned by Input::next() and Output::write()
 */
enum {
	IO_OK = 1,  // Okay, returned next block of data
	IO_FIN = 2, // Okay, input is finished (no data block returned though)
	IO_ERR = 3, // Error
};

/**
 * Input stream that works by returning a buffer with at least one byte,
 * allowing the callee to determine the exact amount and location of data that
 * is returned. Note: The buffer structure may be modified between calls.
 */
typedef struct {
	void *context;
	int (*next)(void *context, Buffer *buffer);
} Input;

#define InputNext(INPUT, BUFFER) ((INPUT)->next((INPUT)->context, BUFFER))

/**
 * Output stream. This one's a bit more boring.
 */
typedef struct {
	void *context;
	int (*write)(void *context, Buffer *buffer);
} Output;

#define OutputWrite(OUTPUT, BUFFER) ((OUTPUT)->write((OUTPUT)->context, BUFFER))

int copy_test(Input *input, Output *output) {
	/**
	 * Copy function for testing input and output stream implementation.
	 */
	
	Buffer buffer;
	
	while (true) {
		int status = InputNext(input, &buffer);
		
		if (status == IO_FIN) {
			return 0;
		}
		else if (status == IO_ERR) {
			return 1;
		}
		
		status = OutputWrite(output, &buffer);
		
		if (status != IO_OK) {
			return 2;
		}
	}
}

typedef struct {
	uint8_t *start;
	size_t size;
	size_t offset;
} Ring;

#define RingPush(CB, BYTE) (CB->start[ CB->offset++ % CB->size ] = BYTE)
#define RingGet(CB, INDEX) (CB->start[ (CB->offset + INDEX) % CB->size ])
