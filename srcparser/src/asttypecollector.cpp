#include "asttypecollector.h"

namespace trailofbits {
ASTTypeCollector::ASTTypeCollector(
    std::unordered_map<std::string, FunctionType> &functions,
    std::unordered_set<std::string> &blacklisted_functions)
    : functions(functions), blacklisted_functions(blacklisted_functions) {}

void ASTTypeCollector::HandleTranslationUnit(clang::ASTContext &context) {
  ASTVisitor recursive_visitor(functions, blacklisted_functions);

  auto translation_unit_declaration_list =
      context.getTranslationUnitDecl()->decls();

  for (auto &current_declaration : translation_unit_declaration_list) {
    recursive_visitor.TraverseDecl(current_declaration);
  }
}
}  // namespace trailofbits
