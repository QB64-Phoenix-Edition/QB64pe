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
    cat "$RESULTS_DIR/$1-errorcompilelog.txt"
}

show_incorrect_result()
{
    printf "EXPECTED: '%s'\n" "$1"
    printf "GOT:      '%s'\n" "$2"
}

for test in $(ls ./tests/compile_tests)
do 
    TESTCASE=$test
    EXE="$RESULTS_DIR/$test-output"

    # Clear out temp folder before next compile, avoids stale compilelog files
    rm -fr ./internal/temp/*

    "$QB64" -x  "./tests/compile_tests/$test/test.bas" -o "$EXE" 1>$RESULTS_DIR/$test-compile_result.txt
    ERR=$?
    cp_if_exists ./internal/temp/compilelog.txt $RESULTS_DIR/$test-compilelog.txt
    cp_if_exists ./internal/temp/errorcompilelog.txt $RESULTS_DIR/$test-errorcompilelog.txt

    (exit $ERR)
    assert_success_named "Compile" "Compilation Error:" show_failure $test

    test -f "$EXE"
    assert_success_named "exe exists" "$test-output executable does not exist!" show_failure $test

    if [ ! -f "./tests/compile_tests/$test/test.output" ]; then
        continue
    fi

    expectedResult="$(cat ./tests/compile_tests/$test/test.output)"

    pushd . > /dev/null
    cd "./tests/compile_tests/$test"
    testResult=$("../../../$EXE" 2>&1)
    ERR=$?
    popd > /dev/null

    (exit $ERR)
    assert_success_named "run" "Execution Error:" echo "$testResult"

    [ "$testResult" == "$expectedResult" ]
    assert_success_named "result" "Result is wrong:" show_incorrect_result "$expectedResult" "$testResult"
done
