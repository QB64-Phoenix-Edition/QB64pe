#!/bin/bash

# Pull the version out of the QB64 source. A little ugly but we're match for the first assignment line
versionLine=$(cat ./source/global/version.bas | grep "Version\$ = " | head -n 1 | tr -d '\n' | tr -d '\r')

# This uses a regular expression to pull the X.Y.Z version out of the assignment line
version=$(echo "$versionLine" | sed -r 's/^Version\$ = "([0-9]*\.[0-9]*\.[0-9]*)"/\1/' | tr -d '\n')

# if ./internal/version.txt exists, then we have a tag at the end of our version number
# (IE. non-release version)
if [ -f "./internal/version.txt" ]
then
    version="$version$(cat ./internal/version.txt | tr -d '\n' | tr -d '\r')"
fi

printf "$version"
