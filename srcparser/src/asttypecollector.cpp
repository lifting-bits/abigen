#include "asttypecollector.h"

namespace trailofbits {
ASTTypeCollector::ASTTypeCollector(clang::CompilerInstance &compiler_instance,
                                   std::list<FunctionType> &function_type_list)
    : compiler_instance(compiler_instance),
      function_type_list(function_type_list) {}

void ASTTypeCollector::HandleTranslationUnit(clang::ASTContext &context) {
  ASTVisitor recursive_visitor(function_type_list);

  auto translation_unit_declaration_list =
      context.getTranslationUnitDecl()->decls();

  for (auto &current_declaration : translation_unit_declaration_list) {
    recursive_visitor.TraverseDecl(current_declaration);
  }
}
}  // namespace trailofbits