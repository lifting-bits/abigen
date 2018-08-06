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
