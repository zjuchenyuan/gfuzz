import subprocess, sys, os, json, gzip
from pprint import pprint
from functools import lru_cache
DEBUG = os.environ.get("DEBUG", False)
BASEFOLDER = os.path.dirname(os.path.abspath(__file__))

def filenamefuncname2dotfuncname(filepath, fname):
    pkg = "/".join(filepath.split("/")[:-1])
    if fname[0]=="(":
        classname = fname.split(")")[0].replace("(", " ").split(" ")[-1]
        if classname[0]=="*":
            newname="(*gvisor.dev/gvisor/"+pkg+"."+classname[1:]+")"+fname.split(")")[1]
        else:
            newname="(gvisor.dev/gvisor/"+pkg+"."+classname+")"+fname.split(")")[1]
    else:
        newname = "gvisor.dev/gvisor/"+pkg+"."+fname
    return newname

def load_edgefile(filepath, return_call_map=False):
    call_map = {} #a->b {a:[b]}
    called_map = {} #a->b {b:[a]}
    if os.path.isfile(filepath+".gz"):
        fp = gzip.open(filepath+".gz", "rt")
    else:
        fp = open(filepath)
    for line in fp:
        a, b = line.strip().split("\t")[1:3]
        #a, b = a.strip('"').split("#")[0], b.strip('"').split("#")[0]
        a, b = a.strip('"').split("#")[0].split("$")[0], b.strip('"').split("#")[0].split("$")[0]
        call_map.setdefault(a, []).append(b)
        called_map.setdefault(b, []).append(a)
    if return_call_map:
        return call_map
    else:
        return called_map

def load_globalcfg(filepath, return_call_map=False):
    call_map = {} #a->b {a:[b]}
    called_map = {} #a->b {b:[a]}
    if os.path.isfile(filepath+".gz"):
        fp = gzip.open(filepath+".gz", "rt")
    else:
        fp = open(filepath)
    for line in fp:
        l = line.strip().split("@")
        if ":entry" in l[0]:
            continue
        if len(l)==3:
            curbb = (l[2], int(l[0].split(":")[1]))
            nextbbs = [(l[2], int(i)) for i in l[0].split(":")[0].split(",") if i]
        elif len(l)==5:
            curbb = (l[4].split("#")[0], int(l[0].split(":")[1]))
            nextbbs = [(l[2], int(l[0].split(":")[0]))]
        else:
            raise Exception("not recognized "+line)
        if not nextbbs:
            continue
        #print(curbb, nextbbs)
        call_map.setdefault(curbb, []).extend(nextbbs)
        for b in nextbbs:
            called_map.setdefault(b, []).append(curbb)
    if return_call_map:
        return call_map
    else:
        return called_map

def bfs_search(called_map, target, show=DEBUG, self=None, filter_func=None):
    queue = [(target,0)]
    result = {target:0}
    handled = set()
    while queue:
        item,dis = queue.pop(0)
        if item in handled:
            continue
        #if show:
        #    print(len(queue), item)
        if item in called_map:
            if show:
                if self:
                    #print(dis, item, self.pc2info(self.bbid_to_pc(*item))[1], "<-", [self.pc2info(self.bbid_to_pc(*i))[1] for i in called_map[item]])
                    print(self.pc2info(self.bbid_to_pc(*item))[1], "<-", set([self.pc2info(self.bbid_to_pc(*i))[1] for i in called_map[item]]))
                else:
                    #print(dis, item,  "<-", called_map[item])
                    print(item,  "<-", called_map[item])
            callers = called_map[item]
            if filter_func:
                callers = filter_func(item, callers, dis)
            for caller in callers:
                if caller in result:
                    assert result[caller]<=dis+1
                else:
                    result[caller]=dis+1
            queue.extend([(i,dis+1) for i in callers])
        handled.add(item)
    return result

def load_syscall(folder):
    lines = []
    syscalls = {}
    func2syscall = {}
    for l in open(folder+"/gvisor.dev#gvisor#pkg#sentry#syscalls#linux#linux64.go"):
        if 'syscalls.' in l and ":" in l:
            lines.append(l)
        if "ARM64" in l:
            break
    for l in open(folder+"/gvisor.dev#gvisor#pkg#sentry#syscalls#linux#vfs2#vfs2.go"):
        if "s.Table" in l:
            lines.append(l)
        if "ARM64" in l:
            break
    #pprint(lines)
    for line in lines:
        if "syscalls" not in line or "syserror" in line or "CapError" in line:
            continue
        if "s.Table" in line:
            l = line.replace(" = "," ").split()
            l[0] = l[0].split("[")[1].replace("]","")
            prefix = "pkg/sentry/syscalls/linux/vfs2."
        else:
            l = line.split()
            prefix = "pkg/sentry/syscalls/linux."
        id = int(l[0].replace(":",""))
        name = l[1].split('"')[1]
        assert "." not in name
        funcname = prefix+(l[2].split(")")[0].split(",")[0])
        status = l[1].split("(")[0].replace("syscalls.","")
        syscalls[id] = (name,funcname,status)
        func2syscall[funcname] = name
    func2syscall["(*pkg/tcpip/stack.NIC).DeliverNetworkPacket"] = "syz_emit_ethernet"
    return func2syscall

@lru_cache(10000)
def readlines(filepath):
    return [i.strip() for i in open(filepath).readlines()]

def harmonic_mean(distances):
    s = 0
    try:
        for dis in distances:
            s += 1/dis
    except:
        return 0
    return len(distances)/s

class RUNSC():
    def __init__(self, bin, codefolder_r, edgefile, cfgfile):
        self.bin = bin
        self.rfolder = codefolder_r
        self.info = None
        self.edgefile = edgefile
        self.called_map = None
        self.cfgfile = cfgfile
        self.cfg_called_map = None
        self.func2pcs_dict = None
        self.filepath2pcPREFIX = {}
        self.func2syscall = None
        self.getinfo()
    
    def getinfo(self):
        res = {}
        raw = subprocess.check_output([self.bin, "symbolize", "-all"]).decode().split("\n")
        for idx in range(0, len(raw), 2):
            if not raw[idx]:
                continue
            #print(raw[idx], raw[idx+1])
            filename, t = raw[idx+1].split(":")
            t = t.replace(",", ".").split(".")
            pc = raw[idx]
            res[pc] = filename, int(t[0]), int(t[1]), int(t[2]), int(t[3]) #filename, start_linenumber, start_col, end_linenumber, end_col
            self.filepath2pcPREFIX[filename] = int(pc, 16)>>16
        return res
    
    @lru_cache(10000)
    def instrumentedcode_read(self, filepath):
        curfunc = "UNKNOWN_FUNC"
        pc2func = {}
        pc2line = {}
        lineno = 0
        f = self.rfolder+"/"+filepath.replace("/","#")
        #print("reading", f)
        for line in readlines(f):
            lineno += 1
            if line.startswith("func "):
                line = line[5:]
                if line[0]=="(":
                    funcname = "(".join(line.split("(")[:2]).replace(") ",").")
                else:
                    funcname = line.split("(")[0]
                curfunc = filenamefuncname2dotfuncname(filepath.replace("#", "/").replace("gvisor.dev/gvisor/",""), funcname)
            if "Cover_" in line and "var " not in line:
                #print(line)
                try:
                    pc_suffix = int(line.split(".Count[")[1].split("]")[0])
                except:
                    print(line)
                    raise
                pc2func[pc_suffix] = curfunc
                pc2line[pc_suffix] = lineno
        return pc2func, pc2line
    
    def pc2info(self, pc):
        # filename，funcname, [original_filename, start_line, start_col, end_line, end_col, line_number_after_instrument]
        if not self.info:
            self.info = self.getinfo()
        if pc not in self.info:
            return None
        _info = list(self.info[pc])
        if "gvisor.dev/gvisor" not in _info[0]:
            return None
        suffix2func, suffix2lineno = self.instrumentedcode_read(_info[0])
        suffix = int(pc[-4:], 16)
        _info.append(suffix2lineno[suffix])
        return _info[0].replace("gvisor.dev/gvisor/", ""), suffix2func[suffix], _info
    
    def func2pcs(self, dotfuncname):
        if self.func2pcs_dict:
            return self.func2pcs_dict.get(dotfuncname, [])
        if not self.info:
            self.info = self.getinfo()
        func2pcs_dict = {}
        for pc, info in self.info.items():
            if "gvisor.dev/gvisor" not in info[0]:
                continue
            suffix2func, suffix2lineno = self.instrumentedcode_read(info[0])
            suffix = int(pc[-4:], 16)
            func2pcs_dict.setdefault(suffix2func[suffix], []).append(pc)
        self.func2pcs_dict = func2pcs_dict
        return func2pcs_dict.get(dotfuncname, [])
    
    def cgdistance(self, pc, filter_func=None, returnraw=False):
        # pc to {pc:dis}
        if not self.called_map:
            self.called_map = load_edgefile(self.edgefile)
        target = self.pc2info(pc)[1]
        if not filter_func:
            def filter_callers(item, callers, dis):
                callers = [i for i in callers if "gvisor.dev" in i and "gvisor/runsc" not in i]
                return callers
            filter_func = filter_callers
        cgd = bfs_search(self.called_map, target, filter_func=filter_func)
        if returnraw:
            return cgd
        res = {}
        for funcname, dis in cgd.items():
            for pc in self.func2pcs(funcname):
                res[pc] = dis
        return res
    
    def aflgodistance(self, input_pc):
        if not self.called_map:
            self.called_map = load_edgefile(self.edgefile)
        self.call_map = call_map = load_edgefile(self.edgefile, return_call_map=True)
        if not self.cfg_called_map:
            self.cfg_called_map = load_globalcfg(self.cfgfile)
        self.cfg_call_map = cfg_call_map = load_globalcfg(self.cfgfile, return_call_map=True)
        cfg_call_map_pc = {}
        for k, v in cfg_call_map.items():
            #print(k,v)
            if k[0] in self.filepath2pcPREFIX:
                cfg_call_map_pc[self.bbid_to_pc(*k)] = [self.bbid_to_pc(*i) for i in v if i[0] in self.filepath2pcPREFIX]
        cfg_called_map_pc = {}
        for k, v in self.cfg_called_map.items():
            #print(k,v)
            if k[0] in self.filepath2pcPREFIX:
                cfg_called_map_pc[self.bbid_to_pc(*k)] = [self.bbid_to_pc(*i) for i in v if i[0] in self.filepath2pcPREFIX]
        
        targetfuncname = self.pc2info(input_pc)[1]
        funcpcs = self.func2pcs(targetfuncname)
        cgd = bfs_search(self.called_map, targetfuncname)
        res = {}
        pc2dis_tmp = {}
        for pc, funccfg_dis in bfs_search(cfg_called_map_pc, input_pc, filter_func=lambda caller,pcs,dis:[i for i in pcs if i in funcpcs]).items():
            pc2dis_tmp.setdefault(pc, []).append(funccfg_dis)
        
        for pc, distances in pc2dis_tmp.items():
            res[pc] = int(100*harmonic_mean(distances))
        
        for funcname, dis in cgd.items():
            #print("\n----",funcname, dis)
            callsitedis = {}
            funcpcs = self.func2pcs(funcname)
            for pc in funcpcs: # find call sites
                if pc not in cfg_call_map_pc: #ignore leaf nodes:
                    continue
                childpcs = cfg_call_map_pc[pc]
                outpcs = [i for i in childpcs if i not in funcpcs]
                for outpc in outpcs:
                    pcinfo = self.pc2info(outpc)
                    if not pcinfo:
                        continue
                    outfunc = pcinfo[1]
                    if outfunc in cgd:
                        #print(pc, outfunc, cgd[outfunc])
                        if pc in callsitedis: # if a pc call multiple functions, choose the smallest one
                            callsitedis[pc] = min(cgd[outfunc]+1, callsitedis[pc])
                        else:
                            callsitedis[pc] = cgd[outfunc]+1
            #print(callsitedis)
            #funccfg = {k:[i for i in v if i in funcpcs] for k,v in cfg_called_map_pc.items() if k in funcpcs}
            pc2dis_tmp = {}
            for callsite_pc, callsite_dis in callsitedis.items():
                for pc, funccfg_dis in bfs_search(cfg_called_map_pc, callsite_pc, filter_func=lambda caller,pcs,dis:[i for i in pcs if i in funcpcs]).items():
                    pc2dis_tmp.setdefault(pc, []).append(funccfg_dis+10*callsite_dis)
            #print(pc2dis_tmp)
            for pc, distances in pc2dis_tmp.items():
                if pc not in res:
                    res[pc] = int(100*harmonic_mean(distances))
        return res
    
    def bbid_to_pc(self, filepath, idx):
        filepath = filepath.replace("#", "/")
        prefix = self.filepath2pcPREFIX[filepath]
        return hex((prefix<<16)+idx)
    
    def cfgdistance(self, pc):
        if not self.cfg_called_map:
            self.cfg_called_map = load_globalcfg(self.cfgfile)
        bbid = int(pc[-4:], 16)
        filepath = self.pc2info(pc)[2][0]
        cfgd = bfs_search(self.cfg_called_map, (filepath, bbid), self=self)
        cfg = {}
        for (filepath, bbid), dis in cfgd.items():
            pc = self.bbid_to_pc(filepath, bbid)
            cfg[pc] = dis
        return cfg
    
    def cfg_distancelimit(self, targetpc):
        cfg = self.cfgdistance(targetpc)
        entrypc = self.func2pcs("(*gvisor.dev/gvisor/pkg/sentry/kernel.Task).executeSyscall")[0]
        if entrypc in cfg:
            maxdis = cfg[entrypc]
            #print("before", len(cfg))
            cfg = {k:v for k,v in cfg.items() if v<=maxdis}
        return cfg
    
    def expand_cfgdistance(self, targetpc, round=100, distancelimit=True):
        # round: the function depth we consider to expand
        if distancelimit:
            cfg = self.cfg_distancelimit(targetpc)
        else:
            cfg = self.cfgdistance(targetpc)

        cm={}
        for k, v in load_globalcfg(self.cfgfile, True).items():
            if k[0]=="no filename info":
                continue
            vv = []
            for i in [i for i in v if i[0]!="no filename info"]:
                try:
                    vv.append(self.bbid_to_pc(*i) )
                except:
                    pass
            cm[self.bbid_to_pc(*k)] = vv
        for _ in range(round):
            newcfg = {}
            for pc, dis in sorted(cfg.items(), key=lambda i:i[1]):
                if pc not in cm:
                    continue
                called = [i for i in cm[pc] if i not in cfg]
                if not called:
                    continue
                for calledpc in called:
                    try:
                        funcname = self.pc2info(calledpc)[1]
                    except:
                        #print("error:", calledpc, cm[pc], pc)
                        continue
                    funcpcs = self.func2pcs(funcname)
                    if funcpcs[0]!=calledpc:
                        continue
                    for fpc in funcpcs:
                        if fpc in newcfg:
                            newcfg[fpc] = max(newcfg[fpc], dis+1) #should use the maxist distance
                        else:
                            newcfg[fpc] = dis+1
            for pc,dis in newcfg.items():
                if pc in cfg:
                    cfg[pc] = max(cfg[pc], dis)
                else:
                    cfg[pc] = dis

        return cfg

    
    def pc2syscall(self, pc, returnraw=False):
        if not self.func2syscall:
            self.func2syscall = load_syscall(self.rfolder)
        if not self.called_map:
            self.called_map = load_edgefile(self.edgefile)
        target = self.pc2info(pc)[1]
        res = []
        #print(target)
        def filter_callers(item, callers, dis):
            callers = [i for i in callers if "DecRef" not in i and "gvisor.dev" in i]
            return callers
        cgd = bfs_search(self.called_map, target, filter_func=filter_callers)
        seensyscalls = set()
        for _func, dis in sorted(cgd.items(), key=lambda i:i[1]):
            #print(_func, dis)
            #if _func.endswith("doVsyscallInvoke") and not os.environ.get("NOSTOP", None):
            #    break
            _func = _func.replace("gvisor.dev/gvisor/","")
            if _func in self.func2syscall:
                if DEBUG:
                    print(_func, dis)
                mindis = dis
                s = self.func2syscall[_func]
                if s in seensyscalls:
                    continue
                res.append((s, dis))
                seensyscalls.add(s)
        #print(self.func2syscall)
        if DEBUG:
            print("RAW:", res)
        rawres = res
        res = sorted(set([i[0] for i in res if i[1]==min([j[1] for j in res])]))
        if returnraw:
            return rawres, res
        else:
            return res
    
    def pc2syscalls_applyrules(self, pc):
        r = self
        syscall_dis, syscalls= r.pc2syscall(pc, returnraw=True)
        syscall_dis = {i:j for (i,j) in syscall_dis}
        syscalls_cg = set(syscalls)
        syscalls = syscalls_cg.copy()
        bbdis = r.cfgdistance(pc)
        filepath, funcname = r.pc2info(pc)[:2]
        
        # rule1: ioctl constants
        ioctl_syscalls = set() 
        ioctlpc2syscalls = r.get_pc2syscalls_ioctl()
        for i in set(bbdis) & set(ioctlpc2syscalls.keys()):
            for s in ioctlpc2syscalls[i]:
                sname = s.split("$")[0]
                if sname in syscall_dis and syscall_dis[sname]<=min(syscall_dis.values())+3:
                    if DEBUG:
                        print(ioctlpc2syscalls[i], bbdis[i], i, r.pc2info(i), s, syscall_dis[sname])
                    ioctl_syscalls.add(s)
        #ioctl_prefixes = [i.split("$")[0] for i in ioctl_syscalls]
        if ioctl_syscalls and len(ioctl_syscalls)<5:
            syscalls.update(ioctl_syscalls)
            prefixes = [i.split("$")[0] for i in syscalls if "$" in i]
            syscalls = set([i for i in syscalls if i not in prefixes])
        
        #rule2: infer fsimpl
        syscall_staticfsimpl = set()
        for fstype, scs in fsimpl2syscalls.items():
            if "/"+fstype+"/" in filepath:
                syscall_staticfsimpl.update(scs)
        syscalls.update(syscall_staticfsimpl)
        
        #rule3: copy usermem
        copypc2syscalls = r.get_pc2syscalls_copyusermem()
        syscall_copy = set()
        if pc in copypc2syscalls:
            syscall_copy = copypc2syscalls[pc]
        syscalls.update(syscall_copy)
        
        #rule4: pkg/tcpip
        if "pkg/tcpip/" in filepath:
            si = str(filepath+" "+funcname).replace("tcpip", "").lower()
            syscalls_net = []
            if "ipv4" in si:
                syscalls_net.append("syz_emit_ethernet$ipv4")
                for proto in ["tcp", "udp", "icmp", "igmp"]:
                    if proto in si:
                        syscalls_net.append("syz_emit_ethernet$ipv4_"+proto)
            if "ipv6" in si:
                syscalls_net.append("syz_emit_ethernet$ipv6")
                for proto in ["tcp", "udp", "icmp"]:
                    if proto in si:
                        syscalls_net.append("syz_emit_ethernet$ipv6_"+proto)
            if not syscalls_net:
                for proto in ["tcp", "udp", "icmp"]:
                    if proto in si:
                        syscalls_net.append("syz_emit_ethernet$ipv4_"+proto)
                        syscalls_net.append("syz_emit_ethernet$ipv6_"+proto)
                if "igmp" in si:
                    syscalls_net.append("syz_emit_ethernet$ipv4_igmp")
                if "arp" in si:
                    syscalls_net.append("syz_emit_ethernet$arp")
            #for s in ["getsockopt", "setsockopt", "connect"]:
            #    if s in si:
            #        syscalls_net.append(s)
            if syscalls_net:
                syscalls.update(syscalls_net)
            else:
                syscalls = list(syscalls)
                if "syz_emit_ethernet" not in syscalls:
                    syscalls.extend(["syz_emit_ethernet"]*max(len(syscalls),1))
        if "syz_emit_ethernet" in syscalls:
            syscalls = [i for i in syscalls if i!="syz_emit_ethernet"]+["syz_emit_ethernet#"] # do not expand to syz_emit_ethernet$...

        #rule5: epoll readiness
        if "Readiness" in funcname:
            syscalls=syscall_staticfsimpl
            syscalls.update(['select', 'epoll_ctl$EPOLL_CTL_ADD', 'pselect6', 'ppoll', 'poll'])
        
        #rule6: seccomp bpf
        if "pkg/bpf/" in filepath or "seccomp" in filepath:
            syscalls.update(["prctl$PR_SET_SECCOMP", "seccomp$SECCOMP_SET_MODE_FILTER", "seccomp$SECCOMP_SET_MODE_FILTER_LISTENER"])

        #rule7: file permission
        filepermissionpc2syscalls = r.get_pc2syscalls_filepermission()
        if pc in filepermissionpc2syscalls:
            syscalls.update(filepermissionpc2syscalls[pc])

        return syscalls_cg, syscalls
    
    def get_pc2syscalls_ioctl(self):
        constant2syscall = {}
        for line in open(BASEFOLDER+"/syscalls.txt"):
            if "$" not in line:
                continue
            l = line.strip().split(" ")
            constant = l[2].split("$")[1]
            syscall = l[2]
            #assert constant not in constant2syscall, constant+" "+syscall+" "+constant2syscall[constant]
            constant2syscall.setdefault(constant, []).append(syscall)

        pc2constants = {}
        pc2syscalls = {}
        for f in os.listdir(self.rfolder):
            code = readlines(self.rfolder+"/"+f)
            for idx,line in enumerate(code):
                if line.startswith("case linux."):
                    constants = []
                    while "Cover_" not in line:
                        constants.extend(line.replace("case ","").replace(":", "").split(","))
                        idx += 1
                        line = code[idx]
                    constants = [i.replace("linux.","").strip() for i in constants if i.strip()]
                    pc_suffix = int(line.split("[")[1].split("]")[0])
                    pc = self.bbid_to_pc(f.replace("#","/"), pc_suffix)
                    #print(constants, pc)
                    pc2constants[pc] = constants
                    for c in [i for i in constants if i in constant2syscall]:
                        #if pc in pc2syscalls:
                        #    print(pc, c, pc2syscalls[pc])
                        pc2syscalls.setdefault(pc, []).extend(constant2syscall[c])
        return pc2syscalls
    
    def extract_pc_by_rule(self, rule_f):
        pcs = set()
        for f in os.listdir(self.rfolder):
            code = readlines(self.rfolder+"/"+f)
            for idx,line in enumerate(code):
                line = line.strip()
                if rule_f(line):
                    #print(f, line)
                    while "Cover_" not in line:
                        idx += 1
                        line = code[idx]
                    #print(line)
                    pc_suffix = int(line.split("[")[1].split("]")[0])
                    pc = self.bbid_to_pc(f.replace("#","/"), pc_suffix)
                    #print(constants, pc)
                    pcs.add(pc)
        return pcs
    
    def get_pc2syscalls_copyusermem(self):
        MEMORYCALLS = ["munmap", "mmap", "mprotect"]

        pc2syscalls = {pc: MEMORYCALLS for pc in self.extract_pc_by_rule(lambda line:".Copy" in line and "err != nil" in line and ("ptr" in line.lower() or "addr" in line.lower() or "primitive.Copy" in line ))}
        return pc2syscalls
    
    def get_pc2syscalls_filepermission(self):
        return {pc: ["setuid", "setresuid"] for pc in self.extract_pc_by_rule(lambda line:"err != nil" in line and "permissions" in line.lower())}

@lru_cache()
def RUNSC_wrapper(version):
    if not os.path.isdir("/g/gvisor_bin/"+version+"/r"):
        assert os.system("cd /g/gvisor_bin/"+version+" && mkdir r && cd r && unzip -q ../r.zip")==0, "unzip error"
    r = RUNSC("/g/gvisor_bin/"+version+"/runsc", "/g/gvisor_bin/"+version+"/r", 
        "/g/gvisor_bin/"+version+"/edge.txt", "/g/gvisor_bin/"+version+"/global_cfg.txt")
    return r

@lru_cache()
def rtaRUNSC_wrapper(version):
    if not os.path.isdir("/g/gvisor_bin/"+version+"/r"):
        assert os.system("cd /g/gvisor_bin/"+version+" && mkdir r && cd r && unzip -q ../r.zip")==0, "unzip error"
    r = RUNSC("/g/gvisor_bin/"+version+"/runsc", "/g/gvisor_bin/"+version+"/r", 
        "/g/gvisor_bin/"+version+"/rtaedge.txt", "/g/gvisor_bin/"+version+"/rtaglobal_cfg.txt")
    return r

fsimpl2syscalls = {
    "devpts":["openat$ptmx", "syz_open_pts"],
    "eventfd":["eventfd2", "eventfd"],
    "kernfs": ["syz_open_procfs"],
    "pipefs": ["pipe", "pipe2"],
    "signalfd": ["signalfd4", "signalfd"],
    "timerfd": ["timerfd_create", "timerfd_settime", "timerfd_gettime"],
}

if __name__ == "__main__":
    version = sys.argv[1]
    r = RUNSC_wrapper(version)
    pc = sys.argv[2]
    #print(r.pc2info(pc))
    print(r.pc2syscall(pc))
    #print(r.get_pc2syscalls_ioctl())
