import os,sys,glob
def s(cmd):
    assert os.system(cmd) == 0
os.chdir("/tmp/r")
NAME=sys.argv[1]
os.makedirs("/tmp/r_"+NAME)
#pack all *.go files to /g/gvisor_bin/{NAME}/r.zip
#this script will empty /tmp/r
for i in os.listdir():
    if i in ["main#main.go", "main#version.go"]:
        continue
    code = open(i, "r").readlines()
    code = [i.replace('import "github.com/bazelbuild/rules_go/go/tools/coverdata"', '') for i in code][:-6]
    if code[0].startswith("//line"):
        code = code[1:]
    dst2="/tmp/r_"+NAME+"/"+i
    open(dst2, "w").write("".join(code))
os.chdir("/tmp/r_"+NAME)
s("zip -q -r '/g/gvisor_bin/"+NAME+"/r.zip' .")
s("rm -r /tmp/r && mkdir /tmp/r")
