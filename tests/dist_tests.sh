#!/bin/bash
# Arg 1: Dist location

PREFIX="dist"

# Get absolute paths for everything since we're going to cd into the distribution directory
ROOT=$(pwd)
TEST_CASES="$ROOT/tests/dist"

RESULTS_DIR="$ROOT/tests/results/$PREFIX"

mkdir -p "$RESULTS_DIR"

# Move into distribution location
cd $1

# Verify that ./internal/temp/ is empty save for temp.bin
# xargs trims the front whitespace on OSX
tempCount=$(find ./internal/temp/ -type f | wc -l | xargs)
[ "$tempCount" == "1" ]
assert_success_named "./Internal/temp file count" echo "Temp has too many files: $tempCount"

# Specific steps for each platform
case "$2" in
    win)
        # Verify that the Resource information was correctly applied
        # windres returns an error if the exe has no resource section
        windresResult=$($ROOT/internal/c/c_compiler/bin/llvm-objdump -s -j .rsrc ./qb64pe.exe)
        assert_success_named "Windows Resource Section" printf "\n$windresResult\n"
        ;;

    lnx)
        ./setup_lnx.sh "dont_run" 1>"$RESULTS_DIR/linux-setup.txt"
        assert_success_named "Linux setup" cat "$RESULTS_DIR/linux-setup.txt"
        ;;

    osx)
        # When testing the OSX script we run it from a different directory as
        # that is the typical way it is used.
        pushd . > /dev/null
        cd "$ROOT"

        $1/setup_osx.command "dont_run" 1>"$RESULTS_DIR/osx-setup.txt"
        ERR=$?

        popd > /dev/null

        (exit $ERR)
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
    outputExe="$RESULTS_DIR/$test-output"

    TESTCASE=$test

    ./qb64pe -x  "$TEST_CASES/$test.bas" -o "$outputExe" 1>$RESULTS_DIR/$test-compile_result.txt
    ERR=$?
    cp_if_exists ./internal/temp/compilelog.txt $RESULTS_DIR/$test-compilelog.txt

    (exit $ERR)
    assert_success_named "compile" "Compilation Error:" show_failure $test

    test -f "$outputExe"
    assert_success_named "exe exists" "output.exe does not exist!" show_failure $test

    testResult=$("$outputExe" 2>&1)
    assert_success_named "run" "Execution Error:" echo "$testResult"

    [ "$testResult" == "$(cat $TEST_CASES/$test.result)" ]
    assert_success_named "result" "Result is wrong:" echo "$testResult"
done
