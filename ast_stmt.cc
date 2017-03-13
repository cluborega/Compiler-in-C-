/* File: ast_stmt.cc
 * -----------------
 * Implementation of statement node classes.
 */
#include "ast_stmt.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "ast_expr.h"
#include "symtable.h"

#include "irgen.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h"                                                   


Program::Program(List<Decl*> *d) {
    Assert(d != NULL);
    (decls=d)->SetParentAll(this);
}

void Program::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    printf("\n");
}

void Program::Emit() {
    // TODO:
    // This is just a reference for you to get started
    //
    // You can use this as a template and create Emit() function
    // for individual node to fill in the module structure and instructions.
    //
   
   /*
    llvm::Module *mod = irgen.GetOrCreateModule("foo.bc");

    for (int i = 0; i < decls->NumElements(); ++i)
       decls->Nth(i)->Emit();
   */

    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Module *mod = irgen.GetOrCreateModule("foo.bc");

    cerr << "symtab.size()" << symtab->tables.size() <<endl;

    if ( decls->NumElements() > 0 ) { 
        for (int i = 0; i < decls->NumElements(); ++i){

            cerr << " i = " <<i <<endl;
            Decl *d = decls->Nth(i);

            d->Emit();
        }
    }

    mod->dump();
    llvm::WriteBitcodeToFile(mod, llvm::outs());

    // cerr << "in program emit "<<endl;

   /* // create a function signature
    std::vector<llvm::Type *> argTypes;
    llvm::Type *intTy = irgen.GetIntType();
    argTypes.push_back(intTy);
    llvm::ArrayRef<llvm::Type *> argArray(argTypes);
    llvm::FunctionType *funcTy = llvm::FunctionType::get(intTy, argArray, false);

    // llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction("foo", intTy, intTy, (Type *)0));
    llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction("Name_the_function", funcTy));
    llvm::Argument *arg = f->arg_begin();
    arg->setName("x");

    // insert a block into the runction
    llvm::LLVMContext *context = irgen.GetContext();
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*context, "entry", f);

    // create a return instruction
    llvm::Value *val = llvm::ConstantInt::get(intTy, 1);
    llvm::Value *sum = llvm::BinaryOperator::CreateAdd(arg, val, "", bb);
    llvm::ReturnInst::Create(*context, sum, bb);

    // write the BC into standard output
    llvm::WriteBitcodeToFile(mod, llvm::outs()); */


    //uncomment the next line to generate the human readable/assembly file
    // mod->dump();
}

StmtBlock::StmtBlock(List<VarDecl*> *d, List<Stmt*> *s) {
    Assert(d != NULL && s != NULL);
    (decls=d)->SetParentAll(this);
    (stmts=s)->SetParentAll(this);
}

void StmtBlock::PrintChildren(int indentLevel) {
    decls->PrintAll(indentLevel+1);
    stmts->PrintAll(indentLevel+1);
}

bool StmtBlock::hasReturn() {
    if (stmts->NumElements() > 0) {
        int num_elems = stmts->NumElements();
        for (int i = 0; i < num_elems; ++i) {
            ReturnStmt *rs = dynamic_cast<ReturnStmt*>(stmts->Nth(i));
            if (rs == NULL) {
                StmtBlock *sb = dynamic_cast<StmtBlock*>(stmts->Nth(i));
                if (sb != NULL)
                    return sb->hasReturn();
            }
            else
                return true;
        }
    }
    return false;

}

void StmtBlock::Emit() {
    cerr << "stmt block "<<endl;
    IRGenerator &irgen = IRGenerator::Instance();
    
    symtab->push(); //creates a new scope
    ScopedTable *currScope = symtab->currentScope();
    /*
      For each element 'a' in formals:
      Create local variable (as in VarDecl::Emit)
      VarDecl emit should take it to AllocalInst in the else block
    */
    for (int i =0; i < decls->NumElements(); ++i) {
        cerr <<" name of var "<<decls->Nth(i)->GetType()<<endl;
        decls->Nth(i)->Emit();

    }

    for (int i=0; i < stmts->NumElements(); ++i) {
        /* Returns a pointer to the terminator instruction that appears 
          at the end of the BasicBlock. If there is no terminator instruction, 
          or if the last instruction in the block is not a terminator, 
          then a null pointer is returned.*/
       if(irgen.GetBasicBlock()->getTerminator()) break;//

       stmts->Nth(i)->Emit();
        

    }

    // if(hasReturn()){
        // symtab->setReturn(true);
        //TODO
        //what to do when there is return?? or no return??

    // }
    // else
        // symtab->setReturn(false);

    // symtab->pop(); //delete current scope
}

DeclStmt::DeclStmt(Decl *d) {
    Assert(d != NULL);
    (decl=d)->SetParent(this);
}

void DeclStmt::PrintChildren(int indentLevel) {
    decl->Print(indentLevel+1);
}

void DeclStmt::Emit(){
    VarDecl *vd = dynamic_cast<VarDecl*>(this->decl); //this one is similar to p3 check... make sure with tutor
    vd->Emit();
}

ConditionalStmt::ConditionalStmt(Expr *t, Stmt *b) { 
    Assert(t != NULL && b != NULL);
    (test=t)->SetParent(this); 
    (body=b)->SetParent(this);
}

ForStmt::ForStmt(Expr *i, Expr *t, Expr *s, Stmt *b): LoopStmt(t, b) { 
    Assert(i != NULL && t != NULL && b != NULL);
    (init=i)->SetParent(this);
    step = s;
    if ( s )
      (step=s)->SetParent(this);
}

void ForStmt::PrintChildren(int indentLevel) {
    init->Print(indentLevel+1, "(init) ");
    test->Print(indentLevel+1, "(test) ");
    if ( step )
      step->Print(indentLevel+1, "(step) ");
    body->Print(indentLevel+1, "(body) ");
}

void ForStmt::Emit(){

}


void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::Emit(){

}

void BreakStmt::Emit() {

}

void ContinueStmt::Emit() {
    
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Emit(){

}

void IfStmt::PrintChildren(int indentLevel) {
    if (test) test->Print(indentLevel+1, "(test) ");
    if (body) body->Print(indentLevel+1, "(then) ");
    if (elseBody) elseBody->Print(indentLevel+1, "(else) ");
}


ReturnStmt::ReturnStmt(yyltype loc, Expr *e) : Stmt(loc) { 
    expr = e;
    if (e != NULL) expr->SetParent(this);
}

void ReturnStmt::PrintChildren(int indentLevel) {
    if ( expr ) 
      expr->Print(indentLevel+1);
}

void ReturnStmt::Emit(){

}

SwitchLabel::SwitchLabel(Expr *l, Stmt *s) {
    Assert(l != NULL && s != NULL);
    (label=l)->SetParent(this);
    (stmt=s)->SetParent(this);
}

SwitchLabel::SwitchLabel(Stmt *s) {
    Assert(s != NULL);
    label = NULL;
    (stmt=s)->SetParent(this);
}

void SwitchLabel::PrintChildren(int indentLevel) {
    if (label) label->Print(indentLevel+1);
    if (stmt)  stmt->Print(indentLevel+1);
}

SwitchStmt::SwitchStmt(Expr *e, List<Stmt *> *c, Default *d) {
    Assert(e != NULL && c != NULL && c->NumElements() != 0 );
    (expr=e)->SetParent(this);
    (cases=c)->SetParentAll(this);
    def = d;
    if (def) def->SetParent(this);
}

void SwitchStmt::PrintChildren(int indentLevel) {
    if (expr) expr->Print(indentLevel+1);
    if (cases) cases->PrintAll(indentLevel+1);
    if (def) def->Print(indentLevel+1);
}

void SwitchStmt::Emit(){

}

