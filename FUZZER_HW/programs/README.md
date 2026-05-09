# Fuzzer Test Program Build System

This document describes the build system used to compile all the
programs in the `programs` directory. It also enables the coverage
sanitizer so that it can be used to test the fuzzer.

The top level Makefile in the Fuzzer assignment directory automatically
runs the build system for the programs here.

## General Structure

The general structure of the `programs` directory is:

<pre>
programs/
├── Makefile
├── Makefile.variables
├── README.md
├── Template.mk
├── coverage.c
├── cascade
│   ├── Makefile
│   └── cascade.c
└── hello-world
    ├── Makefile
    ├── include
    │   └── hello.h
    └── src
        ├── hello.c
        └── main.c
</pre>

* The entry point to the build system is `programs/Makefile`. In this
  file, you can specify the subdirectories to build by setting the
  `PROGRAM-DIR` variable on the top of the file. This file will 
  iterate through `PROGRAM-DIR` directories and attempt to call
  `make all`, `make debug`, or `make clean` within that directory
  depending on the operation you choose. For the build system to work
  correctly, each subdirectory should have a Makefile that specify
  an `all`, `debug`, and `clean` target. 
* `Makefile.variables` contain default variables used in the build system.
  It also contains logic to configure the behavior of the build system.
  This file should not be modified.
* `Template.mk` is a template makefile which can be included in Makefiles
  in the subdirectories. This makefile provides an `all`, `debug`, and `clean`
  targets. The behavior of `Template.mk` can be configured by defining
  certain variables described later on in this document. This file should not
  be modified.
* `coverage.c` provides the code which will provide coverage data to the
  fuzzer. This file should not be modified.

## Adding a New Program

To add a new program to the build system, take the following steps.

1. Create a new subdirectory under the `programs` directory. This
   directory will contain all the files for the program. Let the
   name of this subdirectory to be `$DIR_NAME`
2. Under `programs/$DIR_NAME`, add a `Makefile` file in the directory.
   You can write your own Makefile or use our provided template
   Makefile.
    - If you write your own Makefile, be sure to include an `all`
      target which builds production code, `debug` for building debug
      code, and `clean` for cleaning up any build files. 
    - If you use our template Makefile, be sure to define any required
      variables before including the template file. See the examples
      section.
3. Add or write your program in `$DIR_NAME`.
4. Within `programs/Makefile`, add `$DIR_NAME` to the `PROGRAM-DIR`
   variable at the top of the file.
5. Within `programs` directory, call `make all`, `make debug`, and
   `make clean` to make sure your project directory is included in
   the build system.

### Configuration

Here is the configuration options provided by `programs/Template.mk`

|    Variable    | Description                                                                                       |
| :------------: | :------------------------------------------------------------------------------------------------ |
|    `CFLAGS`    | Specifies the compiler flags                                                                      |
|    `DFLAGS`    | Specifies the debug compiler flags to be appended to `CFLAGS`                                     |
|     `LIBS`     | Specifies the libraries to be included in the project                                             |
| `EXTRA_CFLAGS` | Specifies any extra compiler flags to be added                                                    |
|    `POSIX`     | If `POSIX = y`, then the `-D_POSIX_SOURCE` is added to the `CFLAGS`                               |
|     `BSD`      | If `BSD = y`, then the `-D_DEFAULT_SOURCE` is added to the `CFLAGS`                               |
|     `GNU`      | If `GNU = y`, then the `-D_GNU_SOURCE` is added to the `CFLAGS`                                   |
|   `EXEC-NM`    | Specifies the name of the executable. By default this is the name of the directory                |
|     `BIN`      | Specifies the directory to store if the binary if necessary. Defaults to `projects/$DIR_NAME/bin` |
|     `BLD`      | Specifies the build directory which defaults to `projects/$DIR_NAME/build`                        |
|     `INC`      | Specifies the include directory which defaults to `projects/$DIR_NAME/include`                    |
|     `SRC`      | Specifies the program source directory which defaults to `projects/$DIR_NAME/src`                 |

### Examples

Here are some examples of using `template.mk`:

```makefile
# programs/hello-world/Makefile

# Just use the default
# In this case, source files must go in programs/hello-world/src
# Header files go in programs/hello-world/include
# Will not include POSIX, BSD, and GNU source
# etc.
include ../Template.mk
```

```makefile
# programs/cascade/Makefile

# The source directory is programs/cascade
SRC = ./

# Enable POSIX, BSD, and GNU source
POSIX = y
BSD = y
GNU = y

# Will the cascade executable because EXEC-NM was not defined
include ../Template.mk
```

The beginning of `programs/Makefile` will look like this:

```makefile
# programs/Makefile

# === NOTE ===
# During grading, we will replace the entire programs directory with
# our own. Feel free to add additional programs to test your fuzzer
# against here using the build system. See 
# $PROJECT_DIR/programs/README.md for how to use the build system.
# === NOTE ===

# Add program subdirectories here to add to build script

PROGRAM-DIR := cascade hello-world

# ...
```