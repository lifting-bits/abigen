#include "asttypecollector.h"

namespace trailofbits {
ASTTypeCollector::ASTTypeCollector(
    clang::CompilerInstance &compiler_instance,
    std::vector<FunctionType> &function_type_list,
    std::vector<std::string> &overloaded_functions_blacklisted)
    : compiler_instance(compiler_instance),
      function_type_list(function_type_list),
      overloaded_functions_blacklisted(overloaded_functions_blacklisted) {}

void ASTTypeCollector::HandleTranslationUnit(clang::ASTContext &context) {
  std::unordered_map<std::string, FunctionType> discovered_function_types;
  ASTVisitor recursive_visitor(discovered_function_types,
                               overloaded_functions_blacklisted);

  auto translation_unit_declaration_list =
      context.getTranslationUnitDecl()->decls();

  for (auto &current_declaration : translation_unit_declaration_list) {
    recursive_visitor.TraverseDecl(current_declaration);
  }

  function_type_list.reserve(function_type_list.size() +
                             discovered_function_types.size());
  for (const auto &p : discovered_function_types) {
    const auto &type = p.second;
    function_type_list.push_back(type);
  }
}
}  // namespace trailofbits