#pragma once

#include "astvisitor.h"

#include <vector>

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class ASTTypeCollector final : public clang::ASTConsumer {
  std::vector<FunctionType> &function_type_list;
  std::vector<std::string> &overloaded_functions_blacklisted;

 public:
  ASTTypeCollector(std::vector<FunctionType> &function_type_list,
                   std::vector<std::string> &overloaded_functions_blacklisted);

  virtual ~ASTTypeCollector() = default;

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;
};
}  // namespace trailofbits
