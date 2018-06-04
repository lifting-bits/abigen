#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <trailofbits/srcparser/isourcecodeparser.h>
#include <sstream>

namespace trailofbits {
class ASTVisitor final : public clang::RecursiveASTVisitor<ASTVisitor> {
  std::list<FunctionType> &function_type_list;
  std::list<FunctionType>::iterator current_function;

  int unnamed_variable_counter;

 public:
  ASTVisitor(std::list<FunctionType> &function_type_list);

  virtual ~ASTVisitor() = default;

  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration);
  virtual bool VisitParmVarDecl(clang::ParmVarDecl *declaration);
};
}  // namespace trailofbits