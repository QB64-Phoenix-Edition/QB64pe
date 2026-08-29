#!/bin/bash

os=$1

if [ "$PLATFORM" = "x86" ]; then
    BITS=32
else
    BITS=64
fi

./qb64pe_bootstrap -f:TargetBits=$BITS -x -w source/qb64pe.bas
SUCCESS=$?

rm qb64pe_bootstrap
rm internal/source/*
rm internal/temp/debug_* internal/temp/recompile_*
rm internal/temp/qb64pe.sym
rm internal/temp/qb64pe_bootstrap.sym

mv internal/temp/* internal/source/

# We mark what bitness ./internal/source was generated for so that
# ./setup_lnx.sh knows whether the build being attempted is valid (IE. They
# don't try to build the 64-bit version on a 32-bit machine).
echo "$BITS" > ./internal/source/.bits

# Build libqb test executables
make -j8 OS=$os BITS=$BITS build-tests

make clean OS=$os

exit $SUCCESS
