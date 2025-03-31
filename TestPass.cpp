//=============================================================================
// FILE:
//    TestPass.cpp
//
// DESCRIPTION:
//    Visits all functions in a module and prints their names. Strictly speaking,
//    this is an analysis pass (i.e. //    the functions are not modified). However,
//    in order to keep things simple there's no 'print' method here (every analysis
//    pass should implement it).
//
// USAGE:
//    New PM
//      opt -load-pass-plugin=<path-to>libTestPass.so -passes="test-pass" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/MathExtras.h" // Per Log2_64
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

LLVMContext Context; 

/* PSEUDOCODICE PROGRAMMA STRENGTH REDUCTION:

	if istruzione = moltiplicazione
	 |	
	 |	if primoOperando è valore costante 
	 |	 |	if primoOperando è potenza di 2
	 |	 |	 |	trasforma istruzione in var << log2(primoOperando)
	 |	 |	else
	 |	 |       if (primoOperando + 1) è potenza di 2
	 |	 |		 |	trasforma istruzione in var << log2(primoOperando) - secondoOperando
	 | 	 | 		else if (primoOperando - 1) è potenza di 2
	 |	 |			trasforma istruzione in var << log2(primoOperando) + secondoOperando
	 |	 |		
	 |	 |
	 |	else if secondoOperando è valore costante
	 |		if secondoOperando è potenza di 2
	 |		 |	trasforma istruzione in var << log2(secondoOperando)
	 |		else
	 |			if (secondoOperando + 1) è potenza di 2
	 |			 |	trasforma istruzione in var << log2(secondoOperando) - primooOperando
	 |			else if (secondoOperando - 1) è potenza di 2
	 |				trasforma istruzione in var << log2(secondoOperando) + primooOperando
	 |
	else if istruzione = divisone
		if secondoOperando è potenza di 2
			trasforma istruzione in var >> log2(secondoOperando)
*/






bool runOnBasicBlock(BasicBlock &B) {
	for (auto iter = B.begin(); iter != B.end();) { //iter --> puntatore che scorre ogni istruzione del BB
		Instruction &I = *iter++; 
	
		auto *Iter = I.op_begin(); // Iter -> puntatore che scorre i singoli operandi di un'istruzione
		Value *Operand1 = *Iter;	
		 ++Iter;
		Value *Operand2 = *Iter;
		
		if (I.getOpcode() == Instruction::Mul) {
			unsigned int int_val_op = 0; // valore intero primo operando
			unsigned int int_val_op2 = 0; // valore intero secondo operando
			
			if (llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand1)) { //verifico tramite dynamic_cast che Operand1 è una costante
				int_val_op = CI->getValue().getZExtValue(); //getZExtValue(); trasforma APInt in unsigned int
			}
			
			if (llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand2)) { 
				int_val_op2 = CI->getValue().getZExtValue();
			}
			
			if (int_val_op != 0) { // se opeando uno ha valore diverso da 0 (con cui è stato inizializzato), vuol dire che contiene una costante
				llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand1);
				unsigned int log2op = CI->getValue().exactLogBase2(); //exactLogBase2() restituisce log2(N) se N è una potenza di 2, -1 altrimenti
				
				if (log2op != -1) { 
					Instruction *NewInst = BinaryOperator::Create(
						Instruction::Shl, I.getOperand(1), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2op, false)); // Nuova istruzione
					NewInst->insertBefore(&I); //Inserisco la nuova istruzione nel BB
					I.replaceAllUsesWith(NewInst); // Rimpiazzo tutti i rifrimenti della vecchia istruzione con quelli della nuova
					I.eraseFromParent(); //rimuovo la vecchia istruzione dal BB
				} else {
					llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand1);
					llvm::APInt incValue = CI->getValue() + 1;
					llvm::APInt decValue = CI->getValue() - 1;
					
					unsigned int log2opI = incValue.exactLogBase2(); //Calcolo il log2 su operando+1
					unsigned int log2opD = decValue.exactLogBase2(); //Calcolo il log2 su oeprando-1
				
					if (log2opI != -1) {
						Instruction *ShiftInst = BinaryOperator::Create(
							Instruction::Shl, I.getOperand(1), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2opI, false));
						ShiftInst->insertBefore(&I);
						
						Instruction *SubInst = BinaryOperator::Create(
							Instruction::Sub, ShiftInst, I.getOperand(1));
						SubInst->insertBefore(&I);
						
						I.replaceAllUsesWith(SubInst);
						I.eraseFromParent();
					} else if (log2opD != -1) {
						Instruction *ShiftInst = BinaryOperator::Create(
							Instruction::Shl, I.getOperand(1), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2opD, false));
						ShiftInst->insertBefore(&I);
						
						Instruction *AddInst = BinaryOperator::Create(
							Instruction::Add, ShiftInst, I.getOperand(1));
						AddInst->insertBefore(&I);
						
						I.replaceAllUsesWith(AddInst);
						I.eraseFromParent();
					}
				}
			} else if (int_val_op2 != 0) {
				llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand2);
				unsigned int log2op = CI->getValue().exactLogBase2();
				
				if (log2op != -1) {
					Instruction *NewInst = BinaryOperator::Create(
						Instruction::Shl, I.getOperand(0), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2op, false));
					NewInst->insertBefore(&I);
					I.replaceAllUsesWith(NewInst);
					I.eraseFromParent();
				} else {
					llvm::ConstantInt *CI = llvm::dyn_cast<llvm::ConstantInt>(Operand2);
					llvm::APInt incValue = CI->getValue() + 1;
					llvm::APInt decValue = CI->getValue() - 1;
					
					unsigned int log2opI = incValue.exactLogBase2();
					unsigned int log2opD = decValue.exactLogBase2();
					
					if (log2opI != -1) {
						Instruction *ShiftInst = BinaryOperator::Create(
							Instruction::Shl, I.getOperand(0), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2opI, false));
						ShiftInst->insertBefore(&I);
						
						Instruction *SubInst = BinaryOperator::Create(
							Instruction::Sub, ShiftInst, I.getOperand(0));
						SubInst->insertBefore(&I);
						
						I.replaceAllUsesWith(SubInst);
						I.eraseFromParent();
					} else if (log2opD != -1) {
						Instruction *ShiftInst = BinaryOperator::Create(
							Instruction::Shl, I.getOperand(0), ConstantInt::get(Type::getInt32Ty(I.getContext()), log2opD, false));
						ShiftInst->insertBefore(&I);
						
						Instruction *AddInst = BinaryOperator::Create(
							Instruction::Add, ShiftInst, I.getOperand(0));
						AddInst->insertBefore(&I);
						
						I.replaceAllUsesWith(AddInst);
						I.eraseFromParent();
					}
				}
			}
		} else if (I.getOpcode() == Instruction::SDiv || I.getOpcode() == Instruction::UDiv) {
			if (llvm::ConstantInt *CI2 = llvm::dyn_cast<llvm::ConstantInt>(Operand2)) {
				unsigned int int_val_op2 = CI2->getValue().exactLogBase2();
				
				if (int_val_op2 != -1) {
					Instruction *NewInst = BinaryOperator::Create(
						Instruction::BinaryOps::LShr, I.getOperand(0), ConstantInt::get(Type::getInt32Ty(I.getContext()), int_val_op2, false));
					NewInst->insertBefore(&I);
					I.replaceAllUsesWith(NewInst);
					I.eraseFromParent();
				}
			}
		}
	}
	return true;
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
