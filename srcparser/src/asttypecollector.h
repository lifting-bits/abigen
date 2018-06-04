#pragma once

#include "astvisitor.h"

#include <clang/AST/ASTContext.h>
#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class ASTTypeCollector final : public clang::ASTConsumer {
  clang::CompilerInstance &compiler_instance;

  std::list<FunctionType> &function_type_list;

 public:
  ASTTypeCollector(clang::CompilerInstance &compiler_instance,
                   std::list<FunctionType> &function_type_list);
  virtual ~ASTTypeCollector() = default;

  virtual void HandleTranslationUnit(clang::ASTContext &context) override;
};
}  // namespace trailofbits
