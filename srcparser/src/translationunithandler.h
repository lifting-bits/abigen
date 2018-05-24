#pragma once

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class TranslationUnitHandler final : public clang::ASTConsumer {
  clang::CompilerInstance &compiler_instance;

 public:
  TranslationUnitHandler(clang::CompilerInstance &compiler_instance);
  virtual ~TranslationUnitHandler() = default;

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;
};
}  // namespace trailofbits
