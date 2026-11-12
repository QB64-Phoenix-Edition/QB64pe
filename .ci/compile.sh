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
rm -f internal/temp/*.o internal/temp/*.gch internal/temp/*.d

# This file contains machine-specific information, it can't work with
# ./internal/source and is not needed
rm -f internal/temp/embeds.txt

# globalx.txt is only included by a split build's qbx_gen.h, and the bootstrap
# is never a split build.
rm -f internal/temp/globalx.txt

# These are extra files only created for split builds
rm -f internal/temp/tu_*.cpp internal/temp/split.mk internal/temp/objs.rsp \
      internal/temp/qbx_gen.h internal/temp/main_split.txt internal/temp/compinfo.cpp

# If SplitObjectFiles=true is set then the compiler will not produce a
# main.txt. The build scripts are currently not set up to handle this, so
# ./internal/source requires that main.txt exist.
if [ ! -f internal/temp/main.txt ]; then
    echo "ERROR: internal/temp/main.txt is missing - the snapshot cannot be bootstrapped."
    echo "       Check if it was compiled with SplitObjectFiles=true"
    exit 1
fi

mv internal/temp/* internal/source/

# We mark what bitness ./internal/source was generated for so that
# ./setup_lnx.sh knows whether the build being attempted is valid (IE. They
# don't try to build the 64-bit version on a 32-bit machine).
echo "$BITS" > ./internal/source/.bits

# Build libqb test executables
make -j8 OS=$os BITS=$BITS build-tests

make clean OS=$os

exit $SUCCESS
