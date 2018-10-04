#pragma once

#include "compilerinstance.h"
#include "istatus.h"
#include "types.h"

#include <memory>
#include <unordered_set>

#include <clang/AST/Mangle.h>
#include <clang/AST/RecursiveASTVisitor.h>

/// A list of classes, used when enumerating base classes
using ClassList = std::unordered_set<clang::CXXRecordDecl *>;

/// A list of method, used when acquiring the method list of a class hierarchy
using MethodList = std::unordered_set<clang::CXXMethodDecl *>;

/// A list of correlated types
using TypeList = std::unordered_set<const clang::Type *>;

/// This class is used to receive events from the AST
class ASTVisitor final : public IASTVisitor {
  struct PrivateData;

  /// Private class data
  std::unique_ptr<PrivateData> d;

  /// Private constructor; use ::create() instead
  ASTVisitor();

  /// Returns true if the given function declaration is in fact a method
  bool isClassMethod(clang::FunctionDecl *decl);

  /// Returns the class that owns the given method
  clang::CXXRecordDecl *getClass(clang::FunctionDecl *decl);

  /// Returns the class itself plus all the base classes
  ClassList collectClasses(clang::CXXRecordDecl *decl);

  /// Returns all the methods contained in the given class list
  MethodList collectClassMethods(const ClassList &class_list);

  /// Returns all the types used by the methods of the given classes
  TypeList collectClassMemberTypes(const ClassList &class_list);

  /// Returns all the types used in the member variables of the given class
  TypeList collectRecordMemberTypes(clang::RecordDecl *decl);

  /// Returns the types passed to the function or method
  TypeList collectFunctionParameterTypes(clang::FunctionDecl *decl);

  /// Descends into the given type list, enumerating all child types
  void enumerateTypeDependencies(const TypeList &root_type_list);

  /// Descends into the given type, enumerating all child types
  void enumerateTypeDependencies(const clang::Type *root_type);

  /// Returns the mangled name for the given function
  std::string getMangledFunctionName(clang::FunctionDecl *function_declaration);

  /// Returns the friendly (i.e.: unmangled) function name
  std::string getFriendlyFunctionName(
      clang::FunctionDecl *function_declaration);

  /// Returns name and location for the given type
  bool getTypeInformation(std::string &type_name,
                          SourceCodeLocation &type_location,
                          const clang::Type *type);

 public:
  /// Status code, used with ASTVisitor::Status
  enum class StatusCode { MemoryAllocationFailure, Unknown };

  /// Status object
  using Status = IStatus<StatusCode>;

  /// Factory method
  static Status create(IASTVisitorRef &ref);

  /// Destructor
  virtual ~ASTVisitor();

  /// This method is called when entering a new translation unit
  virtual void initialize(clang::ASTContext *ast_context,
                          clang::SourceManager *source_manager,
                          clang::MangleContext *name_mangler) override;

  /// This method is called each time a new function (or method) declaration is
  /// found
  virtual bool VisitFunctionDecl(clang::FunctionDecl *declaration) override;

  /// Called after the last AST callback
  virtual void finalize() override;

  /// Returns the blacklisted functions
  virtual BlacklistedFunctionList blacklistedFunctions() const override;

  /// Returns the whitelisted functions
  virtual WhitelistedFunctionList whitelistedFunctions() const override;
};
