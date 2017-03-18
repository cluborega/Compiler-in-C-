/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}
void IntConstant::PrintChildren(int indentLevel) { 
    printf("%d", value);
}

llvm::Value* IntConstant::getEmit() {
  return llvm::ConstantInt::get(IRGenerator::Instance().GetIntType(), this->value, true);
}

FloatConstant::FloatConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}
void FloatConstant::PrintChildren(int indentLevel) { 
    printf("%g", value);
}

llvm::Value* FloatConstant::getEmit() {
    return llvm::ConstantFP::get(IRGenerator::Instance().GetFloatType(), this->value);
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}
void BoolConstant::PrintChildren(int indentLevel) { 
    printf("%s", value ? "true" : "false");
}

llvm::Value* BoolConstant::getEmit() {
  return llvm::ConstantInt::get(IRGenerator::Instance().GetBoolType(), this->value, true);
}

VarExpr::VarExpr(yyltype loc, Identifier *ident) : Expr(loc) {
    Assert(ident != NULL);
    this->id = ident;
}

//only returns loaded value
llvm::Value* VarExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    Symbol *s = symtab->find(id->GetName()); //find the var in symtable

    llvm::Value* llvm_value = NULL;
    
    if(s){
        llvm_value = s->value; //get its value
        VarDecl* vd = dynamic_cast<VarDecl*>(s->decl);
        if (vd) this->type = vd->GetType();
    }


    llvm::LoadInst* var_expr = new llvm::LoadInst(llvm_value, this->GetIdentifier()->GetName(), irgen.GetBasicBlock());
    return var_expr;
}

void VarExpr::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
}

Operator::Operator(const char *tok) {
    strncpy(tokenString, tok, sizeof(tokenString));
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}

void Operator::PrintChildren(int indentLevel) {
    printf("%s",tokenString);
}

bool Operator::IsOp(const char *op) const {
    return strcmp(tokenString, op) == 0;
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Expr *l, Operator *o) 
  : Expr(Join(l->GetLocation(), o->GetLocation())) {
    Assert(l != NULL && o != NULL);
    (left=l)->SetParent(this);
    (op=o)->SetParent(this);
}

void CompoundExpr::PrintChildren(int indentLevel) {
   if (left) left->Print(indentLevel+1);
   op->Print(indentLevel+1);
   if (right) right->Print(indentLevel+1);
}

llvm::Value* EqualityExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::BasicBlock *bb = irgen.GetBasicBlock();

    llvm::Value *leftValue = left->getEmit();
    llvm::Value *rightValue = right->getEmit();

    bool check;

    if ( leftValue->getType()->isFloatTy() || leftValue->getType()->isVectorTy() ) {
        check = true;
    }
    else
        check = false;

    llvm::CmpInst::OtherOps llvmOP;

    if (check) {
        llvmOP = llvm::FCmpInst::FCmp;
    }
    else {
        llvmOP = llvm::ICmpInst::ICmp;
    }
    llvm::CmpInst::Predicate pred;

    if (op->IsOp("==")){
        if (check) pred = llvm::CmpInst::FCMP_OEQ; 
        else pred = llvm::CmpInst::ICMP_EQ;
    }
    else {
        if (check) pred = llvm::CmpInst::FCMP_ONE; 
        else pred = llvm::CmpInst::ICMP_NE;
    }

    llvm::Value* result = llvm::CmpInst::Create(llvmOP, pred, leftValue, rightValue, "", bb);
    if (result->getType()->isVectorTy())
        result = irgen.BoolVectorToBool(result);

    return result;
}

llvm::Value* ArithmeticExpr::getEmit() {

    IRGenerator &irgen = IRGenerator::Instance();
    llvm::BasicBlock *bb = irgen.GetBasicBlock();

    Symbol* sym = NULL;

    llvm::BinaryOperator::BinaryOps llvmOP;

    bool is_float_op;

    if (!left) { //Unary expr
        llvm::Value *varVal = right->getEmit();
        llvm::Value *newVal = NULL;
        llvm::Value *value_to_assign = NULL;

        if (op->IsOp("+")){
            newVal = varVal;
        }
        else if (op->IsOp("-")){
            bool is_float_op = varVal->getType()->isFloatTy() || varVal->getType()->isVectorTy();
            llvmOP = is_float_op ? llvm::BinaryOperator::FSub : llvm::BinaryOperator::Sub;

            llvm::Value *scalarVal;
            scalarVal = is_float_op ? llvm::ConstantFP::get(IRGenerator::Instance().GetFloatType(), -1) : llvm::ConstantInt::get(IRGenerator::Instance().GetIntType(), -1, true);
            
            if (varVal->getType()->isVectorTy())
                scalarVal = irgen.checkLLVMvec(scalarVal, varVal->getType()->getVectorNumElements());

            newVal = llvm::BinaryOperator::Create(llvmOP, varVal, scalarVal, "", irgen.GetBasicBlock());
        }
        else if (op->IsOp("++"))
             newVal = irgen.PostFixIncrementInst(varVal);
        else if (op->IsOp("--"))
             newVal = irgen.PostFixDecrementInst(varVal);

        //  VarExpr *var = dynamic_cast<VarExpr*>(right);
          FieldAccess *swizzle = dynamic_cast<FieldAccess*>(right);
                  // value_to_assign = newVal->getEmit();
        VarExpr *is_var = dynamic_cast<VarExpr*>(right);

        llvm::Value* tempVal = newVal;

        if (swizzle){
            newVal = swizzle->getEmit();
        }
        else if (is_var) {
            // int index = symtab->tables.size() - 1; 
            sym = symtab->find(is_var->GetIdentifier()->GetName());
            llvm::Type* varType = sym->value->getType();

            if (varType->isVectorTy()){
                tempVal = irgen.checkLLVMvec(newVal, varType->getVectorNumElements());
                newVal = new llvm::StoreInst(tempVal, sym->value, true, irgen.GetBasicBlock());
            }
            (void)new llvm::StoreInst(newVal, sym->value, true, irgen.GetBasicBlock());

        }

        // // else if (var)
        return tempVal;
    }

    //for regular expressions 
    llvm::Value *leftValue = left->getEmit();
    llvm::Value *rightValue = right->getEmit();

    if (leftValue->getType()->isVectorTy())
        rightValue = irgen.checkLLVMvec(rightValue, leftValue->getType()->getVectorNumElements());

    if (rightValue->getType()->isVectorTy())
        leftValue = irgen.checkLLVMvec(leftValue, rightValue->getType()->getVectorNumElements());

     is_float_op = leftValue->getType()->isFloatTy();


    if (op->IsOp("+")) {
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::FAdd;
        else
            llvmOP = llvm::BinaryOperator::Add;
    }
    else if (op->IsOp("-")) {
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::FSub;
        else
            llvmOP = llvm::BinaryOperator::Sub;
    }
    else if (op->IsOp("*")) {
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::FMul;
        else
            llvmOP = llvm::BinaryOperator::Mul;
    }
    else{
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::FDiv;
        else
            llvmOP = llvm::BinaryOperator::SDiv;
    }

    return llvm::BinaryOperator::Create(llvmOP, leftValue, rightValue, "", bb);
}

llvm::CmpInst::Predicate determinePredicate (bool b, Operator* op){
    if (op->IsOp("<")){
        if (b)
            return llvm::CmpInst::FCMP_OLT; 
        else 
            return llvm::CmpInst::ICMP_SLT;
    }
    else if (op->IsOp(">")){
        if (b) return llvm::CmpInst::FCMP_OGT;
        else return llvm::CmpInst::ICMP_SGT;
    }
    else if (op->IsOp("<=")){
        if (b) return llvm::CmpInst::FCMP_OLE;
        else return llvm::CmpInst::ICMP_SLE;
    }
    if (b) return llvm::CmpInst::FCMP_OGE; 

    return llvm::CmpInst::ICMP_SGE;

}
llvm::Value* RelationalExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::BasicBlock *bb = irgen.GetBasicBlock();

    llvm::Value *leftValue = left->getEmit();
    llvm::Value *rightValue = right->getEmit();

    bool is_float_cmp = leftValue->getType()->isFloatTy();
    llvm::CmpInst::OtherOps llvmOP; 
    llvm::CmpInst::Predicate pred;

    if (is_float_cmp)
        llvmOP = llvm::FCmpInst::FCmp;
    else
        llvmOP = llvm::ICmpInst::ICmp;

    pred = determinePredicate(is_float_cmp, op);

    // if (op->IsOp("<")){
    //     pred = is_float_cmp ? llvm::CmpInst::FCMP_OLT : llvm::CmpInst::ICMP_SLT;
    // }
    // else if (opStr == ">"){
    //     pred = is_float_cmp ? llvm::CmpInst::FCMP_OGT : llvm::CmpInst::ICMP_SGT;
    // }
    // else if (opStr == "<="){
    //     pred = is_float_cmp ? llvm::CmpInst::FCMP_OLE : llvm::CmpInst::ICMP_SLE;
    // }
    // else{
    //     pred = is_float_cmp ? llvm::CmpInst::FCMP_OGE : llvm::CmpInst::ICMP_SGE;
    // }

    return llvm::CmpInst::Create(llvmOP, pred, leftValue, rightValue, "", bb);
}

llvm::Value* LogicalExpr::getEmit() {
  IRGenerator &irgen = IRGenerator::Instance();
  llvm::BasicBlock *bb = irgen.GetBasicBlock();

  llvm::Value *lhs = left->getEmit();
  llvm::Value *rhs = right->getEmit();

  // std::string operator_ = op->to_string();
  if (op->IsOp("&&"))
    return llvm::BinaryOperator::CreateAnd(lhs, rhs, "", bb);
  else
    return llvm::BinaryOperator::CreateOr(lhs, rhs, "", bb);
}

llvm::Value* AssignExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Value *value_to_assign = NULL;
    Symbol* sym = NULL;

     // llvm::Value* lhs = left->getEmit();
     // llvm::Value* rhs = right->getEmit();


    if (op->IsOp("=")) {
        value_to_assign = right->getEmit();
        
        // if (sym)
            // value_to_assign = new llvm::StoreInst(value_to_assign, sym->value, false, irgen.GetBasicBlock());
    }
    else {
        Operator *temp_Op = NULL;
        // const char token[];

        if (op->IsOp("+=")) //handle cases for +=, -= etc , one idea is just extract the first "+, -" ..
            temp_Op = new Operator("+");
        else if (op->IsOp("-="))
            temp_Op = new Operator("-");
        else if (op->IsOp("*="))
            temp_Op = new Operator("*");
        else if (op->IsOp("/="))
            temp_Op = new Operator("/");


        ArithmeticExpr *arith_expr = new ArithmeticExpr(left, temp_Op, right);
        value_to_assign = arith_expr->getEmit();
        // VarExpr *is_var = dynamic_cast<VarExpr*>(left);
        // if (is_var) {
        //     int index = symtab->tables.size() - 1; 
        //     Symbol* sym = symtab->find(is_var->GetIdentifier()->GetName());

            // if (sym)
                // value_to_assign = new llvm::StoreInst(value_to_assign, sym->value, false, irgen.GetBasicBlock());
    }


    VarExpr *is_var = dynamic_cast<VarExpr*>(left);
    FieldAccess *is_swizzle = dynamic_cast<FieldAccess*>(left);
    
    if (is_swizzle) {
        // llvm::Value* swiz_value = is_swizzle->GetBase()->getEmit();

        // std::string swizzle = std::string(is_swizzle->GetIdentifier()->GetName());

        // if (!value_to_assign->getType()->isVectorTy())
        //     value_to_assign = irgen.checkLLVMvec(value_to_assign, swizzle.length());

        // for (int i = 0; i < value_to_assign->getType()->getVectorNumElements(); ++i) {
        //     int replaceAt;
        //     switch (swizzle.at(i)) {
        //         case 'x': replaceAt = 0; break;
        //         case 'y': replaceAt = 1; break;
        //         case 'z': replaceAt = 2; break;
        //         case 'w': replaceAt = 3; break;
        //         default: replaceAt = -1;
        //     }
        //     if (replaceAt >= 0) {
        //         llvm::Value *newElem = irgen.GetExtractInst(value_to_assign, i);
        //         swiz_value = irgen.GetInsertInst(swiz_value, newElem, replaceAt);
        //         sym->value = newElem;
        //     }
        // }

        // // (void) new llvm::StoreInst(swiz_value, sym->value, false, irgen.GetBasicBlock());
        // value_to_assign = swiz_value;

        // int index = symtab->tables.size() - 1; 
         // sym = symtab->find(is_var->GetIdentifier()->GetName());
    }
    else if (is_var) {
        // int index = symtab->tables.size() - 1; 
        sym = symtab->find(is_var->GetIdentifier()->GetName());
        llvm::Type* varType = sym->value->getType();
        if (varType->isVectorTy())
            value_to_assign = irgen.checkLLVMvec(value_to_assign, varType->getVectorNumElements());
    }
    (void) new llvm::StoreInst(value_to_assign, sym->value, false, irgen.GetBasicBlock());

     // VarExpr *is_var = dynamic_cast<VarExpr*>(left);
     // if (is_var) value_to_assign = left->getEmit();
    //     cerr << "in assign emit dynamic casted" << endl;
    //     ScopedTable *curr = symtab->currentScope(); //find the var in symtable

    //      Symbol* s = curr->find(is_var->GetIdentifier()->GetName());

    // //             if (!s) cerr << "symbol is null "<<endl;
    //      llvm::Type* varType = s->value->getType();

    //      if (is_var->isVectorTy()) {
    //          int num = varType->getVectorNumElements();
    //          value_to_assign = irgen.checkLLVMvec(value_to_assign, num);
    //      }

    //     (void) new llvm::StoreInst(value_to_assign, s->value, true, irgen.GetBasicBlock());
    return value_to_assign;
}

llvm::Value* PostfixExpr::getEmit() {
  IRGenerator &irgen = IRGenerator::Instance();

  llvm::Value *varVal = left->getEmit();
  llvm::Value *newVal = NULL;

  if (op->IsOp("++"))
    newVal = irgen.PostFixIncrementInst(varVal);
  else
    newVal = irgen.PostFixDecrementInst(varVal);

  VarExpr *var = dynamic_cast<VarExpr*>(left);
  FieldAccess * swizzle = dynamic_cast<FieldAccess*>(left);

  if (var) { 
    Symbol *s = symtab->find(var->GetIdentifier()->GetName()); //find the var in symtable
    (void) new llvm::StoreInst(newVal, s->value, true, irgen.GetBasicBlock());
  }
  else if (swizzle) {
    Symbol *s = symtab->find(swizzle->GetIdentifier()->GetName()); //find the var in symtable
    (void) new llvm::StoreInst(newVal, s->value, true, irgen.GetBasicBlock());
  }

  return varVal;
}
   
ConditionalExpr::ConditionalExpr(Expr *c, Expr *t, Expr *f)
  : Expr(Join(c->GetLocation(), f->GetLocation())) {
    Assert(c != NULL && t != NULL && f != NULL);
    (cond=c)->SetParent(this);
    (trueExpr=t)->SetParent(this);
    (falseExpr=f)->SetParent(this);
}

void ConditionalExpr::PrintChildren(int indentLevel) {
    cond->Print(indentLevel+1, "(cond) ");
    trueExpr->Print(indentLevel+1, "(true) ");
    falseExpr->Print(indentLevel+1, "(false) ");
}
ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::PrintChildren(int indentLevel) {
    base->Print(indentLevel+1);
    subscript->Print(indentLevel+1, "(subscript) ");
}

llvm::Value* ArrayAccess::getEmit(){

    IRGenerator &irgen = IRGenerator::Instance();

    llvm::Value *llvmBase = base->getEmit();  //actual array
    llvm::Value *index = subscript->getEmit();

    // char* name;
    // VarExpr *ver = dynamic_cast<VarExpr*>(base);
    // if (ver)
    //     name = ver->GetIdentifier()->GetName();

    // ArrayType *arrayType = dynamic_cast<ArrayType *>(type);
    // llvm:Type* array_type = llvm::ArrayType::get(irgen.get_ll_type(arrayType), arrayType->GetArraySize());

    vector<llvm::Value*> array_access;
    array_access.push_back(llvm::ConstantInt::get(irgen.GetIntType(), 0)); //index starts at 0
    array_access.push_back(index);

    llvm::ArrayRef<llvm::Value*> argarray(array_access);
    return llvm::GetElementPtrInst::CreateInBounds(llvmBase, argarray, llvmBase->getName(), irgen.GetBasicBlock());
}
     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}


void FieldAccess::PrintChildren(int indentLevel) {
    if (base) base->Print(indentLevel+1);
    field->Print(indentLevel+1);
}

llvm::Value* FieldAccess::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();

    std::vector<llvm::Constant*> indices;
    std::string swizzle = std::string(field->GetName());

    for (int i = 0; i < swizzle.length(); ++i) {
        int index;

        if (swizzle.at(i) == 'x') {
            index = 0;
            break;
        }
        else if (swizzle.at(i) == 'y') {
            index = 0;
            break;
        }
        else if (swizzle.at(i) == 'z') {
            index = 0;
            break;
        }
        else if (swizzle.at(i) == 'w') {
            index = 0;
            break;
        }
        else
            index = 0;

        indices.push_back(llvm::ConstantInt::get(IRGenerator::Instance().GetIntType(), index, true));
    }

    llvm::Value *vec = base->getEmit();
    if (indices.size() == 1)
        return llvm::ExtractElementInst::Create(vec, indices[0], "", irgen.GetBasicBlock());

    llvm::Value *mask = llvm::ConstantVector::get(indices);
    llvm::Value *undef = llvm::UndefValue::get(vec->getType());

    return new llvm::ShuffleVectorInst(vec, undef, mask, "", irgen.GetBasicBlock());
}

Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::PrintChildren(int indentLevel) {
   if (base) base->Print(indentLevel+1);
   if (field) field->Print(indentLevel+1);
   if (actuals) actuals->PrintAll(indentLevel+1, "(actuals) ");
}

