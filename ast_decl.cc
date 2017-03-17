/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "symtable.h"        
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/raw_ostream.h" 
#include "irgen.h"    
         
Decl::Decl(Identifier *n) : Node(*n->GetLocation()) {
    Assert(n != NULL);
    (id=n)->SetParent(this); 
}

VarDecl::VarDecl(Identifier *n, Type *t, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL);
    (type=t)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    typeq = NULL;
}

VarDecl::VarDecl(Identifier *n, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && tq != NULL);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
    type = NULL;
}

VarDecl::VarDecl(Identifier *n, Type *t, TypeQualifier *tq, Expr *e) : Decl(n) {
    Assert(n != NULL && t != NULL && tq != NULL);
    (type=t)->SetParent(this);
    (typeq=tq)->SetParent(this);
    if (e) (assignTo=e)->SetParent(this);
}
  
void VarDecl::PrintChildren(int indentLevel) { 
   if (typeq) typeq->Print(indentLevel+1);
   if (type) type->Print(indentLevel+1);
   if (id) id->Print(indentLevel+1);
   if (assignTo) assignTo->Print(indentLevel+1, "(initializer) ");
}

FnDecl::FnDecl(Identifier *n, Type *r, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r!= NULL && d != NULL);
    (returnType=r)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
    returnTypeq = NULL;
}

FnDecl::FnDecl(Identifier *n, Type *r, TypeQualifier *rq, List<VarDecl*> *d) : Decl(n) {
    Assert(n != NULL && r != NULL && rq != NULL&& d != NULL);
    (returnType=r)->SetParent(this);
    (returnTypeq=rq)->SetParent(this);
    (formals=d)->SetParentAll(this);
    body = NULL;
}

void FnDecl::SetFunctionBody(Stmt *b) { 
    (body=b)->SetParent(this);
}

void FnDecl::PrintChildren(int indentLevel) {
    if (returnType) returnType->Print(indentLevel+1, "(return type) ");
    if (id) id->Print(indentLevel+1);
    if (formals) formals->PrintAll(indentLevel+1, "(formals) ");
    if (body) body->Print(indentLevel+1, "(body) ");
}


void VarDecl::Emit(){

    // cerr << "entering vardecl "<<endl;

    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Module *mod = irgen.GetOrCreateModule("foo.bc");

    llvm::BasicBlock *bb = irgen.GetBasicBlock();

    llvm::Type *type = irgen.get_ll_type(this->GetType());

    char* name = this->GetIdentifier()->GetName();
    llvm::Twine *twine = new llvm::Twine(name);

    llvm::Function *fun = irgen.GetFunction();

    if(!fun){
        cerr << "inside vardecl creating global var" <<endl;
        llvm::GlobalVariable *llvm_Gl_var = llvm::cast<llvm::GlobalVariable>(mod->getOrInsertGlobal(name, type));
        llvm_Gl_var->setConstant(false);

        Symbol sym(id->GetName(), this, E_VarDecl, llvm_Gl_var);
        symtab->insert(sym);
    }
    else{
        cerr << "inside vardecl creating local var" <<endl;
        llvm::AllocaInst *local_var = new llvm::AllocaInst(type, *twine, fun->begin());
        Symbol sym(name, this, E_VarDecl, local_var);
        symtab->insert(sym);
    }
}

void FnDecl::Emit() {

    cerr << "entering funcdecl....." <<endl;
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Module *mod = irgen.GetOrCreateModule(NULL);

    std::vector<llvm::Type *> argTypes;
    Type* argType;
    for (int i = 0; i < formals->NumElements(); ++i) {
        argType = formals->Nth(i)->GetType();

        argTypes.push_back(irgen.get_ll_type(argType));
    }

    llvm::ArrayRef<llvm::Type *> argArray(argTypes);
    llvm::Type *retType = irgen.get_ll_type(returnType);
    // cerr<<"fndecl::function return type is " << returnType <<endl;
    llvm::FunctionType *funcTy = llvm::FunctionType::get(retType, argArray, false);
     
    // llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction("foo", intTy, intTy, (Type *)0));
    llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction(id->GetName(), funcTy));
    irgen.SetFunction(f);
    Symbol* sym = new Symbol(this->GetIdentifier()->GetName(), this, E_FunctionDecl, f);
    symtab->insert(*sym);
  
    // insert a block into the runction
    llvm::LLVMContext *context = irgen.GetContext();
    llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(*context, "entry", f);
    irgen.SetBasicBlock(entry_bb);

    //push a new scope
    // symtab->push();

    llvm::Argument *arg = f->arg_begin(); //iterator 
    // add formal parameters into scope
    List<VarDecl*> *formals = this->GetFormals();

    for (int i = 0; arg != f->arg_end() && i < formals->NumElements(); ++i, ++arg) {
        VarDecl* parameter = formals->Nth(i);
        char* name = parameter->GetIdentifier()->GetName();
        arg->setName(name);
        parameter->Emit();
     }

    llvm::BasicBlock *next_bb = llvm::BasicBlock::Create(*(irgen.GetContext()), "next", f);
    irgen.SetBasicBlock(next_bb);
    
    llvm::BasicBlock::iterator it = entry_bb->begin();
    arg = f->arg_begin();
    for(int i = 0; i < formals->NumElements(); i++, it++, arg++){
        (void) new llvm::StoreInst(arg, it, next_bb);
    }

     body->Emit();

     llvm::BranchInst *branch = llvm::BranchInst::Create(next_bb, entry_bb);
 }
