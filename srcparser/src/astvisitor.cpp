#include "astvisitor.h"

namespace trailofbits {
/// \todo Overloaded functions, detect callbacks, varargs
ASTVisitor::ASTVisitor(
    std::unordered_map<std::string, FunctionType> &function_type_list,
    std::vector<std::string> &overloaded_functions_blacklisted)
    : function_type_list(function_type_list),
      overloaded_functions_blacklisted(overloaded_functions_blacklisted) {}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *declaration) {
  FunctionType new_function;
  new_function.name = declaration->getNameAsString();

  // Skip functions we already blacklisted
  if (std::find(overloaded_functions_blacklisted.begin(),
                overloaded_functions_blacklisted.end(),
                new_function.name) != overloaded_functions_blacklisted.end()) {
    return false;
  }

  // If this function name is repeated, then it is overloaded; remove it from
  // the list and blacklist the name
  auto function_it = function_type_list.find(new_function.name);
  if (function_it != function_type_list.end()) {
    function_type_list.erase(function_it);
    overloaded_functions_blacklisted.push_back(new_function.name);

    return false;
  }

  new_function.return_type = declaration->getReturnType().getAsString();

  for (const auto &parameter : declaration->parameters()) {
    std::string name = parameter->getNameAsString();
    if (name.empty()) {
      std::stringstream string_helper;
      string_helper << "unnamed" << unnamed_variable_counter;
      unnamed_variable_counter++;

      name = string_helper.str();
    }

    new_function.parameters[name] = parameter->getType().getAsString();
  }

  function_type_list.insert({new_function.name, new_function});
  return true;
}
}  // namespace trailofbits