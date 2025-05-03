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

#include "llvm/Analysis/LoopInfo.h"

#include "llvm/IR/Dominators.h"

#include "llvm/Analysis/PostDominators.h"

#include "llvm/IR/BasicBlock.h"

#include "llvm/IR/CFG.h"

#include "llvm/Analysis/MemorySSA.h"

#include "llvm/Analysis/MemorySSAUpdater.h"

using namespace llvm;

LLVMContext Context;

void stampaSet(SmallPtrSet < Value * , 32 > & set) {
    for (Value * v: set) {
        v -> print(errs());
        errs() << "\n";
    }
}

void stampaCFG(Function * F) {
    for (auto & BB: * F) {
        llvm::errs() << "BasicBlock: ";
        BB.printAsOperand(llvm::errs(), false);
        errs() << "\n";
        errs() << BB << "\n";
    }
}

void visitaDomTree(DomTreeNodeBase < BasicBlock > * rn, SmallPtrSet < DomTreeNodeBase < BasicBlock > * , 32 > & Visited) {
    if (!rn || Visited.contains(rn)) {
        return;
    }
    Visited.insert(rn);
    BasicBlock * BB = rn -> getBlock();
    BB -> printAsOperand(llvm::errs(), false);
    errs() << "\n";

    if (!rn -> isLeaf()) {
        for (DomTreeNodeBase < BasicBlock > * Child: rn -> children()) {
            visitaDomTree(Child, Visited);
        }

    }
}


  llvm::SmallPtrSet <Value*,32> createLoopInvariantSet(Loop *L){
    
    SmallPtrSet < Value * , 32 > loopInvariantSet;

        
            llvm::BasicBlock * hd = L -> getHeader();

            for (Loop::block_iterator BI = L -> block_begin(); BI != L -> block_end(); ++BI) {
                BasicBlock * BB = * BI;
                for (llvm::BasicBlock::iterator it = BB -> begin(), end = BB -> end(); it != end; ++it) {
                    llvm::Instruction & Inst = * it;

                    if (Instruction::isBinaryOp(Inst.getOpcode())) {
                        bool isInv = true;
                        bool OpInv = true;
                        for (auto Iter = Inst.op_begin(); Iter != Inst.op_end(); ++Iter) {

                            Value * Operand = * Iter;

                            if (Instruction * operandInst = llvm::dyn_cast < Instruction > (Operand)) { //se no sono costanti
                                llvm::BasicBlock * parentBB = (operandInst -> getParent());

                                if (L->contains(parentBB)) {
                                    isInv = false;
                                }

                                if (!loopInvariantSet.contains(Operand)) {
                                    OpInv = false;
                                }
                            }

                        }
                        if (isInv && OpInv) {
                            loopInvariantSet.insert(llvm::dyn_cast < Value > ( & Inst));
                           

                        }
                    }
                }

            }

        

        return loopInvariantSet;
    }


    void calculateLoopInvariantDominance(DominatorTree & DT, SmallPtrSet < Value * , 32 > loopInvariantSet, SmallVector < BasicBlock * , 8 > ExitBlocks ){
        for (Value * v: loopInvariantSet) {
            BasicBlock * Burrito = llvm::dyn_cast < Instruction > (v) -> getParent();
            bool dominaTutti = true;
            for (BasicBlock * b: ExitBlocks) {
                if (!DT.dominates(Burrito, b)) {
                    dominaTutti = false;
                }
            }

            if (!dominaTutti) {
                loopInvariantSet.erase(v);

            }

        }
    }


    void copyPHINodes(std::vector < PHINode * > &copiePhi, BasicBlock * loopHeader){


       
        for (llvm::Instruction & instr: * loopHeader) {
            if (auto * originalPhi = llvm::dyn_cast < llvm::PHINode > ( & instr)) {

                llvm::PHINode * newPhi = llvm::PHINode::Create(
                    originalPhi -> getType(),
                    originalPhi -> getNumIncomingValues(),
                    "copyphi"

                );

                // 2. Copia ogni incoming value e blocco
                for (unsigned i = 0; i < originalPhi -> getNumIncomingValues(); ++i) {
                    llvm::Value * val = originalPhi -> getIncomingValue(i);
                    llvm::BasicBlock * bb = originalPhi -> getIncomingBlock(i);
                    newPhi -> addIncoming(val, bb);
                }

                copiePhi.push_back(newPhi);
            }
        }
    }

    void swapBadPHINodes(BasicBlock* loopHeader, std::vector < PHINode * > &copiePhi){
        // Itera su tutte le istruzioni nel loop header
        for (auto it = loopHeader -> begin(); it != loopHeader -> end();) {
            Instruction & instr = * it;
            // Incrementa l'iteratore PRIMA di eventuali modifiche (per sicurezza)
            ++it;

            if (auto * phi = dyn_cast < PHINode > ( & instr)) {
                // Se ci sono ancora copie da processare
                if (!copiePhi.empty()) {
                    PHINode * newPhi = copiePhi.back(); // Prendi l'ultima copia
                    copiePhi.pop_back();

                    // Sposta newPhi prima del phi originale (mantenendo l'ordine)
                    newPhi -> insertBefore(phi);

                    // Sostituisci tutti gli usi del vecchio phi con la nuova copia
                    phi -> replaceAllUsesWith(newPhi);

                    // Elimina il phi originale (ora inutile)
                    phi -> eraseFromParent();
                }
            }
        }
    }


    void updateNewPHINodes(BasicBlock * loopHeader,BasicBlock * preHeader ,DominatorTree & DT ){
        // Aggiorna i PHI node in targetBB per rimpiazzare il predecessore
        for (llvm::Instruction & instr: * loopHeader) {
            if (auto * phi = llvm::dyn_cast < llvm::PHINode > ( & instr)) {
                
                for (unsigned i = 0; i < phi -> getNumIncomingValues(); ++i) {
                    
                    if (phi -> getIncomingBlock(i) != nullptr && llvm::isa < llvm::BasicBlock > (phi -> getIncomingBlock(i)) && phi -> getIncomingBlock(i) != preHeader) {
                       
                        if (llvm::BasicBlock * predBB = phi -> getIncomingBlock(i)) {
                            
                            for (llvm::BasicBlock * pred: llvm::predecessors(preHeader)) {
                                
                                if (pred == predBB && !DT.dominates(loopHeader, pred)) {
                                    
                                    int index = phi -> getBasicBlockIndex(pred);
                                    
                                    phi -> setIncomingBlock(index, preHeader);
                                }
                            }
                        }
                    }
                }
            } else {
                break; // Finito coi PHI node
            }
        }
    }

    void codeMotion(SmallPtrSet < Value * , 32 > loopInvariantSet, BasicBlock * preHeader){

        for(Value *lick : loopInvariantSet){
            Instruction *istrToMove = dyn_cast<Instruction>(lick);

            istrToMove->removeFromParent();

            istrToMove->insertBefore(preHeader->begin());

        }
    }

namespace {

    struct TestPass: PassInfoMixin < TestPass > {

        PreservedAnalyses run(Function & F, FunctionAnalysisManager & AM) {

            llvm::LLVMContext & Context = F.getContext();

            LoopInfo & LI = AM.getResult < LoopAnalysis > (F);

            SmallPtrSet < Value * , 32 > loopInvariantSet;

            if (LI.empty()) {

                return PreservedAnalyses::all();
            }

           

            DominatorTree & DT = AM.getResult < DominatorTreeAnalysis > (F);

            for (Loop * L: LI) {

                loopInvariantSet = createLoopInvariantSet(L);

                SmallVector < BasicBlock * , 8 > ExitBlocks;

                L -> getExitBlocks(ExitBlocks);

                calculateLoopInvariantDominance(DT,loopInvariantSet,ExitBlocks);
                
                BasicBlock * loopHeader = L -> getHeader();

                std::vector < PHINode * > copiePhi;


 		//mi copio i phi node perchè il comanda che fa lo split (riga 281) dei loopHeader cambia i phinode e quindi noi ce li salviamo per poi modificarli nel modo giusto
                copyPHINodes(copiePhi,loopHeader);

                llvm::Function * parentFunc = loopHeader -> getParent();
                llvm::LLVMContext & context = parentFunc -> getContext();
                // 1. Dividi il loopHeader prima della prima istruzione (creando un nuovo "preheader")
                llvm::Instruction * firstInst = & ( * loopHeader -> begin());
                llvm::BasicBlock * preHeader = loopHeader -> splitBasicBlockBefore(firstInst, "preheader");

                // 2. Dopo lo split, il backedge potrebbe puntare a newPreheader invece che a loopHeader.
                //    Correggiamo manualmente tutti i backedges per puntare a loopHeader.
                for (auto * pred: predecessors(preHeader)) {
                    if (L -> contains(pred)) { // Se è un blocco interno al loop (backedge)
                        llvm::BranchInst * term = dyn_cast < llvm::BranchInst > (pred -> getTerminator());
                        if (term) {
                            for (unsigned i = 0; i < term -> getNumSuccessors(); ++i) {
                                if (term -> getSuccessor(i) == preHeader) {
                                    term -> setSuccessor(i, loopHeader); // Reindirizza il backedge all'header originale
                                }
                            }
                        }
                    }
                }
                

                std::vector < PHINode * > toRemove;
                
                swapBadPHINodes(loopHeader,copiePhi);

                DT.recalculate(F);

                updateNewPHINodes(loopHeader, preHeader, DT);


                codeMotion(loopInvariantSet, preHeader);



            }

            stampaCFG( & F);


            return PreservedAnalyses::all();

        }
        static bool isRequired() {
            return true;
        }

    };
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getTestPassPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "TestPass",
        LLVM_VERSION_STRING,
        [](PassBuilder & PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager & FPM,
                    ArrayRef < PassBuilder::PipelineElement > ) {
                    if (Name == "test-pass") {
                        FPM.addPass(TestPass());
                        return true;
                    }
                    return false;
                });
        }
    };
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize TestPass when added to the pass pipeline on the
// command line, i.e. via '-passes=test-pass'
extern "C"
LLVM_ATTRIBUTE_WEAK::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getTestPassPluginInfo();
}
