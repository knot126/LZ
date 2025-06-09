# LZ compressors

Trying to implement data compression :3

* `1_lz` - i think this one suffered from too much abstraction
* `lz2` - attempt at rewriting `1_lz`, it also sucks
* `lz_pico` - implements a in-memory LZ77 like compressor which alternates between constants and matches; produces an optimal (for the format used) compression
* `puff` - not really done, streaming byte aligned LZSS style compressor
