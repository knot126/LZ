enum EndOfFile = 0x100;
enum MinMatch = 4;

class InputStream {
	size_t function(void *context, ubyte *data, size_t size) _read;
	void *context;
	bool atEof;
	
	this(size_t function(void *, ubyte *, size_t) f, void *context) {
		this._read = f;
		this.context = context;
	}
	
	size_t read(ubyte *data, size_t size) {
		size_t s = this._read(this.context, data, size);
		
		this.atEof = (s == 0);
		
		return s;
	}
	
	bool eof() {
		return this.atEof;
	}
	
	ushort readByte() {
		ubyte b;
		
		if (this.read(&b, 1) != 1) {
			if (this.eof()) {
				return EndOfFile;
			}
			else {
				throw new Exception("readByte failed");
			}
		}
		
		return b;
	}
}

class OutputStream {
	size_t function(void *context, ubyte *data, size_t size) _write;
	void *context;
	
	this(size_t function(void *, ubyte *, size_t) f, void *context) {
		this._write = f;
		this.context = context;
	}
	
	size_t write(ubyte *data, size_t size) {
		return this._write(this.context, data, size);
	}
	
	void writeByte(ubyte b) {
		if (!this.write(&b, 1)) {
			throw new Exception("writeByte failed");
		}
	}
	
	void writeShort(ushort b) {
		ubyte[2] sh = [cast(ubyte) (b << 8), cast(ubyte) (b & 0xff)];
		
		if (this.write(sh.ptr, 2) != 2) {
			throw new Exception("writeShort failed");
		}
	}
}

class CyclicBuffer {
	ubyte[] buffer;
	public size_t extent;
	alias length = extent;
	
	this(size_t allocated_size) {
		this.buffer.length = allocated_size;
		this.extent = 0;
	}
	
	ubyte opIndex(size_t i) {
		return this.buffer[((extent < buffer.length) ? i : extent + i) % this.buffer.length];
	}
	
	void push(ubyte b) {
		this.buffer[this.extent % $] = b;
		this.extent++;
	}
}

size_t match_length(CyclicBuffer buffer, size_t first_index, size_t second_index, size_t max_size) {
	for (size_t i = 0; i < max_size; i++) {
		if (buffer[first_index + i] != buffer[second_index + i]) {
			return i;
		}
	}
	
	return max_size;
}

struct MatchInfo {
	size_t dist;
	size_t size;
	
	this(size_t dist, size_t size) {
		this.dist = dist;
		this.size = size;
	}
}

MatchInfo find_match(CyclicBuffer buffer, size_t forward_start, size_t forward_size) {
	size_t match_dist = 0;
	size_t match_size = 0;
	
	for (size_t i = forward_start; i;) {
		i--;
		size_t cand_size = match_length(buffer, i, forward_start, forward_size);
		
		if (cand_size > match_size) {
			match_dist = forward_start - i;
			match_size = cand_size;
		}
	}
	
	return MatchInfo(match_dist, match_size);
}

void write_literals(CyclicBuffer buffer, OutputStream output, size_t start, size_t end) {
	for (size_t i = start; i < end; i++) {
		output.writeByte(buffer[i]);
	}
}

void copy(InputStream input, OutputStream output) {
	while (true) {
		ushort b = input.readByte();
		
		if (b == EndOfFile) {
			break;
		}
		
		output.writeByte(cast(ubyte) b);
	}
}

void write_ext_num(OutputStream output, size_t num) {
	if (num > 15) {
		size_t t = num - 15;
		
		while (true) {
			immutable size_t z = (t > 255) ? 255 : t;
			output.writeByte(cast(ubyte) z);
			t -= z;
			
			if (t == 0) {
				break;
			}
		}
	}
}

void compress(InputStream input, OutputStream output, size_t back_size, size_t forward_target_size) {
	CyclicBuffer buffer = new CyclicBuffer(back_size + forward_target_size);
	
	size_t forward_size = 0;
	size_t literal_size = 0;
	
	// Queue in `forward_size` bytes
	for (size_t i = 0; i < forward_target_size; i++) {
		ushort b = input.readByte();
		
		if (b == EndOfFile) {
			break;
		}
		else {
			forward_size++;
		}
		
		buffer.push(cast(ubyte) b);
	}
	
	size_t literal_size = 0;
	size_t split_point = 0;
	
	while (true) {
		MatchInfo info = find_match(buffer, (back_size > buffer.length ? ));
		
		if (info.size > MinMatch) {
			// Control byte
			ubyte ctl = cast(ubyte) (((literal_size > 14 ? 15 : literal_size) << 4) | (info.size > 14 ? 15 : info.size));
			output.writeByte(ctl);
			
			// Rest of literal length
			write_ext_num(output, literal_size);
			
			// Flush literal
			write_literals(buffer, output, back_size - literal_size, back_size);
			literal_size = 0;
			
			// Rest of match size
			write_ext_num(output, info.size);
			
			// Match offset
			output.writeShort(cast(ubyte) info.dist);
		}
		else {
			literal_size++;
			
			// Dump literals in buffer when we're out of buffer space
			if (literal_size >= back_size) {
				output.writeByte(cast(ubyte) (((literal_size > 14) ? 15 : literal_size) << 4)); // Unless the lookback buffer is really small this is an okay assumption to make
				write_ext_num(output, literal_size);
				write_literals(buffer, output, back_size - literal_size, back_size);
				output.writeShort(0);
				literal_size = 0;
			}
		}
		
		split_point++;
		
		// Read another byte
		ushort b = input.readByte();
		
		// Leave compression loop and move on to finialisation tasks
		if (b == EndOfFile) {
			break;
		}
		
		buffer.push(cast(ubyte) b);
	}
}
