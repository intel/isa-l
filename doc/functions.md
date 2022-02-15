# ISA-L Function Overview

ISA-L is logically broken into mostly independent units based on the source
directories of the same name.
- erasure_codes
- crc
- raid
- mem
- igzip

The library can also be built with subsets of available units. For example
`$ make -f Makefile.unx units=crc` will only build a library with crc
functions.

## ISA-L Functions

### Erasure Code Functions

Functions pertaining to erasure codes implement a general Reed-Solomon type
encoding for blocks of data to protect against erasure of whole blocks.
Individual operations can be described in terms of arithmetic in the Galois
finite field GF(2^8) with the particular field-defining primitive or reducing
polynomial \f$ x^8 + x^4 + x^3 + x^2 + 1 \f$ (0x1d).

For example, the function ec_encode_data() will generate a set of parity blocks
\f$P_i\f$ from the set of k source blocks \f$D_i\f$ and arbitrary encoding
coefficients \f$a_{i,j}\f$ where each byte in P is calculated from sources as:

\f[ P_i = \sum_{j=1}^k a_{i,j} \cdot D_j \f]

where addition and multiplication \f$\cdot\f$ is defined in GF(2^8).  Since any
arbitrary set of coefficients \f$a_{i,j}\f$ can be supplied, the same
fundamental function can be used for encoding blocks or decoding from blocks in
erasure.

#### EC Usage

Various examples are available in examples/ec and unit tests in `erasure_code`
to show an encode and decode (re-hydrate) cycle or partial update operation. As
seen in [ec example] the process starts with picking an
encode matrix, parameters k (source blocks) and m (total parity + source
blocks), and expanding the necessary coefficients.

~~~c
	// Initialize g_tbls from encode matrix
	ec_init_tables(k, p, &encode_matrix[k * k], g_tbls);
~~~

In the example, a symmetric encode matrix is used where only the coefficients
describing the parity blocks are used for encode and the upper matrix is
initialized as an identity to simplify generation of the corresponding decode
matrix. Next the parity for all (m - k) blocks are calculated at once.

~~~c
	// Generate EC parity blocks from sources
	ec_encode_data(len, k, p, g_tbls, frag_ptrs, &frag_ptrs[k]);
~~~

### RAID Functions

Functions in the RAID section calculate and operate on XOR and P+Q parity found
in common RAID implementations.  The mathematics of RAID are based on Galois
finite-field arithmetic to find one or two parity bytes for each byte in N
sources such that single or dual disk failures (one or two erasures) can be
corrected.  For RAID5, a block of parity is calculated by the xor across the N
source arrays.  Each parity byte is calculated from N sources by:

\f[ P = D_0 + D_1 + ... + D_{N-1} \f]

where \f$D_n\f$ are elements across each source array [0-(N-1)] and + is the
bit-wise exclusive or (xor) operation.  Elements in GF(2^8) are implemented as
bytes.

For RAID6, two parity bytes P and Q are calculated from the source array.  P is
calculated as in RAID5 and Q is calculated using the generator g as:

\f[ Q = g^0 D_0 + g^1 D_1 + g^2 D_2 + ... + g^{N-1} D_{N-1} \f]

where g is chosen as {2}, the second field element.  Multiplication and the
field are defined using the primitive polynomial \f$ x^8 + x^4 + x^3 + x^2 + 1 \f$
(0x1d).

#### RAID Usage

RAID function usage is similar to erasure code except no coefficient expansion
step is necessary. As seen in [raid example] the xor_gen() and xor_check()
functions are used to generate and check parity.

### CRC Functions

Functions in the CRC section include fast implementations of cyclic redundancy
check using specialized instructions such as PCLMULQDQ, carry-less
multiplication.  Generally, a CRC is the remainder in binary division of a
message and a CRC polynomial in GF(2).

\f[ CRC(M(x)) = x^{deg(P(x))} \cdot M(x) \, mod \, P(x) \f]

CRC is used in many storage applications to ensure integrity of data by
appending the CRC to a message.  Various standards choose the polynomial P and
may vary by initial seeding value, bit reversal and inverting the result and
seed.

#### CRC Usage

CRC functions have a simple interface such as in [crc example].

~~~c
	crc64_checksum = crc64_ecma_refl(crc64_checksum, inbuf, avail_in);
~~~

Updates with new buffers are possible with subsequent calls. No extra finalize
step is necessary.

### Compress/Inflate Functions

Functions in the igzip unit perform fast, loss-less data compression and
decompression within the [deflate](https://www.ietf.org/rfc/rfc1951.txt),
[zlib](https://www.ietf.org/rfc/rfc1950.txt), and
[gzip](https://www.ietf.org/rfc/rfc1952.txt) binary standards. Functions for
stream based (data pieces at a time) and stateless (data all at once) are
available as well as multiple parameters to change the speed vs. compression
ratio or other features.  In addition, there are functions to fine tune
compression by pre-computing static Huffman tables and setting for subsequent
compression runs, parsing compression headers and other specific tasks to give
more control.

#### Compress/Inflate Usage

The interface for compression and decompression functions is similar to zlib,
zstd and others where a context structure keeps parameters and internal state to
render from an input buffer to an output buffer.  I/O buffer pointers and size
are often the only required settings.  ISA-L, unlike zlib and others, does not
allocate new memory and must be done by the user explicitly when required (level
1 and above).  This gives the user more flexibility to when dynamic memory is
allocated and reused. The minimum code for starting a compression is just
allocating a stream structure and initializing it.  This can be done just once
for multiple compression runs.

~~~c
	struct isal_zstream stream;
	isal_deflate_init(&stream);
~~~

Using level 1 compression and above requires an additional, initial allocation
for an internal intermediate buffer.  Suggested sizes are defined in external
headers.

~~~c
	stream.level = 1;
	stream.level_buf = malloc(ISAL_DEF_LVL1_DEFAULT);
	stream.level_buf_size = ISAL_DEF_LVL1_DEFAULT;
~~~

After init, subsequent, multiple compression runs can be performed by supplying
(or re-using) I/O buffers.

~~~c
	stream.next_in = inbuf;
	stream->next_out = outbuf;
	stream->avail_in = inbuf_size;
	stream->avail_out = outbuf_size;

	isal_deflate(stream);
~~~

See [igzip example] for a simple example program or review the perf or check
tests for more.

**igzip**: ISA-L also provides a user program *igzip* to compress and decompress
files.  Optionally igzip can be compiled with multi-threaded compression.  See
`man igzip` for details.

## General Library Features

### Multi-Binary Dispatchers

Multibinary support is available for all units in ISA-L.  With multibinary
support functions, an appropriate version is selected at first run and can be
called instead of architecture-specific versions. This allows users to deploy a
single binary with multiple function versions and choose at run time based on
platform features. All functions also have base functions, written in portable
C, which the multibinary function will call if none of the required instruction
sets are enabled.

### Included Tests and Utilities

ISA-L source [repo] includes unit tests, performance tests and other utilities.

Examples:
- [ec example]
- [raid example]
- [crc example]
- [igzip example]

---

[repo]: https://github.com/intel/isa-l
[ec example]: https://github.com/intel/isa-l/blob/master/examples/ec/ec_simple_example.c
[raid example]: https://github.com/intel/isa-l/blob/master/raid/xor_example.c
[crc example]: https://github.com/intel/isa-l/blob/master/crc/crc64_example.c
[igzip example]: https://github.com/intel/isa-l/blob/master/igzip/igzip_example.c
