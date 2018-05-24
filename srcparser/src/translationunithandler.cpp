#include "translationunithandler.h"

namespace trailofbits {
TranslationUnitHandler::TranslationUnitHandler(
    clang::CompilerInstance &compiler_instance)
    : compiler_instance(compiler_instance) {}

void TranslationUnitHandler::HandleTranslationUnit(clang::ASTContext &context) {
  static_cast<void>(context);
}
}  // namespace trailofbits