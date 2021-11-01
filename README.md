# gfuzz
Directed fuzzing framework for gVisor.

## Pipeline

Note: We assume all gfuzz files placed under folder `/g` on the host machine.

### 1. Prepare environment

We use [GoLLVM](https://go.googlesource.com/gollvm/) for compiling gVisor code to a LLVM bc file.

Commands for preparing a gollvm environment are given as a [Dockerfile](./dockerfiles/Dockerfile.gollvm), and you may build it as follows:

```
git clone https://github.com/zjuchenyuan/gfuzz /g
cd /g
docker build -t zjuchenyuan/gollvm -f dockerfiles/Dockerfile.gollvm
```

This will build and install gollvm version `d30fc0bf` in the image `zjuchenyuan/gollvm`.

```
cd /g
./scripts/build/build_syzkaller.sh
```

This will build our modified syzkaller in `/g/syzkaller`, which contains many gfuzz modifications.

### 2. Build target gVisor to LLVM IR, and get global CFG

Here we take gVisor version `release-20210125.0` as an example.

Also we provide a Dockerfile for building and analyzing, see [here](./dockerfiles/Dockerfile.buildstatic).

```
docker build -t gfuzz:release-20210125.0 -f dockerfiles/Dockerfile.buildstatic
```

After this build, you will get a folder inside the image `/g/gvisor_bin/release-20210125.0`, which includes these files:

- edge.txt: the edge relationship given by `go-callvis`, using default [pointer analysis](https://pkg.go.dev/golang.org/x/tools/go/pointer)
- rtaedge.txt: the edge relationship given by our modified `go-callvis`, using [Rapid Type Analysis](https://pkg.go.dev/golang.org/x/tools/go/callgraph/rta), see also our [patch file](./scripts/build/gocallvis_rta.patch)
- global_cfg.txt: basic block level control flow graph analysis result given by our `ir2cfg` component, which is used in distance calculation afterwards
- rtaglobal_cfg.txt: Same as global_cfg.txt, but rely on rtaedge.txt instead
- r.zip: packed instrumented Go files, can be used for source code level analysis
- runsc: the build binary, used for fuzzing experiment

You may copy these build files to the host machine, as we will conduct the fuzzing experiment outside the Docker container.

```
docker run -it --rm --name tmp gfuzz:release-20210125.0 /bin/bash
# in another host terminal
docker cp tmp:/g/gvisor_bin /g/
```

### 3. Distance calculation, syscall inference and so on...

We take general experiment setting as an example, to show you how to generate conf files for fuzzing.

24 targets in version `release-20210125.0` are involved in the general experiment, and the corresponding pc list is given in the [data.py](./fuzz_experiments/general/data.py).

```
cd /g/fuzz_experiments/general
python3 data.py
```

This will conduct several analysis towards the target, and generate these files: (links are given to an example file)

- [\*\_bbdis.json](./fuzz_experiments/general/1_bbdis.json): bb level distance of each reachable pc, this is used in gfuzz setting
- [\*\_cgdis.json](./fuzz_experiments/general/1_cgdis.json): function level distance of each reachable pc, not used in our paper
- [\*\_aflgodis.json](./fuzz_experiments/general/1_aflgodis.json): use aflgo approximation distance calculation method to estimate the distance
- [\*\_syscalls.json](./fuzz_experiments/general/1_syscalls.json): inferred syscalls for the target by gfuzz static analysis

> If you want to check the pc meaning, try this:
>     python3 /g/scripts/static/showpc.py release-20210125.0 0x4dd0000

### 4. Generate fuzzing configuration

Next step is generate `*.conf` files for fuzzing, and we need to write the template and `genconfig.py`.

You can related files for general fuzzing here: [./fuzz_experiments/general](./fuzz_experiments/general). 

This [genconfig.py](./fuzz_experiments/general/genconfig.py) will generate 6 batch experiment scripts, and in each batch, 4 targets are tested with 4 repetitions and 2 different settings (gfuzz and aflgo), with a baseline setting (origin) 4 repetitions. To conduct 20 repetitions, you may run each `start.sh` on 5 different machines, and each machine should have at least 36 CPUs available.

```
cd /g/fuzz_experiments/general
python3 genconfig.py
```

After running this python script, these files will be generated: (`M`, `N`, `BATCH`, `VERSION` are variables, links are given to an example file)

- [general\_aflgo\_M\_N.conf](./fuzz_experiments/general/general_aflgo_1_1.conf): aflgo fuzzing conf for target M and repeat N (1<=M<=24, 1=<N<=4 in this experiment)
- [general\_gfuzz\_M\_N.conf](./fuzz_experiments/general/general_gfuzz_1_1.conf): gfuzz fuzzing conf for target M and repeat N
- [general\_originBATCH\_VERSION\_N.conf](./fuzz_experiments/general/general_origin1_release-20210125.0_1.conf): baseline conf for gvisor VERSION in batch BATCH (1<=BATCH<=6, VERSION=release-20210125.0, 1<=N<=4 in this experiment)
- [start.sh](./fuzz_experiments/general/start.sh): the script to run the whole experiment batches on one machine
- [startBATCH.sh](./fuzz_experiments/general/start1.sh): the script to start the batch BATCH experiment

Now, just running `cd /g/fuzz_experiments/general; sh start.sh` to conduct the experiment!

## Docs

### 1. Why origin_syzkaller is also patched?

For easier analysis of target pc trigger time, we modified syzkaller to write a file when a new target pc is triggered.

Besides, to make a fair comparison and avoid resource exhaustion, we need to limit the available CPU and memory for each container.

See this patch file: [origin_syzkaller.patch](./scripts/build/origin_syzkaller.patch)

### 2. What is syscalls.txt?

[syscalls.txt](./scripts/static/syscalls.txt) shows which syscalls are enabled during the fuzzing. This file is used in syscall extension inference, and can be generated by removing [this comment of gfuzz](https://github.com/zjuchenyuan/gfuzz/blob/cc7d311176f4817aabca3335228ba8cbee365a44/scripts/build/gfuzz-changes.patch#L3297).

### 3. GFUZZ experiment configuration explanation

gfuzz added these settings to its conf file, take general_gfuzz_1_1.conf as an example:

```
    "cover": true, // we used gVisor kcov support, if you need to test old versions, you need to back-port certain commits
    "procs": 1, // gvisor kcov support does not support process coverage isolation
    "type": "gvisor",
    "vm": {
        "count": 1,
        "runsc_args": "-platform=kvm -vfs2" // use kvm for better performance, and enable vfs2
    },
    "distance": "/g/fuzz_experiments/general/1_bbdis.json", // distance information for each basic block
    "random_choose": false, // can be used for baseline comparison, pick random seeds
    "globaldistance": false, // discarded setting
    "reproduce": false, // disable syzkaller crash reproducing
    "limitexec": false, // discarded setting
    "cooling_tx": 0, // used in aflgo setting, the tx param used in aflgo
    "moresmash": 200, // when a seed with less than 10 distance found, do smash 200 times instead of default 100 times
    "useclosetdistance": false, // use closet distance of a seed instead of average distance
    "mimicbaseline": true, // in coverage mode, mimic what syzkaller does
    "directchooseentertime": 300, // if coverage mode get stuck for 5 minutes, enter directed mode
    "directchooseforceentertime": 864000, // force enter directed mode after 10 days, this simply disables force switch
    "directchooseexittime": 300, // if directed mode get stuck for 5 minutes, switch back to coverage mode
    "directchooseforceexittime": 864000, // disable force switch
    "syscallseeds": ["syz_emit_ethernet#"], // inferred syscall result, here the last # character disables selecting syscall extensions
    "disable_rotate": true, // disable syzkaller rotate mechanism, to avoid our inferred syscalls not used if rotate
    "mutatedirectchance": 80, // 80% chance to inject inferred syscall in mutate operation, this value will linearly decrease to 20% in 10 hours
    "orderinfer": true, // enable syscall order inference
    "targetpc": ["0x4dd0000"] //target pc, when triggered, manager will write a file containing found time
```