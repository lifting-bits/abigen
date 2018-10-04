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

#pragma once

#include "cmdline.h"
#include "compilerinstance.h"
#include "generate_command.h"
#include "types.h"

#include <clang/AST/RecursiveASTVisitor.h>

/// Returns the source code location for the given declaration
SourceCodeLocation getSourceCodeLocation(clang::ASTContext &ast_context,
                                         clang::SourceManager &source_manager,
                                         const clang::Decl *declaration);

/// Returns true if the given function declaration accepts (directly or
/// otherwise) a function pointer
bool containsFunctionPointer(const clang::FunctionDecl *func_decl);

/// Creates a new compiler instance object configured according to the command
/// line options
bool createCompilerInstance(CompilerInstanceRef &compiler,
                            ProfileManagerRef &profile_manager,
                            const LanguageManager &language_manager,
                            const CommandLineOptions &cmdline_options);

/// Recursively enumerates all the include files found in the given folder
bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const std::string &header_folder);

/// Recursively enumerates all the include files found in the given folder list
bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const StringList &header_folders);

/// Given a header descriptor, it generates al possible include directives that
/// can import it. It works by mixing the header name with several prefixes
StringList generateIncludeDirectives(const HeaderDescriptor &header_descriptor);

/// Generates a compilable source code buffer that includes all the given
/// headers
std::string generateSourceBuffer(const StringList &include_list,
                                 const StringList &base_includes);

/// This AST function callback is used to filter and collect functions that
/// are suitable for the ABI library
bool astFunctionCallback(clang::Decl *declaration,
                         clang::ASTContext &ast_context,
                         clang::SourceManager &source_manager,
                         void *user_defined,
                         clang::MangleContext *name_mangler);
