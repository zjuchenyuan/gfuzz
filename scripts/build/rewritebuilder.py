import glob
for f in glob.glob("bazel-*/external/io_bazel_rules_go/go/tools/builders/cover.go"):
    code = open(f).read()
    if "/tmp/r/" not in code:
        print("rewriting", f)
        code = code.split("\n")
        ok=False
        for i in range(len(code)):
            if code[i].strip()=="import (":
                code[i] = 'import (\n\t"strings"'
                ok=True
                break
        assert ok, "import modify failed"
        ok=False
        for i in range(len(code)):
            if "ioutil.WriteFile" in code[i]:
                code[i] = """if err := ioutil.WriteFile("/tmp/r/"+strings.ReplaceAll(srcName, "/", "#"), buf.Bytes(), 0666); err != nil{
\treturn fmt.Errorf("write /tmp/r failed: %v", err)
}
"""+code[i]
                ok=True
                break
        assert ok, "ioutil.WriteFile modify failed"
        open(f,"w").write("\n".join(code))
        print("rewrite", f)
