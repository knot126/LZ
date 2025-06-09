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

#define Max(a, b) ( ((a) > (b)) ? (a) : (b) )
#define Min(a, b) ( ((a) < (b)) ? (a) : (b) )

/**
 * In memory buffer with length
 */
typedef struct {
	size_t size;
	uint8_t *data;
	size_t progress; // for internal use only when reading buffers, should
	                 // be set to zero when reading new buffers.
} Buffer;

#define BufferEmpty(BUFFER) ( (BUFFER)->progress >= (BUFFER)->size )

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
	int32_t (*next)(void *context, Buffer *buffer);
} Input;

#define InputNext(INPUT, BUFFER) ((INPUT)->next((INPUT)->context, BUFFER))

/**
 * Output stream. This one's a bit more boring.
 */
typedef struct {
	void *context;
	int32_t (*write)(void *context, Buffer *buffer);
} Output;

#define OutputWrite(OUTPUT, BUFFER) ((OUTPUT)->write((OUTPUT)->context, BUFFER))

int32_t copy_test(Input *input, Output *output) {
	/**
	 * Copy function for testing input and output stream implementation.
	 */
	
	Buffer buffer;
	
	while (true) {
		int32_t status = InputNext(input, &buffer);
		
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

#define RingInit(CB, MEM, LEN) { (CB)->start = MEM; (CB)->size = LEN; (CB)->offset = 0; }
#define RingPush(CB, BYTE) (CB->start[ CB->offset++ % CB->size ] = BYTE)
#define RingGet(CB, INDEX) (CB->start[ (CB->offset + INDEX) % CB->size ])

#define LookbackSize 65536
#define LookaheadSize 1024
#define RingSize (LookbackSize + LookaheadSize)

typedef enum : uint8_t {
	COMP_INITING = 1,
	COMP_COMPRESSING = 2,
} CompressState;

int64_t compress(Input *input, Output *output) {
	CompressState state = COMP_INITING;
	Buffer buffer = {};
	int32_t status = IO_OK;
	
	// Initialise the ring buffer
	Ring ring; uint8_t _lookback[RingSize];
	RingInit(&ring, _lookback, RingSize);
	
	// Fill the lookback with a common value (I prefer zero/nul)
	for (size_t i = 0; i < LookbackSize; i++) {
		_lookback[i] = 0;
	}
	
	// The main loop and compressor state machine starts here
	int_fast32_t initial_input_remaining = LookaheadSize; // tracking for the amount of lookahead buffer that is present at the start of compression
	int_fast32_t lookahead_remaining = 0;
	
	while (true) {
		if (BufferEmpty(&buffer)) {
			status = InputNext(input, &buffer);
		}
		
		switch (state) {
			case COMP_INITING: {
				if (status == IO_FIN) {
					// Fill rest with zeros (not really needed, but probably
					// helpful if there is ever an error)
					for (size_t i = 0; i != LookaheadSize; i++) {
						_lookback[lookahead_remaining + i] = 0;
					}
					
					state = COMP_COMPRESSING;
					break;
				}
				
				// Read all possible input
				size_t amt_copy = Min(LookaheadSize - lookahead_remaining, buffer.size - buffer.progress);
				
				for (size_t i = 0; i < amt_copy; i++) {
					_lookback[LookbackSize + i] = buffer.data[buffer.progress + i];
				}
				
				buffer.progress += amt_copy;
				lookahead_remaining += amt_copy;
				
				if (lookahead_remaining == LookaheadSize) {
					state = COMP_COMPRESSING;
				}
				
				break;
			}
			case COMP_COMPRESSING: {
				break;
			}
		}
	}
}
