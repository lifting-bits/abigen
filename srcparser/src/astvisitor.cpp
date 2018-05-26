#include "astvisitor.h"

namespace trailofbits {
/// \todo Overloaded functions, detect callbacks, varargs
ASTVisitor::ASTVisitor(std::list<StructureType> &structure_type_list,
                       std::list<FunctionType> &function_type_list)
    : structure_type_list(structure_type_list),
      function_type_list(function_type_list) {}

bool ASTVisitor::VisitRecordDecl(clang::RecordDecl *declaration) {
  StructureType new_structure;
  new_structure.name = declaration->getNameAsString();

  structure_type_list.push_front(new_structure);
  current_structure = structure_type_list.begin();

  return true;
}

bool ASTVisitor::VisitFieldDecl(clang::FieldDecl *declaration) {
  current_structure->fields[declaration->getNameAsString()] =
      declaration->getType().getAsString();

  return true;
}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *declaration) {
  FunctionType new_function;
  new_function.name = declaration->getNameAsString();
  new_function.return_type = declaration->getReturnType().getAsString();

  function_type_list.push_front(new_function);
  current_function = function_type_list.begin();

  return true;
}

bool ASTVisitor::VisitParmVarDecl(clang::ParmVarDecl *declaration) {
  std::string name = declaration->getNameAsString();
  if (name.empty()) {
    std::stringstream string_helper;
    string_helper << "unnamed" << unnamed_variable_counter;
    unnamed_variable_counter++;

    name = string_helper.str();
  }

  current_function->parameters[name] = declaration->getType().getAsString();
  return true;
}
}  // namespace trailofbits