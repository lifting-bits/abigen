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
