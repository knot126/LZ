struct InputStream {
	ubyte[] buffer;
	size_t position;
	
	this(ubyte[] buffer) {
		this.position = 0;
		this.buffer = buffer;
	}
	
	bool eof() {
		return this.position >= this.buffer.length;
	}
	
	ubyte read() {
		if (!this.eof()) {
			return this.buffer[this.position++];
		}
		else {
			return 0;
		}
	}
	
	ulong readNumber() {
		ulong sum = 0;
		ubyte value;
		
		while (true) {
			value = this.read();
			sum += value;
			if (value != 0xff || this.eof()) {
				break;
			}
		}
		
		return sum;
	}
}

struct OutputStream {
	ubyte[] buffer;
	
	void write(byte b) {
		this.buffer.length++;
		this.buffer[$ - 1] = b;
	}
	
	void write16(ushort data) {
		this.write(data >> 8);
		this.write(data & 0xff);
	}
	
	ubyte[] getData() {
		return this.buffer;
	}
}

struct WindowBuffer {
	ubyte[] buffer;
	size_t end;
	size_t true_size;
	
	alias length = true_size;
	
	this(size_t size) {
		this.buffer.length = size;
	}
	
	ubyte opIndex(size_t i) {
		if (i > this.length) {
			throw new Exception("Out of bounds window buffer access");
		}
		
		return this.buffer[(this.end + i) % $];
	}
	
	size_t opDollar!0() {
		return this.true_size;
	}
	
	void append(ubyte d) {
		if (this.buffer.length == this.true_size) {
			this.buffer[this.end] = d;
			this.end = (this.end + 1) % this.buffer.length;
		}
		else {
			this.buffer[$ - 1] = d;
			this.true_size++;
		}
	}
}

ulong djb2(size_t size)(ubyte[size] content) {
	ulong hash = 5381;
	
	foreach (ubyte c in content) {
		hash = ((hash << 5) + hash) ^ c;
	}
	
	return hash;
}

struct HashTable {
	long[][] entries;
	
	this(size_t htsize, size_t htdepth) {
		this.entries.length = htsize;
		
		for (size_t i = 0; i < this.entries.length; i++) {
			this.entries[i].length = htdepth;
			
			for (size_t j = 0; j < this.entries[i].length; i++) {
				this.entries[i][j] = -1;
			}
		}
	}
	
	void insert(size_t hash, long ptr) {
		long prev = ptr;
		
		for (size_t i = 0; i < this.entries[hash % this.entries.length]; i++) {
			long tmp = this.entries[hash % this.entries.length][0];
			this.entries[hash % this.entries.length][0] = prev;
			prev = tmp;
		}
	}
	
	long[] getPossibleMatches(size_t hash) {
		return this.entries[hash % this.entries.length];
	}
}

size_t match_size(WindowBuffer *wb, size_t a, size_t b, size_t max_size) {
	for (size_t i = 0; i < max_size; i++) {
		if (wb[a + i] != wb[b + i]) {
			return i;
		}
	}
	
	return max_size;
}

struct Match {
	size_t length;
	size_t offset;
	
	this(size_t l, size_t o) {
		this.length = l;
		this.offset = o;
	}
}

Match find_match(WindowBuffer *wb, immutable size_t lhsize) {
	immutable size_t wsize = wb.length - lhsize;
	
	size_t best_offset = 0;
	size_t best_len = 0;
	
	for (size_t i = wsize; i > 0; i--) {
		const size_t match_size = match_size(wb, wsize, i, lhsize);
		
		if (match_size > best_len) {
			best_len = match_size;
			best_offset = i;
		}
	}
	
	return Match(best_len, best_offset);
}

void compress(InputStream *input, OutputStream *output, size_t window_size, size_t lookahead_size) {
	WindowBuffer wb(window_size + lookahead_size);
	ubyte[] literals;
	
	// Initial lookahead stuff
	for (size_t i = 0; i < lookahead_size && !input.eof(); i++) {
		wb.append(input.read());
	}
	
	// Start finding matches
	for (size_t i = 0; !input.eof(); i++) {
		wb.append(input.read());
		
		Match m = find_match(&wb, lookahead_size);
		
		if (m.length >= 3) {
			
		}
		else {
			literals.length += 1;
			literals[$-1] = wb[$-lookahead_size];
		}
	}
}

void main() {
	
}
