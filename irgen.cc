/* irgen.cc -  LLVM IR generator
 *
 * You can implement any LLVM related functions here.
 */

#include "irgen.h"

IRGenerator::IRGenerator() :
    context(NULL),
    module(NULL),
    currentFunc(NULL),
    currentBB(NULL)
{
}

IRGenerator& IRGenerator::Instance() {
  static IRGenerator irgen;
  return irgen;
}

IRGenerator::~IRGenerator() {
}

llvm::Module *IRGenerator::GetOrCreateModule(const char *moduleID)
{
   if ( module == NULL ) {
     context = new llvm::LLVMContext();
     module  = new llvm::Module(moduleID, *context);
     module->setTargetTriple(TargetTriple);
     module->setDataLayout(TargetLayout);
   }
   return module;
}

void IRGenerator::SetFunction(llvm::Function *func) {
   currentFunc = func;
}

llvm::Function *IRGenerator::GetFunction() const {
   return currentFunc;
}

void IRGenerator::SetBasicBlock(llvm::BasicBlock *bb) {
   currentBB = bb;
}

llvm::BasicBlock *IRGenerator::GetBasicBlock() const {
   return currentBB;
}

llvm::Type *IRGenerator::GetIntType() const {
   llvm::Type *ty = llvm::Type::getInt32Ty(*context);
   return ty;
}

llvm::Type *IRGenerator::GetBoolType() const {
   llvm::Type *ty = llvm::Type::getInt1Ty(*context);
   return ty;
}

llvm::Type *IRGenerator::GetFloatType() const {
   llvm::Type *ty = llvm::Type::getFloatTy(*context);
   return ty;
}

llvm::Type *IRGenerator::GetVoidType() const {
   llvm::Type *ty = llvm::Type::getVoidTy(*context);
   return ty;
}

llvm::Type *IRGenerator::GetVecType(int size) const {
  return llvm::VectorType::get(GetFloatType(), size);
}

llvm::Value *IRGenerator::PostFixIncrementInst(llvm::Value *value) {
  llvm::Type *valType = value->getType();
  bool is_float_op = valType->isFloatTy() || valType->isVectorTy();
  llvm::BinaryOperator::BinaryOps op;
  int scalar = 1;

  llvm::Value *scalarVal;
  if (is_float_op){
    op = llvm::BinaryOperator::FAdd;
    scalarVal = llvm::ConstantFP::get(GetFloatType(), scalar);
  } 
  else{
    op = llvm::BinaryOperator::Add;
    scalarVal = llvm::ConstantInt::get(GetIntType(), (int)scalar, true);
  }

  return llvm::BinaryOperator::Create(op, value, scalarVal, "", GetBasicBlock());
}

llvm::Value *IRGenerator::PostFixDecrementInst(llvm::Value *value) {
  llvm::Type *valType = value->getType();
  bool is_float_op = valType->isFloatTy() || valType->isVectorTy();
  llvm::BinaryOperator::BinaryOps op;
  int scalar = 1;

  llvm::Value *scalarVal;
  if (is_float_op){
    op = llvm::BinaryOperator::FAdd;
    scalarVal = llvm::ConstantFP::get(GetFloatType(), scalar);
  } 
  else{
    op = llvm::BinaryOperator::Add;
    scalarVal = llvm::ConstantInt::get(GetIntType(), (int)scalar, true);
  }

  return llvm::BinaryOperator::Create(op, value, scalarVal, "", GetBasicBlock());
}

const char *IRGenerator::TargetLayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128";

const char *IRGenerator::TargetTriple = "x86_64-redhat-linux-gnu";

/*
llvm::Type *IRGenerator::GetType(Type* ty) const {
	llvm::Type *ty = llvm::Type::getFloatTy(*context);
	
	
	return ty;
}
*/
