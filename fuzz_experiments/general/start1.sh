#!/bin/bash
NAME="general_aflgo_1_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_1_1.conf &
sleep 5
NAME="general_gfuzz_1_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_1_1.conf &
sleep 5
NAME="general_aflgo_1_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_1_2.conf &
sleep 5
NAME="general_gfuzz_1_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_1_2.conf &
sleep 5
NAME="general_aflgo_1_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_1_3.conf &
sleep 5
NAME="general_gfuzz_1_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_1_3.conf &
sleep 5
NAME="general_aflgo_1_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_1_4.conf &
sleep 5
NAME="general_gfuzz_1_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_1_4.conf &
sleep 5
NAME="general_aflgo_2_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_2_1.conf &
sleep 5
NAME="general_gfuzz_2_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_2_1.conf &
sleep 5
NAME="general_aflgo_2_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_2_2.conf &
sleep 5
NAME="general_gfuzz_2_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_2_2.conf &
sleep 5
NAME="general_aflgo_2_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_2_3.conf &
sleep 5
NAME="general_gfuzz_2_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_2_3.conf &
sleep 5
NAME="general_aflgo_2_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_2_4.conf &
sleep 5
NAME="general_gfuzz_2_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_2_4.conf &
sleep 5
NAME="general_aflgo_3_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_3_1.conf &
sleep 5
NAME="general_gfuzz_3_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_3_1.conf &
sleep 5
NAME="general_aflgo_3_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_3_2.conf &
sleep 5
NAME="general_gfuzz_3_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_3_2.conf &
sleep 5
NAME="general_aflgo_3_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_3_3.conf &
sleep 5
NAME="general_gfuzz_3_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_3_3.conf &
sleep 5
NAME="general_aflgo_3_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_3_4.conf &
sleep 5
NAME="general_gfuzz_3_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_3_4.conf &
sleep 5
NAME="general_aflgo_4_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_4_1.conf &
sleep 5
NAME="general_gfuzz_4_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_4_1.conf &
sleep 5
NAME="general_aflgo_4_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_4_2.conf &
sleep 5
NAME="general_gfuzz_4_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_4_2.conf &
sleep 5
NAME="general_aflgo_4_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_4_3.conf &
sleep 5
NAME="general_gfuzz_4_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_4_3.conf &
sleep 5
NAME="general_aflgo_4_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_aflgo_4_4.conf &
sleep 5
NAME="general_gfuzz_4_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_gfuzz_4_4.conf &
sleep 5
NAME="general_origin1_release-20210125.0_1"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/origin_syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_origin1_release-20210125.0_1.conf &
sleep 5
NAME="general_origin1_release-20210125.0_2"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/origin_syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_origin1_release-20210125.0_2.conf &
sleep 5
NAME="general_origin1_release-20210125.0_3"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/origin_syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_origin1_release-20210125.0_3.conf &
sleep 5
NAME="general_origin1_release-20210125.0_4"
mkdir -p /g/output/general/${NAME}
cd /g/output/general/${NAME}/
timeout -k 60 86400 /g/origin_syzkaller/bin/syz-manager -bench bench.log -config /g/conf/general/general_origin1_release-20210125.0_4.conf &
sleep 5
