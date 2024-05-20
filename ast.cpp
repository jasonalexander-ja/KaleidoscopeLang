#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ast.h"


using namespace llvm;

std::unique_ptr<ExprAST> LogError(const char *Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
Value *LogErrorV(const char *Str) 
{
  LogError(Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
{
    LogError(Str);
    return nullptr;
}


Value* NumberExprAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    return ConstantFP::get(context, APFloat(Val));
}

Value* VariableExprAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    // Look this variable up in the function. 
    Value* V = namedVals[Name];
    if (!V)
        LogErrorV("Unknown variable name");
    return V;
}

Value* BinaryExprAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    Value* L = LHS->codegen(context, mod, builder, namedVals);
    Value* R = RHS->codegen(context, mod, builder, namedVals);
    if (!L || !R)
        return nullptr;

    switch (Op)
    {
        case '+':
            return builder.CreateFAdd(L, R, "addtmp");
        case '-':
            return builder.CreateFSub(L, R, "addtmp");
        case '*':
            return builder.CreateFMul(L, R, "addtmp");
        case '<':
            L = builder.CreateFCmpULT(L, R, "cmptmp"); 
            // Convert bool 0/1 to double 0.0 or 1.0
            return builder.CreateUIToFP(L, Type::getDoubleTy(context), "addtmp");
    }
    return LogErrorV("invalid binary operator");
}

Value* CallExprAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    // Look up the name in the global module table. 
    Function* CalleeF = mod.getFunction(Callee);
    if (CalleeF)
        return LogErrorV("Uknown function referenced");
    
    // If argument mismatch error. 
    if (CalleeF->arg_size() != Args.size())
        return LogErrorV("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; i++)
    {
        ArgsV.push_back(Args[i]->codegen(context, mod, builder, namedVals));
        if (!Args.back())
            return nullptr;
    }

    return builder.CreateCall(CalleeF, ArgsV, "calltmp");
}


Function* PrototypeAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    // Make the function type: double(double, double) etc.
    std::vector<Type*> Doubles(Args.size(), Type::getDoubleTy(context));

    FunctionType* FT = FunctionType::get(Type::getDoubleTy(context), Doubles, false);

    Function* F = Function::Create(FT, Function::ExternalLinkage, Name, mod);

    unsigned Idx = 0;

    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function* FunctionAST::codegen(LLVMContext& context, Module& mod, IRBuilder<>& builder, std::map<std::string, llvm::Value *>& namedVals)
{
    Function* TheFunction = mod.getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen(context, mod, builder, namedVals);

    if (!TheFunction)
        return nullptr;
    
    if (!TheFunction->empty())
        return (Function*)LogErrorV("Function cannot be redefined. ");

    // Create a new basic block to start insertion into.
    BasicBlock* BB = BasicBlock::Create(context, "entry", TheFunction);
    builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map. 
    namedVals.clear();
    for (auto& Arg : TheFunction->args())
        namedVals[std::string(Arg.getName())] = &Arg;

    if (Value* RetVal = Body->codegen(context, mod, builder, namedVals))
    {
        // Finish off the function.
        builder.CreateRet(RetVal);

        // Validate the generated code, checking for consistency. 
        verifyFunction(*TheFunction);

        return TheFunction;
    }

    // Error reading body, remove function. 
    TheFunction->eraseFromParent();
    return nullptr;
}

