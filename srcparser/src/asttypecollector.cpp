/*
 * Copyright (c) 2018 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
