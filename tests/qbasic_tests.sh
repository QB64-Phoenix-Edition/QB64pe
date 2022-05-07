#!/bin/bash
# Arg 1: qb54 location

PREFIX="QBasic"

RESULTS_DIR="./tests/results/$PREFIX"
EXES_DIR="./tests/exes/$PREFIX"

mkdir -p $RESULTS_DIR
mkdir -p $EXES_DIR

QB64=$1

show_failure()
{
    cat "$RESULTS_DIR/$1-compile_result.txt"
    cat "$RESULTS_DIR/$1-compilelog.txt"
}

for sourceFile in $(find ./tests/qbasic_testcases/n54/ -name '*.bas') \
                  ./tests/qbasic_testcases/open_gl/*.bas \
                  $(find ./tests/qbasic_testcases/pete -name '*.bas') \
                  $(find ./tests/qbasic_testcases/qb45com -name '*.bas') \
                  $(find ./tests/qbasic_testcases/thebob -name '*.bas') \
                  ./tests/qbasic_testcases/misc/*.bas
do 
    test=$(basename $sourceFile .bas)

    TESTCASE=$test

    "$QB64" -x  "$sourceFile" -o "./$EXES_DIR/$test-output.exe" 1>$RESULTS_DIR/$test-compile_result.txt
    ERR=$?
    cp ./internal/temp/compilelog.txt $RESULTS_DIR/$test-compilelog.txt

    (exit $ERR)
    assert_success_named "Compile" "Compilation Error:" show_failure $test
done
