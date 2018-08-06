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

#include "astvisitor.h"

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class ASTTypeCollector final : public clang::ASTConsumer {
  std::unordered_map<std::string, FunctionType> &functions;
  std::unordered_set<std::string> &blacklisted_functions;

 public:
  ASTTypeCollector(std::unordered_map<std::string, FunctionType> &functions,
                   std::unordered_set<std::string> &blacklisted_functions);

  virtual ~ASTTypeCollector() = default;

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;
};
}  // namespace trailofbits
