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

    if ( decls->NumElements() > 0 ) { 
        for (int i = 0; i < decls->NumElements(); ++i){

            // cerr << " i = " <<i <<endl;
            Decl *d = decls->Nth(i);

            d->Emit();
        }
    }

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
    mod->dump();
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
    // cerr << "stmt block "<<endl;
    IRGenerator &irgen = IRGenerator::Instance();
    
    symtab->push(); //creates a new scope
    ScopedTable *currScope = symtab->currentScope();

    for (int i =0; i < decls->NumElements(); ++i) {
        decls->Nth(i)->Emit();
    }

    for (int i=0; i < stmts->NumElements(); ++i) {
       if(irgen.GetBasicBlock()->getTerminator()) break;//

       stmts->Nth(i)->Emit();     
    }
    /*
      For each element 'a' in formals:
      Create local variable (as in VarDecl::Emit)
      VarDecl emit should take it to AllocalInst in the else block
    */

    symtab->pop(); //delete current scope
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
	IRGenerator *irgen = &(IRGenerator::Instance());
		
	/*
	llvm::BasicBlock *fb = irgen-> CreateAndPushBlock("Footer Block");
	llvm::BasicBlock *sb = irgen ->CreateAndPushBlock("Step Block");
	llvm::BasicBlock *bb = irgen->CreateAndPushBlock("Body Block");
	llvm::BasicBlock *hb = irgen->CreateAndPushBlock("Header Block");
*/
	cout << "before blocks" <<endl;
	
	
	llvm::BasicBlock *fb = llvm::BasicBlock::Create(*irgen->GetContext(), "Footer block", irgen->GetFunction());
	llvm::BasicBlock *sb = llvm::BasicBlock::Create(*irgen->GetContext(), "Step block", irgen->GetFunction());
	llvm::BasicBlock *bb = llvm::BasicBlock::Create(*irgen->GetContext(), "Body block", irgen->GetFunction());
	llvm::BasicBlock *hb = llvm::BasicBlock::Create(*irgen->GetContext(), "Header block", irgen->GetFunction());
		
	//footer step, body, header
	
	//push?
	//irgen->breakBlockStack.push(sb);
	//irgen->continueBlockStack.push(fb);
	
	
	cout << "before emit\n" << endl;
	init->Emit();
	
	cout << "after emit init\n" << endl;
	
	llvm::BranchInst::Create(hb, irgen->GetBasicBlock());
	//head block is popped
	irgen->SetBasicBlock(hb);
	
	if (test){
		test->Emit();
		llvm::BranchInst::Create(fb, irgen->GetBasicBlock());
	}
	else{ // infinite loop
		llvm::BasicBlock *curPos = irgen->GetBasicBlock();
		llvm::BranchInst::Create(fb, curPos);
	}
	
	if (body){
		//pop body block
		if (step){
			irgen->breakBlockStack.push(sb);
		}
		else{
			irgen->continueBlockStack.push(hb);
		}
		
		//BCTarget target (fb, step ? sb : hb);
		//irgen->PushBCTarget(target);
		body->Emit();
		//pop
	}
	
	//check for terminator instruction
	llvm::BasicBlock *curPos = irgen->GetBasicBlock();
	if (curPos->getTerminator() == NULL){
		llvm::BranchInst::Create(sb, curPos);
	}
	
	
	//branch for jump - basicblock and branch
	// headblock is popbasic
	//setting to header
	
	// chekc if test not null
	   //conditional - get emit of test
	   // jump to foot bb
	 // else infinite loop
	    // get basic block
		// create branch inst
		
	// if body exist
	  // pop body block
	  // BCTarget target(footBB, step ? stepBB : headBB);
	  //irgen ->PushBCTarget(target)
	  //emit on body
	  //pop it
	  
	  //basicblock cur = getbasic
	  // if curpos->getterminator = null
	    // cerate branchinst from header to curpos
		
	
	
	
	
	/*
	IRGenerator *irgen = &(IRGenerator::Instance());
	
	if(step != NULL) {
		llvm::BasicBlock *hb = llvm::BasicBlock::Create(*irgen->GetContext(), "Header block", irgen->GetFunction());
		llvm::BasicBlock *sb = llvm::BasicBlock::Create(*irgen->GetContext(), "Step block", irgen->GetFunction());
		llvm::BasicBlock *fb = llvm::BasicBlock::Create(*irgen->GetContext(), "Footer block", irgen->GetFunction());
		llvm::BasicBlock *bb = llvm::BasicBlock::Create(*irgen->GetContext(), "Body block", irgen->GetFunction());

		irgen->breakBlockStack.push(sb);
		irgen->continueBlockStack.push(fb);

		init->Emit();
		llvm::BranchInst::Create(hb, irgen->GetBasicBlock());

		irgen->SetBasicBlock(hb);
		
		test->Emit();
		
		llvm::BranchInst::Create(bb, sb, test->getEmit(), hb);

		irgen->SetBasicBlock(bb);
		body->Emit();
		llvm::BranchInst::Create(fb, irgen->GetBasicBlock());
		irgen->SetBasicBlock(fb);
		
		
		step->Emit();
		llvm::BranchInst::Create(hb, fb);

		irgen->breakBlockStack.pop();
		irgen->continueBlockStack.pop();
		
		irgen->SetBasicBlock(sb);

	}
	else if(step == NULL) {
		llvm::BasicBlock *hb = llvm::BasicBlock::Create(*irgen->GetContext(), "Header block", irgen->GetFunction());
		llvm::BasicBlock *bb = llvm::BasicBlock::Create(*irgen->GetContext(), "Body block", irgen->GetFunction());
		llvm::BasicBlock *sb = llvm::BasicBlock::Create(*irgen->GetContext(), "Footer block", irgen->GetFunction());


		irgen->breakBlockStack.push(sb);
		irgen->continueBlockStack.push(hb);

		init->Emit();
		llvm::BranchInst::Create(hb, irgen->GetBasicBlock());

		irgen->SetBasicBlock(hb);
		
		test->Emit();
		
		llvm::BranchInst::Create(bb, sb, test->getEmit(), hb);

		irgen->SetBasicBlock(bb);
		body->Emit();
		llvm::BranchInst::Create(hb, irgen->GetBasicBlock());


		irgen->breakBlockStack.pop();
		irgen->continueBlockStack.pop();

		irgen->SetBasicBlock(sb);
	}
	*/
}


void WhileStmt::PrintChildren(int indentLevel) {
    test->Print(indentLevel+1, "(test) ");
    body->Print(indentLevel+1, "(body) ");
}

void WhileStmt::Emit(){

}

void BreakStmt::Emit() {
    IRGenerator *irgen = &(IRGenerator::Instance());
    llvm::BranchInst::Create(irgen->breakBlockStack.top(), irgen->GetBasicBlock());
}

void ContinueStmt::Emit() {
    IRGenerator *irgen = &(IRGenerator::Instance());
    llvm::BranchInst::Create(irgen->continueBlockStack.top(), irgen->GetBasicBlock());
}

IfStmt::IfStmt(Expr *t, Stmt *tb, Stmt *eb): ConditionalStmt(t, tb) { 
    Assert(t != NULL && tb != NULL); // else can be NULL
    elseBody = eb;
    if (elseBody) elseBody->SetParent(this);
}

void IfStmt::Emit(){

    cerr << "inside if emit "<<endl;

    IRGenerator &irgen = IRGenerator::Instance();

    //create basicblocks for if and else 
    llvm::BasicBlock *block_of_then = irgen.CreateEmptyBlock("ThenBB");
    llvm::BasicBlock *block_of_else = NULL;
    llvm::BasicBlock *endBlock = irgen.CreateEmptyBlock("footerBB");

    if (elseBody) {
         cerr << "ifstmtEmit:: elsebody exists" <<endl;
        block_of_else = irgen.CreateEmptyBlock("ElseBB");
        (void) llvm::BranchInst::Create(block_of_then, block_of_else, test->getEmit(), irgen.GetBasicBlock());
    }
    else {
        cerr << "ifstmtEmit:: no else body" <<endl;
        (void) llvm::BranchInst::Create(block_of_then, endBlock, test->getEmit(), irgen.GetBasicBlock());
    }

    irgen.SetBasicBlock(block_of_then);

    body->Emit();

    if (!irgen.GetBasicBlock()->getTerminator())
        (void) llvm::BranchInst::Create(endBlock, irgen.GetBasicBlock());

    if (block_of_else) {
        block_of_else->moveAfter(irgen.GetBasicBlock());
        irgen.SetBasicBlock(block_of_else);  
        elseBody->Emit();

        if (!irgen.GetBasicBlock()->getTerminator())
            (void) llvm::BranchInst::Create(endBlock, irgen.GetBasicBlock());
    }

    endBlock->moveAfter(irgen.GetBasicBlock());
    irgen.SetBasicBlock(endBlock);
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
    IRGenerator *irgen = &(IRGenerator::Instance());
    if (expr == NULL)
        llvm::ReturnInst::Create(*irgen->GetContext(), irgen->GetBasicBlock());
    else {
        llvm::ReturnInst::Create(*irgen->GetContext(), expr->getEmit(), irgen->GetBasicBlock());
    }

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
