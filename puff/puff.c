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

typedef struct {
	size_t size;
	uint8_t *data;
} Buffer;

enum {
	BUF_OK = 1,
	BUF_FIN = 2,
	BUF_ERR = 3,
};

typedef struct {
	void *context;
	int (*next_block)(void *context, Buffer *buffer);
} Input;

#define InputNext(INPUT, BUFFER) ((INPUT)->next_block((INPUT)->context, BUFFER))

typedef struct {
	void *context;
	int (*write)(void *context, Buffer *buffer);
} Output;

#define OutputWrite(OUTPUT, BUFFER) ((OUTPUT)->write((OUTPUT)->context, BUFFER))

typedef struct {
	uint8_t *start;
	uint8_t *end;
	uint8_t *whead;
	uint8_t *rhead;
} CyclicBuffer;

#define CyclicBufferSetup(THIS, BLOCK, SIZE) {\
	(THIS)->start = BLOCK;\
	(THIS)->end = (BLOCK) + SIZE;\
	(THIS)->whead = BLOCK;\
	(THIS)->rhead = BLOCK;\
}

inline void CyclicBufferWrite(CyclicBuffer *this, uint8_t byte) {
	*this->whead = byte;
	this->whead++;
	
	if (this->whead >= this->end) {
		this->whead = this->start;
	}
	
	// If the rhead is too slow, bump it
	if (this->whead == this->rhead) {
		this->rhead++;
		
		if (this->rhead > this->end) {
			this->rhead = this->start;
		}
	}
}

inline uint8_t CyclicBufferConsume(CyclicBuffer *this) {
	uint8_t b = *this->rhead;
	
	// Awaiting a write
	if (this->rhead == this->whead) {
		return -1;
	}
	
	this->rhead++;
	
	if (this->rhead > this->end) {
		this->rhead = this->start;
	}
	
	return b;
}

inline uint8_t CyclicBufferPeek(CyclicBuffer *this, size_t i) {
	uint8_t *ptr = this->rhead + i;
	
	if (ptr > this->end) {
		ptr -= (size_t)(this->end - this->start);
	}
	
	return *ptr;
}

void copy_test(Input *input, Output *output) {
	Buffer buffer;
	size_t buffer_progress;
	CyclicBuffer cyclic_buffer;
	uint8_t cyclic_buffer_mem[4096];
	
	CyclicBufferSetup(&cyclic_buffer, cyclic_buffer_mem, 4096);
	
	while (true) {
		int status = InputNext(input, &buffer);
		
		if (status != BUF_OK) {
			break;
		}
		
		
	}
}
