import os,sys,glob
SRC=sys.argv[1]+"/"
DST=sys.argv[2]+"/"
for i in os.listdir(SRC):
    if i in ["main#main.go", "main#version.go", "gvisor.dev/gvisor/pkg/sentry/loader/vdsodata/vdso_amd64.go".replace("/","#")]:
        continue
    if "vdso_bin.go" in i:
        continue
    I = i.split("#")
    dst = DST+"/".join(I)
    if len(glob.glob(dst))!=1:
        #print(dst, glob.glob(dst))
        pass
        #continue
    else:
        dst = glob.glob(dst)[0]
    code = open(SRC+i, "r").readlines()
    os.makedirs("/".join(dst.split("/")[:-1]), exist_ok=True)
    try:
        open(dst, "w").write("".join(code))
    except:
        print(dst)
