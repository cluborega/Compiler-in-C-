/* File: ast_decl.cc
 * -----------------
 * Implementation of Decl node classes.
 */
#include "ast_decl.h"
#include "ast_type.h"
#include "ast_stmt.h"
#include "symtable.h"        

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
	// llvm::Value* val = NULL;
	// llvm::Value* init = NULL;
    Symbol sym;
    char* name = this->GetIdentifier()->GetName();

    // auto *thistype = this->GetType();
    // llvm::Type* llType = NULL;

    // Symbol* symbol_f = symtab->tables[index]->find(name);
    // symbol_f.name;

    llvm::Module *mod = irgen->GetOrCreateModule("foo.bc");
    llvm::Type *type = irgen->get_ll_type(this->GetType());


    llvm::Twine *twine = new llvm::Twine(name);	
    llvm::BasicBlock *bb = irgen->GetBasicBlock();

/*

  GlobalVariable(Module &M, 
                 Type *Ty, bool isConstant, 
                 LinkageTypes Linkage, Constant *Initializer,
                 const Twine &Name = "", 

                 GlobalVariable *InsertBefore = nullptr,
                 ThreadLocalMode = NotThreadLocal, unsigned AddressSpace = 0,
                 bool isExternallyInitialized = false);

*/

	if (symtab->isGlobalScope()) {
        llvm::GlobalVariable *variable = new 
llvm::GlobalVariable(*mod, type, false, llvm::GlobalValue::ExternalLinkage, llvm::Constant::getNullValue(type), name);

        sym.name = name;
        sym.value = variable;
        symtab->insert(sym);
    }
    else {  //if not global
        llvm::AllocaInst *allocInst = new llvm::AllocaInst(type, *twine, bb);
        sym.value = allocInst;
        symtab->insert(sym);
    }
	//return val;
}

void FnDecl::Emit() {
	llvm::Value* val = NULL;
	llvm::Value* init = NULL;
	
	//return val;
}
