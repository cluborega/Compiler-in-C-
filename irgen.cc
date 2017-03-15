/* irgen.cc -  LLVM IR generator
 *
 * You can implement any LLVM related functions here.
 */

#include "irgen.h"

using namespace std;

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

llvm::BasicBlock *IRGenerator::CreateEmptyBlock(string name) {
  return llvm::BasicBlock::Create(*context, name, GetFunction());
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

llvm::Value *IRGenerator::ToVector(llvm::Value *value, int size) {
  if (value->getType()->isVectorTy() == true)
    return value;

  llvm::Constant* fc_ptr_zero = llvm::ConstantFP::get(GetFloatType(), 0.0);
  vector<llvm::Constant*> contents(size, fc_ptr_zero);
  llvm::Value *vec = llvm::ConstantVector::get(contents);

  for (int i = 0; i < size; ++i) {
    llvm::Value* index = llvm::ConstantInt::get(GetIntType(), i, true);
    vec = llvm::InsertElementInst::Create(vec, value, index, "", GetBasicBlock());
  }

  return vec;
}

llvm::Value *IRGenerator::BoolVectorToBool(llvm::Value *vector) {
  IRGenerator &irgen = IRGenerator::Instance();

  llvm::Value* idx = llvm::ConstantInt::get(GetIntType(), 0, true);

  llvm::Value *andResult = llvm::ExtractElementInst::Create(vector, llvm::ConstantInt::get(GetIntType(), 0, true), "", GetBasicBlock());
    //llvm::ExtractElementInst::Create(vec, index, "", GetBasicBlock());

  for (int i = 1; i < vector->getType()->getVectorNumElements(); ++i) {
    llvm::Value *elem = llvm::ExtractElementInst::Create(vector, llvm::ConstantInt::get(GetIntType(), i, true), "", GetBasicBlock());
    andResult = llvm::BinaryOperator::CreateAnd(andResult, elem, "", irgen.GetBasicBlock());
  }

  return andResult;
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

// llvm::Value *IRGenerator::PreNegativeInst(llvm::Value *value) {
//   llvm::Type *valType = value->getType();
//   bool is_float_op = valType->isFloatTy() || valType->isVectorTy();
//   llvm::BinaryOperator::BinaryOps op;
//   int scalar = -1;

//   llvm::Value *scalarVal;
//   if (is_float_op){
//     op = llvm::BinaryOperator::FMul;
//     scalarVal = llvm::ConstantFP::get(GetFloatType(), scalar);
//   } 
//   else{
//     op = llvm::BinaryOperator::Mul;
//     scalarVal = llvm::ConstantInt::get(GetIntType(), (int)scalar, true);
//   }

//   return llvm::BinaryOperator::Create(op, value, scalarVal, "", GetBasicBlock());
// }

const char *IRGenerator::TargetLayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128";

const char *IRGenerator::TargetTriple = "x86_64-redhat-linux-gnu";

