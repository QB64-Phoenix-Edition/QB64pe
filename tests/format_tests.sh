#!/bin/bash
# Arg 1: qb64 location
# Arg 2: Optional category to test

# Format tests perform multiple test cases on the same source input, varying the format
# settings to produce a different output. The mapping between "foo.bas" and its variant
# outputs are given by the "foo.flagmap" file, which specifies an output file name and list
# of compiler flags to use, separated by a space. e.g.
#
# foo-indent-only.out -f:autoindent=true -f:indentsize=4 -f:autolayout=false

PREFIX="Format"

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
    cat "$RESULTS_DIR/$1-$2-$3-compile_result.txt"
}

show_incorrect_result()
{
    diff -u <(echo -n "$1") <(echo -n "$2")
}


while IFS= read -r test
do 
    category=$(basename "$(dirname "$test")")
    testName=$(basename "$test" .bas)

    flagmapping="$(< "tests/format_tests/$category/$testName.flagmap" tr -d '\r')"

    while IFS=' ' read -ra map
    do
        variant="${map[0]}"
        TESTCASE="$category/$testName/$variant"
        output="$RESULTS_DIR/$category-$testName-$variant-output"
        compileResultOutput="$RESULTS_DIR/$category-$testName-$variant-compile_result.txt"
        expectedResult="$(< "tests/format_tests/$category/$variant" tr -d '\r')"
        compilerFlags=("${map[@]:1}")
        pushd . >/dev/null
        cd "tests/format_tests/$category"
        "../../../$QB64" -y -m "${compilerFlags[@]}" "$testName.bas" -o "../../../$output" 1>"../../../$compileResultOutput"
        ERR=$?
        popd >/dev/null
        (exit $ERR)
        assert_success_named "Format" "Formatting Error:" show_failure "$category" "$testName" "$variant"
        test -f "$output"
        assert_success_named "output exists" "$test-output does not exist!" show_failure "$category" "$testName" "$variant"
        testResult="$(< "$output" tr -d '\r')"
        [ "$testResult" == "$expectedResult" ]
        assert_success_named "result" "Result is wrong:" show_incorrect_result "$expectedResult" "$testResult"
    done <<< "$flagmapping"
done < <(find "./tests/format_tests$CATEGORY" -name "$TESTS_TO_RUN" -print)
