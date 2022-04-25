#!/bin/bash

OS=$1

filename="qb64_${OS}.tar.gz"

cd ..
tar --create --auto-compress --file ${filename} --exclude-from=./QB64pe/.ci/common-exclusion.list --exclude-from=./QB64pe/.ci/$OS-exclusion.list ./QB64pe
mv "./$filename" "./QB64pe/"
