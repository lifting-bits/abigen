/*
 * Copyright (c) 2018-present, Trail of Bits, Inc.
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

#include "compilerinstance.h"
#include "generate_utils.h"
#include "std_filesystem.h"

#include <iostream>

#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>

/// Private class data
struct CompilerInstance::PrivateData final {
  /// The compiler settings, such as language and include directories
  CompilerInstanceSettings compiler_settings;
};

CompilerInstance::CompilerInstance(const CompilerInstanceSettings &settings)
    : d(new PrivateData) {
  d->compiler_settings = settings;
}

CompilerInstance::Status CompilerInstance::create(
    CompilerInstanceRef &obj, const CompilerInstanceSettings &settings) {
  obj.reset();

  try {
    auto ptr = new CompilerInstance(settings);
    obj.reset(ptr);

    return Status(true);

  } catch (const std::bad_alloc &) {
    return Status(false, StatusCode::MemoryAllocationFailure);

  } catch (const Status &status) {
    return status;
  }
}

CompilerInstance::~CompilerInstance() {}

CompilerInstance::Status CompilerInstance::processAST(
    const std::string &buffer, IASTVisitorRef ast_visitor) {
  std::unique_ptr<clang::CompilerInstance> compiler;
  auto status =
      createClangCompilerInstance(compiler, d->compiler_settings, ast_visitor);

  if (!status.succeeded()) {
    return status;
  }

  auto &source_manager = compiler->getSourceManager();

  clang::FileID file_id =
      source_manager.createFileID(llvm::MemoryBuffer::getMemBuffer(
          llvm::StringRef(buffer), llvm::StringRef("main.cpp")));

  source_manager.setMainFileID(file_id);

  std::string clang_output_buffer;
  llvm::raw_string_ostream clang_output_stream(clang_output_buffer);

  clang::DiagnosticsEngine &diagnostics_engine = compiler->getDiagnostics();

  clang::TextDiagnosticPrinter diagnostic_consumer(
      clang_output_stream, &diagnostics_engine.getDiagnosticOptions());

  diagnostics_engine.setClient(&diagnostic_consumer, false);

  clang::Preprocessor &preprocessor = compiler->getPreprocessor();

  diagnostic_consumer.BeginSourceFile(compiler->getLangOpts(), &preprocessor);

  clang::ParseAST(preprocessor, &compiler->getASTConsumer(),
                  compiler->getASTContext());

  diagnostic_consumer.EndSourceFile();

  if (diagnostic_consumer.getNumErrors() != 0) {
    return Status(false, StatusCode::CompilationError, clang_output_buffer);
  }

  if (diagnostic_consumer.getNumWarnings() != 0) {
    return Status(true, StatusCode::CompilationWarning, clang_output_buffer);
  }

  return Status(true);
}
