#!/bin/bash
set -ex
P=$PWD
commit=$1
export COMMIT=$1
if [ "$NOCLEAN" == "" ]; then
    git status|grep "working tree clean" || git checkout .
    git status|grep "$commit"||git checkout $COMMIT
    bazel clean
fi
rm -rf /root/.cache/bazel
rm -rf bazel-bin/gopath
bazel build //:gopath
cd bazel-bin/gopath
export GOPATH=`pwd`
cp ${P}/runsc/*.go src/gvisor.dev/gvisor/runsc/
cp ${P}/go* src/gvisor.dev/gvisor/
#GO111MODULE=off go build gvisor.dev/gvisor/runsc
rm -rf src.bak r
cp -r -L src src.bak
mkdir r
cd r
unzip -q /g/gvisor_bin/$COMMIT/r.zip 
cd ..
python3 /g/scripts/build/r2gopath.py r src/
grep InitCoverageData src/gvisor.dev/gvisor/pkg/coverage/coverage.go ||cp /g/scripts/build/coverage.go src/gvisor.dev/gvisor/pkg/coverage/coverage.go
#GO111MODULE=off go build gvisor.dev/gvisor/runsc
GO111MODULE=off /g/go-callvis -file /g/gvisor_bin/$COMMIT/edge.txt gvisor.dev/gvisor/runsc
GO111MODULE=off /g/go-callvis.rta -file /g/gvisor_bin/$COMMIT/rtaedge.txt gvisor.dev/gvisor/runsc
cd ..
mkdir -p /gvisor_build/$COMMIT/
cp -r -L gopath /gvisor_build/$COMMIT/

echo ${commit} gopath done
