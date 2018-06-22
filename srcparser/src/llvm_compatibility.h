#pragma once

#include <clang/Frontend/FrontendOptions.h>

namespace trailofbits {
#if LLVM_MAJOR_VERSION <= 4
const auto kClangFrontendInputKindCxx = clang::IK_CXX;
const auto kClangFrontendInputKindC = clang::IK_C;
#else
const auto kClangFrontendInputKindCxx = clang::InputKind::CXX;
const auto kClangFrontendInputKindC = clang::InputKind::C;
#endif
}  // namespace trailofbits
