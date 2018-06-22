#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <trailofbits/srcparser/isourcecodeparser.h>

#include <sstream>
#include <unordered_map>
#include <vector>

namespace trailofbits {
class ASTVisitor final : public clang::RecursiveASTVisitor<ASTVisitor> {
  std::unordered_map<std::string, FunctionType> &function_type_list;
  std::vector<std::string> &overloaded_functions_blacklisted;

  int unnamed_variable_counter;

 public:
  ASTVisitor(std::unordered_map<std::string, FunctionType> &function_type_list,
             std::vector<std::string> &overloaded_functions_blacklisted);
  virtual ~ASTVisitor() = default;

  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration);
};
}  // namespace trailofbits