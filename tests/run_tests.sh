#!/bin/bash

result=0

./tests/assert.sh ./tests/compile_tests.sh ./qb64pe || result=1
./tests/assert.sh ./tests/qbasic_tests.sh ./qb64pe || result=1
./tests/assert.sh ./tests/format_tests.sh ./qb64pe || result=1

exit $result
