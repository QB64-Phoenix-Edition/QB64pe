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
produce when attempting to compile that code. If test fails are expected on
specific platforms only, but success on other platforms, then rather specify any
of `*.win.err`, `*.lnx.err` or `*.mac.err` for the failing OS's and provide the
`*.output` file along with it for the success check.

To avoid unexpected test failures, you should write the `*.bas` in such a way
that it at best does not produce lines with trailing spaces. If this is not
possible e.g. by the use of formatted output, then make sure these trailing
spaces also exist in the `*.output` and/or `*.err` files respectively, those
spaces are often overseen or even get stripped by some Editors when saving the
file. Also the test **can not** produce trailing empty lines, you **must** make
sure the test output ends with the last meaningful line.

Tests can also have an optional `*.flags` file. The contents of this file will
be provide as command line arguments to QB64-PE when compiling the test source,
one argument per line.

A test that should be run with multiple sets of compiler settings can have an
optional `*.variants` file. Every non-empty line that does not start with `#`
adds one extra run of the test using the compiler flags listed on that line.
The run without any of those extra flags happens as well.

For a variant run, every run of a test compiles the same `*.bas`, but each run
gets its own name, that allows the runs can be told apart in the test output and
ensures they cannot overwrite each other's results. A run is named after the test
plus a label, where the label is the position of the line in the `*.variants`
file (`foo-1`), or the name given by writing the line as `label: flags` instead
(`foo-split`).

Every run is checked against the same `*.output` or `*.err`, because a program is
normally expected to behave the same whatever it was compiled with. A run that
genuinely has to differ can be given its own expected output named after the run
(`foo-split.output`, `foo-split.err`), which is used in place of the shared one
for that run only.

Setting `QB64PE_TEST_FLAGS` in the environment adds compiler flags to every run
of every test, in both the compile tests and the QBasic testcases

The tests run with `QB64PE_NOPROMPT=y` set, this ensures that any unhandled or
critical errors do not prompt and wait for input (which would hang the test
runner). To change this behavior tests can have an optional `*.noprompt` file,
the contents of this file will be provided as the value of `QB64PE_NOPROMPT`
when running the test executable. This is mostly useful for testing the
'Continue' behavior.

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
