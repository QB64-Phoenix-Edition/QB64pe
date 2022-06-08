#!/bin/bash

OS=$1

echo -n "Bootstrapping QB64..."
make -j8 OS=$OS BUILD_QB64=y EXE=./qb64_bootstrap

if [ $? -ne 0 ]; then
  echo "QB64 bootstrap failed"
  exit 1
fi
echo "Done"
