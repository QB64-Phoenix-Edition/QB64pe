#!/bin/bash

buildPlatform=$1

case "$buildPlatform" in
    windows-latest)
        filename="qb64_win-$PLATFORM.7z"

        cd ..
        7z a "-xr@QB64pe\.ci\common-exclusion.list" "-xr@QB64pe\.ci\win-exclusion.list" "$filename" "QB64pe"
        ;;

    ubuntu-latest)
        filename="qb64_lnx.tar.gz"

        cd ..
        tar --create --auto-compress --file ${filename} --exclude-from=./QB64pe/.ci/common-exclusion.list --exclude-from=./QB64pe/.ci/lnx-exclusion.list ./QB64pe
        ;;

    macos-latest)
        filename="qb64_osx.tar.gz"

        cd ..
        tar --create --auto-compress --file ${filename} --exclude-from=./QB64pe/.ci/common-exclusion.list --exclude-from=./QB64pe/.ci/osx-exclusion.list ./QB64pe
        ;;

esac

mv "./$filename" "./QB64pe/"
