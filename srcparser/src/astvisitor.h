#pragma once

#include <clang/AST/RecursiveASTVisitor.h>
#include <trailofbits/srcparser/isourcecodeparser.h>
#include <sstream>

namespace trailofbits {
class ASTVisitor final : public clang::RecursiveASTVisitor<ASTVisitor> {
  std::list<StructureType> &structure_type_list;
  std::list<FunctionType> &function_type_list;

  std::list<StructureType>::iterator current_structure;
  std::list<FunctionType>::iterator current_function;

  int unnamed_variable_counter;

 public:
  ASTVisitor(std::list<StructureType> &structure_type_list,
             std::list<FunctionType> &function_type_list);

  virtual ~ASTVisitor() = default;

  virtual bool VisitRecordDecl(clang::RecordDecl *declaration);
  virtual bool VisitFieldDecl(clang::FieldDecl *declaration);
  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration);
  virtual bool VisitParmVarDecl(clang::ParmVarDecl *declaration);
};
}  // namespace trailofbits