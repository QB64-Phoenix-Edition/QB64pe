Build System
============

Build Process
-------------

This describes the process that goes on to compile a QB64 program into an executable.

1. `QB64-PE` takes the QB64 source code and produces C++ source code for the program. That C++ source is placed into various files and goes into `./internal/temp`.
2. `make` is used to run the `Makefile` at the root of this repository. It is supplied the `OS` parameter, the `EXE` parameter for the final executable name, and a variety of `DEP_*` parameters depending on what dependencies the QB64 program requires.
    1. 3rd party dependencies located in `./internal/c/parts/` are compiled if requested. Each of these has its own `build.mk` file that includes the `make` logic to build that dependency.
    2. `./internal/c/libqb.cpp` is compiled with various C preprocessor flags depending on the provided `DEP_*` flags. The name is changed depending on the settings it is compiled with so that we don't accidentally use a libqb compiled with different settings.
    3. `./internal/c/qbx.cpp` is compiled, which uses `#include` to pull in the generated C++ source located in `./internal/temp`. It finds it by `-I`, so every instance of QB64-PE compiles the same file with its own temp folder on the include path. The object is written to `./internal/temp/qbx.o`, because it belongs to the program being built rather than to QB64-PE.
    4. All of `qbx.o`, `libqb.o`, and the 3rd party dependencies are linked together to produce the executable. With `SPLIT_TUS=y` there are also the `tu_*.o` objects, passed to the linker in a response file rather than on the command line.
    5. Debug symbols are stripped from the executable to make it smaller, the symbols are stored into a symbol file ending in `.sym` in `./internal/temp`.

CI Process
----------

This describes how QB64-PE itself is built by the CI process to produce a release of QB64-PE. The actual build scripts that do all of this are located in `.ci/`.`

1. Before CI ever begins, `./internal/source/` contains the generated C++ source of a previous version of `qb64pe.bas`.
2. `.ci/calculate_version.sh` is run to determine what version this CI build should be considered. That information is written to `./internal/version.txt`, if it is a release version `./internal/version.txt` is removed.
3. `qb64pe_bootstrap` is compiled from `./internal/source`
    1. `make` is used to call the `Makefile` with the proper `OS` setting, `EXE=qb64pe_bootstrap`, and `BUILD_QB64=y`.
    2. The Makefile will take care of copying `./internal/source` into `./internal/temp` and compiling QB64-PE with the proper settings.
4. `qb64pe_bootstrap` is used to compile `./source/qb64pe.bas` into a proper `qb64pe` executable
    1. We run `./qb64pe_bootstrap -x ./source/qb64pe.bas` to compile QB64-PE. This compiles QB64-PE the same way as a regular QB64 program as detailed in 'Build Process'
5. `./internal/source` is cleared out and, excluding a few files, the contents of `./internal/temp` are copied into `./internal/source` as the new generated C++ source of QB64-PE.
6. `tests/run_tests.sh` is run to test the compiled version of QB64-PE. A failure of these tests fails the build at this stage.
7. `.ci/make-dist.sh` is run to produce the distribution of QB64-PE, which is stored as a build artifact and potentially a release artifact..
    1. The contents of the distribution depends on the OS it is being created for.
        1. The Windows distribution contains a precompiled copy of QB64-PE as `qb64pe.exe` and can be used immediately.
        2. The Linux and OSX distributions do not contain a precompiled copy of QB64-PE. Those version come with a "setup" script that has to be run which builds QB64-PE based on the contents of `./internal/source`.
8. `tests/run_dist_tests.sh` is run to verify the distribution of QB64-PE works correctly.
9. If this is a CI build of `main` and `./internal/source` has changed due to the newly compiled version, then `git` is used to commit and push the updated version of `./internal/source` to the GitHub repo.

Repository Layout
-----------------

- `.ci/` - All files in this folder are related to the CI build process.
  - `bootstrap.bat`
    - Windows only, Downloads MinGW compiler, builds the precompiled version of QB64-PE located in `./internal/source` as `qb64pe_bootstrap.exe`.
  - `bootstrap.sh`
    - Linux and OSX, builds the precompiled version of QB64-PE located in `./internal/source` as `qb64pe_bootstrap.exe`.
  - `compile.bat`
    - Uses `qb64pe_bootstrap.exe` to build `./source/qb64pe.bas`. This compiled QB64-PE is the released `qb64pe.exe`.
  - `compile.sh`
    - Uses `qb64pe_bootstrap` to build `./source/qb64pe.bas`. This built QB64-PE is what is used for testing, and the sources from this QB64-PE are placed into `./internal/source`.
  - `make-dist.sh`
    - Copies all the relevant parts of QB64-PE into a new folder, which can be distributed as a release.
  - `push-internal-source.sh`
    - If `./internal/source` is different after building `./source/qb64pe.bas`, then this will automatically push those changes to the repository to update the sources used to build `qb64pe_bootstrap`.
- `internal/`
  - `c/` - Contains everything related to the C/C++ source files for QB64-PE.
    - `c_compiler/`
      - On Windows, this folder is populated with the MinGW C++ compiler during the CI process (or when using `setup_win.cmd`).
    - `parts/`
      - Contains the sources to many of the dependencies that QB64 uses. Most of the dependencies have a `build.mk` file that is used by the main `Makefile` to build them.
  - `source/`
    - Contains a copy of the generated C++ source of a previous version of QB64-PE. This is used to build a copy of QB64-PE using only a C++ compiler. This is updated automatically via the CI process.
  - `version.txt`
    - QB64-PE checks this file to determine if there is a version tag (`-foobar` on the end of the version) for this version of QB64-PE.
- `source/`
  - Contains the QB64 source to QB64-PE itself.
- `tests/` - Contains the tests run on QB64-PE during CI to verify changes.
  - `compile_tests/`
    - Testcases related to specific dependencies that QB64 can pull in. These tests are largely intended to test that QB64-PE and the Makefile correctly pulls in the proper dependencies.
  - `c`
    - The source for the C++-based tests.
  - `qbasic_testcases/`
    - A variety of collected QB64 sample programs
  - `dist/`
    - Test files for distribution tests.
  - `compile_tests.sh`
    - Runs the `compile_tests` test cases.
  - `qbasic_tests.sh`
    - Compiled all the testcases in `qbasic_testcases` and verifies they compile successfully.
  - `dist_tests.sh`
    - Verifies the output of `make-dist.sh` is a functioning distribution of QB64-PE
  - `run_dist_tests.sh`
    - Runs the distribution test collections.
  - `run_tests.sh`
    - Runs all individual test collections.
  - `run_c_tests.sh`
    - Runs all the C++ test cases.
- `setup_lnx.sh`
  - Used as part of the Linux release to install dependencies and compile QB64-PE.
- `setup_osx.command`
  - Used as part of the OSx release to compile QB64-PE.
- `setup_win.cmd`
  - Used only for compiled QB64-PE directly from a clone of the repository (not a release, we distribute QB64-PE already compiled in the Windows release)
- `Makefile`
  - Used for building QB64 programs.

Makefile Usage and Parameters
-----------------------------

> Note: These parameters are not guaranteed to stay the same. The 'setup' scripts and QB64-PE are the only things intended to call the Makefile directly.

These flags control some basic settings for how the `Makefile` can compile the program.

| Parameter | Default Value | Valid Values | Required | Description |
| :-------- | :-----------: | :----------: | :------: |  :---------- |
| `OS` | None | `win`, `lnx`, or `osx` | Yes | Indicates to the Makefile which OS it is being used to compile for. Controls what utilities the Makefile assumes is available and where they are located, and also platform dependent compiler settings. |
| `BUILD_QB64` | None | `y` | No | If set then the Makefile will build the QB64-PE source located in `./internal/source`. If `EXE` is not specified, that is set to `qb64pe`. Dependency settings for QB64-PE are set automatically. |
| `EXE` | None | Executable name | Yes, if not using `BUILD_QB64` | Specifies the name of the executable to build. If on Windows, it should include `.exe` on the end. Spaces in the filename should be escaped with `\` |
| `TEMP_ID` | None | blank, or an integer | No | Controls which `./internal/temp*` folder is used for compilation. This is mainly relevant for multiple instances of the IDE, where the second instance compiles to `./internal/temp2` and tells the Makefile which to use. Further instances will use higher numbers. |
| `SPLIT_TUS` | None | `y` | No | Builds the generated C++ as one object file per group of SUB/FUNCTIONs instead of a single one. QB64-PE passes this when its `SplitObjectFiles` setting is on. See 'Splitting the generated C++' below. |
| `CXXFLAGS_EXTRA` | None | Collection of C++ compiler flags | No | These flags are provided to the compiler when building the C++ files. This includes the generate source files, but also files like `libqb.cpp`. |
| `CXXLIBS_EXTRA` | None | Collection of linker flags | No | These flags are provided to the compiler (`g++` or `clang++`) when linking the final executable. Typically includes the `-l` library flags |
| `CFLAGS_EXTRA` | None | Collection of C compiler flags | No | These flags are provided to the compiler when building C files |
| `STRIP_SYMBOLS` | None | `n` | No | Symbols are stripped by default, if this is `n` then symbols will not be stripped from the executable |
| `GENERATE_LICENSE` | None | `y` | No | Generates a `.license.txt` file next to your executable, which contains all the licenses that apply to the executable |
| `LICENSE` | `$(EXE).license.txt`| License file name | No | Specifies an alternative filename for the license file |

These flags controls whether certain dependencies are compiled in or not. All of them are blank by default and should be set to `y` to use them. Note that the program being built may not compile if you supply the wrong dependency settings - may of them supply functions that will not be defined if the dependency is required, and some of them (Ex. `DEP_GL`) have requirements the program being compiled must meet.

| Parameter | Description |
| :-------- | :---------- |
| `DEP_GL` | Enables the `_GL` support |
| `DEP_SCREENIMAGE` | Enables `_SCREENIMAGE` support, also implies `DEP_IMAGE_CODEC=y` |
| `DEP_IMAGE_CODEC` | Enables `_LOADIMAGE` and the various image decoders. |
| `DEP_SOCKETS` | Adds networking support. |
| `DEP_PRINTER` | Adds `_PRINTIMAGE` support. |
| `DEP_ICON` | Adds `_ICON` support. |
| `DEP_ICON_RC` | Adds `$EXEICON` and `$VERSIONINFO` support, compiles `.rc` file into the executable on Windows |
| `DEP_FONT` | Enables various `_FONT` related support. |
| `DEP_DEVICEINPUT` | Enables game controller input support. |
| `DEP_ZLIB` | Adds `_DEFLATE` and `_INFLATE` support. |
| `DEP_EMBED` | Compiles in data embedded via `$EMBED` statements. |
| `DEP_CONSOLE` | On Windows, this gives the program console support (graphical support is still allowed) |
| `DEP_CONSOLE_ONLY` | Same as `DEP_CONSOLE`, but also removes GLUT and graphics support. |
| `DEP_AUDIO_MINIAUDIO` | Pulls in sound support using miniaudio for playing sounds via `PLAY`, `_SNDPLAY`, and various other functions that makes sounds. |
| `DEP_HTTP` | Enables http support via libcurl. Should only be used if `DEP_SOCKETS` is on. |

Splitting the generated C++ into separate files
-----------------------------------------------

QB64-PE includes the ability to split the generated C++ across several different
C++ translation units (compiled files). For larger programs this has the dual
benefit of decreasing total amount of memory required (the separate translation
units require less memory each) and increasing the compilation speed (the
translation units can be compiled in parallel).

When split translation units is enabled, the SUB/FUNCTIONs are grouped by the
`$INCLUDE` file they are declared in, and a file whose generated C++ is much
larger than the size aimed for per unit is cut into chunks. This approach
retains source locality, so changes in one `$INCLUDE` file will not trigger
recompilation of source in other `$INCLUDE` files.

Note that grouping is never finer than one unit per file: a program that keeps
each of its routines in its own tiny `$INCLUDE` gets a unit per routine, which
can end up slower than not splitting at all, because every unit pays the cost of
reading the shared header.

QB64-PE writes the units themselves into the program's temp folder as
`tu_*.cpp`, along with three files describing them:

- `split.mk` - the list of units in `QB_SPLIT_SRCS`, plus a rule per unit naming
  the generated files it is built from to ensure the dependencies are correctly
  tracked.
- `objs.rsp` - the same list of objects as a linker response file. A large
  program generates far more of them than can fit in a Windows command line.
- `qbx_gen.h` - the function declarations and various other global pieces of
  code used by all the translation units.

The main module's own code stays in `main_split.txt`, which `qbx.cpp` includes
in place of the `main.txt` a non-split build produces. A program that calls
`_COMPILEDATE$`, `_COMPILETIME$` or `_COMPILERVERSION$` also gets a small
`compinfo.cpp` holding just those functions.

There is some special handling for `DECLARE LIBRARY` due to compatibility
issues. When a header file is found for the library, QB64-PE lets the header
declare the routines to be called. The header cannot be included into
every translation unit however, because a header-only library defines its
routines as well as declaring them and that would result in duplicate
definitions at linker time. The same applies when `DECLARE LIBRARY` is used
with no named library, that indices the declaration comes from a file already
included which is potentially a header provided by a different `DECLARE
LIBRARY` entry.

This problem is solved by "pinning" any SUB/FUNCTION which calls one of these
`DECLARE LIBRARY` routines into a single translation unit (the "main" unit also
including the main module code). That translation unit continues to include the
header as normal, and it is not included by any other translation units,
avoiding any duplicate definition issues. It's worth placing these calls into
simple 'wrapper' functions if they're widely used to avoid moving significant
amounts of code into a single translation unit (defeating the point of the
feature).

`DECLARE DYNAMIC LIBRARY` and `DECLARE CUSTOMTYPE LIBRARY` never pin anything
because QB64-PE calls those through a function pointer it declares itself. The
call compiles wherever it appears, whether or not the library also brought a
header along. `DECLARE STATIC LIBRARY` behaves the same as a plain
`DECLARE LIBRARY`.

Rebuilding incrementally
------------------------

`make` can only skip work if the files it compares mtimes against stop changing,
so QB64-PE leaves a generated file alone when its contents are the same as what
is already on disk. The comparison reads the file back and compares it in
bounded chunks, stopping at the first difference.

A current limitation of this system is that any changes that are seen globally
by the program will still trigger a full recompilation. We don't currently
track and generate individual sets of global declarations for each translation
unit, so any changes to this information triggers a recompile of all the
translation units including it. This includes actions like adding a new
SUB/FUNCTION, declaring a global variable, etc.

`embedded.cpp`, the C arrays a program's `$EMBED` files are turned into, is
handled differently: producing the bytes to compare means re-reading and
re-deflating every payload which can be quite expensive. Instead, `embeds.txt`
in the temp folder records what `embedded.cpp` was generated from - the ordered
list of embedded files and, per file, its size, modification time, content hash
and whether it was compressed - and the pass is skipped outright when the list
and every file in it are unchanged. A file whose size and stamp both match is
taken as unchanged without being read; one whose stamp moved is read and
hashed, and the new stamp recorded so the next build does not have to.

On the make side we make use of `.d` dependency files from the C++ compiler to
trigger recompilations when dependency #include files change (Ex. The contents
of a SUB/FUNCTION changes). Alongside them, `buildflags.txt` records the
settings that decide how the C++ is compiled - `SPLIT_TUS`, `CXXFLAGS_EXTRA`,
`BITS` and the `DEP_*` flags. The object files all take a dependency on this
file, so changing any of these settings also triggers a full recompile.

At the end of translation, and before `make` is invoked, QB64-PE deletes the
generated files still present in temp folder from an earlier build that this
one did not produce. This avoids generated files cluttering the temp folder and
making it unclear which files are from the current compilation. It also ensures
there is no risk of a make target accidentally picking one of them to build and
link (though we don't use wildcards for that, so the risk is minimal).

This table shows a few examples of changes and how much of the code will be rebuilt:

| Change | Rebuilds |
| :----- | :------- |
| nothing | nothing, unless the program uses `_COMPILEDATE$`/`_COMPILETIME$` |
| a statement inside a SUB/FUNCTION | that routine's translation unit only |
| a statement at module level | the main unit |
| adding or removing a SUB or adding a `DIM SHARED` | All translation units are rebuilt |

The first row's exception is `compinfo.cpp`, these are always rebuilt to ensure
the correct date and time are recorded.

The last row represents changes to global information - this information is
supplied to every translation unit, so changing it triggers a recompilation of
all of them.

Incremental writing of generated C++
------------------------------------

Most generated files are held in memory until the end of translation, but the
four belonging to a SUB/FUNCTION (`main<N>.txt`, `data<N>.txt`, `free<N>.txt`
and `ret<N>.txt`) are written out as its `END SUB`/`END FUNCTION` is read, and
their memory handed back. Nothing adds to them after that point, and they are
the bulk of a program's generated C++, so holding them uses significantly more
memory.

Two consequences of writing a SUB/FUNCTION as it finishes are worth knowing:

* A translation that fails leaves the routines it had already finished on disk.
  Nothing is built from them - `make` is not invoked and the stale-file pass
  below does not run - and the next build rewrites them.
* A translation that restarts itself writes a routine twice if the two attempts
  disagree about its code, which gives that routine's translation unit a new
  mtime and rebuilds it. For the most part only a handful of legacy features
  restart translation late enough for this to happen: conventional-memory
  resolution (`SADD`, `VARPTR`, `VARSEG`), `COMMON` arrays whose dimensions are
  not yet known, and a SUB name used as a label.

A command-line build always features that behavior as there's nothing
to gain by keeping a finished routine in memory. For the IDE on the other hand
it is optional, it happens only when *Options -> Write Files While Translating*
is on (it is off by default, and stored as `WriteFilesWhileTranslating` under
`[COMPILER SETTINGS]` in `settings/config.ini`). The IDE retranslates the whole
program on every edit to check its syntax, writing to disk every time this
happens can itself can have its own performance impact, so it's made optional
depending on which trade-off the user wants to take.

Versioning
----------

QB64 Phoenix Edition follows SemVer, which means that major releases indicate a breaking change, minor releases indicate new features, and patch release indicates bug fixes.

- Release versions of QB64-PE will have just a version number in the form `X.Y.Z`. All other versions of QB64-PE will have some kind of 'tag' at the end of the version, which is arbitrary text after a `-` placed on the end of the version.
- CI versions get a tag in the form of `-XX-YYYYYYYY`, where `XX` is the number of commits since the last release, and `YYYYYYYY` is the first 8 digits of the commit hash of that build.
- If you build the repository directly, you would get an `-UNKNOWN` version, which indicates that due to not running through the CI process we do not know what particular version (if any) that you are using.

Release Process
---------------

1. Update the version listed in `./source/global/version.bas`. Update both the `Version$` label and also the `$VERSIONINFO` entries.
2. After `version.bas` is compiled, wait for the CI build on `main` and also update of `./internal/source` to finish.
3. Tag the latest commit in `main` with the format of `vX.Y.Z` where `X.Y.Z` is the version being released.
4. Wait for the tag build to finish, it will create a draft release for the new version.
5. Edit the draft release to include release notes, and publish it when ready.
