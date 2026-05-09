# Homework 1 - CSE 320 - Spring 2026
#### Professor Dan Benz

Readme version 1.0, see Piazza for any updates

### **Due Date: Sunday February 15th @ 11:59 **

**Read the entire doc before you start**

## Introduction

In this assignment, you will write a program to interact with PNG image files.
The specification for PNG can be found at: [https://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html](https://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html).
The goal of this homework is to familiarize yourself with C programming,
with a focus on input/output, strings in C, bitwise manipulations,
and the use of pointers. Writing C code to parse PNGs,
poses a realistic and challenging exercise involving these topics.

> :nerd_face: Reference for pointers: [https://beej.us/guide/bgc/html/#pointers](https://beej.us/guide/bgc/html/#pointers).

For all assignments in this course, you **MUST NOT** put any of the functions
that you write into the `main.c` file.  The file `main.c` **MUST ONLY** contain
`#include`s, local `#define`s and the `main` function.  The reason for this
restriction has to do with our use of the Criterion library to test your code
(this is discussed further at the end of this document).
Beyond this, you may have as many or as few additional `.c` files in the `src`
directory as you wish.  Also, you may declare as many or as few headers as you wish.
Note, however, that header and `.c` files distributed with the assignment base code
often contain a comment at the beginning which states that they are not to be
modified.  **PLEASE** take note of these comments and do not modify any such files,
as they will be replaced by the original versions during grading.

For all the assignments in this course, you are expected to write your own code;
you are not permitted to submit source code that you did not write yourself,
except for any code that is distributed as part of the basecode repository,
or code for which written permission has explicitly been given.
You are also not permitted to use library functions other than those provided
as part of the official Linux Mint software environment for the course.
In addition, for this particular assignment, there are some further restrictions
on what libraries you are allowed to use.

- Allowed:
  - `glibc` (GNU standard C library), including dynamic memory allocation
     (`malloc`, `calloc`) and standard I/O library functions (`fgetc`, *etc.*).
  - `zlib` library for data compression and decompression.
  - The sample code in the Appendix of the PNG website (CRC)

- Disallowed:
  - Binary libraries other than `glibc` and `zlib`.

Submitting or using code that is not allowed will quite likely result in your
receiving a zero for this assignment and could potentially expose you to
charges of Academic Dishonesty.

> :scream:  Many students will find this to be a relatively challenging project.
> It is extremely unlikely that anyone will be able to just read through the handout
> materials and then sit down and write the code.  You should certainly expect to have to
> experiment with the data files you have been provided, in order to develop and
> validate your understanding of the data formats discussed here.  This experimentation
> will take time, so you need get started on it as soon as possible and allocate
> time regularly over the next few weeks in order to be able to have a working
> implementation for at least part of the specification by the time the due date arrives.


# Getting Started

This semester we will be using codegrade to submit and grade your projects.
If you are reading this document then you should have successfully used codegrade to
create your own git repo of my template linked to codegrade. Codegrade will rerun the
basic test scripts every time you do a git push. 

> :scream: The tests that will be used to determine your grade are NOT the same
> as the tests you will see in this template repository or in your view of codegrade.
> I provide these tests to help you check basic functionality, ensure that your print 
> formatting matches mine, and show you if your codegrade submission compiles. 
> The real test cases will be run after the submission deadline. 
> I STRONGLY encourage you to write your own additional tests to ensure that your code handles 
> all of the required functionality.

Here is the structure of the base code:

<pre>
.
├── include
│   ├── png_chunks.h
│   ├── debug.h
│   ├── png_crc.h
│   ├── png_overlay.h
│   ├── png_steg.h
│   ├── global.h
│   ├── png_reader.h
│   └── util.h
├── Makefile
├── src
│   ├── main.c
│   ├── png_chunks.c
│   ├── png_crc.c
│   ├── png_reader.c
│   ├── png_steg.c
│   ├── png_overlay.c
│   └── util.c
└── tests
    ├── test_chunks.c
    ├── test_crc.c
    ├── test_overlay.c
    ├── test_reader.c
    ├── test_steg.c
    └── data
        └── [test files]
</pre>

- The `.gitignore` file is a file that tells `git` to ignore files with names
matching specified patterns, so that they don't accidentally end up getting
committed to the repository.

> :scream:  The CI testing is for your own information; it does not directly have
> anything to do with assignment grading.
> The purpose of the tests are to alert you to possible problems with your code; 
> if you see that testing has failed it is worth investigating why that has happened.  

- The `Makefile` is a configuration file for the `make` build utility, which is what
you should use to compile your code.  In brief, `make` or `make all` will compile
anything that needs to be, `make debug` does the same except that it compiles the code
with options suitable for debugging, and `make clean` removes files that resulted from
a previous compilation.  These "targets" can be combined; for example, you would use
`make clean debug` to ensure a complete clean and rebuild of everything for debugging.

- The `include` directory contains C header files (with extension `.h`) that are used
by the code.  Note that these files often contain `DO NOT MODIFY` instructions at the beginning.
You should observe these notices carefully where they appear.

- The `src` directory contains C source files (with extension `.c`).

- The `tests` directory contains C source code (and sometimes headers and other files)
that are used by the Criterion tests.

## A Note about Program Output

What a program does and does not print is VERY important.
In the UNIX world stringing together programs with piping and scripting is
commonplace. Although combining programs in this way is extremely powerful, it
means that each program must not print extraneous output. For example, you would
expect `ls` to output a list of files in a directory and nothing else.
Similarly, your program must follow the specifications for normal operation.
One part of our grading of this assignment will be to check whether your program
produces EXACTLY the specified output.  If your program produces output that deviates
from the specifications, even in a minor way, or if it produces extraneous output
that was not part of the specifications, it will adversely impact your grade
in a significant way, so pay close attention.

> :scream: Use the debug macro `debug` for any other program output or messages you many need
> while coding (e.g. debugging output).

# Part 1: Program Operation and Argument Validation

Your program will be a command-line utility which when invoked will open a PNG file
and process queries specified as command-line arguments.
The results of the query processing will be output to the standard output.  
In case of successful execution, your program should terminate with exit status `EXIT_SUCCESS` (0).  
In case of any error, your program should print a one-line message describing the error to the standard error output,
and your program should terminate with exit status `EXIT_FAILURE` (1).
Unless the program has been compiled for debugging (using `make debug`),
in a successful run that exits with `EXIT_SUCCESS` no unspecified output may be produced
by the program.

The specific usage scenarios for your program are as follows:

<pre>
Usage: bin/png -f png_file [options]
Options:
  -f png_file        Input PNG file (required)
  -h                    Print this help message
  -s                    Print chunk summary
  -p                    Print palette summary
  -i                    Print IHDR fields
  -e message -o out_file Encode message and write to output file
  -d                    Decode and print hidden message
  -m file2 -o out_file [-w width] [-g height]  Overlay file2 (smaller) over input and write to output
</pre>

> :scream: A `PRINT_USAGE` macro has already been provided for you in the `global.h`
> header file.  You should use this macro to print the help message.

Note that the square brackets in the description are not part of what the user types,
but rather are used to indicate that the enclosed arguments are optional.
The `-f <png_file>` flag is **required** and must be provided for all operations except `-h`.

The program uses a two-pass argument processing approach:

**First Pass**: The program traverses the command-line arguments to:
- Check if `-h` is specified (if so, print usage and exit with `EXIT_SUCCESS`)
- Locate the `-f` flag and extract the associated filename
- Validate that `-f` is present and has a filename argument

If `-h` is specified, the program prints the usage message to standard output
and exits with status `EXIT_SUCCESS`, ignoring all other arguments.

If `-f` is not specified or is missing its filename argument, the program prints
an error message to standard error and exits with `EXIT_FAILURE`.

**Second Pass**: After successfully validating arguments in the first pass,
the program traverses the command-line arguments again to process the requested operations.
The results of these operations are printed to standard output in the specific formats
described below.  The meaning of the various option flags are as follows:

- `-s` (Summary): The program prints a chunk summary for the PNG file, listing all chunks
  with their type, length, and CRC validity status.  The output format is:
  ```
  Chunk Summary for <filename>:
    Chunk 0: Type=IHDR, Length=13, CRC=valid
    Chunk 1: Type=PLTE, Length=768, CRC=valid
    Chunk 2: Type=IDAT, Length=12345, CRC=valid
    ...
    Chunk N: Type=IEND, Length=0, CRC=valid
  ```

- `-p` (Palette): The program prints a palette summary for the PNG file (only valid for
  color type 3 images).  The output format is:
  ```
  Palette Summary for <filename>:
    Number of colors: <count>
    Color 0: RGB(<r>, <g>, <b>)
    Color 1: RGB(<r>, <g>, <b>)
    ...
  ```
  If the PNG file does not contain a PLTE chunk or parsing fails, an error message
  is printed to standard error.

- `-i` (IHDR): The program prints the IHDR fields for the PNG file.  The output format is:
  ```
  IHDR Fields for <filename>:
    Width: <width>
    Height: <height>
    Bit Depth: <bit_depth>
    Color Type: <color_type> (<description>)
    Compression: <compression>
    Filter: <filter>
    Interlace: <interlace> (<description>)
  ```
  The color type description will be one of: "(Grayscale)", "(RGB)", "(Palette)",
  "(Grayscale with Alpha)", or "(RGB with Alpha)".
  The interlace description will be "(None)" for 0 or "(Adam7)" for 1.


- `-e <message> -o <out>` (Encode): The program encodes a secret message into the least
  significant bits of the PNG image data and writes the result to the output file.
  The message must be provided immediately after `-e`, and `-o` must be followed by
  the output filename.  If encoding is successful, the program prints:
  ```
  Message encoded successfully to <out>
  ```
  If encoding fails (e.g., message too long, file error), an error message is printed
  to standard error and the program exits with `EXIT_FAILURE`.

- `-d` (Decode): The program extracts and prints a hidden message from the PNG image data.
  The output format is:
  ```
  Hidden message: <extracted_message>
  ```
  If extraction fails, an error message is printed to standard error and the program
  exits with `EXIT_FAILURE`.

- `-m <file2> -o <out> [-w <width>] [-g <height>]` (Overlay): The program overlays a smaller
  PNG image (`file2`) onto the input PNG image at the specified coordinates and writes
  the result to the output file.  The `-w` flag specifies the X offset (width/column),
  and the `-g` flag specifies the Y offset (height/row).  If not specified, both offsets
  default to 0.  The `-w` and `-g` flags are optional and may appear in any order after
  the `-o <out>` argument.  If overlay is successful, the program prints:
  ```
  Overlay completed successfully to <out>
  ```
  If overlay fails, an error message is printed to standard error and the program
  exits with `EXIT_FAILURE`.

Multiple operations may be specified in a single invocation, and they will be processed
in the order they appear on the command line.  For example:
```
bin/png -f image.png -s -i -p
```
will print the chunk summary, IHDR fields, and palette summary in that order.

If any unrecognized options or options with missing arguments are encountered during
the argument processing phase, then the program should print a single line error message
to stderr that describes the issue (using the appropriate error macro from `global.h`)
and then exit with status `EXIT_FAILURE`.

> :nerd_face: Reference for command line arguments: [https://beej.us/guide/bgc/html/#command-line-arguments](https://beej.us/guide/bgc/html/#command-line-arguments).

The `main` function in `main.c` implements the two-pass argument processing approach:

I have provided macros in `global.h` for all output formatting to ensure consistency.

# Part 2:

*PNG files* are a graphics file format used for lossless data compression.
PNG files are constructed out of multiple chunks to encode different data
about the image being encoded. While there are many types of chunks that can
be used in a valid PNG file, we will only concern ourselves with the four
critical chunk types which are enough to render a PNG file.

The first 8 bytes of all PNG files are the same. It consists of the following values:
0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A. These bytes each mean something (but that isn't
important for your project).

All chunks in a PNG file consist of 4 fields:
1. **Length** (4 bytes): The number of bytes in the data field, represented as a big-endian unsigned 32-bit integer.
2. **Type** (4 bytes): A 4-byte ASCII string identifying the chunk type (e.g., "IHDR", "PLTE", "IDAT", "IEND").
3. **Data** (variable length): The actual chunk data, whose length is specified by the Length field.
4. **CRC** (4 bytes): A cyclic redundancy check computed over the type and data of the chunk, represented as a big-endian unsigned 32-bit integer.

The CRC is a cyclic redundancy check computed over the type and data of the chunk.
I would strongly recommend reading the sample CRC code included in the PNG website
referenced throughout the document.

IHDR is the header chunk which contains the meta-data for the image.
This includes image height and width (4 bytes each), the number of bits per pixel (1 byte),
the way colors are represented (1 byte), and three flags that are useful for compression:
compression method, filter method, and interlace method (1 byte each).

The PLTE chunk stores a palette of colors. This is another trick for compression,
instead of needing to store the full Red-Blue-Green value for each pixel PNG files
can include the PLTE chunk with a list of up to 256 colors used in the image. This allows 
each pixel to encode 24-bits of color (8-bits per channel) in an 8-bit index.
This chunk is only used in color-type 3 images.

The IDAT chunk stores the pixel data for the image. A PNG files is allowed to have as many
IDAT chunks as it needs to fully express the image. The IDAT chunk is stored in a compressed
data format that can be decompressed with the inflate function of the zlib library.

The final chunk in any PNG file is the IEND chunk. It contains no data and is just used to indicate
the end of the file.

I recommend the following reference for PNG:
[https://www.libpng.org/pub/png/spec/1.2/](https://www.libpng.org/pub/png/spec/1.2/) and in particular the page
[https://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html#C.IHDR](https://www.libpng.org/pub/png/spec/1.2/PNG-Chunks.html#C.IHDR),
which gives a detailed description of the PNG chunks and format
and you should consider it to be your main reference.
You should certainly refer to the website mentioned above for any details that I don't happen
to cover completely.

That's pretty much all there is to it.  What you have to do for this part of the
assignment is to implement functions for reading and traversing a PNG
from a C input stream.  Specifically, you have to implement functions having the
following prototypes:

## PNG Data Structures

The definitions of the types `png_chunk_t`, `png_ihdr_t`, and `png_color_t` are
already given for you in the header file `include/png_chunks.h`.  These structures
represent the core data types used throughout the PNG library:

```c
typedef struct {
    uint32_t width;      // Image width in pixels (big-endian)
    uint32_t height;     // Image height in pixels (big-endian)
    uint8_t  bit_depth;  // Bits per sample (1, 2, 4, 8, or 16)
    uint8_t  color_type; // Color representation (0, 2, 3, 4, or 6)
    uint8_t  compression;// Compression method (must be 0)
    uint8_t  filter;     // Filter method (must be 0)
    uint8_t  interlace;  // Interlace method (0 = none, 1 = Adam7)
} png_ihdr_t;

typedef struct {
    uint8_t r;  // Red component (0-255)
    uint8_t g;  // Green component (0-255)
    uint8_t b;  // Blue component (0-255)
} png_color_t;

typedef struct {
    uint32_t length;  // Chunk data length in bytes (big-endian)
    char     type[5];  // Chunk type (4 bytes, null-terminated)
    uint8_t *data;     // Pointer to chunk data (dynamically allocated)
    uint32_t crc;      // CRC-32 checksum (big-endian)
} png_chunk_t;
```

## PNG Chunk Parsing Functions (`png_chunks.c`)

The `png_chunks.c` module provides functions for parsing PNG chunk data into
structured formats:

### `int png_parse_ihdr(const png_chunk_t *chunk, png_ihdr_t *out)`

Parses IHDR chunk data into a structured format. The chunk must be an IHDR chunk
with length exactly 13 bytes. The function extracts the width, height, bit depth,
color type, compression method, filter method, and interlace method from the
chunk data. All multi-byte integers are stored in big-endian format.

**Parameters:**
- `chunk`: Pointer to a PNG chunk that must be an IHDR chunk with length 13
- `out`: Pointer to output structure where parsed data will be written

**Returns:**
- `0` on success
- `-1` on error (NULL pointers, invalid chunk length, or invalid chunk data)

### `int png_parse_plte(const png_chunk_t *chunk, png_color_t **out_colors, size_t *out_count)`

Parses PLTE chunk data into an array of color structures. The chunk must be a
PLTE chunk with length that is a multiple of 3 (each color is 3 bytes: R, G, B).
The function allocates memory for the color array, which the caller must free.

**Parameters:**
- `chunk`: Pointer to a PNG chunk that must be a PLTE chunk
- `out_colors`: Pointer to output pointer where allocated color array will be written
- `out_count`: Pointer to output variable where color count will be written

**Returns:**
- `0` on success
- `-1` on error (NULL pointers, invalid length, palette too large, or allocation failure)

**Memory Management:** The caller is responsible for freeing the allocated color array
using `free()`.

## PNG Reader Functions (`png_reader.c`)

The `png_reader.c` module provides functions for reading PNG files and extracting
chunk information:

### `FILE *png_open(const char *path)`

Opens a PNG file and validates its signature. The PNG signature is the 8-byte
sequence: `0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A`. If the signature is valid,
the file pointer is returned positioned immediately after the signature, ready to
read the first chunk.

**Parameters:**
- `path`: File path to open

**Returns:**
- `FILE*` pointer on success (positioned after signature)
- `NULL` on error (file not found, invalid signature, etc.)

### `int png_read_chunk(FILE *fp, png_chunk_t *out)`

Reads the next PNG chunk from the file. This function reads the chunk length,
type, data, and CRC. It validates the CRC by computing it over the chunk type
and data, and comparing it to the stored CRC. If validation fails, the function
frees any allocated memory and returns an error.

**Parameters:**
- `fp`: File pointer (must be positioned at start of chunk length field)
- `out`: Pointer to chunk structure where data will be written

**Returns:**
- `0` on success
- `-1` on error (EOF, read error, CRC mismatch, memory allocation failure)

**Memory Management:** The chunk data is dynamically allocated. The caller must
free it using `png_free_chunk()`.

### `void png_free_chunk(png_chunk_t *chunk)`

Frees memory allocated inside a chunk structure. This function safely handles
NULL pointers and sets the data pointer to NULL after freeing.

**Parameters:**
- `chunk`: Pointer to chunk structure to free

### `int png_extract_ihdr(FILE *fp, png_ihdr_t *out)`

Extracts and parses the IHDR chunk from a PNG file. This is a convenience function
that reads the first chunk, validates it is IHDR, parses it, and frees the chunk.

**Parameters:**
- `fp`: File pointer (must be positioned after PNG signature)
- `out`: Pointer to IHDR structure where parsed data will be written

**Returns:**
- `0` on success
- `-1` on error (chunk not found, invalid chunk, parse error)

### `int png_extract_plte(FILE *fp, png_color_t **out_colors, size_t *out_count)`

Extracts and parses the first PLTE chunk from a PNG file. This function reads
chunks sequentially until it finds a PLTE chunk. It stops and returns an error
if IDAT or IEND is encountered before PLTE (PLTE must come before IDAT according
to the PNG specification).

**Parameters:**
- `fp`: File pointer (must be positioned after PNG signature)
- `out_colors`: Pointer to output pointer where allocated color array will be written
- `out_count`: Pointer to output variable where color count will be written

**Returns:**
- `0` on success
- `-1` on error (PLTE not found, parse error, or IDAT/IEND encountered before PLTE)

**Memory Management:** The caller is responsible for freeing the allocated color array
using `free()`.

### `int png_summary(const char *filename, png_chunk_t **out_summary)`

Reads all chunks from a PNG file and returns a summary array. This function reads
chunks until IEND is encountered, storing only the chunk type, length, and CRC
validity status (not the actual chunk data). This is useful for displaying chunk
information without loading all chunk data into memory.

**Parameters:**
- `filename`: Path to PNG file
- `out_summary`: Pointer to output pointer where allocated summary array will be written

**Returns:**
- `0` on success
- `-1` on error

**Memory Management:** The caller is responsible for freeing the summary array using `free()`.
Note that the summary chunks have `data = NULL` (data is not stored in the summary).

## PNG CRC Functions (`png_crc.c`)

The `png_crc.c` module provides functions for computing CRC-32 checksums used in PNG
chunk validation. PNG uses CRC-32 with the polynomial `0xEDB88320` to ensure data
integrity. The CRC is computed over the chunk type name (4 bytes) concatenated with
the chunk data.

### `uint32_t png_crc(const uint8_t *buf, size_t len)`

Computes the CRC-32 checksum over a buffer of bytes using the PNG-specific polynomial.
This function uses a lookup table for efficient computation. The lookup table is
computed once on first use and reused for subsequent calls.

**Parameters:**
- `buf`: Pointer to the buffer containing the data to compute CRC over
- `len`: Number of bytes in the buffer

**Returns:**
- The 32-bit CRC value

**How to Recreate:**

The CRC-32 algorithm can be implemented as follows:

1. **Initialize a lookup table**: Create a 256-entry table where each entry is computed
   by applying the CRC polynomial `0xEDB88320` to each possible byte value (0-255).
   For each byte value `n`, iterate 8 times: if the least significant bit is set,
   XOR with the polynomial and shift right; otherwise just shift right.

2. **Initialize CRC register**: Start with `0xFFFFFFFF` (all bits set).

3. **Process each byte**: For each byte in the input buffer, XOR the current CRC value
   with the byte, use the result (masked to 8 bits) as an index into the lookup table,
   then XOR the table value with the CRC shifted right by 8 bits.

4. **Finalize**: XOR the final CRC value with `0xFFFFFFFF` to get the result.

If this doesn't make sense to you, look closely at the provided PNG documentation where a more
thorough discussion of how to implement CRC is provided.

## Debugging and Testing Your PNG Code

As test input for your code, I have provided several PNG files in the `tests/data/`
directory. PNG files are binary files that encode image data using a chunk-based
format.
All PNG files start with an 8-byte signature, followed by a sequence of
chunks. Each chunk consists of a 4-byte length field, a 4-byte type identifier,
variable-length data, and a 4-byte CRC checksum.

All multi-byte integer values in PNG files (length, width, height, CRC) are stored
in "big-endian" format, meaning the most-significant byte comes first. However, the
Intel x64 architecture is a "little-endian" architecture, which means that
multi-byte values are stored in memory with the least-significant byte first.
So, you will have to rearrange the bytes of data in order to properly interpret them
as integers.

This is as good a time as any to introduce you to the `hd` (hex dump) tool for
displaying the content of a binary file.  You can (should) read the manual page
for this, but for now I will just use it to display the initial part of a
PNG file:

```
$ hd tests/data/Large_batman_3.png | less
00000000  89 50 4e 47 0d 0a 1a 0a  00 00 00 0d 49 48 44 52  |.PNG........IHDR|
00000010  00 00 01 40 00 00 01 40  08 03 00 00 00 fa 4e 55  |...@...@......NU|
00000020  e6 00 00 01 f8 50 4c 54  45 47 70 4c 00 00 00 02  |.....PLTEGpL....|
00000030  01 00 05 04 00 06 05 01  0a 08 01 0b 09 01 0c 0b  |................|
00000040  00 0f 0d 01 11 0f 01 12  0f 01 1c 18 02 28 23 02  |.............(#.|
00000050  35 2e 02 45 3c 03 53 49  03 61 55 03 71 62 03 7e  |5..E<.SI.aU.qb.~|
00000060  6e 03 8a 79 03 96 83 03  a1 8c 03 a9 94 03 b1 9b  |n..y............|
00000070  03 b7 a0 03 be a6 02 c3  aa 03 c7 ad 02 c9 af 02  |................|
00000080  cd b3 03 cf b5 02 63 57  03 37 30 02 1d 1a 01 03  |......cW.70.....|
```

> :nerd_face: As the complete output would be really long, I have piped it into the `less`
> utility to allow it to be displayed one screen at a time.
> (You can type `h` at the `less` prompt to get help information on what
> it can do for you.  Type `q` at the prompt to exit `less`.)

The `hd` command displays the output in canonical hex+ASCII format.
The first column in the display is a hexadecimal number giving the offset
(number of bytes) from the beginning of the file of the data that is
displayed in that row.
Following the offset is a display of sixteen hexadecimal bytes, corresponding
to the first sixteen bytes of the file.
The rightmost column displays the same sixteen bytes interpreted as ASCII characters,
with non-printable characters shown as dots.

From this display we can see that the first eight bytes are:
`0x89`, `0x50`, `0x4E`, `0x47`, `0x0D`, `0x0A`, `0x1A`, `0x0A`. This is the
PNG signature that must appear at the start of every valid PNG file. The signature
serves as a magic number to identify PNG files and includes specific byte values
that help detect file corruption or incorrect file types.

After the 8-byte signature comes the first chunk. The first four bytes of the chunk
(at offset 0x0000000c) are: `0x00`, `0x00`, `0x00`, `0x0D`, which in big-endian format
represents the value `0x0D` in hexadecimal or `13` in decimal.
This is the length of the chunk data that follows.

The next four bytes (at offset 0x00000010) are: `0x49`, `0x48`, `0x44`, `0x52`, which
when interpreted as ASCII characters spell out `IHDR`. This is the chunk type,
indicating this is the Image Header chunk, which contains metadata about the image.

So far we have decoded the following portion of the input file:

```
00000000  89 50 4e 47 0d 0a 1a 0a  00 00 00 0d 49 48 44 52  |.PNG........IHDR|
00000010  00 00 01 40 00 00 01 40  08 03 00 00 00           |...@...@......|
```

The IHDR chunk data consists of exactly 13 bytes (as specified by the length field).
Breaking down these 13 bytes:

- **Width** (4 bytes): `0x00`, `0x00`, `0x01`, `0x40` = `0x00000140` in hex = `320` in decimal
- **Height** (4 bytes): `0x00`, `0x00`, `0x01`, `0x40` = `0x00000140` in hex = `320` in decimal
- **Bit depth** (1 byte): `0x08` = `8` bits per sample
- **Color type** (1 byte): `0x03` = palette (indexed color)
- **Compression method** (1 byte): `0x00` = deflate/inflate compression
- **Filter method** (1 byte): `0x00` = adaptive filtering
- **Interlace method** (1 byte): `0x00` = no interlacing

After the 13 bytes of IHDR data comes the 4-byte CRC checksum: 
`0xFA4E55E6` in hexadecimal. This CRC is computed over the chunk type ("IHDR")
and the chunk data (the 13 bytes we just analyzed).

The next chunk starts immediately after the CRC. Looking at offset 0000040, we see:
- **Length** (4 bytes): `0x000001F8` in hex = `504` in decimal
- **Type** (4 bytes): `0x50` `0x4c` `0x54`  `0x45` = `PLTE` (Palette chunk)

Since this is a color type 3 (palette) image, the PLTE chunk contains the color palette.
The length of 504 bytes means there are 504 / 3 = 168 color entries in the palette.
Each color entry consists of 3 bytes: Red, Green, and Blue values (each 0-255).

The PLTE data starts at offset 0000050. The first few color entries are:
- Color 0: R=`001` (0x01), G=`000` (0x00), B=`005` (0x05) - a very dark blue-green
- Color 1: R=`004` (0x04), G=`000` (0x00), B=`006` (0x06) - another dark color
- Color 2: R=`005` (0x05), G=`001` (0x01), B=`012` (0x0A) - slightly lighter

After all 504 bytes of palette data comes the PLTE CRC: `0x47A07000` in hexadecimal.

The pattern continues: each chunk follows the same structure (length, type, data, CRC).
After PLTE, you would typically find IDAT chunks (containing the compressed image data)
and finally an IEND chunk (which has length 0 and marks the end of the file).

> :relieved: This is one of a few points in this assignment where if for some reason
> you are unable to go on and complete the assignment, you could stop and still collect
> a significant fraction of the grading score.  One type of testing we will do
> on your code is to unit-test the PNG parsing functions.  So if you do a reasonable
> job of implementing functions like `png_read_chunk`, `png_parse_ihdr`, and `png_parse_plte`
> so that they can correctly decode PNG chunks, then you will likely get most of the
> points from these unit tests.

# Part 3: PNG Steganography and Image Overlay

This part of the assignment involves implementing two advanced PNG manipulation
features: steganography (hiding messages in images) and image overlay (combining
multiple images). These functions require working with decompressed image data,
applying PNG filters, and re-compressing the modified data.

## PNG Steganography (`png_steg.c`)

Steganography is the practice of hiding information within other data in a way
that is not easily detectable. In this implementation, we hide secret messages
in PNG images using two different methods depending on the image's color type.
When each pixels data is being represented directly then we can use the LSB of 
the first channel of the pixel data to hide a secret message that would be nearly
undetectable by anyone viewing the image. This raises a problem with color type
3 where each pixel is represented by an index into a lookup palette chunk. A single
change in the LSB of these indices could have a significant and noticeable effect
on the visual representation of the image. I have come up with a clever workaround.
Since the PLTE chunk has no restriction on identical colors appearing more than once
we can store all of the palette values twice and use the two different index values
to represent bits of 0 or 1 for the purpose of hidden information. This would result
in an output image that is indistinguishable from the input.

### High-Level Overview

The steganography system supports both encoding (hiding messages) and decoding
(extracting messages) from PNG images. The implementation works as follows:

1. Open and validate the input PNG file
2. Read and parse the IHDR chunk to determine image properties
3. For palette images (color type 3):
   - Extract the PLTE chunk
   - Build a table of identical-color pairs in the palette
   - If no pairs exist, append duplicate colors to create pairs (up to 256 entries)
   - Create a mapping table: `pair[i] = j` if colors at indices i and j are identical
4. Decompress all IDAT chunks into a single buffer
5. Calculate encoding capacity: width × height pixels
6. Check if the message fits (including null terminator): `(strlen(secret) + 1) * 8` bits
7. Encode the message:
   - For non-palette images: modify the LSB of the first channel of each pixel
   - For palette images: swap between identical-color pair indices (lower index = 0, higher = 1)
8. Re-compress the modified image data using PNG-compatible zlib settings
9. Write output PNG file, preserving all chunks except IDAT (replaced) and PLTE (if modified)

**Technical Details**:
- Only supports 8-bit images (`bit_depth == 8`)
- For non-palette images: encodes one bit per pixel in the LSB of the first (greyscale or red) channel
- For palette images: uses index swapping between identical colors (visually undetectable)
- Filter bytes (first byte of each scanline) are **never modified** during encoding
- Uses the provided `util_deflate_data_png()` for compression (windowBits=15, PNG-compatible)
- Preserves all ancillary chunks from the input file

**Error Conditions**:
- Returns `-1` if any parameter is NULL
- Returns `-1` if image is not 8-bit
- Returns `-1` if message is too long for the image capacity
- Returns `-1` on file I/O errors
- Returns `-1` if palette manipulation fails (e.g., palette full at 256 entries)

### Function: `png_extract_lsb`

**Purpose**: Extracts a hidden message from PNG image data.

**High-Level Algorithm**:
1. Open and validate the input PNG file
2. Read and parse the IHDR chunk
3. For palette images: extract PLTE and build identical-color pair table
4. Decompress all IDAT chunks
5. Extract bits from the image data:
   - For non-palette images: read LSB from first channel of each pixel
   - For palette images: determine bit by checking if index is the "higher" of a pair
6. Reconstruct bytes from bits (8 bits per byte)
7. Stop extraction when null terminator (all zero bits) is found
8. Return the length of the extracted string

**Technical Details**:
- Only supports 8-bit images
- Extracts one bit per pixel (same as encoding)
- For palette images: skips pixels whose index doesn't have a pair
- Filter bytes are **never read** during extraction
- Output buffer must be at least `max_len` bytes (reserves space for null terminator)
- Returns the number of characters extracted (excluding null terminator)

**Error Conditions**:
- Returns `-1` if any parameter is NULL or `max_len == 0`
- Returns `-1` if image is not 8-bit
- Returns `-1` on file I/O errors

## PNG Image Overlay (`png_overlay.c`)

The overlay system provides functionality for pasting a smaller PNG image onto a larger PNG image at specified coordinates. The
implementation handles all PNG color types, including special processing for palette-based images. This will introduce one last 
compression feature of PNG files, the filter. We have ignored the filter byte in all previous functions but to properly overlay
two images we need to understand how filters work.

## Understanding PNG Filters

PNG uses filtering to improve compression efficiency by reducing the entropy (randomness) of image data. The key insight is that neighboring pixels in images are often similar, so storing the *difference* between a pixel and its neighbors typically results in smaller values that compress better.

### Filter Structure

Each scanline in a PNG image has a special structure:
- **Filter byte** (first byte): Indicates which filter type (0-4) was applied to this scanline
- **Filtered data** (remaining bytes): The pixel data after filtering was applied

When reading a PNG file, you must **unfilter** the data to reconstruct the original pixel values. When writing a PNG file, you typically **filter** the data before compression (though you can use filter type 0: None to skip filtering).

### The Five Filter Types

PNG defines five filter types, each using different prediction strategies:

**Type 0 (None)**: No filtering applied. The pixel values are stored as-is. This is the simplest case - no unfiltering operation is needed. When you write out the output file after overlaying the images, you should use this type.

**Type 1 (Sub)**: Predicts each byte using the byte to its left (in the same scanline). 
- For the first `bpp` (bytes per pixel) bytes, there's no left neighbor, so they're stored as-is
- For subsequent bytes, the filter stores: `filtered[i] = original[i] - original[i - bpp]`
- To unfilter: `original[i] = filtered[i] + original[i - bpp]` (modulo 256)

**Type 2 (Up)**: Predicts each byte using the byte directly above it (from the previous scanline).
- For the first scanline, there's no above neighbor, so values are stored as-is
- For subsequent scanlines: `filtered[i] = original[i] - above[i]`
- To unfilter: `original[i] = filtered[i] + above[i]` (modulo 256)

**Type 3 (Average)**: Predicts each byte as the average of the left and above neighbors.
- Calculates: `prediction = (left + above) / 2` (integer division)
- Stores: `filtered[i] = original[i] - prediction`
- To unfilter: `original[i] = filtered[i] + prediction` (modulo 256)
- Edge cases: first `bpp` bytes have no left neighbor (use 0), first scanline has no above neighbor (use 0)

**Type 4 (Paeth)**: Uses a sophisticated predictor that chooses the best neighbor based on a gradient calculation.
- Considers three neighbors: left (`a`), above (`b`), and upper-left (`c`)
- The Paeth predictor algorithm:
  1. Calculate `p = a + b - c` (a linear prediction)
  2. Compute distances from `p` to each neighbor: `pa = |p - a|`, `pb = |p - b|`, `pc = |p - c|`
  3. Choose the neighbor closest to `p` (smallest distance)
- To unfilter: `original[i] = filtered[i] + chosen_neighbor` (modulo 256)
- This filter typically provides the best compression for most images

### Key Concepts for Implementation

**Byte-level operations**: Filters operate on individual bytes, not pixels. For multi-byte pixels (e.g., RGB with 3 bytes per pixel), each color channel is filtered independently. This means the "left neighbor" for a byte is `bpp` bytes to the left, not necessarily the same color channel of the previous pixel.

**Modulo arithmetic**: All arithmetic is performed modulo 256 (values wrap around) to keep results in the 0-255 byte range. In C, you can use `& 0xFF` or `% 256` to ensure this.

**Edge case handling**: 
- First scanline: No "above" neighbor exists, so treat it as 0
- First `bpp` bytes of each scanline: No "left" neighbor exists, so treat it as 0
- Upper-left neighbor: Only exists when both left and above neighbors exist

**Filter byte handling**: The filter byte itself is never modified during unfiltering. You must:
- Read the filter byte to determine which filter to apply
- Skip the filter byte when processing the pixel data
- Preserve the filter byte when writing (or set it to 0 if using no filtering)

**In-place operations**: Unfiltering can be done in-place - you modify the filtered data to become the original data. No separate output buffer is strictly necessary, though you may choose to use one for clarity.

### Filtering vs. Unfiltering

When **reading** a PNG file (unfiltering):
- You have filtered data and need to reconstruct original pixel values
- This is what you'll implement for the overlay function

When **writing** a PNG file (filtering):
- You have original pixel values and want to create filtered data for better compression
- For simplicity, you can use filter type 0 (None) which requires no filtering operation

### Function: `png_overlay_paste`

1. Open both input files and validate PNG signatures
2. Read and parse IHDR chunks from both images
3. Validate compatibility: both images must have same bit depth (8) and color type
4. For palette images (color type 3):
   - Extract PLTE chunks from both images (rewind files to read from beginning)
   - Merge palettes: add colors from small image that don't exist in large image
   - Create index mapping: map small image indices to merged palette indices
   - Remap pixel indices in small image data to use merged palette
5. Reopen files and skip IHDR chunks for IDAT processing
6. Decompress all IDAT chunks from both images into single buffers
7. Unfilter scanlines to reconstruct actual pixel values (handles filter types 0-4)
8. Paste operation: replace pixels in large image with pixels from small image
   - Calculate paste region (may be clipped if small image extends beyond large)
   - Copy pixels directly using `memcpy` (no alpha blending)
   - Handle scanline structure: skip filter byte, copy pixel data
9. Re-filter scanlines (set all filter bytes to type 0: None)
10. Compress modified image data using zlib with PNG-compatible settings
11. Write output file with proper chunk ordering:
    - PNG signature
    - IHDR chunk
    - PLTE chunk (if palette image, using merged palette)
    - Ancillary chunks from source (tRNS, bKGD, etc.) between PLTE and IDAT
    - IDAT chunk with compressed data
    - Remaining ancillary chunks from source (after IDAT)
    - IEND chunk

**Technical Tips**:
- **Palette merging strategy**:
  - First, add all colors from large image's palette to merged palette
  - Then, for each color in small image's palette:
    - Search merged palette for matching RGB values
    - If color exists: map small index to existing merged index
    - If color is new: add to merged palette (up to 256 entries max)
  - Create mapping table: `index_map[small_idx] = merged_idx`
  - Initialize all mapping entries to 0 for safety
  - Remap all pixel indices in small image before pasting
- **Filter handling**:
  - Unfilters all scanlines after decompression (supports filter types 0-4: None, Sub, Up, Average, Paeth)
  - Re-filters all scanlines before compression (uses filter type 0: None)
  - Ensures pixel operations work on actual pixel values, not filtered data
- **Direct pixel copying**: No alpha blending - pixels are replaced directly
- **Boundary handling**: Clips paste region if small image extends beyond large image boundaries
- **Chunk preservation**: Preserves ancillary chunks (tRNS, bKGD, tEXt, etc.) from source file
- **Chunk ordering**: Ensures proper PNG chunk ordering (IHDR → PLTE → ancillary → IDAT → ancillary → IEND)

**Parameters**:
- `large_path`: Path to the larger (base) PNG file
- `small_path`: Path to the smaller (overlay) PNG file to paste
- `output_path`: Path where the output PNG file will be written
- `x_offset`: X coordinate where the small image will be pasted (0 = left edge)
- `y_offset`: Y coordinate where the small image will be pasted (0 = top edge)

**Return Value**:
- Returns `0` on success
- Returns `-1` on error (see error conditions below)

**Error Conditions**:
- Returns `-1` if any parameter is NULL
- Returns `-1` if either file cannot be opened or is not a valid PNG
- Returns `-1` if IHDR chunks cannot be read or parsed
- Returns `-1` if bit depths don't match (both must be 8)
- Returns `-1` if color types don't match
- Returns `-1` if bytes per pixel calculation fails or doesn't match between images
- Returns `-1` if PLTE extraction fails (for palette images)
- Returns `-1` if palette merging fails (e.g., merged palette exceeds 256 entries)
- Returns `-1` if IDAT decompression fails
- Returns `-1` if unfiltering fails (invalid filter type or data size mismatch)
- Returns `-1` if data size validation fails
- Returns `-1` if re-filtering fails
- Returns `-1` if output file cannot be written
- Returns `-1` on any memory allocation failure

# Part 4: Implementation Notes

You will need to use the C library `malloc`, `realloc`, and `free` functions
to allocate storage for image data buffers, color arrays, and other dynamic
data structures. All functions that allocate memory document the memory
management requirements in their comments.

For reading data from files, you should use the provided utility functions
from `util.c`:
- `read_exact()`: Safely reads exactly N bytes from a file
- `read_u32_be()`: Converts big-endian bytes to native uint32_t
- `util_inflate_data()`: Decompresses zlib-compressed data
- `util_deflate_data_png()`: Compresses data with PNG-compatible zlib settings

You may use `fseek`, `ftell`, `rewind`, and similar functions for file
positioning when working with disk files, as all PNG operations work with
actual files rather than streams.

Note that any function that does I/O might return an error indication in various
situations. A competent C programmer will check each call for an error return
and take appropriate action if one occurs. All functions in this library return
`-1` on error and `0` on success (unless otherwise documented).


## Unit Testing

Unit testing is a part of the development process in which small testable
sections of a program (units) are tested individually to ensure that they are
all functioning properly. This is a very common practice in industry and is
often a requested skill by companies hiring graduates.

> :nerd_face: Some developers consider testing to be so important that they use a
> work flow called test driven development. In TDD, requirements are turned into
> failing unit tests. The goal is then to write code to make these tests pass.

This semester, we will be using a C unit testing framework called
[Criterion](https://github.com/Snaipe/Criterion), which will give you some
exposure to unit testing. We have provided a basic set of test cases for this
assignment. 

## Compiling and Running Tests

When you compile your program with `make`, a `png_test` executable will be
created in your `bin` directory alongside the normal `png` executable. Running this
executable from the `hw1` directory with the command `bin/png_test` will run
the unit tests described above and print the test outputs to `stdout`. To obtain
more information about each test run, you can use the verbose print option:
`bin/png_test --verbose=0`.

The tests we have provided are very minimal and are meant as a starting point
for you to learn about Criterion, not to fully test your homework. You may write
your own additional tests. However, this is not required
for this assignment. Criterion documentation for writing your own tests can be
found [here](http://criterion.readthedocs.io/en/master/).

# Hand-in instructions
**TEST YOUR PROGRAM VIGOROUSLY!**

Make sure that you have implemented all the required functions specified in `global.h`.

Make sure that you have adhered to the restrictions (no calls to prohibited
library functions, no modifications to files that say "DO NOT MODIFY" at the beginning,
no functions other than `main()` in `main.c`) set out in this assignment document.

Make sure your directory tree looks basically like it did when you started
(there could possibly be additional files that you added, but the original organization
should be maintained) and that your homework compiles (you should be sure to try compiling
with both `make clean all` and `make clean debug` because there are certain errors that can
occur one way but not the other).

> :nerd_face: When writing your program try to comment as much as possible. Try to
> stay consistent with your formatting. It is much easier for your TA and the
> professor to help you if we can figure out what your code does quickly!
