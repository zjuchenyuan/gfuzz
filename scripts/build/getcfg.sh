#!/bin/bash
set -ex
commit=$1
export COMMIT=$1
export CODE=/g/scripts/ir2cfg

cd /out.$COMMIT
cp /g/gvisor_bin/$COMMIT/edge.txt ./
python3 $CODE/extract_func.py
$CODE/main --get-fn gvisor_all.ll
python3 $CODE/demangle.py
$CODE/main --global-cfg gvisor_all.ll
cp global_cfg.txt /g/gvisor_bin/${COMMIT}/

#rta again
cp /g/gvisor_bin/$COMMIT/rtaedge.txt ./edge.txt
python3 $CODE/extract_func.py
$CODE/main --get-fn gvisor_all.ll
python3 $CODE/demangle.py
$CODE/main --global-cfg gvisor_all.ll
cp global_cfg.txt /g/gvisor_bin/${COMMIT}/rtaglobal_cfg.txt

echo ${COMMIT} done
