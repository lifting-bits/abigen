#include "astvisitor.h"

namespace trailofbits {
ASTVisitor::ASTVisitor(std::unordered_map<std::string, FunctionType> &functions,
                       std::unordered_set<std::string> &blacklisted_functions)
    : functions(functions), blacklisted_functions(blacklisted_functions) {}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *declaration) {
  FunctionType new_function;
  new_function.name = declaration->getNameAsString();

  // Skip functions we already blacklisted
  if (blacklisted_functions.count(new_function.name) > 0) {
    return false;
  }

  if (declaration->isVariadic()) {
    blacklisted_functions.insert(new_function.name);
    return false;
  }

  // If this function name is repeated, then it is overloaded; remove it from
  // the list and blacklist the name
  auto function_it = functions.find(new_function.name);
  if (function_it != functions.end()) {
    functions.erase(function_it);
    blacklisted_functions.insert(new_function.name);

    return false;
  }

  new_function.return_type = declaration->getReturnType().getAsString();

  // Go through all parameters; blacklist functions that may be receiving
  // a callback
  bool contains_possible_callback = false;

  for (const auto &parameter : declaration->parameters()) {
    std::string name = parameter->getNameAsString();
    if (name.empty()) {
      std::stringstream string_helper;
      string_helper << "unnamed" << unnamed_variable_counter;
      unnamed_variable_counter++;

      name = string_helper.str();
    }

    const auto &parameter_type = parameter->getType();
    if (parameter_type->isPointerType()) {
      contains_possible_callback = true;
      break;
    }

    new_function.parameters[name] = parameter_type.getAsString();
  }

  if (contains_possible_callback) {
    return false;
  }

  functions.insert({new_function.name, new_function});
  return true;
}
}  // namespace trailofbits
