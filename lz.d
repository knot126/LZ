struct InputStream {
	byte[] buffer;
	size_t position;
	
	this(byte[] buffer) {
		this.position = 0;
		this.buffer = buffer;
	}
	
	byte read() {
		return this.buffer[this.position++];
	}
}

struct OutputStream {
	byte[] buffer;
	
	void write(byte b) {
		this.buffer.length++;
		this.buffer[$ - 1] = b;
	}
	
	byte[] getData() {
		return this.buffer;
	}
}

struct SearchWindow {
	byte[] window;
	byte[][int] hash_table;
	size_t max_size;
	
	this(size_t max_size) {
		
	}
	
	void newByte(byte b) {
		
	}
}

void main() {
	
}
