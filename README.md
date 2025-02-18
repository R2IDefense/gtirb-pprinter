GTIRB Pretty Printer
====================

A pretty printer from the [GTIRB](https://github.com/grammatech/gtirb)
intermediate representation for binary analysis and reverse
engineering to gas-syntax assembly code.


## Building

The pretty-printer uses C++17, and requires a compiler which supports
that standard such as gcc 7, clang 6, or MSVC 2017.

To build and install the pretty printer, the following requirements
should be installed:

* [GTIRB](https://github.com/grammatech/gtirb).
* [Capstone](http://www.capstone-engine.org/). At the moment
  we require our own fork https://github.com/GrammaTech/capstone/tree/next
  that contains some additional fixes (until new official releases are cut).
* [Boost](https://www.boost.org/), version 1.67.0 or later.
  * Requires the libraries:
    * filesystem
    * program_options
    * system

Note that these versions are newer than what your package manager may provide
by default: This is true on Ubuntu 18, Debian 10, and others. Prefer building
these dependencies from sources to avoid versioning problems.

Use the following options to configure cmake:
- You can tell CMake which compiler to use with
  `-DCMAKE_CXX_COMPILER=<compiler>`.
- Normally CMake will find GTIRB automatically, but if it does not you
  can pass `-Dgtirb_DIR=<path-to-gtirb-build>`.
- gtirb-pprinter can make use of GTIRB in static library form (instead of
  shared library form, the default) if you use the flag
  `-DGTIRB_PPRINTER_BUILD_SHARED_LIBS=OFF`.
- Furthermore, if you want to produce a `gtirb-pprinter` executable that links
  statically, specify `-DGTIRB_PPRINTER_STATIC_DRIVERS=ON`.
- You can configure CMake to use a custom location for Capstone by specifying
  `-DCMAKE_LIBRARY_PATH=<path-to-capstone>`.
- You can use vcpkg on Windows to provide some dependencies by passing
  `-DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg\scripts\buildsystems\vcpkg.cmake>`.

Once the dependencies are installed, you can configure and build as follows:

```sh
cmake ./ -Bbuild
cd build
make
```

## Installing
See the [GTIRB readme](https://github.com/GrammaTech/gtirb/#installing).

## Usage

### Generate reassembleable assembly code
Pretty print the GTIRB for a simple hello world executable to an
assembly file named `hello.S`, assemble this file with the GNU
assembler to an object file named `hello.o`, and link this object file
into an executable.

```sh
gtirb-pprinter hello.gtirb --asm hello.S
as hello.S -o hello.o
ld hello.o -o hello
./hello
```
### Generate a new binary
The `--binary` flag to gtirb-pprinter generates a new binary by
calling `gcc` directly.

```sh
gtirb-pprinter hello.gtirb --binary hello
```

This option admits an argument `--library-paths` or `-L` to
specify additional paths where libraries might be located.

For example:
```sh
gtirb-pprinter hello.gtirb --binary hello -L . -L /usr/local/lib
```

### Dummy .so
In some cases, it is desirable to rebuild a dynamically linked ELF executable
without any of the libraries to which it is linked (e.g., if rebuilding an
executable from another system).

Normally, the linker needs to have the libraries in order to link with them.
However, the `--dummy-so` option generates fake libraries that contain the
required symbols used by the binary, which is sufficient for running the
linker. An example is shown:

```sh
gtirb-pprinter hello.gtirb --binary hello --dummy-so=yes
```

If the binary requires verisoned symbols (which is likely if it uses a recent
version of glibc), some further arguments may be necessary. Your system's
startfiles may differ from the startfiles that the binary was originally built
with, causing symbol version incompatibilities when gtirb-pprinter removes
startup code and your compiler replaces it with its own. To best work around
this, add the following arguments:

```sh
gtirb-pprinter hello.gtirb --binary hello --dummy-so=yes --keep-all-functions -c -nostartfiles
```

These ensure that startup code (such as functions like `_start` or
`__libc_csu_init`) present in the GTIRB is used as-is, instead of replacing
them with startup code provided by your compiler, preserving the same symbol
versions that the binary originally used.

## AuxData Used by the Pretty Printer

Generating assembly depends on a number of additional pieces of information
beyond the symbols and instruction/data bytes in the IR. The pretty printer
expects this information to be available in a number of
[AuxData](https://github.com/GrammaTech/gtirb/blob/master/README.md#auxiliary-data)
objects stored with the IR. We document the expected keys along with the
associated types and contents in this table.

| Key              | Type                                           | Purpose                                                                                                                              |
|------------------|------------------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------|
| comments         | `std::map<gtirb::Offset, std::string>`           | Per-instruction or data element comments.                                                                                          |
| functionEntries    | `std::map<gtirb::UUID, std::set<gtirb::UUID>>` | UUIDs of the blocks that are entry points of functions.                                                                                              |
| symbolForwarding | `std::map<gtirb::UUID, gtirb::UUID>`           | Map from symbols to other symbols. This table is used to forward symbols due to relocations or due to the use of plt and got tables. |
| encodings            | `std::map<gtirb::UUID,std::string>`            | Map from (typed) data objects to the encoding of the data,  expressed as a std::string containing an assembler encoding specifier: "string", "uleb128" or "sleb128".     |
| sectionProperties | `std::map<gtirb::UUID, std::tuple<uint64_t, uint64_t>>` | Map from section UUIDs to tuples with the section types and flags. |
| cfiDirectives   | `std::map<gtirb::Offset, std::vector<std::tuple<std::string, std::vector<int64_t>, gtirb::UUID>>>` | Map from Offsets to  vector of cfi directives. A cfi directive contains: a string describing the directive, a vector  of numeric arguments, and an optional symbolic argument (represented with the UUID of the symbol). |
| elfSymbolInfo | `std::map<gtirb::UUID, std::tuple<uint64_t, std::string, std::string, std::string, uint64_t>>` | On ELF targets only: Map from symbols to their type, binding, and visibility categories. |

## AuxData Used by the Binary Printer

In order to generate new binaries, gtirb-binary-printer also uses the following tables:

| Key              | Type                             | Purpose                                                                          |
|------------------|----------------------------------|----------------------------------------------------------------------------------|
| libraries        | `std::vector<std::string>`       | Names of the libraries that are needed.                                          |
| libraryPaths     | `std::vector<std::string>`       | Paths contained in the rpath of the binary                                       |
| elfStackExec     | `bool` |  Stack executable flag specified by PT_GNU_STACK segment in ELF files. Binary-printed with `-Wl,-z,stack,[no]execstack` |
| elfStackSize     | `uint64_t` | Stack size specified by PT_GNU_STACK segment in ELF files. Binary-printed with `-Wl,-z,stack-size=value`. |
