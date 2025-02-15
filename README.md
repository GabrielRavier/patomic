# patomic
This library provides portable access to lock-free atomic operations at runtime
through a unified interface.  
The goal of it is to be used in Python, however there is no reason it can't be 
used in any other language.  
For an overview of the codebase, look at `ARCHITECTURE.md`.  

For a summary of what changes need to be made before version `1.0.0`, go to the
`Future Development` section at the end of this document.

## Table of Contents
<!--ts-->
* [Versioning](#versioning)
* [Requirements](#requirements)
* [Feature Macros](#feature-macros)
* [Build](#build)
  * [Without CMake](#without-cmake)
* [Usage](#usage)
  * [API Call](#1-api-call)
  * [Alignment Requirements](#2-check-alignment-requirements)
  * [Availability](#3-check-availability)
* [Guarantees](#guarantees)
* [Example](#example)
* [Future Development](#future-development)
<!--te-->

## Versioning
During development, this project got to version `1.2.0` before I realised that
this is beta software and not stable. The final commit with this old versioning
system is `b9da1c6`. After this, the project version was reset to `0.1.0`.

Until `devel` gets merged into `main`, all breaking changes in `devel` will only
increment the `MINOR` version (the `MAJOR` version will stay at `0`). All releases
based on the `devel` branch will be marked as `pre-release` to signify this.
Once merged with `main`, the version will be incremented to `1.0.0` and obey 
normal semver rules.

## Requirements
This library requires a C90 standards compliant compiler to be compiled. In
addition to this it requires that fundamental integer types not have a trap value
(or at least that a `memcpy` into such a type produces a valid value). This is not 
expected to be an issue, but architectures such as `IA-64` have [posed issues in
the past](https://devblogs.microsoft.com/oldnewthing/20040119-00/?p=41003).

The library will almost compile in freestanding (`assert` and `memcpy` prevent
that), however future implementations (`TSX`) will require `malloc` (although
you could always remove that implementation).
 
A C++14 compiler and a platform supported by GoogleTest is required to compile
and run the tests.  

Although the library will compile with just C90, it will report as no operations
supported unless the requirements for one of the internal implementations is
satisfied. Currently, the following internal implementations have been implemented:
* MSVC (`_Interlocked` & other intrinsics) - supported on every tested version 
  of the compiler (back to VC6)
* C11 (`_Atomic`) - supported if C11 or higher and `__STD_NO_ATOMICS__` isn't defined

and the following implementations are going to be added:
* WIN32 (`Interlocked`)
* GNU (`__atomic` & other intrinsics)
* TSX (`_xbegin()`/`_xend()`)

## Feature Macros
These macros are used to decide which features to use internally. In the
unlikely case that any of the definitions are wrong, they can be overridden. 
They are always defined to be either `0` or `1` (i.e. it is assumed they will 
always be defined, either through internal or command line means).

| Name | Guarding |
| --- | --- |
| `PATOMIC_HAVE_LONG_LONG` | `long long` |
| `PATOMIC_HAVE_LONG_LONG_EXTN` | `__extension__ long long` |
| `PATOMIC_HAVE_MS_INT128` | `__int128` |
| `PATOMIC_HAVE_MS_INT128_EXTN` | `__extension__ __int128` |
| `PATOMIC_HAVE_FUNC` | `__func__` |
| `PATOMIC_HAVE_GNU_FUNCTION` | `__FUNCTION__` |
| `PATOMIC_HAVE_GNU_FUNCTION_EXTN` | `__extension__ __FUNCTION__` |
| `PATOMIC_HAVE_GNU_PRETTY_FUNCTION` | `__PRETTY_FUNCTION__` |
| `PATOMIC_HAVE_GNU_PRETTY_FUNCTION_EXTN` | `__extension__ __PRETTY_FUNCTION__` |
| `PATOMIC_HAVE_MS_FORCEINLINE` | `__forceinline` |
| `PATOMIC_HAVE_ALWAYS_INLINE_ATTR` | `__attribute__((always_inline))` |
| `PATOMIC_HAVE_INLINE_ALWAYS_INLINE_ATTR` | `inline __attribute__((always_inline)` |
| `PATOMIC_HAVE_GNU_INLINE_ALWAYS_INLINE_ATTR` | `__inline__ __attribute__((always_inline))` |
| `PATOMIC_HAVE_NOINLINE_ATTR` | `__attribute__((noinline))` |
| `PATOMIC_HAVE_NOINLINE_DSPC` | `__declspec(noinline)` |
| `PATOMIC_HAVE_NORETURN` | `_Noreturn` |
| `PATOMIC_HAVE_NORETURN_DSPC` | `__declspec(noreturn)` |
| `PATOMIC_HAVE_NORETURN_ATTR` | `__attribute__((noreturn))` |
| `PATOMIC_HAVE_RESTRICT` | `restrict` |
| `PATOMIC_HAVE_MS_RESTRICT` | `__restrict` |
| `PATOMIC_HAVE_GNU_RESTRICT` | `__restrict__` |
| `PATOMIC_HAVE_ALIGNOF` | `_Alignof` |
| `PATOMIC_HAVE_MS_ALIGNOF_ALIGN_DPSC` | `__alignof` and `__declspec(align(#))` |
| `PATOMIC_HAVE_GNU_ALIGNOF_ALIGNED_ATTR` | `__alignof__` and `__attribute__((aligned(#)))` |
| `PATOMIC_HAVE_MS_ASSUME` | `__assume(int)` |
| `PATOMIC_HAVE_BUILTIN_UNREACHABLE` | `__builtin_unreachable(void)` |
| `PATOMIC_HAVE_BUILTIN_EXPECT` | `__builtin_expect(long, long)` |
| `PATOMIC_HAVE_STD_INT_UINTPTR` | `<stdint.h>` with `uintptr_t` |
| `PATOMIC_HAVE_STD_DEF_UINTPTR` | `<stddef.h>` with `uintptr_t` |
| `PATOMIC_HAVE_STD_ATOMIC` | `_Atomic` and `<stdatomic.h>`|
| `PATOMIC_HAVE_GNU_SYNC` | `__sync` |
| `PATOMIC_HAVE_GNU_ATOMIC` | `__atomic` and `__ATOMIC` |
| `PATOMIC_HAVE_GNU_SYNC_LOCK_FREE_{N}` | `__sync` is lock-free for `sizeof(type) == N` |
| `PATOMIC_HAVE_INTRIN_EFLAGS_CPUID` | `<intrin.h>` with `__cpuid(ex)` and `__(read/write)eflags` |
| `PATOMIC_HAVE_CPUID_CPUID` | `<cpuid.h>` with `__cpuid(_count)` and `__get_cpuid_max` |
| `PATOMIC_HAVE_IR_SIGN_MAGNITUDE` | integer representation is sign-magnitude |
| `PATOMIC_HAVE_IR_ONES_COMPL` | integer representation is ones-complement |
| `PATOMIC_HAVE_IR_TWOS_COMPL` | integer representation is twos-complement |
| `PATOMIC_SIZEOF_{T}` | value of `sizeof(T)` (or `0` if `T` not supported) |

Their implementations can be found in `/src/include/patomic/patomic_config.h`.

## Build
This project uses CMake as its build system.  
To build and test on Windows (or with a multi-config generator):
```shell
mkdir build && cd build
cmake -Dpatomic_DEVELOPER_MODE="ON" ..
cmake --build . --config Release
ctest -C Release
```
To build and test on Unix based systems:
```shell
mkdir build && cd build
cmake -Dpatomic_DEVELOPER_MODE="ON" ..
cmake --build .
ctest
```
`patomic_DEVELOPER_MODE` is required to enable testing. Several CMake presets
are also available in `CMakePresets.json`, however these are currently only used
for CI purposes (although they are a good starting point).

### Without CMake
It is possible to compile without CMake although it is slightly more involved.
The `#include`s of `<patomic/patomic_export.h>` and `<patomic/patomic_version.h>`
in `/include/patomic/patomic.h` must be removed, or equivalents must be provided.
These files are generated by CMake and define the following macros:
* `#define PATOMIC_VERSION "X.Y.Z"`
* `#define PATOMIC_VERSION_MAJOR X`
* `#define PATOMIC_VERSION_MINOR Y`
* `#define PATOMIC_VERSION_PATCH Z`

The macro `PATOMIC_EXPORT` is also defined, and its value depends on whether the
library is being built or used. For example, on Windows (MSVC) it might expand to
`__declspec(dllexport)` when the library is being built, and `__declspec(dllimport)`
when the library is being used. For GNU compilers it's always defined to
`__attribute__ ((visibility ("default")))` in both cases.

Once these macros and files are dealt with, something equivalent to the following
`GCC` command can be used:
```shell
$ echo '#define PATOMIC_VERSION "X.Y.Z"
#define PATOMIC_VERSION_MAJOR X
#define PATOMIC_VERSION_MINOR Y
#define PATOMIC_VERSION_PATCH Z' \
  > ./include/patomic/patomic_version.h

$ echo '#define PATOMIC_EXPORT __attribute__ ((visibility ("default")))' \
  > ./include/patomic/patomic_export.h

$ mkdir build && cd build

$ gcc -I../include -I../src/include                      \
      ../src/patomic.c  $(find ../src/impl/ -name "*.c") \
      -fvisibility=hidden -shared -fpic                  \
      -o libpatomic.so

$ rm ../include/patomic/patomic_*.h
```
You should replace the version in this example with actual versions.  
This will not compile the tests.
## Usage
Usage of this library is split into three stages.

### 1. API Call
The types `patomic_t` and `patomic_explicit_t` contain function pointers and
alignment requirements for the operations. Objects of these types can be obtained
through the following two API calls:
```c
patomic_t
patomic_create(
    size_t byte_width,             /* width of atomic object */
    patomic_memory_order_t order,  /* memory order for all operations */
    int options,
    int impl_id_argc,              /* number of variadic parameters */
    ...                            /* implementation ids */
);

patomic_explicit_t
patomic_create_explicit(
    size_t byte_width,  /* width of atomic object */
    int options,
    int impl_id_argc,   /* number of variadic parameters */
    ...                 /* implementation ids */
);
```
Implementation ids can be found in `/include/patomic/types/ids.h` (each implementation
has its own unique ID). Options can be found in `/include/patomic/types/options.h`
and can be combined with `&`. Options do not change the correctness of the program
and are followed by the library on a best faith effort.

### 2. Check Alignment Requirements
The `patomic_t` (or `patomic_explicit_t`) object contains the alignment required
by the operations for the atomic object (in the `.align` member). The first 
requirement is the `recommended` one and it can be checked like this:
```c
#include <stdint.h>

int check_recommended(const volatile void *obj, patomic_t p)
{
    uintptr_t addr = (uintptr_t) obj;
    return (addr % p.align.recommended == 0);
}
```
The second requirement is `minimum`. Checking it is more involved, and it potentially
provides a lower alignment requirement, with the potential downside of a higher
performance penalty. It can be checked like this:
```c
#include <stdint.h>

int check_minimum(const volatile void *obj, patomic_t p, size_t byte_width)
{
    uintptr_t addr = (uintptr_t) obj;
    if (addr % p.align.minimum == 0)
    {
        addr %= p.align.size_within;
        return (addr + byte_width) <= p.align.size_within;
    }
    else { return 0; }
}
```
The reasoning for the two alignment requirements and their purposes can be read
in `/include/patomic/types/align.h`.

### 3. Check Availability
The `.ops` member is a collection of function pointers denoting operations. These
must be checked for `NULL` before they can be used. Once this is done the
operations can be used on the atomic object as shown below:
```c
#include <assert.h>

int atomic_fetch_add(volatile int *obj, int arg, patomic_ops_explicit_t ops)
{
    assert(ops.signed_ops.fp_fetch_add != NULL);
    int result;
    ops.signed_ops.fp_fetch_add(obj, &arg, patomic_SEQ_CST, &result);
    return result;
}
```
A fully written out example is shown further below. The types for all the 
operations can be found in `/include/patomic/types/ops.h`.

In some cases one may want to check if any operations on a given width are 
supported. To that end, the following two API functions are provided:
```c
int
patomic_nonnull_ops_count(
    patomic_ops_t const *const ops
);

int
patomic_nonnull_ops_count_explicit(
    patomic_ops_explicit_t const *const ops
);
```
## Guarantees
The following guarantees are always in place provided the alignment requirements
are followed:
* `recommended` and `minimum` alignment requirements are positive powers of `2`
* `minimum <= recommended`
* all operations available are lock-free (i.e. there is no underlying mutex)
* all operations available are address-free (i.e. they are process safe)
* the memory ordering of the operation is at least as strong as what was requested
* operations do not require any alignment on any `void *` parameters except the 
first one (atomic object)

## Example
The only file you need to include is `patomic/patomic.h`, which works both in C and C++ (although the library still needs to be built as C++).
```c
#include <stdio.h>
#include <stdint.h>

#include <patomic/patomic.h>

int main()
{
    /* the "atomic" variable */
    volatile int obj = 5;

    /* create explicit struct */
    patomic_explicit_t p;
    p = patomic_create_explicit(
        sizeof obj,               /* byte width */
        patomic_options_DEFAULT,  /* options */
        0                         /* variadic parameter count */
    );

    /* check the alignment of our atomic object */
    uintptr_t addr = (uintptr_t) &obj;
    if (addr % p.align.recommended != 0) {
        printf("Failure: alignment does not meet requirement\n");
        return 1;
    }

    /* check the operation is supported */
    patomic_opsig_explicit_fetch_t fp_fadd_int;
    fp_fadd_int = p.ops.signed_ops.fp_fetch_add;
    if (fp_fadd_int == NULL) {
        printf("Unsupported operation: fetch_add on int\n");
        return 1;
    }

    /* perform addition */
    int arg = 10;
    int old;
    fp_fadd_int(
        &obj,             /* atomic object */
        &arg,             /* input: argument */
        patomic_SEQ_CST,  /* memory order */
        &old              /* output: return value */
    );

    /* final output */
    printf("New value: %d\n", obj);
    printf("Old value: %d\n", old);
    return 0;
}
```
## Future Development

Hitting `v1.0.0` is required before I can merge into the master branch. Proper
documentation will be written once `v1.0.0` is created (in addition to updating
this README).

The following tasks need to be completed before the ABI/API can be made stable:
- **private symbols** - come up with a proper naming convention for symbols that
  get marked `PATOMIC_NO_EXPORT` (maybe `Zpatomic_*_` to also help with IDEs?);
  rework any symbols marked `PATOMIC_FORCE_INLINE` but not `static`
- **two includes** - figure out how include both from `/include` and from
  `/src/include` and not have macros break stuff
- **impl_ids** - should they be powers of 2 in order to be composable or not?
  and should they have some bits reserved for the implementation type?
- **impl_kind** - maybe add an impl kind either as the first couple of bits in
  impl_id or as its own thing? would help prevent running out of ids so fast if
  i decide to make them powers of 2, and would let people exclude stuff like OS
  and library implementations easily
- **va_args** - remove variadic params from API; it's unnecessary and the way
  it's currently implemented doesn't lead to clean code
- **combine** - add api to combine `ops_t` structs, possibly allowing to combine
  multiple with a priority for each one
- **expose more info** - have a macro and corresponding runtime functions for
  min/max impl_id values and max width supported overall and per implementation;
  also improve cache-line size macro and function values
- **naming** - rethink about symbol and file/path naming in general and make
  sure i'm happy with it
- **options** - redo options to match how we change impl stuff and public api

The following tasks aren't part of the public ABI/API, but I want to complete
before `v1.0.0`:
- **impl_t array order** - priorities by default will be based on index of impl
  in the impl_t array in register.h
- **enable/disable impl** - add a way to enable or disable specific
  implementations with a macro at compile time; possible also add a way to
  enable/disable all implementations of a specific kind if it's feasible to do
  so
- **remove "public" macros** - remove macros that are in public files but get
  undefd and are not part of the public interface (simply used to build it); 
  these macros can confuse IDEs and annoy me
- **NULL fp_create** - let members in impl_t struct be NULL instead of having
  to make a dummy function every time
- **transaction** - add a transactional implementation using tsx, just so I can
  make sure I'm happy with the transactional API

The rest of these tasks aren't part of any ABI/API (internal or external), they
just need them to be done before merging with the master branch:
- **simple api** - create a simple api wrapping the current api to mimic C11's
  `atomic_*` api (so `patomic_*` equivalents)
- **testing** - overhaul tests to properly test for logic, atomicity, and
  ordering separately, as well as check whether a width is supported so that the
  test log doesn't end up with thousands of skipped tests; also add separate
  tests for transactional implementations
- **windows tests** - figure out why tests run so slowly on Windows
- **presets and ci** - have CI test multiple configurations with stuff enabled
  and disabled, not just the default config for each compiler (and ideally
  test on multiple architectures, maybe QEMU?)
- **build without cmake** - create a python script to collate the project into
  a single directory (ideally with just .c files) so that people building
  without CMake can easily compile without needing a bunch of special flags
- **macros** - double check all macros are actually correctly named (since most
  macros will be hidden)
- **interlocked** - redo the msvc implementation to actually properly check for
  the availability of the intrinsics it uses (both in software and hardware)
- **counter** - maybe (ab)use `__COUNTER__` to let the user enable only up to N
  implementations somehow?

The following implementations are being considered:
- C11 stdatomic.h (done)
- GNU __atomic (done)
- GNU __sync (done)
- MSC _Interlocked/_interlocked (to be redone from scratch)
- AVX/SSE (intrinsics)
- TSX (intrinsics and inline asm)
- TME (intrinsics and inline asm)
- MSC inline asm
- GNU inline asm
- Win32 Interlocked
- Bsd/Unix/Linux atomic(3C)
- OSX has libkern (but deprecated)

Additionally, many OSs have atomic implementations specific to the kernel. Since
this library is buildable in freestanding, we could use these too in the future.
