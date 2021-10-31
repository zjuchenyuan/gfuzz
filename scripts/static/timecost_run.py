#used for calculating time cost of gfuzz distance calculation
from runscinfo import *
import sys
version, pc = sys.argv[1:]
r=RUNSC_wrapper(version)
self = r
cgd = r.cgdistance(pc, returnraw=True)
r.func2pcs(None)
print(len(cgd), len(self.func2pcs_dict), len(r.cfgdistance(pc)), len(r.info), sep="\t")