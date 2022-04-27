#!/bin/bash

./qb64_bootstrap -x -w source/qb64.bas
SUCCESS=$?

rm qb64_bootstrap
rm internal/source/*
rm internal/temp/debug_* internal/temp/recompile_*

mv internal/temp/* internal/source/

find . -type f -iname "*.a" -exec rm {} \;
find . -type f -iname "*.o" -exec rm {} \;

exit $SUCCESS
