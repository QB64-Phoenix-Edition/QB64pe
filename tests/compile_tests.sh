#!/bin/bash
# Arg 1: qb54 location

PREFIX="Compilation"

RESULTS_DIR="./tests/results/$PREFIX"

mkdir -p $RESULTS_DIR

QB64=$1

show_failure()
{
    cat "$RESULTS_DIR/$1-compile_result.txt"
    cat "$RESULTS_DIR/$1-compilelog.txt"
}

for test in $(ls ./tests/compile_tests)
do 
    TESTCASE=$test

    "$QB64" -x  "./tests/compile_tests/$test/test.bas" -o "$RESULTS_DIR/$test-output.exe" 1>$RESULTS_DIR/$test-compile_result.txt
    ERR=$?
    cp ./internal/temp/compilelog.txt $RESULTS_DIR/$test-compilelog.txt

    (exit $ERR)
    assert_success_named "Compile" "Compilation Error:" show_failure $test
done
