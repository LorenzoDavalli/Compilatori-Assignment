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
#include "llvm/IR/Module.h"

using namespace llvm;

LLVMContext Context; 

typedef std::vector<std::vector<llvm::Value*>> instructionMatrix;

Value *ricercaRicorsivaMatrice(instructionMatrix m, Value *reg, int iterationIndex, int c, std::vector<Instruction*> &optimazibleInstructions) {
	for (int i = iterationIndex - 1; i >= 0; i--) {
		BinaryOperator *previous_inst = dyn_cast<BinaryOperator>(m[i][0]);
		BinaryOperator *current_inst = dyn_cast<BinaryOperator>(reg);

		if (!previous_inst || !current_inst || !previous_inst->isIdenticalTo(current_inst)) {
			continue;
		}
		
		ConstantInt* CI = dyn_cast<ConstantInt>(m[i][2]);
		if (!CI) {
			errs() << "Il valore non è una costante intera.\n";
		continue;
		}

		int intValue = CI->getZExtValue();
		if (previous_inst->getOpcode() == Instruction::Sub) {
			intValue = intValue * -1;
		}

		if (previous_inst->getOpcode() == Instruction::Add || previous_inst->getOpcode() == Instruction::Sub) {
			c += intValue;

			
			optimazibleInstructions.push_back(previous_inst);

			if (c == 0) {
				return m[i][1];
			} else {
				return ricercaRicorsivaMatrice(m, m[i][1], i, c, optimazibleInstructions);
			}
		}
	}
	return nullptr;
}

bool runOnBasicBlock(BasicBlock &B) {
instructionMatrix m;

for (auto iter = B.begin(); iter != B.end(); ++iter) {
	Instruction &I = *iter;
	if (llvm::BinaryOperator *bin_inst = llvm::dyn_cast<llvm::BinaryOperator>(&I)) {
		if (isa<Constant>(bin_inst->getOperand(1))) {
			std::vector<llvm::Value*> newRow = { bin_inst, bin_inst->getOperand(0), bin_inst->getOperand(1) };
			if (newRow.size() == 3) {
				m.push_back(newRow);
			}
		}
	}
}

std::vector<Instruction*> uselessInstructions;
std::vector<std::pair<Instruction*, Value*>> replacements;


for (int i = m.size()-1; i >= 0; i--) {
	Value *Operand1 = m[i][1];
	Value *Operand2 = m[i][2];
	llvm::BinaryOperator *current_inst = llvm::dyn_cast<llvm::BinaryOperator>(m[i][0]);

	if (current_inst->getOpcode() == Instruction::Add || current_inst->getOpcode() == Instruction::Sub) {
		if (llvm::ConstantInt* constant_value = llvm::dyn_cast<llvm::ConstantInt>(m[i][2])) {
			int intValue = constant_value->getZExtValue();
			if (current_inst->getOpcode() == Instruction::Sub) {
				intValue = intValue * -1;
			}
			
			std::vector<Instruction*> optimazibleInstructions;

			Value *ris = ricercaRicorsivaMatrice(m, Operand1, i, intValue, optimazibleInstructions);

			if (ris != nullptr) {
				
				replacements.push_back(std::make_pair(current_inst, ris));


				uselessInstructions.push_back(current_inst);

				
				for (Instruction* inst : optimazibleInstructions) {
					uselessInstructions.push_back(inst);
				}
			}
		}
	}
}

for (const auto &[inst, replacement] : replacements) {
		inst->replaceAllUsesWith(replacement);
	}

	for (auto *inst : uselessInstructions) {
		inst->eraseFromParent();
	}

return true;
}

namespace {

	struct TestPass : PassInfoMixin<TestPass> {		
		PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
			bool Transformed = false;

			for (auto BB = F.begin(); BB != F.end(); ++BB) {
				if (runOnBasicBlock(*BB)) {
					Transformed = true;
				}
			}
			
			return PreservedAnalyses::all();
		}

		static bool isRequired() { return true; }
	};
}

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
