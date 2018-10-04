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

#include "profilemanager.h"

#include <clang/AST/ASTContext.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>

#pragma once

/// Settings for the clang compiler instance
struct CompilerInstanceSettings final {
  /// The profile to use
  Profile profile;

  /// The language to use
  Language language{Language::C};

  /// The language standard
  int language_standard{11};

  /// Additional include folders
  StringList additional_include_folders;

  /// Whether GNU extensions should be enabled or not
  bool enable_gnu_extensions{false};

  /// Whether to use standard C++ name mangling rules or the Visual C++
  /// compatibility mode
  bool use_visual_cxx_mangling{false};
};

class IASTVisitor;

/// A reference to an IASTVisitor object
using IASTVisitorRef = std::shared_ptr<IASTVisitor>;

/// The base visitor
class IASTVisitor : public clang::RecursiveASTVisitor<IASTVisitor> {
 public:
  /// Destructor
  virtual ~IASTVisitor() = default;

  /// This method is called when entering a new translation unit
  virtual void initialize(clang::ASTContext *ast_context,
                          clang::SourceManager *source_manager,
                          clang::MangleContext *name_mangler) = 0;

  /// This method is called each time a new function (or method) declaration is
  /// found
  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration) = 0;

  /// Called after the last AST callback
  virtual void finalize() = 0;

  /// Returns the blacklisted functions
  virtual BlacklistedFunctionList blacklistedFunctions() const = 0;

  /// Returns the whitelisted functions
  virtual WhitelistedFunctionList whitelistedFunctions() const = 0;
};

class CompilerInstance;

/// A reference to a clang compiler instance object
using CompilerInstanceRef = std::unique_ptr<CompilerInstance>;

/// AST callback
using ASTCallback = bool (*)(clang::Decl *declaration,
                             clang::ASTContext &ast_context,
                             clang::SourceManager &source_manager,
                             void *user_defined,
                             clang::MangleContext *name_mangler);

/// A wrapper around clang::CompilerInstance
class CompilerInstance final {
  struct PrivateData;

  /// Private class data
  std::unique_ptr<PrivateData> d;

 public:
  /// Status code, used with ProfileManager::Status
  enum class StatusCode {
    InvalidLanguage,
    InvalidLanguageStandard,
    MemoryAllocationFailure,
    CompilationError,
    CompilationWarning,
    Unknown
  };

  /// Status object
  using Status = IStatus<StatusCode>;

  /// AST callback type, used with CompilerInstance::registerCallback
  enum class ASTCallbackType { Function };

  /// Constructor
  CompilerInstance(const CompilerInstanceSettings &settings);

  /// Creates a new CompilerInstance object
  static Status create(CompilerInstanceRef &obj,
                       const CompilerInstanceSettings &settings);

  /// Destructor
  ~CompilerInstance();

  /// Processes the specified buffer
  Status processBuffer(const std::string &buffer,
                       IASTVisitorRef ast_visitor = IASTVisitorRef());

  /// Disable the copy constructor
  CompilerInstance(const CompilerInstance &other) = delete;

  /// Disable the assignment operator
  CompilerInstance &operator=(const CompilerInstance &other) = delete;
};
