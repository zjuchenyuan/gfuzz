import os, sys, json, random
def randint():
    global ports
    x = random.randint(10000,60000)
    while x in ports:
        x = random.randint(10000,60000)
    ports.append(x)
    return x
from data import dataset
FOLDER=os.getcwd().split("/")[-1]
outfolder = FOLDER
BASELINE_SETTINGS = ["origin"]
CONFFOLDER="/g/fuzz_experiments/general/"
BATCH_SETTINGS = {}
for i in range(6):
    BATCH_SETTINGS[i+1] = range(1+i*4, 1+i*4+4)

def generate_conf(template_file, data):
    t=open(template_file).read()
    for k, v in data.items():
        if "{"+k+"}" not in t:
            print("[warning] key not found in template:", k)
        t = t.replace("{"+k+"}", str(v))
    #print(t)
    jsondata = json.loads(t)
    if "distance" in jsondata:
        assert os.path.isfile(jsondata["distance"]), "cannot find "+jsondata["distance"]
    return t

open("start.sh", "w").write("#!/bin/bash\n") #clean the content
for BATCH, TARGETS in BATCH_SETTINGS.items():
    BATCH = str(BATCH)
    open("start"+BATCH+".sh", "w").write("#!/bin/bash\n") #clean the content
    REPEATS=[1,2,3,4]
    ports=[]
    
    VERSIONS = set([i[0] for idx, i in dataset.items() if idx in TARGETS])
    setting = "aflgo"
    version2pcs = {}
    for target, (version,pc) in dataset.items():
        if target not in TARGETS:
            continue
        version2pcs.setdefault(version, []).append(pc)
        
    syzkaller_path = "/g/syzkaller"
    for target in TARGETS:
        for repeat in REPEATS:
            version, pc = dataset[target]
            for setting in ["aflgo", "gfuzz"]:
                NAME = FOLDER+"_"+setting+"_"+str(target)+"_"+str(repeat)
                if setting == "gfuzz":
                    syscallseeds = open(str(target)+"_syscalls.json").read().strip()
                else:
                    syscallseeds = "[]"
                newjson = generate_conf("template_"+setting+".txt", {
                    "name": NAME, "target": target, 
                    "outfolder": outfolder,
                    "gvisor_folder": "/g/gvisor_bin/"+version,
                    "randint": randint(),
                    "syzkaller_path": syzkaller_path,
                    "CONFFOLDER": CONFFOLDER,
                    "targetpc": json.dumps([pc]),
                    "syscallseeds": syscallseeds,
                })
                open(NAME+".conf", "w").write(newjson)
                open("start"+BATCH+".sh", "a").write("""NAME="%(NAME)s"
mkdir -p /g/output/%(outfolder)s/${NAME}
cd /g/output/%(outfolder)s/${NAME}/
timeout -k 60 86400 %(syzkaller_path)s/bin/syz-manager -bench bench.log -config /g/fuzz_experiments/%(FOLDER)s/%(NAME)s.conf &
sleep 5
"""%(locals()))

    for setting in BASELINE_SETTINGS:
        if setting != "origin":
            syzkaller_path = "/g/syzkaller"
        else:
            syzkaller_path = "/g/origin_syzkaller"
        target = None
        t=open("template_"+setting+".txt").read()
        for version in sorted(VERSIONS):
            allpcs = version2pcs[version]
            for repeat in REPEATS:
                NAME = FOLDER+"_"+setting+str(BATCH)+"_"+version+"_"+str(repeat)
                newjson = generate_conf("template_"+setting+".txt", {
                    "name": NAME, 
                    "outfolder": outfolder,
                    "gvisor_folder": "/g/gvisor_bin/"+version,
                    "randint": randint(),
                    "syzkaller_path": syzkaller_path,
                    "targetpc": json.dumps(allpcs)
                })
                open(NAME+".conf", "w").write(newjson)
                open("start"+BATCH+".sh", "a").write("""NAME="%(NAME)s"
mkdir -p /g/output/%(outfolder)s/${NAME}
cd /g/output/%(outfolder)s/${NAME}/
timeout -k 60 86400 %(syzkaller_path)s/bin/syz-manager -bench bench.log -config /g/fuzz_experiments/%(FOLDER)s/%(NAME)s.conf &
sleep 5
"""%(locals()))

    os.system("chmod +x start"+BATCH+".sh")
    os.system("grep 'cd ' start"+BATCH+".sh|wc -l")
    open("start.sh", "a").write("sh start"+BATCH+".sh; sleep 87000\n")
