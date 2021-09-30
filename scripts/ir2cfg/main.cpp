#include "llvm/Support/CommandLine.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/lib/IR/ConstantsContext.h"

// #include "llvm/Pass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/Format.h"

#include "debug.h"
#include <unistd.h>

#include <map>
#include <set>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>

// using namespace std;
using namespace llvm;

// static cl::opt<std::string> TargetFile("target", cl::Required, cl::desc("<Target file>"));
// static cl::opt<int> LineNumber("line", cl::Required, cl::desc("<Line number>"));
static cl::opt<bool> Verbose("v", cl::desc("more info"));
static cl::opt<bool> CGONLY("cg-only", cl::desc("CG level only"));
static cl::opt<bool> GCG("global-cfg", cl::desc("Get global control flow graph"));
static cl::opt<bool> GETFN("get-fn", cl::desc("Get functions names"));
static cl::opt<std::string> InputFilename(cl::Positional, cl::Required, cl::desc("<Filename>"));
static cl::opt<std::string> TargetsFile("targets", cl::desc("Input file containing the target lines of code."), cl::value_desc("targets"));
static cl::opt<std::string> OutDirectory("outdir", 
        cl::desc("Output directory where Ftargets.txt, Fnames.txt, BBnames.txt and BBcalls.txt are generated. "), 
        cl::value_desc("outdir"));

template <class T>
class VectorSet {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    iterator begin() { return vs_vector.begin(); }
    iterator end() { return vs_vector.end(); }
    const_iterator begin() const { return vs_vector.begin(); }
    const_iterator end() const { return vs_vector.end(); }
    const T &front() const { return vs_vector.front(); }
    const T &back() const { return vs_vector.back(); }
    bool insert(const T &item) { 
        if (vs_set.insert(item).second) {
            vs_vector.push_back(item);
            return true; // marking repetition 
        } else {
            return false;
        }
    }
    template <class InputIterator>
    void insert (InputIterator first, InputIterator last) {
        while (first != last) {
            insert(*first);
            advance(first, 1);
        }
    }
    size_t count(const T &item) const { return vs_set.count(item); }
    bool empty() const { return vs_set.empty(); }
    size_t size() const { return vs_set.size(); }
private:
    std::vector<T> vs_vector;
    std::set<T> vs_set;
};

using BBListTy = VectorSet<BasicBlock*>;
using CallerIndexTy = std::pair<Function*, int>;
using CallerListTy = VectorSet<CallerIndexTy>;
using CallPathTy = std::vector<CallerListTy*>;
using FileName = std::string;
using LineIndexTy = std::pair<int, FileName>;
using BB2IDTy = std::map<BasicBlock*, int>;
using Line2BBsTy = std::map<LineIndexTy, BBListTy*>;
using Func2BBsTy = std::map<Function*, BBListTy*>;
using Func2CGNTy = std::map<Function*, CallGraphNode*>;
using InstCalleListTy = VectorSet<std::pair<int, Function*>>;
using Func2CalleTy = std::map<Function*, InstCalleListTy*>; // <callee, <call_inst, caller>>

int GetInstrucmentIndices(Instruction *I, bool verbose=false);
void Init(Module &M, bool verbose=false);
void InitDS(Module &M, bool verbose=false);
void InitCallGraph(bool verbose=false);
void DumpGCFG(Module *_m);
BasicBlock *GetBB(LineIndexTy line_ind, int delta=0, bool verbose=false);
InstCalleListTy *GetCalle(Function *_target_func, Func2CalleTy *_f2c);
std::pair<int, int> GetBBLineNo(BasicBlock *target_BB);
std::string DemangleGoFunc(Function *F, bool verbose=false);
std::string PadFilename(Function *F);
void DumpCallRelation(Function *_f, InstCalleListTy *_fs, std::ofstream *_of, bool is_caller2callee);
void DumpCGLevelLengthOnly(Function *F, bool verbose=false);
void InitCG(Function *_F, BasicBlock *_BB);
void DumpCGLevelLength(Function *F, BasicBlock *BB, bool verbose=false);
int DumpCFGLevelLength(BasicBlock *BB, int bb_len_delta, bool verbose=false);
int GetGoBBIndex(BasicBlock *BB);
void DumpBBRelation(BasicBlock *BB, llvm::pred_range *BBs);

void testCallReation(Function *F);


Line2BBsTy Line2BBs; // line number to bb list
BB2IDTy BB2ID; // bb to index
std::map<std::string, BasicBlock*> name2BB; // name to bb
Func2CalleTy Func2Caller; // function to caller instruction and caller CGN, enable bottom-up CG tranversal
std::map<std::string, Function*> demangled_name2Func;
std::map<std::string, std::string> llvm_name2demangled_name;
Func2CalleTy Func2Callee;
std::set<Function*> TargetRelatedFunc;
std::set<Function*> CGRelatedFunc;
std::set<BasicBlock*> handled_bb;

std::string gocallvis_filename = "gocallvis_fun.txt";
std::string llvm_func_name_filename = "all_name.txt";
std::string target_bb_candidates_info_filename = "target_bb_candidates_info.txt";
std::string inst_type_info_filename = "instruction_type_info.txt";
std::string padding_func_info_filename = "padding_func_name_list_info.txt";
std::string not_found_func_info_filename = "not_found_func_info.txt";
std::string dump_call_path_info_filename = "dump_call_path_info.txt";
std::string cg_level_length_filename = "cg_level_distance.txt";
std::string dump_bb_relation_info_filename = "dump_bb_relation_info.txt";
// std::string cfg_level_length_filename = "cfg_level_length.txt";
std::string go_bb_relation_filename = "go_bb_level_distance.txt";
std::string demangled_llvm_fname_filename = "demangled_llvm_fname.txt";

void WorkHard(std::string _filename, int _line_no)
{
    // std::cout << filename << " " << line_no << "\n";
    std::string filename = _filename;
    int line_no = _line_no;
    LineIndexTy target_line_ind = std::make_pair(line_no, filename);
    BasicBlock *ptarget_bb = GetBB(target_line_ind, 0, Verbose);
    if (!ptarget_bb) {
        llvm::errs().changeColor(raw_ostream::RED, /*bold=*/true, /*bg=*/false);
        llvm::errs() << "ERROR!\n";
        llvm::errs().resetColor();
        llvm::errs() << "Line Number:" << line_no << " File:" << filename << " has no corresponding Basic Block.\n" ;
        return ;
    }
    Function *ptarget_func = ptarget_bb->getParent();
    if (Verbose) {
        llvm::outs() << "Target function: " << DemangleGoFunc(ptarget_func) << "\n";
    }
    if (CGONLY) DumpCGLevelLengthOnly(ptarget_func, Verbose);
    else {
        // InitCG(ptarget_func, ptarget_bb); 
        // testCallReation(ptarget_func);
        // return;
        DumpCGLevelLength(ptarget_func, ptarget_bb, Verbose);
        handled_bb.clear();
    }
    // DumpBBPath(target_line_ind, ptarget_func, GetCallPath(ptarget_func), true);
}

static bool isBlacklisted(const Function *F) {
    static const SmallVector<std::string, 8> Blacklist = {
        "asan.",
        "llvm.",
        "sancov.",
        "__ubsan_handle_",
        "free",
        "malloc",
        "calloc",
        "realloc"
    };

    for (auto const &BlacklistFunc : Blacklist) {
        if (F->getName().startswith(BlacklistFunc)) {
            return true;
        }
    }
    if (F->isIntrinsic()) return true;
    
    auto fname =  F->getName().str();
    auto res = llvm_name2demangled_name.find(fname);
    if (res == llvm_name2demangled_name.end()) {
        return true;
    }

    return false;
}

static void getDebugLoc(const Instruction *I, std::string &Filename,
                        unsigned &Line) {
#ifdef LLVM_OLD_DEBUG_API
//   DebugLoc Loc = I->getDebugLoc();
//   if (!Loc.isUnknown()) {
//     DILocation cDILoc(Loc.getAsMDNode(M.getContext()));
//     DILocation oDILoc = cDILoc.getOrigLocation();

//     Line = oDILoc.getLineNumber();
//     Filename = oDILoc.getFilename().str();

//     if (filename.empty()) {
//       Line = cDILoc.getLineNumber();
//       Filename = cDILoc.getFilename().str();
//     }
//   }
#else
  if (DILocation *Loc = I->getDebugLoc()) {
    Line = Loc->getLine();
    Filename = Loc->getFilename().str();

    if (Filename.empty()) {
      DILocation *oDILoc = Loc->getInlinedAt();
      if (oDILoc) {
        Line = oDILoc->getLine();
        Filename = oDILoc->getFilename().str();
      }
    }
  }
#endif /* LLVM_OLD_DEBUG_API */
}

namespace llvm {

template<>
struct DOTGraphTraits<Function*> : public DefaultDOTGraphTraits {

    DOTGraphTraits(bool isSimple=true) : DefaultDOTGraphTraits(isSimple) {}

    static std::string getGraphName(Function *F) {
        return "CFG for '" + F->getName().str() + "' function";
    }

    std::string getNodeLabel(BasicBlock *Node, Function *Graph) {
        if (!Node->getName().empty()) {
            return Node->getName().str();
        }

        std::string Str;
        raw_string_ostream OS(Str);

        Node->printAsOperand(OS, false);
        return OS.str();
    }
};

} // namespace llvm

int main(int argc, char **argv)
{
    cl::ParseCommandLineOptions(argc, argv);
    if (Verbose) {
        // llvm::outs() << "LineNumber: " << LineNumber << "\n";
        llvm::outs() << "InputFilename: " << InputFilename << "\n";
        llvm::outs() << "GETFN" << (GETFN ? "true" : "false") << "\n";;
        // std::cout << "TargetFile: " << TargetFile << "\n";
        std::cout << "GCG: " << (GCG ? "true" : "false") << "\n";
        std::cout << "CGONLY: " << (CGONLY ? "true" : "false") << "\n";
        std::cout << "Verbose: " << (Verbose ? "true" : "false") << "\n";   
    }

    std::cout << "loading LLVM IR\n";
    fflush(stdout);
    // Parse the input LLVM IR file into a module.
    SMDiagnostic Err;
    LLVMContext Context;
    std::unique_ptr<Module> M(parseIRFile(InputFilename, Err, Context));
    if (!M) {
        Err.print(argv[0], llvm::errs());
        return 1;
    }
    std::cout << "loading done\n";

    if (GETFN) {
        std::ofstream fn_of;
        fn_of.open(llvm_func_name_filename, std::ios::out);
        for (auto &f: *M) {
            auto fn = f.getName().str();
            fn_of << fn.c_str() << "\n";
        }
        fn_of.close();
        return 0;
    }

    // Init the global data structure.
    Init(*M, Verbose);
    
    std::list<std::string> targets;
    bool is_aflgo_preprocessing = false;
    if (!TargetsFile.empty()) {
        if (OutDirectory.empty()) {
            FATAL("Provide output directory '-outdir <directory>'");
            return 1;
        }
        std::ifstream targets_file(TargetsFile);
        std::string line;
        while (std::getline(targets_file, line))
            targets.push_back(line);
        targets_file.close();

        is_aflgo_preprocessing = true;
    }
    if (isatty(2) && !getenv("AFL_QUIET")) {
        if (is_aflgo_preprocessing)
            SAYF(cCYA "aflgo-llvm-pass (yeah!) " cBRI VERSION cRST " (%s mode)\n",
                (is_aflgo_preprocessing ? "preprocessing" : "distance instrumentation"));
    }
    if (is_aflgo_preprocessing) {
        std::ofstream bbnames(OutDirectory + "/BBnames.txt", std::ofstream::out);// | std::ofstream::app);
        std::ofstream bbcalls(OutDirectory + "/BBcalls.txt", std::ofstream::out);// | std::ofstream::app);
        std::ofstream fnames(OutDirectory + "/Fnames.txt", std::ofstream::out);// | std::ofstream::app);
        std::ofstream ftargets(OutDirectory + "/Ftargets.txt", std::ofstream::out);// | std::ofstream::app);

        /* Create dot-files directory */
        std::string dotfiles(OutDirectory + "/dot-files");
        if (sys::fs::create_directory(dotfiles)) {
            FATAL("Could not create directory %s.", dotfiles.c_str());
        }

        for (auto &F : *M) {
            bool has_BBs = false;
            /* Black list of function names */
            if (isBlacklisted(&F)) {
                continue;
            }
            std::string funcName = F.getName().str();

            bool is_target = false;
            for (auto &BB : F) {

                std::string bb_name("");
                std::string filename;
                unsigned line;

                for (auto &I : BB) {
                    getDebugLoc(&I, filename, line);
                    filename = PadFilename(&F);

                    /* Don't worry about external libs */
                    static const std::string Xlibs("/usr/");
                    if (filename.empty() || line == 0 || !filename.compare(0, Xlibs.size(), Xlibs))
                        continue;

                    if (bb_name.empty()) {
                        // std::size_t found = filename.find_last_of("/\\");
                        // if (found != std::string::npos)
                        //     filename = filename.substr(found + 1);
                        bb_name = filename + ":" + std::to_string(line);
                    }

                    if (!is_target) {
                        for (auto &target : targets) {
                            // std::size_t found = target.find_last_of("/\\");
                            // if (found != std::string::npos)
                            //     target = target.substr(found + 1);

                            std::size_t pos = target.find_last_of(":");
                            std::string target_file = target.substr(0, pos);
                            unsigned int target_line = atoi(target.substr(pos + 1).c_str());

                            if (!target_file.compare(filename) && target_line == line)
                                is_target = true;

                        }
                    }

                    if (auto *c = dyn_cast<CallInst>(&I)) {
                        // std::size_t found = filename.find_last_of("/\\");
                        // if (found != std::string::npos)
                        //     filename = filename.substr(found + 1);

                        if (auto *CalledF = c->getCalledFunction()) {
                            if (!isBlacklisted(CalledF))
                                bbcalls << bb_name << "," << CalledF->getName().str() << "\n";
                        }
                    }
                }

                if (!bb_name.empty()) {
                    BB.setName(bb_name + ":");
                    if (!BB.hasName()) {
                        std::string newname = bb_name + ":";
                        Twine t(newname);
                        SmallString<256> NameData;
                        StringRef NameRef = t.toStringRef(NameData);
                        BB.setValueName(ValueName::Create(NameRef));
                    }

                    bbnames << BB.getName().str() << "\n";
                    has_BBs = true;
                }
            }

            if (has_BBs) {
                /* Print CFG */
                std::string cfgFileName = dotfiles + "/cfg.@" + funcName + "@.dot";
                std::error_code EC;
                raw_fd_ostream cfgFile(cfgFileName, EC, sys::fs::F_None);
                if (!EC) {
                    WriteGraph(cfgFile, &F, true);
                }

                if (is_target)
                    ftargets << F.getName().str() << "\n";
                fnames << F.getName().str() << "\n";
            }
        }
        return 0;
    }    
    
    if (GCG) {
        DumpGCFG(M.get());
        return 0;
    }

    // work hard
    std::cout << "start: filename line_number\n";
    fflush(stdout);
    while (1) {
        int line_no;
        char tmp[501];
        scanf("%s%d", tmp, &line_no);
        fflush(stdin);
        // std::cout << tmp << " " << line_no << "\n";
        WorkHard(tmp, line_no);
        std::cout << "end\n";
        fflush(stdout);
    }
    
    return 0;
}

std::string DemangleGoFunc(Function *F, bool verbose)
{
    auto fname =  F->getName().str();
    // auto fname = std::string("gvisor.x2edev..z2fgvisor..z2fpkg..z2ftcpip..z2fstack.gvisor.x2edev..z2fgvisor..z2fpkg..z2ftcpip..z2fstack.NeighborEntry..eq");
    if (verbose) {
        llvm::outs() << "Before demangling, function name: " << fname << "\n";
    }
    auto res = llvm_name2demangled_name.find(fname);
    if (res != llvm_name2demangled_name.end()) {
        fname = res->second;
    }
    if (verbose) {
        llvm::outs() << "After demangling, function name: " << fname << "\n";
    }
    return fname;
}

std::string PadFilename(Function *F)
{
    std::string f_name = DemangleGoFunc(F);
    if (F->getSubprogram()) {
        auto filename = llvm::sys::path::filename(F->getSubprogram()->getFilename());
        std::stringstream ss(f_name);
        std::string ss_part;
        std::vector<std::string> cur_parts;
        std::string nf_name;
        int count = 0;
        while (getline(ss, ss_part, '.')) {
            if (count++ < 2) {
                nf_name.append(ss_part);
                if (count < 2)  nf_name.append(".");
            }
            else break;
        }
        nf_name.append("/").append(filename);
        return nf_name;        
    } else {
        // llvm::errs() << llvm::format("[%s:%d] %s no debug info\n", __FILE__, __LINE__, f_name.c_str());
        // llvm::outs() << "Warning: [" __FILE__ << ":" << __LINE__ << "] " << f_name.c_str() << " no debug info\n";
        return std::string("no filename info");
    }
}

int GetInstrucmentIndices(Instruction *I, bool verbose) 
{
    std::ofstream inst_of;
    if (verbose) {
        inst_of.open(inst_type_info_filename, std::ios::out | std::ios::app);
    }
    if (I) {
        if (auto s = dyn_cast<StoreInst>(I)) {
            auto store_target = s->getOperand(1);
            if (auto gepce = dyn_cast<llvm::GetElementPtrConstantExpr>(store_target)) {
                int n = gepce->getNumOperands();
                if (verbose) { 
                    inst_of << "Current GetElementPtr:\n";
                    for (int i = 0; i < n; ++i) {
                        auto v = gepce->getOperand(i);
                        std::string type_str;
                        llvm::raw_string_ostream rso(type_str);
                        v->getType()->print(rso);
                        inst_of << i << "th " << rso.str().c_str() << " ";
                        inst_of << v->getName().str().c_str() <<  " ";
                        if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
                            uint64_t idx = ci->getZExtValue();
                            inst_of << idx;
                        }
                        inst_of << "\n";
                    }
                }
                auto gep_target = gepce->getOperand(0);
                std::string gep_target_name = gep_target->getName().str();
                std::string mark = "Cover_";
                if (n == 4 && gep_target_name.find(mark) != std::string::npos) {
                    if (verbose) inst_of << "yeahyeahyeah!\n";
                    int op_ind = 3;
                    auto v = gepce->getOperand(op_ind);
                    if (!v) return -1;
                    if (ConstantInt *ci = dyn_cast<ConstantInt>(v)) {
                        uint64_t idx = ci->getZExtValue(); // unsigned64 to int
                        if (verbose) inst_of.close();
                        return idx;
                    }
                }
            }
        }
    }
    if (verbose) inst_of.close();
    return -1;
}

void Init(Module &M, bool verbose)
{   
    std::cout << "begin Init\n";  
    InitDS(M, verbose);
    InitCallGraph(verbose);
    std::cout << "end Init\n";
}

void InitCallGraph(bool verbose)
{
     // Supplement from go-callvis
    std::string line;
    std::ifstream gocallvis_file(gocallvis_filename);
    std::vector<std::vector<std::string>> gocallvis_res;
    // line00 - line01 - ...
    // line00: caller@callee@call location*
    if (gocallvis_file.is_open()) {
        while (getline(gocallvis_file, line))
        {
            std::stringstream ss_line(line);
            std::string line_part;
            std::vector<std::string> cur_part;
            while (getline(ss_line, line_part, '@')) {
                cur_part.push_back(line_part);
            }
            gocallvis_res.push_back(cur_part);
        }
        gocallvis_file.close();
    } 
    else {
        llvm::errs() << "file:" << gocallvis_filename << " no found!\n";
        exit(-1);
    }
    // llvm::outs() << "before, size of Func2Caller: " << Func2Caller.size() << "\n";
    std::ofstream no_fun_of;
    if (verbose)
        no_fun_of.open(not_found_func_info_filename, std::ios::out);
    for (auto calls : gocallvis_res) {
        std::string cur_caller, cur_callee;
        std::vector<LineIndexTy> call_loc_index;
        cur_caller = calls[0];
        cur_callee = calls[1];
        
        auto i = calls.begin();
        advance(i, 2);
        for (auto e = calls.end(); i != e; ++i) {    
            std::stringstream ss_loc(*i);
            std::string tmp;
            getline(ss_loc, tmp, ':');
            std::string cur_fname = tmp;
            getline(ss_loc, tmp, ':');
            int cur_line_no = stoi(tmp);
            call_loc_index.push_back(std::make_pair(cur_line_no, cur_fname));
        }
        auto p_cer2f = demangled_name2Func.find(cur_caller); // function filter01
        auto p_cee2f = demangled_name2Func.find(cur_callee);
        // if (p_cee2f == demangled_name2Func.end() || p_cer2f == demangled_name2Func.end()) {
        //     llvm::errs() << "Caller function: " << cur_caller << " and callee function: " << cur_callee << " not found!\n";
        //     continue;
        // }
        
        if (p_cee2f == demangled_name2Func.end()) {
            if (verbose) {
                no_fun_of << "Callee function: " << cur_callee << " not found!\n";
            }
            continue;
        }
        if (p_cer2f == demangled_name2Func.end()) {
            if (verbose) {
                no_fun_of << "Caller function: " << cur_caller << " not found!\n"; 
            }
            continue;
        }

        Function *callee_F = p_cee2f->second;
        Function *caller_F = p_cer2f->second;
        CGRelatedFunc.insert(callee_F);
        CGRelatedFunc.insert(caller_F);
        auto callee_f2c = Func2Caller.find(callee_F);
        if (Func2Caller.end() == callee_f2c) {
            InstCalleListTy *p_inst_caller_list = new InstCalleListTy;
            callee_f2c = Func2Caller.insert(std::make_pair(callee_F, p_inst_caller_list)).first;
        }

        for (auto call_loc : call_loc_index) {
            callee_f2c->second->insert(std::make_pair(call_loc.first, caller_F));
        }
    }   
    if (verbose) no_fun_of.close(); 
}

void DumpGCFG(Module *_m)
{
    std::ofstream go_bb_of;
    go_bb_of.open("global_cfg.txt", std::ios::out);
    std::ofstream bb_no_found_of;
    bb_no_found_of.open("bb_no_found.txt", std::ios::out);

    std::cout << "begin DumpGCFG\n";
    for (auto &f : *_m) {
        if (f.isIntrinsic()) continue;
        auto fi = CGRelatedFunc.find(&f);
        if (fi == CGRelatedFunc.end()) continue;//std::cout << "not in\n";
        std::string filename = PadFilename(&f);
        int entry_idx = -1;
        
        // introprocedure
        for (auto &bb : f) {
            // get the entry bb of this function
            auto pre_bbs = predecessors(&bb);
            if (pre_bbs.empty() && bb.getName() == "entry") {
                std::queue<BasicBlock*> q0;
                std::set<BasicBlock*> q0_handled;
                q0.emplace(&bb);
                while(!q0.empty()) {
                    BasicBlock *cur_bb = q0.front(); q0.pop();
                    if (!q0_handled.insert(cur_bb).second) continue;
                    auto suc_bbs = successors(cur_bb);
                    for (auto suc_bb : suc_bbs) {
                        int sub_bb_idx = GetGoBBIndex(suc_bb);
                        if (sub_bb_idx != -1) {
                            entry_idx = sub_bb_idx;
                            while(!q0.empty()) q0.pop();
                            break;
                        }
                        q0.push(suc_bb);
                    }
                }
            }

            // if (DemangleGoFunc(&f) == "gvisor.dev/gvisor/runsc/container.Container.ForwardSignals") {
            //     std::cout << entry_idx << "\n";
            //     getchar();
            // }
            
            int line_no = -1;
            int idx = GetGoBBIndex(&bb);
            if (idx != -1) {
                // get successors information
                std::queue<BasicBlock*> suc_que;
                std::set<BasicBlock*> suc_handled;
                suc_que.emplace(&bb);
                while(!suc_que.empty()) {
                    BasicBlock *suc_cur_bb = suc_que.front(); suc_que.pop();
                    if (!suc_handled.insert(suc_cur_bb).second) continue;
                    auto suc_bbs = successors(suc_cur_bb);
                    for (auto suc_bb : suc_bbs) {
                        // BE suc_be = {suc_bb, GetGoBBIndex(suc_bb)};
                        int suc_bb_idx = GetGoBBIndex(suc_bb);
                        if (suc_bb_idx != -1) {
                            go_bb_of << suc_bb_idx << ",";
                            // std::cout << suc_bb_idx << ",";
                            continue;
                        }
                        suc_que.push(suc_bb);
                    }
                }
                //  successor index:predecessor index@current function name@current filename
                go_bb_of << ":" << idx << "@" << DemangleGoFunc(&f) << "@" << PadFilename(&f) << "\n";
                // std::cout << ":" << idx << "@" << DemangleGoFunc(&f) << "@" << PadFilename(&f) << "\n";
            }
        }
        go_bb_of << entry_idx << ":" << "entry" << "@" << DemangleGoFunc(&f) << "@" << PadFilename(&f) << "\n";
        // std::cout << entry_idx << ":" << "entry" << "@" << DemangleGoFunc(&f) << "@" << PadFilename(&f) << "\n";

        // interprocedure
        // get callers
        InstCalleListTy *caller_pairs = GetCalle(&f, &Func2Caller);    
        if (!caller_pairs) continue;
        // loop over the callers
        for (auto caller_pair : *caller_pairs) {
            int call_loc = caller_pair.first;
            Function *caller = caller_pair.second;
            std::string filename = PadFilename(caller);
            LineIndexTy target_line_ind = std::make_pair(call_loc, filename);
            BasicBlock *ptarget_bb = GetBB(target_line_ind, 0);
            if (!ptarget_bb) {
                bb_no_found_of << "Warning!\n";
                bb_no_found_of << "Line Number:" << call_loc << " Function:" << DemangleGoFunc(caller).c_str() << " File:" << filename << " has no corresponding Basic Block.\n";
            }
            else {
                // if entry_idx == -1, that means this function has no go bb
                // callee entry index:call statement index@callee function name@callee filename@caller function name@caller filename#call statement location
                go_bb_of << entry_idx << ":" << GetGoBBIndex(ptarget_bb) << "@" << DemangleGoFunc(&f) << "@" << PadFilename(&f) << "@" << DemangleGoFunc(caller) << "@" << PadFilename(caller) << "#" << call_loc << "\n";
                // std::cout << entry_idx << ":" << GetGoBBIndex(ptarget_bb) << "@" << DemangleGoFunc(caller) << "@" << PadFilename(caller) << "\n";
            }
        }
    }
    std::cout << "end DumpGCFG\n";
    go_bb_of.close();
    bb_no_found_of.close();
}

void InitDS(Module &M, bool verbose)
{   
    // Load demangled functions names
    std::string line0;
    std::ifstream df(demangled_llvm_fname_filename);
    // line00: llvm function name@ demangled funciton name
    if (df.is_open()) {
        while (getline(df, line0))
        {
            std::stringstream ss_line(line0);
            std::string line_part;
            std::vector<std::string> cur_part;
            while (getline(ss_line, line_part, '@')) {
                cur_part.push_back(line_part);
            }
            llvm_name2demangled_name.insert(std::make_pair(cur_part[0], cur_part[1]));
        }
        df.close();
    } 
    else {
        llvm::errs() << "file:" << demangled_llvm_fname_filename << " no found!\n";
        exit(-1);
    }
    // Go over the module
    for (auto &Func : M) {
        if (Func.isIntrinsic()) continue;
        std::string filename =  PadFilename(&Func);
        demangled_name2Func.insert(std::make_pair(DemangleGoFunc(&Func), &Func));
        if (verbose) {
            std::ofstream outfile;
            outfile.open(padding_func_info_filename, std::ios::out | std::ios::app);
            outfile << Func.getName().str().c_str() << "@" << DemangleGoFunc(&Func) << "@" << filename.c_str() << "\n";
            outfile.close();
        }
    
        for (auto &BB : Func) {
            BBListTy *p_bb_list = new BBListTy;
            std::string index_name;
            index_name.append(Func.getName()).append("@").append(BB.getName());
            name2BB[index_name] = &BB;
            int line_no = -1;
            for (auto &Inst : BB) {
                if (const DebugLoc &location = Inst.getDebugLoc()) {
                    line_no = location.getLine();
                    LineIndexTy line_ind = std::make_pair(line_no, filename);
                    
                    int idx = GetInstrucmentIndices(&Inst, verbose);
                    if (idx != -1) {
                        assert(BB2ID.insert(std::make_pair(&BB, idx)).second != false); // one llvm bb can only have one go-bb index.
                    }
                    // if (DemangleGoFunc(&Func) == "gvisor.dev/gvisor/pkg/sentry/mm.translateIOError") {
                    //     std::cout << line_no << ":\n";
                    //     Inst.dump();
                    // }
                    auto res_l2b = Line2BBs.find(line_ind);
                    if (Line2BBs.end() == res_l2b) {
                        res_l2b = Line2BBs.insert(std::make_pair(line_ind, p_bb_list)).first;
                        // res_l2b->second->insert(&BB); 
                    } 
                    res_l2b->second->insert(&BB);
                } 
                else {
                    // No location metadata available
                }
            }
        }
    }
}

BasicBlock *GetBB(LineIndexTy line_ind, int delta, bool verbose)
{
    auto res_l2b = Line2BBs.find(line_ind);
    BasicBlock *tbb = nullptr;
    if (Line2BBs.end() != res_l2b) {
        if (res_l2b->second->size() > 0) {
            auto i = res_l2b->second->begin();
            auto e = res_l2b->second->end(); 
            advance(i, delta);
            for (; i != e; ++i) {
                auto bb = *i;
                if (GetGoBBIndex(bb) != -1) {
                    tbb = bb;
                    return tbb;
                }   
                else {
                    std::queue<BasicBlock*> tmp_que;
                    tmp_que.emplace(bb);
                    std::set<BasicBlock*> tmp_handled;
                    while(!tmp_que.empty()) {
                        BasicBlock *tmp_cur_bb = tmp_que.front(); tmp_que.pop();
                        // outs() << tmp_cur_bb->getName() << "\t";
                        // Check repetition
                        if (!tmp_handled.insert(tmp_cur_bb).second) continue;
                        auto tmp_pre_bbs = predecessors(tmp_cur_bb);
                        for (auto tmp_pre_bb : tmp_pre_bbs) {
                            int idx = GetGoBBIndex(tmp_pre_bb);
                            if (idx != -1) {
                                tbb = tmp_pre_bb;
                                return tbb;
                            }
                            tmp_que.push(tmp_pre_bb);
                        }
                    }
                }
            }

            if (verbose) {
                auto i = res_l2b->second->begin();
                auto e = res_l2b->second->end(); 
                advance(i, delta);
                std::ofstream bbs_of;
                bbs_of.open(target_bb_candidates_info_filename, std::ios::out | std::ios::app);
                bbs_of << "Filename:" << line_ind.second << "@Line:" << line_ind.first << "\n";
                bbs_of << "Current BB list size:" << res_l2b->second->size() << "\n";
                for (; i != e; ++i) {
                    auto bb = *i;
                    bbs_of << bb->getParent()->getName().str().c_str() << "@" << bb->getName().str().c_str() << "@" << GetGoBBIndex(bb) << "\n"; // func_name@bb_name@go_id
                }
                bbs_of.close();
            }
        } 
    }
    return nullptr;
}

InstCalleListTy *GetCalle(Function *_target_func, Func2CalleTy *_f2c)
{
    auto f2c_i = _f2c->find(_target_func);
    if (_f2c->end() != f2c_i) {
        InstCalleListTy *c_list = f2c_i->second;
        return c_list;
    }
    return nullptr;
}

std::pair<int, int> GetBBLineNo(BasicBlock *target_BB)
{
    int bb_begin = 0, bb_end = 0;
    auto i = target_BB->begin(), e = target_BB->end();
    for (--e; i != e; --e) {
        auto e_dbg = e->getDebugLoc().get();
        if (!e_dbg) {
            continue;
        }
        bb_end = e->getDebugLoc().getLine();
        break;
    }
    for (; i != e; ++i) {
        auto i_dbg = i->getDebugLoc().get();
        if (!i_dbg) {
            continue;
        }
        bb_begin = i->getDebugLoc().getLine();
        break;
    }
    if (bb_begin == 0 && bb_end != 0) bb_begin = bb_end;
    return std::make_pair(bb_begin, bb_end);
}

void DumpCallRelation(Function *_f, InstCalleListTy *_fs, std::ofstream *_of, bool is_caller2callee)
{
    std::ofstream *of = _of;
    Function *f = _f;
    InstCalleListTy *fs = _fs;
    std::string calle = is_caller2callee ? "caller" : "callee";
    // of->open(_filename, std::ios::out | std::ios::app);
    *of << "func:" << DemangleGoFunc(f) << " " << calle <<  ":\n";
    if (fs != nullptr) {
        for (auto fp : *fs) {
            int call_loc = fp.first;
            Function *calle = fp.second;
            *of << DemangleGoFunc(calle) << ":" << PadFilename(calle) << ":" << call_loc << "\n";
        }
    }
    *of << "\n";
}

void DumpCGLevelLengthOnly(Function *F, bool verbose)
{
    std::ofstream outfile;
    outfile.open(cg_level_length_filename, std::ios::out);
    outfile << "CG_level_length" << "@" << "function_name_llvm" << "@" << "function_name_demangled" << "@" << "call_line_number" << "@" << "padded_filename\n";
    
    typedef struct func_element {
        Function *f;
        int length;
    } FE;
    std::queue<FE> que;
    FE fe = {F, 0};
    que.emplace(fe);

    std::set<Function*> handled;
    handled.insert(F);

    while(!que.empty()) {
        FE &cur_fe = que.front(); que.pop();
        Function *cur_f = cur_fe.f;
        int cur_len = cur_fe.length;
        // Get callers
        InstCalleListTy *caller_pairs = GetCalle(cur_f, &Func2Caller);
        // if (verbose) DumpCallRelation(cur_f, caller_pairs);
        if (!caller_pairs) continue;
        // Loop over the callers
        for (auto caller_pair : *caller_pairs) {
            int call_loc = caller_pair.first;
            Function *caller = caller_pair.second;
            int caller_len = cur_len + 1;
            FE caller_fe = {caller, caller_len};
            // Check repetition
            if (!handled.insert(caller).second) continue;
            
            std::string filename = PadFilename(caller);
            outfile << caller_len << "@" << caller->getName().str().c_str() << "@" << DemangleGoFunc(caller).c_str() << "@" << call_loc << "@" << filename << "\n";
            que.push(caller_fe);
        }
    }
    outfile.close();
}

void InitCG(Function *_F, BasicBlock *_BB)
{
    Function *F = _F;
    BasicBlock *BB = _BB;
    std::queue<Function*> que;
    que.emplace(F);

    std::set<Function*> handled;
    handled.insert(F);

    TargetRelatedFunc.insert(F);

    // construct func2callee & gather target related functions
    while(!que.empty()) {
        Function *cur_f = que.front(); que.pop();
        // Get callers
        InstCalleListTy *caller_pairs = GetCalle(cur_f, &Func2Caller);
        // if (verbose) DumpCallRelation(cur_f, caller_pairs);
        if (!caller_pairs) continue;
        // Loop over the callers
        
        for (auto caller_pair : *caller_pairs) {
            int call_loc = caller_pair.first;
            Function *caller = caller_pair.second;
            
            Function *callee_F = cur_f;
            Function *caller_F = caller;
            auto callee_f2c = Func2Callee.find(caller_F);
            if (Func2Callee.end() == callee_f2c) {
                InstCalleListTy *p_inst_callee_list = new InstCalleListTy;
                callee_f2c = Func2Callee.insert(std::make_pair(caller_F, p_inst_callee_list)).first;
            }
            callee_f2c->second->insert(std::make_pair(call_loc, callee_F));

            // Check repetition
            if (!handled.insert(caller).second) continue;
            TargetRelatedFunc.insert(caller);

            que.push(caller);
        }
    }
}

void DumpCGLevelLength(Function *F, BasicBlock *BB, bool verbose)
{
    // std::ofstream outfile;
    // outfile.open(cg_level_length_filename, std::ios::out);
    // outfile << "CG_level_length" << "@" << "function_name_llvm" << "@" << "function_name_demangled" << "@" << "call_line_number" << "@" << "padded_filename\n";
    
    typedef struct func_element {
        Function *f;
        int length;
        int bb_len_delta;
    } FE;
    std::queue<FE> que;
    FE fe = {F, 0, DumpCFGLevelLength(BB, 0, verbose)};
    que.emplace(fe);

    using pFB_t = std::pair<Function*, BasicBlock*>;
    std::set<pFB_t> handled;
    handled.insert(std::make_pair(F, BB));

    while(!que.empty()) {
        FE &cur_fe = que.front(); que.pop();
        Function *cur_f = cur_fe.f;
        int cur_len = cur_fe.length;
        int cur_bb_delta = cur_fe.bb_len_delta;

        // Get callers
        InstCalleListTy *caller_pairs = GetCalle(cur_f, &Func2Caller);
        // if (verbose) DumpCallRelation(cur_f, caller_pairs);
        if (!caller_pairs) continue;
        // Loop over the callers
        for (auto caller_pair : *caller_pairs) {
            int call_loc = caller_pair.first;
            Function *caller = caller_pair.second;
            int caller_len = cur_len + 1;
            FE caller_fe = {caller, caller_len, 0};            
            
            std::string filename = PadFilename(caller);
            // outfile << caller_len << "@" << caller->getName().str().c_str() << "@" << DemangleGoFunc(caller).c_str() << "@" << call_loc << "@" << filename << "\n";
            
            int line_no = call_loc;
            LineIndexTy target_line_ind = std::make_pair(line_no, filename);
            BasicBlock *ptarget_bb = GetBB(target_line_ind, 0, verbose);
            if (!ptarget_bb) {
                llvm::errs().changeColor(raw_ostream::RED, /*bold=*/true, /*bg=*/false);
                llvm::errs() << "Warning!\n";
                llvm::errs().resetColor();
                llvm::errs() << "Line Number:" << line_no << " Function:" << DemangleGoFunc(caller).c_str() << " File:" << filename << " has no corresponding Basic Block.\n";
            }
            else {
                // Check repetition
                if (!handled.insert(std::make_pair(caller, ptarget_bb)).second) continue;

                caller_fe.bb_len_delta = DumpCFGLevelLength(ptarget_bb, cur_bb_delta, verbose); // the CFG of the caller function
                // if (verbose) outs() << "\ndelta distance:" << DemangleGoFunc(caller) << ":" << caller_fe.bb_len_delta << "\n";
                que.push(caller_fe);
            }
            
        }
    }
    // outfile.close();
    // std::cout <<  "123\n";
}

int GetGoBBIndex(BasicBlock *BB)
{  
    auto bb2id = BB2ID.find(BB);
    if (BB2ID.end() != bb2id) {
        int go_id = bb2id->second;
        return go_id;
    }
    return -1;
}

void DumpBBRelation(BasicBlock *BB, llvm::pred_range *BBs)
{
    Function *f = BB->getParent();
    std::ofstream outfile_path;
    outfile_path.open(dump_bb_relation_info_filename, std::ios::out | std::ios::app);
    outfile_path << "Current BB:" << BB->getName().str().c_str() << "@Go BB index:" << GetGoBBIndex(BB) \
            << "@Function:" << DemangleGoFunc(f) <<  "@file:" << PadFilename(f) << "\n";
    
    if (BBs) {
        for (auto bb : *BBs) {
            auto line_range = GetBBLineNo(bb);
            outfile_path << "Predeseccor BB:" << bb->getName().str().c_str() << "@Go BB index:" << GetGoBBIndex(bb) \
                << "@Line:" << line_range.first << "-" << line_range.second << "\n";
        }
    }

    outfile_path << "\n";
    outfile_path.close();
}


int DumpCFGLevelLength(BasicBlock *BB, int bb_len_delta, bool verbose)
{
    Function *cur_f = BB->getParent();
    int inc_bb_len_delta = 0;

    typedef struct bb_element {
        BasicBlock *bb;
        int go_bb_id;
    } BE;
    BE be = {BB, GetGoBBIndex(BB)};
    // outs() << "init:" << be.go_bb_id << ":" << be.length << "\n";
    assert(be.go_bb_id != -1 && "the first llvm bb hasn't corresponding go bb!");
    std::queue<BE> que;
    que.emplace(be);

    using GoBBId = int;
    using GoBBLength = int;
    std::map<GoBBId, GoBBLength> id2len;
    id2len.insert(std::make_pair(be.go_bb_id, 0 + bb_len_delta));

    std::ofstream go_bb_of;
    go_bb_of.open(go_bb_relation_filename, std::ios::out | std::ios::app);

    go_bb_of << "" << ":" << be.go_bb_id << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[be.go_bb_id] << "\n"; 
    std::cout << "" << ":" << be.go_bb_id << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[be.go_bb_id] << "\n";

    // std::set<BasicBlock*> handled;
    // handled.insert(BB);
    handled_bb.insert(BB);

    int entry_id = 0;
    while(!que.empty()) {
        BE cur_be = que.front(); que.pop();  
        BasicBlock *cur_bb = cur_be.bb;
        // Get predecessors
        auto pre_bbs = predecessors(cur_bb);
        if (verbose) DumpBBRelation(cur_bb, &pre_bbs);
        // Find entry id
        if (pre_bbs.empty()) {
            std::queue<BE> tmp_que;
            tmp_que.emplace(cur_be);
            while(!tmp_que.empty()) {
                BE tmp_cur_be = tmp_que.front(); tmp_que.pop();
                BasicBlock *tmp_cur_bb = tmp_cur_be.bb;
                // outs() << tmp_cur_bb->getName() << "\n";
                auto tmp_suc_bbs = successors(tmp_cur_bb);
                for (auto tmp_suc_bb : tmp_suc_bbs) {
                    // outs() << tmp_suc_bb->getName() << "\n";
                    BE tmp_suc_be = {tmp_suc_bb, GetGoBBIndex(tmp_suc_bb)};
                    if (tmp_suc_be.go_bb_id != -1) {
                        // outs() << "entry: " << tmp_suc_be.go_bb_id << "\n";
                        entry_id = tmp_suc_be.go_bb_id;
                        while(!tmp_que.empty()) tmp_que.pop();
                        break;
                    }
                    tmp_que.push(tmp_suc_be);
                }
            }
        }
        // Loop over the predecessors
        for (auto pre_bb : pre_bbs) {
            BE pre_be = {pre_bb, GetGoBBIndex(pre_bb)};
            // Check repetition
            if (!handled_bb.insert(pre_bb).second) continue;
            // if (!handled.insert(pre_bb).second) continue;

            if (pre_be.go_bb_id != -1) {
                id2len.insert(std::make_pair(pre_be.go_bb_id, -1));

                std::queue<BE> suc_que;
                std::set<BasicBlock*> suc_handled;
                suc_que.emplace(pre_be);
                int min_len = -1;
                while(!suc_que.empty()) {
                    BE suc_cur_be = suc_que.front(); suc_que.pop();
                    BasicBlock *suc_cur_bb = suc_cur_be.bb;
                    if (!suc_handled.insert(suc_cur_bb).second) continue;
                    auto suc_bbs = successors(suc_cur_bb);
                    for (auto suc_bb : suc_bbs) {
                        BE suc_be = {suc_bb, GetGoBBIndex(suc_bb)};
                        if (suc_be.go_bb_id != -1) {
                            go_bb_of << suc_be.go_bb_id << ",";
                            std::cout << suc_be.go_bb_id << ",";
                            auto id_len_it = id2len.find(suc_be.go_bb_id);
                            if (id_len_it != id2len.end()) {
                                int suc_len = id_len_it->second;
                                min_len = (unsigned)suc_len < (unsigned)min_len ? suc_len : min_len;
                            }
                            else {
                                id2len.insert(std::make_pair(suc_be.go_bb_id, -1));
                            }
                            continue;
                        }
                        suc_que.push(suc_be);
                    }
                }
                id2len[pre_be.go_bb_id] = min_len + 1;
                go_bb_of << ":" << pre_be.go_bb_id << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[pre_be.go_bb_id] << "\n";
                std::cout << ":" << pre_be.go_bb_id << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[pre_be.go_bb_id] << "\n";
            }
            
            que.push(pre_be);
            //successor_id01,successor_id02,...,:go_bb_id@function@demangled_filename@bb_length
        }
    }
    // go_bb_of << entry_id << ":" << "entry" << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[entry_id] << "\n"; 
    // std::cout << entry_id << ":" << "entry" << "@" << DemangleGoFunc(cur_f) << "@" << PadFilename(cur_f) << "@" << id2len[entry_id] << "\n"; 
    go_bb_of.close();    
    return inc_bb_len_delta = id2len[entry_id] + 1;
}


void testCallReation(Function *F)
{
    std::ofstream of;
    std::string filename = "call_relation.txt";
    of.open(filename, std::ios::out);
    of << "\n=============================testCallReation=============================\n";

    std::queue<Function*> que;
    que.emplace(F);

    std::set<Function*> handled;
    handled.insert(F);

    while(!que.empty()) {
        Function *cur_f = que.front(); que.pop();
        // Get callees
        InstCalleListTy *callee_pairs = GetCalle(cur_f, &Func2Callee);
        DumpCallRelation(cur_f, callee_pairs, &of, false);
        // Get callers
        InstCalleListTy *caller_pairs = GetCalle(cur_f, &Func2Caller);
        DumpCallRelation(cur_f, caller_pairs, &of, true);
        if (!caller_pairs) continue;
        // Loop over the callers
        for (auto caller_pair : *caller_pairs) {
            int call_loc = caller_pair.first;
            Function *caller = caller_pair.second;
            
            Function *callee_F = cur_f;
            Function *caller_F = caller;
            auto caller_f2c = Func2Callee.find(caller_F);
            if (Func2Caller.end() == caller_f2c) {
                InstCalleListTy *p_inst_callee_list = new InstCalleListTy;
                caller_f2c = Func2Caller.insert(std::make_pair(caller_F, p_inst_callee_list)).first;
            }
            caller_f2c->second->insert(std::make_pair(call_loc, callee_F));

            // Check repetition
            if (!handled.insert(caller).second) continue;
            TargetRelatedFunc.insert(caller);

            que.push(caller);
        }
    }
    of << "==================================================================\n";
    of.close();
}
