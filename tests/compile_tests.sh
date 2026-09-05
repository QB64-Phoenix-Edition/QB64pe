#!/bin/bash
# Arg 1: qb64 location
# Arg 2: Optional category to test

PREFIX="Compilation"

RESULTS_DIR="./tests/results/$PREFIX"

mkdir -p $RESULTS_DIR

QB64=$1

if [ "$#" -ge 2 ]; then
    CATEGORY="/$2"
fi

if [ "$#" -eq 3 ]; then
    TESTS_TO_RUN="$3"
else
    TESTS_TO_RUN='*.bas'
fi

show_failure()
{
    cat "$RESULTS_DIR/$1-$2-compile_result.txt"
    cat "$RESULTS_DIR/$1-$2-compilelog.txt"
}

show_incorrect_result()
{
    printf "EXPECTED: '%s'\n" "$1"
    printf "GOT:      '%s'\n" "$2"
}

filesize_without_trailing_newlines()
{
    local file="$1"
    local size
    local byte

    size=$(wc -c < "$file")

    while (( size > 0 )); do
        byte=$(dd if="$file" bs=1 skip=$((size - 1)) count=1 2>/dev/null | od -An -tu1)
        byte=${byte//[[:space:]]/}

        # Bash command substitution historically ignored trailing newlines.
        # Treat both LF and CRLF as line endings, but preserve a bare CR.
        if [ "$byte" != "10" ]; then
            break
        fi

        # Remove LF.
        size=$((size - 1))

        # On Windows the line ending may be CRLF, so remove the CR that
        # belongs to this LF as well.
        if (( size > 0 )); then
            byte=$(dd if="$file" bs=1 skip=$((size - 1)) count=1 2>/dev/null | od -An -tu1)
            byte=${byte//[[:space:]]/}

            if [ "$byte" == "13" ]; then
                size=$((size - 1))
            fi
        fi
    done

    printf '%s\n' "$size"
}

compare_output_files()
{
    expectedSize=$(filesize_without_trailing_newlines "$1")
    actualSize=$(filesize_without_trailing_newlines "$2")

    if [ "$expectedSize" -ne "$actualSize" ]; then
        return 1
    fi

    cmp <(head -c "$expectedSize" "$1") <(head -c "$actualSize" "$2") > /dev/null
}

# This env variable exists when running in CI. It can also be defined locally
# to enable the small OS-dependent testing
#
# This is either win, lnx, or osx
OS=$CI_OS

# On Linux, we make use of xvfb-run to provide each test with a framebuffer
# based X server, which allows graphics to work.
if [ "$OS" == "lnx" ]; then
    LNX_PREFIX=xvfb-run
fi

# Each .bas file represents a separate test.
while IFS= read -r test
do
    category=$(basename "$(dirname "$test")")
    testName=$(basename "$test" .bas)

    TESTCASE="$category/$testName"
    EXE="$RESULTS_DIR/$category-$testName - output"

    if [ "$OS" == "win" ]; then
        EXE="$EXE.exe"
    fi

    # If a .err file exists, then this test is actually testing a compilation error
    testType="success"
    if test -f "./tests/compile_tests/$category/$testName.err"; then
        testType="error"
    fi

    # Clear out temp folder before next compile, avoids stale compilelog files
    rm -fr ./internal/temp/*

    # Clean up existing EXE, so we don't use it by accident
    rm -f "$EXE*"

    compileResultOutput="$RESULTS_DIR/$category-$testName-compile_result.txt"

    # A .flags file contains any extra compiler flags to provide to QB64 for this test
    compilerFlags=
    if test -f "./tests/compile_tests/$category/$testName.flags"; then
        compilerFlags=$(cat "./tests/compile_tests/$category/$testName.flags")
    fi

    # If a license file for this OS exists, then we also check the generated license is correct
    checkLicense=
    if [ ! -z "$OS" ] && test -f "./tests/compile_tests/$category/$testName.$OS.license"; then
        compilerFlags="$compilerFlags -f:GenerateLicenseFile=true"
        checkLicense=y
    fi

    # A .noprompt file overrides the QB64PE_NOPROMPT value used when running this
    # test. The default stops the program at an unhandled error; a test that needs
    # the 'continue' behavior can put "Continue" in this file to get it.
    nopromptValue=y
    if test -f "./tests/compile_tests/$category/$testName.noprompt"; then
        nopromptValue=$(cat "./tests/compile_tests/$category/$testName.noprompt")
    fi

    # If the "compile-from-base" file exists, then this test should be compiled
    # from the ./qb64pe directory instead of the test directory
    compileFromBase=
    if test -f "./tests/compile_tests/$category/$testName.compile-from-base"; then
        compileFromBase=y
    fi

    if [ "$compileFromBase" == "y" ]; then
        # -m and -q make sure that we get predictable results
        "$QB64" "-f:OptimizeCppProgram=true" "-f:StripDebugSymbols=false" $compilerFlags -q -m -x "./tests/compile_tests/$category/$testName.bas" -o "$EXE" 1>"$compileResultOutput"
        ERR=$?
    else
        pushd . >/dev/null
        cd "./tests/compile_tests/$category"

        # -m and -q make sure that we get predictable results
        "../../../$QB64" "-f:OptimizeCppProgram=true" "-f:StripDebugSymbols=false" $compilerFlags -q -m -x "$testName.bas" -o "../../../$EXE" 1>"../../../$compileResultOutput"
        ERR=$?

        popd >/dev/null
    fi

    cp_if_exists ./internal/temp/compilelog.txt "$RESULTS_DIR/$category-$testName-compilelog.txt"

    if [ "$testType" == "success" ]; then
        (exit $ERR)
        assert_success_named "Compile" "Compilation Error:" show_failure "$category" "$testName"

        test -f "$EXE"
        assert_success_named "exe exists" "$test-output executable does not exist!" show_failure "$category" "$testName"

        if [ "$checkLicense" == "y" ]; then
            expectedResult="$(cat "./tests/compile_tests/$category/$testName.$OS.license")"
            testResult="$(cat "$EXE.license.txt")"

            [ "$testResult" == "$expectedResult" ]
            assert_success_named "license" "License file is wrong:" show_incorrect_result "$expectedResult" "$testResult"
        fi

        # Some tests do not have an output or err file because they should
        # compile successfully but cannot be run on the build agents
        if [ ! -f "./tests/compile_tests/$category/$testName.output" ]; then
            continue
        fi

        runOutput="$RESULTS_DIR/$category-$testName-run-output.txt"

        pushd . > /dev/null
        cd "./tests/compile_tests/$category"
        QB64PE_NOPROMPT="$nopromptValue" \
        QB64PE_LOG_HANDLERS=file \
        QB64PE_LOG_SCOPES="qb64,libqb,libqb-image,libqb-audio" \
        QB64PE_LOG_FILE_PATH="../../../$RESULTS_DIR/$category-$testName-log.txt" \
        $LNX_PREFIX "../../../$EXE" "../../../$RESULTS_DIR" "$category-$testName" \
            > "../../../$runOutput" 2>&1
        ERR=$?
        popd > /dev/null

        (exit $ERR)
        assert_success_named "run" "Execution Error:" cat "$runOutput"

        (exit $ERR) || (echo "Log from failed run is below:"; cat "$RESULTS_DIR/$category-$testName-log.txt")

        compare_output_files "./tests/compile_tests/$category/$testName.output" "$runOutput"
        assert_success_named "result" "Result is wrong:" \
            diff "./tests/compile_tests/$category/$testName.output" "$runOutput"

        # Restart pulseaudio between each test to make sound tests work on Linux
        if [ "$CI_TESTING" == "y" ] && command -v pulseaudio > /dev/null
        then
            pulseaudio -k
            sleep .5
            pulseaudio -D
        fi
    else
        ! (exit $ERR)
        assert_success_named "Compile" "Compilation Success, was expecting error:" show_failure "$category" "$testName"

        ! test -f "$EXE"
        assert_success_named "Exe exists" "'$category-$testName - output' exists, it should not!" show_failure "$category" "$testName"

        expectedErr="$(cat "./tests/compile_tests/$category/$testName.err")"

        diffResult=$(diff -y "./tests/compile_tests/$category/$testName.err" "$compileResultOutput")
        assert_success_named "Error result" "Error reporting is wrong:" echo "$diffResult"
    fi
done < <(find "./tests/compile_tests$CATEGORY" -name "$TESTS_TO_RUN" -print)
