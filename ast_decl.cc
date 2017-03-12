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

    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Module *mod = irgen.GetOrCreateModule(NULL);

    Symbol* sym = new Symbol(this->GetIdentifier()->GetName(), this, E_VarDecl, NULL);

    // Symbol* symbol_f = symtab->tables[index]->find(name);
    // symbol_f.name;

    // llvm::Module *mod = irgen.GetOrCreateModule("foo.bc");
    llvm::Type *type = irgen.get_ll_type(this->GetType());


    llvm::Twine *twine = new llvm::Twine(sym->name);	
    llvm::BasicBlock *bb = irgen.GetBasicBlock();

	if (symtab->isGlobalScope()) {
        llvm::GlobalVariable *variable = new llvm::GlobalVariable(*mod, type, false, llvm::GlobalValue::ExternalLinkage, llvm::Constant::getNullValue(type), sym->name);

        sym->value = variable;
        symtab->insert(*sym);
    }
    else {  //if not global
        llvm::AllocaInst *allocInst = new llvm::AllocaInst(type, *twine, bb);
        sym->value = allocInst;
        symtab->insert(*sym);
    }
}

void FnDecl::Emit() {

  IRGenerator &irgen = IRGenerator::Instance();
  llvm::Module *mod = irgen.GetOrCreateModule(NULL);
    // create a function signature
    std::vector<llvm::Type *> argTypes;
    // llvm::Type *intTy = irgen.GetIntType();
    Type* argType;
    for (int i = 0; i < formals->NumElements(); ++i) {
        argType = formals->Nth(i)->GetType();

        argTypes.push_back(irgen.get_ll_type(argType));
    }
    // llvm::Type *declType = irgen.get_ll_type(this->GetType());
    // argTypes.push_back(declType);

    llvm::ArrayRef<llvm::Type *> argArray(argTypes);
    // cerr<<"function return type is " << this->GetType() <<endl;
    llvm::FunctionType *funcTy = llvm::FunctionType::get(irgen.get_ll_type(this->GetType()), argArray, false);\
     
    symtab->funcFlag = true;
    Symbol* sym = new Symbol(this->GetIdentifier()->GetName(), this, E_FunctionDecl, NULL);
    // llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction("foo", intTy, intTy, (Type *)0));
    llvm::Function *f = llvm::cast<llvm::Function>(mod->getOrInsertFunction(sym->name, funcTy));
    irgen.SetFunction(f);
    sym->value = f;
    symtab->insert(*sym);


    // llvm::Argument *arg = f->arg_begin();
    // arg->setName("x");

  
    // insert a block into the runction
    llvm::LLVMContext *context = irgen.GetContext();
    llvm::BasicBlock *bb = llvm::BasicBlock::Create(*context, "entry", f);
    irgen.SetBasicBlock(bb);

    //push a new scope
    symtab->push();

    llvm::Argument *arg = f->arg_begin(); //iterator 
    // add formal parameters into scope
    List<VarDecl*> *formals = this->GetFormals();

    for (int i = 0; arg != f->arg_end() && i < formals->NumElements(); ++i, ++arg) {
        VarDecl* parameter = formals->Nth(i);
        char* name = parameter->GetIdentifier()->GetName();
        // cout << "parameter name is " << name << endl;
        // cerr << "parameter type is " << parameter->GetType() <<endl;;
        arg->setName(name);
        // llvm::BasicBlock *bb1 = irgen->GetBasicBlock();
        parameter->Emit();

        /*  varDecl should assign value and everything by now
         *  So just search for the symbol to get the scope and store the instance ??
         */

         int index = symtab->tables.size() - 1; // get the current scope
         Symbol* symbol_f = symtab->tables[index]->find(name); //find the symbol
         (void) new llvm::StoreInst(arg, symbol_f->value, "", irgen.GetBasicBlock());
     }

     // body->Emit();

    // if(this->body != NULL){
    //     StmtBlock *block = dynamic_cast<StmtBlock*>(this->body);

    //     if(block != NULL){
    //         block->Emit();
    //     }
    //     else{
    //         this->body->Emit();
    //     }

     if(bb->getTerminator() == NULL){
        llvm::ReturnInst::Create(*context,bb);
    }

    symtab->pop();

    // body->Emit();
/**
    // create a return instruction
    llvm::Value *val = llvm::ConstantInt::get(intTy, 1);
    llvm::Value *sum = llvm::BinaryOperator::CreateAdd(arg, val, "", bb);
    llvm::ReturnInst::Create(*context, sum, bb);

**/

    // write the BC into standard output
     llvm::WriteBitcodeToFile(mod, llvm::outs());
/*

• Constant* getOrInsertFunction (StringRef Name, FunctionType *T)
• Module->getOrInsertFunction()
• Specifying FunctionType:
• FunctionType*get(Type*Result,ArrayRef<Type*>Params,boolisVarArg) • NeedtocreateParamsfirst.Gothroughtheformalslistandcreateit.
• Set names for arguments/formals
• http://llvm.org/docs/doxygen/html/classllvm_1_1Argument.html
• Create BasicBlock(s) ?
• http://llvm.org/docs/doxygen/html/classllvm_1_1BasicBlock.html
• BasicBlock*Create(LLVMContext&Context,constTwine&Name="",Function
  *Parent=nullptr, BasicBlock *InsertBefore=nullptr)


*/
    // llvm::Module *mod = irgen->GetOrCreateModule("foo.bc");
    // llvm::Type *type = irgen->get_ll_type(this->GetType());

    // std::vector<llvm::Type *> argTypes;
    // Type *argType;

    //for (int i = 0; i < formals->NumElements(); ++i) {
     //   argType = formals->Nth(i)->GetType();
     //   argTypes.push_back(irgen->get_ll_type(argType));
    //}


	// llvm::Value* val = NULL;
	// llvm::Value* init = NULL;

    //Need to create Params first
    // List<VarDecl*> params = formals;
     // for (int i = 0; )
	
	//return val;
}
