#!/bin/bash
set -x
export COMMIT=$1 
[ -f "/out.$COMMIT/gvisor_all.ll" ] && exit
cd /gvisor_build/$COMMIT/gopath
export GOPATH=`pwd`
find -name '*.s' -delete
rm -rf src/golang.org/x/sys
go get -x -work gvisor.dev/gvisor/runsc |& tee lastbuild.log
python3 /g/scripts/build/buildlog_rewrite.py $COMMIT > build.sh
sh build.sh

while [ "`pgrep -c llvm-goc`" -gt 0 ]; do 
    echo cnt: `pgrep -c llvm-goc`
    sleep 10; 
done
cd /out.$COMMIT
/build-debug/bin/llvm-link -S `find gvisor.dev/ -name '*.ll'` -o gvisor_all.ll
echo $COMMIT llvm IR build finished
