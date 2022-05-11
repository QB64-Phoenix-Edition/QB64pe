#!/bin/bash

TEST_SCRIPT=$1
shift

. ./tests/colors.sh

export TOTAL_RESULT=0
export PREFIX=""
export TESTCASE=""
export TEST_COUNT=0

# Asserts that the exit code of the previous command was successful (IE. zero)
#
# Argument 1: Name for this specific test
# Argument 2: Optional message to display on error
# Arguments 3+: Optional Command + arguments to run on error
assert_success_named ()
{
    RESULT=$?
    NAME=$1
    ERRORMSG=$2
    shift
    shift

    TEST_COUNT=$(($TEST_COUNT + 1))

    printf "$PREFIX: $TEST_COUNT:"

    if ! [ -z "$TESTCASE" ]; then
        printf " $TESTCASE:"
    fi

    if ! [ -z "$NAME" ]; then
        printf " $NAME:"
    fi

    if [ "$RESULT" -eq 0 ]; then
        echo "$GREEN PASS!$RESET"
    else
        printf "$RED FAILURE!$RESET"

        if ! [ -z "$ERRORMSG" ]; then
            echo " - $ERRORMSG"
        else
            echo
        fi

        TOTAL_RESULT=$(($TOTAL_RESULT + 1))

        if [ $# -gt 0 ]; then
            "$@"
        fi
    fi
}

# Asserts that the exit code of the previous command was successful (IE. zero)
#
# Argument 1: Optional message to display on error
# Arguments 2+: Optional Command + arguments to run on error
assert_success ()
{
    assert_success_named "" "$@"
}

assert_ignored ()
{
    TEST_COUNT=$(($TEST_COUNT + 1))
    echo "$PREFIX: $TEST_COUNT: $TESTCASE:$YELLOW IGNORED!$RESET"
}

cp_if_exists ()
(
    [ -f "$1" ] && cp "$1" "$2"
)

. "$TEST_SCRIPT" "$@"

exit $TOTAL_RESULT
