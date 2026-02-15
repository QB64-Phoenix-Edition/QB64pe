#!/bin/bash

buildPlatform=$1
version=$2
format=

ARCHIVE_ROOT=qb64pe
DIST_ROOT=./dist/$ARCHIVE_ROOT

# populate internal/help from static download and updates from Wiki
curl --silent --retry 3 -o ./help.zip https://qb64phoenix.com/qb64_files/help.zip
unzip -oqq ./help.zip -d ./internal
./qb64pe -u
# end internal/help

case "$buildPlatform" in
    win)
        ./internal/c/c_compiler/bin/mingw32-make.exe OS=win clean
        ;;
    lnx)
        make OS=lnx clean
        ;;
    osx)
        make OS=osx clean
        ;;
esac

mkdir -p $DIST_ROOT
mkdir -p $DIST_ROOT/internal
mkdir -p $DIST_ROOT/internal/c

rm -f ./source/.gitignore
cp -rp ./source   $DIST_ROOT
cp -rp ./licenses $DIST_ROOT
cp ./COPYING.txt  $DIST_ROOT
cp ./README.md    $DIST_ROOT
cp ./qb64pe.1     $DIST_ROOT
cp ./Makefile     $DIST_ROOT

cp -rp ./internal/source  $DIST_ROOT/internal/
cp -rp ./internal/help    $DIST_ROOT/internal/
cp -rp ./internal/support $DIST_ROOT/internal/
cp -rp ./internal/temp    $DIST_ROOT/internal/
cp ./internal/config.ini  $DIST_ROOT/internal/
cp ./internal/version.txt $DIST_ROOT/internal/

cp -rp ./internal/c/libqb $DIST_ROOT/internal/c/
cp -rp ./internal/c/parts $DIST_ROOT/internal/c/
cp -p ./internal/c/*      $DIST_ROOT/internal/c/

case "$buildPlatform" in
    win)
        filename="qb64pe_win-$PLATFORM-$version.7z"

        format=7zip

        cp ./qb64pe.exe $DIST_ROOT
        cp -r ./internal/c/c_compiler $DIST_ROOT/internal/c/
        ;;

    lnx)
        filename="qb64pe_lnx-$version.tar.gz"
        format=tar

        # Not sure if we should distribute this
        # cp -p ./qb64pe       $DIST_ROOT
        cp -p ./setup_lnx.sh $DIST_ROOT
        ;;

    osx)
        filename="qb64pe_osx-$version.tar.gz"
        format=tar

        cp -p ./qb64pe_start.command $DIST_ROOT
        cp -p ./qb64pe.1             $DIST_ROOT
        cp -p ./setup_osx.command    $DIST_ROOT
        ;;
esac

cd ./dist

case "$format" in
    7zip)
        7z a "../$filename" ./$ARCHIVE_ROOT
        ;;

    tar)
        tar --create --auto-compress --file ../${filename} ./$ARCHIVE_ROOT
        ;;
esac
