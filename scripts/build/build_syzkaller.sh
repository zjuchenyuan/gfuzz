#!/bin/bash
ROOT=`git rev-parse --show-toplevel`
git clone https://github.com/google/syzkaller
cd syzkaller
git checkout 9d751681c
cd ..
cp -r syzkaller origin_syzkaller
cd syzkaller
git apply $ROOT/scripts/build/gfuzz-changes.patch
make
cd ../origin_syzkaller
git apply $ROOT/scripts/build/origin_syzkaller.patch
make