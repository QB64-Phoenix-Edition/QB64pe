#!/bin/bash

os=$1

./qb64pe_bootstrap -x -w source/qb64pe.bas
SUCCESS=$?

rm qb64pe_bootstrap
rm internal/source/*
rm internal/temp/debug_* internal/temp/recompile_*
rm internal/temp/qb64pe.sym
rm internal/temp/qb64pe_bootstrap.sym

mv internal/temp/* internal/source/


# Build libqb test executables
gmake -j8 OS=$os build-tests

gmake clean OS=$os

exit $SUCCESS
