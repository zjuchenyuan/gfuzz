#!/bin/bash
set -ex
commit=$1
#[ -z "$NOCLEAN" ] && bazel clean
rm -rf /tmp/r /tmp/r_${commit} /g/gvisor_bin/${commit}
mkdir /tmp/r
git checkout .
rm -rf /root/.cache/bazel
git status|grep ${commit} || git checkout ${commit}

buildit(){
    bazel build --collect_code_coverage --instrumentation_filter="...,-//pkg/sentry/platform,-//pkg/coverage:coverage,-//pkg/ring0" //runsc:runsc
}

checkbuild(){
    $(ls -t `find bazel-out -name runsc -follow`|head -n1) symbolize -all|md5sum
    ls /tmp/r/ | grep gvisor.dev >/dev/null
}

retrybuild(){
    python3 /g/scripts/build/rewritebuilder.py
    buildit
    checkbuild
}


retry2(){
    rm -rf /root/.cache/bazel
    buildit
}

buildit || retry2
checkbuild || retrybuild

mkdir -p /g/gvisor_bin/${commit}
cp $(ls -t `find bazel-out -name runsc -follow`|head -n1) /g/gvisor_bin/${commit}/
python3 /g/scripts/build/packtmpr.py ${commit}

/g/gvisor_bin/${commit}/runsc symbolize -all|md5sum
echo ${commit}
