#!/bin/bash
set -ex
commit=$1
export COMMIT=$1
cd /out.$COMMIT
cp /g/gvisor_bin/$COMMIT/edge.txt ./
export CODE=/g/scripts/ir2cfg
python3 $CODE/extract_func.py
$CODE/main --get-fn gvisor_all.ll
python3 $CODE/demangle.py
$CODE/main --global-cfg gvisor_all.ll
cp global_cfg.txt /g/gvisor_bin/${COMMIT}/
echo ${COMMIT} done
