#!/bin/bash
# Arg 1: Dist location

PREFIX="dist"

# Get absolute paths for everything since we're going to cd into the distribution directory
ROOT=$(pwd)
TEST_CASES="$ROOT/tests/dist"

RESULTS_DIR="$ROOT/tests/results/$PREFIX"
OUTPUT_DIR="$ROOT/tests/output/$PREFIX"

mkdir -p "$RESULTS_DIR"
mkdir -p "$OUTPUT_DIR"

# Move into distribution location
cd $1

# Specific steps for each platform
case "$2" in
    win)
        ;;

    lnx)
        ./setup_lnx.sh 1>"$RESULTS_DIR/linux-setup.txt"
        assert_success_named "Linux setup" cat "$RESULTS_DIR/linux-setup.txt"
        ;;

    osx)
        ./setup_osx.command 1>"$RESULTS_DIR/osx-setup.txt"
        assert_success_named "OSX setup" cat "$RESULTS_DIR/osx-setup.txt"
        ;;
esac

show_failure()
{
    cat "$RESULTS_DIR/$1-compile_result.txt"
    cat "$RESULTS_DIR/$1-compilelog.txt"
}

for basFile in $TEST_CASES/*.bas
do 
    test=$(basename $basFile .bas)
    outputExe="$OUTPUT_DIR/$test-output"

    TESTCASE=$test

    ./qb64 -x  "$TEST_CASES/$test.bas" -o "$outputExe" 1>$RESULTS_DIR/$test-compile_result.txt
    ERR=$?
    cp ./internal/temp/compilelog.txt $RESULTS_DIR/$test-compilelog.txt

    (exit $ERR)
    assert_success_named "compile" "Compilation Error:" show_failure $test

    test -f "$outputExe"
    assert_success_named "exe exists" "output.exe does not exist!" show_failure $test

    testResult=$("$outputExe" 2>&1)
    assert_success_named "run" "Execution Error:" echo "$testResult"

    [ "$testResult" == "$(cat $TEST_CASES/$test.result)" ]
    assert_success_named "result" "Result is wrong:" echo "$testResult"
done
