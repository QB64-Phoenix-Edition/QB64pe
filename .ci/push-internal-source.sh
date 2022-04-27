#!/bin/bash

git config --local user.email "41898282+github-actions[bot]@users.noreply.github.com"
git config --local user.name "github-actions[bot]"
git add ./internal/source/

if ! git diff --cached --quiet; then
    echo "Pushing updated ./internal/source"
    git commit -m 'Automatic update of ./internal/source'
    git push
else
    echo "No changes to ./internal/source"
fi
