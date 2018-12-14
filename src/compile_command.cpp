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

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Module.h>

#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/FrontendOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>

#include "generate_command.h"
#include "generate_utils.h"

/// Handler for the 'compile' command
bool compileCommandHandler(ProfileManagerRef &profile_manager,
                           const LanguageManager &language_manager,
                           const CommandLineOptions &cmdline_options) {
  CompilerInstanceSettings clang_settings;
  clang_settings.additional_include_folders =
      cmdline_options.additional_include_folders;
  clang_settings.enable_gnu_extensions = cmdline_options.enable_gnu_extensions;
  clang_settings.use_visual_cxx_mangling =
      cmdline_options.use_visual_cxx_mangling;

  auto prof_mgr_status = profile_manager->get(clang_settings.profile,
                                              cmdline_options.profile_name);
  if (!prof_mgr_status.succeeded()) {
    std::cerr << prof_mgr_status.toString() << "\n";
    return false;
  }

  if (!language_manager.parseLanguageDefinition(
          clang_settings.language, clang_settings.language_standard,
          cmdline_options.language)) {
    std::cerr << "Invalid language definition\n";
    return false;
  }

  std::unique_ptr<clang::CompilerInstance> compiler;
  auto status = createClangCompilerInstance(compiler, clang_settings);
  if (!status.succeeded()) {
    return false;
  }

  std::string clang_output_buffer;
  llvm::raw_string_ostream clang_output_stream(clang_output_buffer);

  clang::DiagnosticsEngine &diagnostics_engine = compiler->getDiagnostics();

  clang::TextDiagnosticPrinter diagnostic_consumer(
      clang_output_stream, &diagnostics_engine.getDiagnosticOptions());

  diagnostics_engine.setClient(&diagnostic_consumer, false);

  // Replicate the invocation here to make this work
  std::vector<std::string> clang_arguments = {
      "-triple",
      "x86_64-pc-linux-gnu",
      "-nostdsysteminc",
      "-nobuiltininc",
      "-resource-dir",
      clang_settings.profile.root_path + "/" +
          clang_settings.profile.resource_dir};

  auto path_list_it =
      clang_settings.profile.internal_isystem.find(clang_settings.language);
  if (path_list_it != clang_settings.profile.internal_isystem.end()) {
    const auto &path_list = path_list_it->second;

    for (const auto &path : path_list) {
      clang_arguments.push_back("-internal-isystem");
      clang_arguments.push_back(clang_settings.profile.root_path + "/" + path);
    }
  }

  path_list_it = clang_settings.profile.internal_externc_isystem.find(
      clang_settings.language);

  if (path_list_it != clang_settings.profile.internal_externc_isystem.end()) {
    const auto &path_list = path_list_it->second;

    for (const auto &path : path_list) {
      clang_arguments.push_back("-internal-externc-isystem");
      clang_arguments.push_back(clang_settings.profile.root_path + "/" + path);
    }
  }

  clang_arguments.push_back("-S");
  clang_arguments.push_back("-emit-llvm");
  clang_arguments.push_back(cmdline_options.abi_library_source_file);

  std::string language_flag = "-std=";
  switch (clang_settings.language) {
    case Language::C: {
      language_flag += "c";
      break;
    }

    case Language::CXX: {
      if (cmdline_options.enable_gnu_extensions) {
        language_flag += "gnu++";
      } else {
        language_flag += "c++";
      }

      break;
    }
  }

  language_flag += std::to_string(clang_settings.language_standard);
  clang_arguments.push_back(language_flag);

  std::vector<const char *> invocation;
  for (const auto &arg : clang_arguments) {
    invocation.push_back(arg.c_str());
  }

  auto compiler_invocation = std::make_shared<clang::CompilerInvocation>();
  clang::CompilerInvocation::CreateFromArgs(
      *compiler_invocation.get(), &invocation[0],
      &invocation[0] + invocation.size(), compiler->getDiagnostics());
  compiler->setInvocation(compiler_invocation);

  clang::EmitLLVMOnlyAction compiler_action;
  if (!compiler->ExecuteAction(compiler_action)) {
    std::cerr << "Error: " << clang_output_buffer << "\n";
    return 1;
  }

  std::error_code stream_error_code;
  llvm::raw_fd_ostream output_stream(cmdline_options.output, stream_error_code,
                                     llvm::sys::fs::F_None);

  auto module = compiler_action.takeModule();
  llvm::WriteBitcodeToFile(*module.get(), output_stream);

  output_stream.flush();
  if (stream_error_code) {
    std::cerr << "Failed to save the output to file\n";
    return false;
  }

  return true;
}
