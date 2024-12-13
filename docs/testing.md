QB64-PE Testing Framework
=========================

QB64-PE features two separate testing frameworks, one for QB64-based tests
(mainly for the compiler and language functionality), and one for C++-based
tests (for `libqb`).

QB64-based Tests
----------------

The QB64-based tests live in `./tests/compile_tests`. Each folder in that
directory represents a different category of tests, and within each of those
category folders are `*.bas` files. Accompanying each `*.bas` file is either an
`*.output` or `*.err` file with the same base name as a corresponding `*.bas`
file.  For test with an `*.output`, the `*.output` file contains the text that
the program compiled from the `*.bas` file should produce. For `*.err` tests,
the `*.err` file contains the text of the error the QB64-PE compiler should
produce when attempting to compile that code.

To avoid unexpected test failures, you should write the `*.bas` in such a way
that it at best does not produce lines with trailing spaces. If this is not
possible e.g. by the use of formatted output, then make sure these trailing
spaces also exist in the `*.output` and/or `*.err` files respectively, those
spaces are often overseen or even get stripped by some Editors when saving the
file. Also the test **can not** produce trailing empty lines, you **must** make
sure the test output ends with the last meaningful line.

Tests can also have an optional `*.flags` file. The contents of this file will
be provide as command line arguments to QB64-PE when compiling the test source.

Tests are compiled from the directory the `.bas` file is in. That is, they are
compiled as though the user `cd`'d into the directory the `.bas` file is in,
and then ran the compiler from that starting location to compile the `.bas`. If
necessary this behavior can be changed by including a `*.compile-from-base`
file next to the associated `*.bas` test file. This will case the test script
to compile the `*.bas` file from the directory the compiler is located in
instead, and the full path to the `*.bas` file will be provided to `-x`.

These tests are run via the `./tests/compile_tests.sh` script. The script
searches the `./tests/compile_tests` directory for any `*.bas` files, and then
uses the provide QB64-PE compiler to test them.

C++-based Tests
---------------

The C++-based testing lives in `./tests/c/`. The tests themselves exist as
`.cpp` files that compile into individual programs (each has its own `main()`).
The tests make use of `test.h` to run their tests and give proper output, and
failure is reported via the exit code.

The C++ tests are compiled using the `./tests/build.mk` make build logic. It's
fairly straight forward, each test executable gets its own name and then its
own combination of source/object files necessary to produce it. If you're
testing functionality from `libqb`, then you need to link the proper object
files into your test.  Additionally, any extra compiler flags can be defined
individually for each test.

`./tests/run_c_tests.sh` is a script that runs all of the C++ tests during the
build. Internally it has a list of the names of all the test executables (to
avoid having to use `make` to discover them). It expects the tests to have
already been built via a `make build-tests` call, and runs the executables one
at a time and checks their exit code for errors.

QBasic Testcases
----------------

The QBasic testcases are a selection of QBasic programs that live in
`./tests/qb64_testcases`. During the build process all of these programs are
tested to verify that QB64-PE is still capable of compiling them with no
errors. The behavior of the compiled programs is not verified.
