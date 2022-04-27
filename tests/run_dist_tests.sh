#!/bin/bash

result=0

./tests/assert.sh ./tests/dist_tests.sh "$1" "$2" || result=1

exit $result
