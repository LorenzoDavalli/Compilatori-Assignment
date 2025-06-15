#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/PointerLikeTypeTraits.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"

using namespace llvm;

LLVMContext Context;

void stampaCFG(Function *F) {
    for (auto &BB : *F) {
        llvm::errs() << "BasicBlock: ";
        BB.printAsOperand(llvm::errs(), false);
        errs() << "\n";
        errs() << BB << "\n";
    }
}

void visitaDomTree(DomTreeNodeBase<BasicBlock> *rn, SmallPtrSet<DomTreeNodeBase<BasicBlock> *, 32> &Visited) {
    if (!rn || Visited.contains(rn)) {
        return;
    }
    Visited.insert(rn);
    BasicBlock *BB = rn->getBlock();
    BB->printAsOperand(llvm::errs(), false);
    errs() << "\n";

    if (!rn->isLeaf()) {
        for (DomTreeNodeBase<BasicBlock> *Child : rn->children()) {
            visitaDomTree(Child, Visited);
        }
    }
}

bool isSameCondition(Instruction *b1, Instruction *b2) {
    if (b1->getOpcode() == b2->getOpcode() && b1->getNumOperands() == b2->getNumOperands()) {

        for (unsigned i = 0; i < b1->getNumOperands(); ++i) {

            llvm::Value *op_br1 = b1->getOperand(i);
            llvm::Value *op_br2 = b2->getOperand(i);

            if (op_br1 != op_br2) {
                return false;
            }
        }
    } else {
        return false;
    }

    return true;
}

bool sonoAdiacenti(Loop *L1, Loop *L2) {

    if (BasicBlock *exitbb = L1->getExitBlock()) {

        if (L1->isGuarded() && L2->isGuarded()) {

            BranchInst *guardInst = L1->getLoopGuardBranch(); // prendo la istruzione brach della guardia del Loop1

            for (BasicBlock *Succ : guardInst->successors()) { // itero su tutti i successori della guardia

                if (!L1->contains(Succ)) { // controllo per prendere il successore fuori dal loop

                    if (Succ == (L2->getLoopGuardBranch())->getParent()) { // controllo che guarda1 è punta a guardia2

                        if (isSameCondition(dyn_cast<Instruction>(guardInst->getOperand(0)), dyn_cast<Instruction>(L2->getLoopGuardBranch()->getOperand(0)))) {
                            // if(guardInst->getCondition() == L2->getLoopGuardBranch()->getCondition()){          //controllo che le condizioni siano uguali

                            return true;
                        }
                    }
                }
            }

        } else {
            if (exitbb == L2->getLoopPreheader()) { // in caso non ci siano le guardie controllo soltanto che l'exitblock di L1 sia il preheader di L2
                if (exitbb->size() == 1) {

                    return true;
                }
            }
        }
    }

    return false;
}

bool hasSameIterationNumber(Loop *L1, Loop *L2, ScalarEvolution &SE) {

    const SCEV *ExitCount1 = SE.getBackedgeTakenCount(L1);
    const SCEV *ExitCount2 = SE.getBackedgeTakenCount(L2);

    if (ExitCount1 == ExitCount2) {
        return true;
    } else {
        return false;
    }
}

PHINode *getInductionVariableManual(Loop *L, ScalarEvolution &SE) { // restituisce la varibile che itera su un ciclo
    for (PHINode &PN : L->getHeader()->phis()) {                    // itero sui phi node dell'header
        const SCEV *S = SE.getSCEV(&PN);
        if (auto *AR = dyn_cast<SCEVAddRecExpr>(S)) { // controlla se l’espressione S è una SCEVAddRecExp
            if (AR->getLoop() == L) {
                return &PN; // Prendi il primo PHI che soddisfa la condizione
            }
        }
    }
    return nullptr; // Nessuna variabile di induzione trovata
}

bool hasNegativeDistanceDependencies(Loop *L1, Loop *L2, DependenceInfo &DI, ScalarEvolution &SE) {

    SmallPtrSet<Value *, 32> L1Inst; // lista di istruzioni store su vettori
    SmallPtrSet<Value *, 32> L2Inst; // lista di istruzioni load su vettori

    for (BasicBlock *BB : L1->blocks()) { // mi salvo tutte le store del loop nella lista
        for (Instruction &Inst : *BB) {
            Instruction *InstPtr = &Inst;
            if (InstPtr->getOpcode() == Instruction::Store) {
                L1Inst.insert(dyn_cast<Value>(InstPtr));
            }
        }
    }

    for (BasicBlock *BB : L2->blocks()) { // mi salvo tutte le load del loop nella lista
        for (Instruction &Inst : *BB) {
            Instruction *InstPtr = &Inst;
            if (InstPtr->getOpcode() == Instruction::Load) {
                L2Inst.insert(dyn_cast<Value>(InstPtr));
            }
        }
    }

    Value *array1;
    Value *array2;

    for (auto Inst : L1Inst) { // filtriamo la lista sulle store del vettore
        Value *arrayElement = (dyn_cast<Instruction>(Inst))->getOperand(1);
        Type *t = arrayElement->getType();
        if (t->isPointerTy()) {

            array1 = dyn_cast<Instruction>(arrayElement)->getOperand(0);
        }
    }

    for (auto Inst : L2Inst) { // filtriamo la lista sulle load del vettore
        Value *arrayElement = (dyn_cast<Instruction>(Inst))->getOperand(0);
        Type *t = arrayElement->getType();
        if (t->isPointerTy()) {
            array2 = dyn_cast<Instruction>(arrayElement)->getOperand(0);
        }
    }

    for (auto Inst1 : L1Inst) {
        for (auto Inst2 : L2Inst) {

            if (DI.depends(dyn_cast<Instruction>(Inst1), dyn_cast<Instruction>(Inst2), true)) {

                const SCEV *writeIdx = SE.getSCEV(dyn_cast<Instruction>(Inst1)->getOperand(1));
                const SCEV *readIdx = SE.getSCEV(dyn_cast<Instruction>(Inst2)->getOperand(0));
                const SCEV *start1;
                const SCEV *start2;

                if (const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(writeIdx)) {
                    start1 = AR->getStart(); // punto iniziale della scalar evolution dell'array nel loop1
                }
                if (const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(readIdx)) {
                    start2 = AR->getStart(); // punto iniziale della scalar evolution dell'array nel loop2
                }

                // calcolo distanza e controllo se negativa
                const SCEV *distance = SE.getMinusSCEV(start1, start2);

                if (const SCEVConstant *distC = dyn_cast<SCEVConstant>(distance)) {
                    int64_t distValue = distC->getAPInt().getSExtValue();
                    if (distValue < 0) {

                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void LoopFusion(Loop *L1, Loop *L2, Function &F, LoopInfo &LI) {

    BasicBlock *H2 = L2->getHeader();
    BasicBlock *H1 = L1->getHeader();

    llvm::BranchInst *b1 = dyn_cast<BranchInst>(H1->getTerminator());
    llvm::BranchInst *b2 = dyn_cast<BranchInst>(H2->getTerminator());

    unsigned successorIndex = 1; // Prendiamo il secondo operando delle store

    BasicBlock *B1;
    BasicBlock *B2;

    for (BasicBlock *succ2 : b2->successors()) { // colleghiamo l'header del loop1 al exitblock di loop2

        if (!L2->contains(succ2)) { // Prendiamo i successori del branch, l'unico successore fuori dal loop è l'exit block

            for (BasicBlock *succ1 : b1->successors()) {
                if (succ1 == L2->getLoopPreheader()) {

                    if (L1->isGuarded() && L2->isGuarded()) {
                        Instruction *guardBr = L1->getLoopGuardBranch();
                        guardBr->setSuccessor(successorIndex, succ2);
                    }

                    b1->setSuccessor(successorIndex, succ2);

                } else {
                    // per evitare di dover iterare di nuovo sui successori
                    B1 = succ1;
                }
            }

        } else {
            // per evitare di dover iterare di nuovo sui successori
            B2 = succ2;
        }
    }

    // colleghiamo i due body
    successorIndex = 0; // Prendiamo il primo (e unico) operando delle load
    if (B1 != nullptr && B2 != nullptr) {
        llvm::BranchInst *b1 = dyn_cast<BranchInst>(B1->getTerminator());

        BasicBlock *L1 = b1->getSuccessor(successorIndex);
        b1->setSuccessor(successorIndex, B2);

        llvm::BranchInst *b2 = dyn_cast<BranchInst>(B2->getTerminator());
        b2->setSuccessor(successorIndex, L1);

        // questi prossimi loop e if servono per correggere i phinode dopo la fusione dei loop

        Instruction *phiB1;
        for (auto it = B1->begin(); it != B1->end(); ++it) {
            Instruction *i = &*it;
            if (i->getOpcode() == Instruction::SExt) {
                phiB1 = dyn_cast<Instruction>(i->getOperand(0));
                if (phiB1->getOpcode() != Instruction::PHI) {
                    phiB1 = nullptr;
                }
            }
        }

        Instruction *phiB2;
        Instruction *oldSetExt;
        for (auto it = B2->begin(); it != B2->end(); ++it) {
            Instruction *i = &*it;
            if (i->getOpcode() == Instruction::SExt) {
                phiB2 = dyn_cast<Instruction>(i->getOperand(0));
                if (phiB2->getOpcode() != Instruction::PHI) {
                    phiB2 = nullptr;
                } else {
                    oldSetExt = i;
                }
            }
        }

        if (phiB1 != nullptr && phiB2 != nullptr) {
            IRBuilder<> builder(oldSetExt);
            Value *newSext = builder.CreateSExt(dyn_cast<Value>(phiB1), Type::getInt64Ty(F.getContext()));
            oldSetExt->replaceAllUsesWith(newSext);
            oldSetExt->eraseFromParent();
        }
    }
}

namespace {

struct TestPass : PassInfoMixin<TestPass> {

    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {

        llvm::LLVMContext &Context = F.getContext();

        LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

        if (LI.empty()) {

            return PreservedAnalyses::all();
        }

        DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
        PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);

        for (Loop *L1 : LI) {
            for (Loop *L2 : LI) {
                if (L1 != L2) {

                    if (sonoAdiacenti(L1, L2)) {

                        BasicBlock *b1;
                        BasicBlock *b2;

                        if (L1->isGuarded() && L2->isGuarded()) {

                            b1 = L1->getLoopGuardBranch()->getParent();
                            b2 = L2->getLoopGuardBranch()->getParent();

                        } else {
                            b1 = L1->getHeader();
                            b2 = L2->getHeader();
                        }

                        if (DT.dominates(b1, b2)) {

                            if (PDT.dominates(b2, b1)) {

                                ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);

                                if (hasSameIterationNumber(L1, L2, SE)) {
                                    DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);

                                    if (!hasNegativeDistanceDependencies(L1, L2, DI, SE)) {
                                        LoopFusion(L1, L2, F, LI);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
	
	llvm::removeUnreachableBlocks(F);
       	stampaCFG(&F);

        return PreservedAnalyses::all();
    }
    static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "TestPass", LLVM_VERSION_STRING, [](PassBuilder &PB) {
                PB.registerPipelineParsingCallback([](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
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
extern "C" LLVM_ATTRIBUTE_WEAK::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() { return getTestPassPluginInfo(); }
