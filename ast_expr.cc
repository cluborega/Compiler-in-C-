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

llvm::Value* VarExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    Symbol *s = symtab->find(id->GetName()); //find the var in symtable

    llvm::Value* llvm_value = NULL;
    // Type *vdType = NULL;
    
    if(s){
        llvm_value = s->value; //get its value
        VarDecl* vd = dynamic_cast<VarDecl*>(s->decl);
        this->type = vd->GetType();
    }

    llvm::LoadInst* var_expr = new llvm::LoadInst(llvm_value, this->GetIdentifier()->GetName(), irgen.GetBasicBlock());
    return var_expr;
}

void VarExpr::PrintChildren(int indentLevel) {
    id->Print(indentLevel+1);
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

// llvm::Value *UnaryExpr::ValEmit() {
//     IRGenerator &irgen = IRGenerator::Instance();

//     llvm::Value *varVal = right->ValEmit();
//     string operation = op->to_string();
//     llvm::Value *newVal = NULL;

//     if (operation == "+")
//         newVal = varVal;
//     else if (operation == "-")
//         newVal = irgen.PreNegativeInst(varVal);  //GetOpWithScalar(value, '*', -1);
//     else if (operation == "++")
//         newVal = irgen.PostFixIncrementInst(varVal);
//     else if (operation == "--")
//         newVal = irgen.PostFixDecrementInst(varVal);

//     return newVal;
// }

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
    result = irgen.BoolVectorToBool(result);
    return result;
}

llvm::Value* ArithmeticExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::BasicBlock *bb = irgen.GetBasicBlock();

    llvm::Value *leftValue = left->getEmit();
    llvm::Value *rightValue = right->getEmit();

    if (leftValue->getType()->isVectorTy())
        rightValue = irgen.ToVector(rightValue, leftValue->getType()->getVectorNumElements());

    if (rightValue->getType()->isVectorTy())
        leftValue = irgen.ToVector(leftValue, rightValue->getType()->getVectorNumElements());

    bool is_float_op = leftValue->getType()->isFloatTy();

    std::string opStr = op->to_string();
    llvm::BinaryOperator::BinaryOps llvmOP;

    if (op->IsOp("+")) {
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::FAdd;
        else
            llvmOP = llvm::BinaryOperator::Add;
    }
    else if (op->IsOp("-")) {
        if (is_float_op)
            llvmOP = llvm::BinaryOperator::Sub;
        else
            llvmOP = llvm::BinaryOperator::Add;
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

    llvm::Value* lhs = left->getEmit();
    llvm::Value* rhs = right->getEmit();

    VarExpr *var = dynamic_cast<VarExpr*>(left);

    int index = symtab->tables.size() - 1; 
    Symbol* s = symtab->tables[index]->find(var->GetIdentifier()->GetName());
    
    if(s == NULL){
         // cerr << "symbol is NULL " << endl;
    }

    if (op->IsOp("=")) {
        value_to_assign = rhs;
        // cerr << "assign val = " << value_to_assign << endl;
    }
    else { //handle cases for +=, -= etc , one idea is just extract the first "+, -" ..
        // std::string op_str = op->to_string();
        // Operator *temp_op = new Operator(op_str.c_str());
        ArithmeticExpr *arith_expr = new ArithmeticExpr(left, op, right);
        value_to_assign = arith_expr->getEmit();
    }

    (void) new llvm::StoreInst(value_to_assign, s->value, "", irgen.GetBasicBlock());
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
    (void) new llvm::StoreInst(newVal, s->value, "", irgen.GetBasicBlock());
  }
  else if (swizzle) {
    Symbol *s = symtab->find(swizzle->GetIdentifier()->GetName()); //find the var in symtable
    (void) new llvm::StoreInst(newVal, s->value, "", irgen.GetBasicBlock());
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

