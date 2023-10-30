#!/bin/bash

OS=$1

echo -n "Bootstrapping QB64-PE..."
gmake -j8 OS=$OS BUILD_QB64=y EXE=./qb64pe_bootstrap

if [ $? -ne 0 ]; then
  echo "QB64-PE bootstrap failed"
  exit 1
fi
echo "Done"
