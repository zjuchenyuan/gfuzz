import sys, os
from collections import Counter
sys.path.append("/g/scripts/static")
from runscinfo import *
rawtext =[i for i in """
0x4dd0000
0x537004a
0x4df0006
0x4d50002
0x4db0000
0x4e20007
0x3c9002a
0x463004c
0x49c0036
0x352005f
0x445006a
0x2a500f4
0x4470015
0x4520007
0x3b80067
0x42e0054
0x3740083
0x422000d
0x4450046
0x462000e
0x4620065
0x46200bb
0x4620116
0x4220008
""".strip().split("\n") if i]
dataset={}
for idx,line in enumerate(rawtext):
    l = line.split()
    pc = l[0]
    dataset[idx+1] = ["release-20210125.0", pc]

if __name__ == "__main__":
  for t, (version, pc) in dataset.items():
    t = str(t)
    r = RUNSC_wrapper(version)
    filepath = r.pc2info(pc)[0]
    syscalls_cg, syscalls = r.pc2syscalls_applyrules(pc)
    if not syscalls_cg:
        r = rtaRUNSC_wrapper(version)
        syscalls_cg, syscalls = r.pc2syscalls_applyrules(pc)
    bbdis = r.cfgdistance(pc)
    cgdis = r.cgdistance(pc)
    aflgodis = r.aflgodistance(pc)
    open(t+"_bbdis.json", "w").write(json.dumps(bbdis))
    open(t+"_cgdis.json", "w").write(json.dumps(cgdis))
    open(t+"_aflgodis.json", "w").write(json.dumps(aflgodis))
    print(t, version, pc, syscalls)
    open(t+"_syscalls.json", "w").write(json.dumps(sorted(syscalls)))