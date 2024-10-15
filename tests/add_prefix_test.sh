#!/bin/bash

PREFIX="addprefix"
RESULTS_DIR="./tests/results/$PREFIX"
mkdir -p "$RESULTS_DIR"
QB64="$1"
OS=$CI_OS

show_failure()
{
    cat "$RESULTS_DIR/addprefix-$1_result.txt"
}

show_incorrect_result()
{
    diff -u <(echo -n "$1") <(echo -n "$2")
}


EXE="$RESULTS_DIR/AddPREFIX"
if [[ "$OS" == "win" ]]; then
    EXE="$EXE.exe"
fi

# First attempt to compile converter
rm -fr internal/temp/*
rm -f "$EXE*"
compileResultOutput="$RESULTS_DIR/addprefix-compile_result.txt"
"$QB64" -x internal/support/converter/AddPREFIX.bas -o "${EXE}" 1>"$compileResultOutput"
ERR=$?
cp_if_exists ./internal/temp/compilelog.txt "$RESULTS_DIR/addprefix-compilelog.txt"
(exit $ERR)
assert_success_named "Compile" "Compilation Error:" show_failure "compile"

test -f "$EXE"
assert_success_named "exe exists" "addprefix-output executable does not exist!" show_failure "compile"

# Copy test case into place so converted result ends up in the results directory
cp tests/converter_tests/addprefix.bas "$RESULTS_DIR/addprefix.bas"

# Do conversion
conversionResultOutput="$RESULTS_DIR/addprefix-convert_result.txt"
"$EXE" "$RESULTS_DIR/addprefix.bas" 1> "$conversionResultOutput"
ERR=$?
(exit $ERR)
assert_success_named "Convert" "Conversion Error:" show_failure "convert"

# Confirm result is as expected
expectedResult="$(cat "tests/converter_tests/addprefix.output")"
actualResult="$(cat "$RESULTS_DIR/addprefix.bas")"
[[ "$expectedResult" == "$actualResult" ]]
assert_success_named "result" "Result is wrong:" show_incorrect_result "$expectedResult" "$actualResult"
