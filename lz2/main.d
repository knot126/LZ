import std.stdio;
import std.string;
import lz;

size_t Read(void *context, ubyte *data, size_t size) {
	return fread(data, 1, size, cast(FILE *) context);
}

size_t Write(void *context, ubyte *data, size_t size) {
	return fwrite(data, 1, size, cast(FILE *) context);
}

enum BackSize = 0x10000;
enum ForwardSize = 0x1000;

void main(string[] args) {
	FILE *input_file = fopen(toStringz(args[1]), toStringz("rb"));
	FILE *output_file = fopen(toStringz(args[2]), toStringz("wb"));
	
	if (!input_file || !output_file) {
		writeln("Failed to open file");
		return;
	}
	
	InputStream input = new InputStream(&Read, cast(void *) input_file);
	OutputStream output = new OutputStream(&Write, cast(void *) output_file);
	
	compress(input, output, BackSize, ForwardSize);
	
	fclose(input_file);
	fclose(output_file);
}
