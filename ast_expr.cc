/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */

#include <string.h>
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include "symtable.h"

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

llvm::Value* AssignExpr::getEmit() {
    IRGenerator &irgen = IRGenerator::Instance();
    llvm::Value *value_to_assign = NULL;

    llvm::Value* lhs = left->getEmit();
    llvm::Value* rhs = right->getEmit();

    VarExpr *var = dynamic_cast<VarExpr*>(left);

    int index = symtab->tables.size() - 1; 
    Symbol* s = symtab->tables[index]->find(var->GetIdentifier()->GetName());
    
    if(s == NULL){
         cerr << "symbol is NULL " << endl;
    }

    if (op->to_string() == "=") {
        value_to_assign = rhs;
        cerr << "assign val = " << value_to_assign << endl;
    }
    else {
        // std::string op_str = op->to_string().substr(0, 1);
        // Operator *temp_op = new Operator(op_str.data());
        // ArithmeticExpr *arith_expr = new ArithmeticExpr(left, temp_op, right);
        // value_to_assign = arith_expr->getEmit();
    }

    (void) new llvm::StoreInst(value_to_assign, s->value, "", irgen.GetBasicBlock());
    return value_to_assign;
}

llvm::Value* PostfixExpr::getEmit() {
  IRGenerator &irgen = IRGenerator::Instance();

  llvm::Value *varVal = left->getEmit();
  std::string operation = op->to_string();
  llvm::Value *newVal = NULL;

  if (operation == "++")
    newVal = irgen.PostFixIncrementInst(varVal);
  else
    newVal = irgen.PostFixDecrementInst(varVal);

  // VarExpr *var = dynamic_cast<VarExpr*>(left);
  // FieldAccess * swizzle = dynamic_cast<FieldAccess*>(left);

  // if (var)
  //   Scope::current->AssignVar(var->GetIdentifier(), newVal);
  // else if (swizzle)
  //   Scope::current->AssignSwizzle(swizzle, newVal);

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

