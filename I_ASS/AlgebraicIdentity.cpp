#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

LLVMContext Context; 
bool isOptimizable(Instruction &Inst) {
    if (Inst.getOpcode() != Instruction::Add && Inst.getOpcode() != Instruction::Mul) {
      return false;
    }
    
    bool replaceable=false;
    
    for (int i = 0; i < 2; i++) {
      if (ConstantInt *ConstInst = dyn_cast<ConstantInt>(Inst.getOperand(i))) {
      
        if (Inst.getOpcode() == Instruction::Mul && ConstInst->getValue() == 1){
          replaceable=true;
        }
        
        if(Inst.getOpcode() == Instruction::Add && ConstInst->getValue() == 0){
          replaceable=true;
        }
        
 	if(replaceable){
 		Inst.replaceAllUsesWith(Inst.getOperand(1 - i));
          	Inst.eraseFromParent();
          	return true;
          
          }
      }
    }
    return false;
  }

bool runOnBasicBlock(BasicBlock &B) {
bool Transformed = false;
		for (auto it = B.begin(), end = B.end(); it != end;) {
			Instruction &I = *it++;
			if (isOptimizable(I)) {
				Transformed = true;
			}
		}
	
return Transformed;
}
//-----------------------------------------------------------------------------
// TestPass implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {


// New PM implementation
struct TestPass: PassInfoMixin<TestPass> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be)
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {

 // errs() <<"Nome Funzione : "<< F.getName() <<"\n";
 
 // errs() <<"Numero di argomenti : "<< F.arg_size();
 
  if (F.isVarArg()){
 
 // errs() << "+*";
  }
 
 // errs() <<"\n";
 
  int numCallInst=0;
  int numBB=0;
 
  for (auto iter = F.begin(); iter != F.end(); ++iter){
 
  BasicBlock &B = *iter;
 
  numBB++;
 
  for (auto iter2 = B.begin(); iter2 != B.end(); ++iter2){
 
  Instruction &inst= *iter2;
 
  if(dyn_cast<CallInst>(&inst) != nullptr)  numCallInst++;
 
  }
 
  }
 
 // errs() <<"Numero di chiamate a funzione : "<< numCallInst <<"\n";
//errs() <<"Numero di di istruzioni : "<< F.getInstructionCount() <<"\n";
//errs() <<"Numero di di BasicBlock : "<< numBB <<"\n";

  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }


  return PreservedAnalyses::all();
}


  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "TestPass", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "test-pass") {
                    FPM.addPass(TestPass());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getTestPassPluginInfo();
}
