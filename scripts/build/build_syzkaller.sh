#!/bin/bash
ROOT=`git rev-parse --show-toplevel`
git clone https://github.com/google/syzkaller
cd syzkaller
git checkout 9d751681c
git apply $ROOT/0001-gfuzz-gfuzz-changes.patch
make
