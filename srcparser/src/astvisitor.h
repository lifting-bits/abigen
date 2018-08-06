#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <trailofbits/srcparser/isourcecodeparser.h>

#include <sstream>
#include <unordered_map>

namespace trailofbits {
class ASTVisitor final : public clang::RecursiveASTVisitor<ASTVisitor> {
  std::unordered_map<std::string, FunctionType> &functions;
  std::unordered_set<std::string> &blacklisted_functions;

  int unnamed_variable_counter;

 public:
  ASTVisitor(std::unordered_map<std::string, FunctionType> &functions,
             std::unordered_set<std::string> &blacklisted_functions);
  virtual ~ASTVisitor() = default;

  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration);
};
}  // namespace trailofbits
