#!/bin/bash

os=$1

./qb64_bootstrap -x -w source/qb64.bas
SUCCESS=$?

rm qb64_bootstrap
rm internal/source/*
rm internal/temp/debug_* internal/temp/recompile_*
rm internal/temp/qb64.sym
rm internal/temp/qb64_bootstrap.sym

mv internal/temp/* internal/source/


# Build libqb test executables
make -j8 OS=$os build-tests

make clean OS=$os

exit $SUCCESS
