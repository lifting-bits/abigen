/*
 * Copyright (c) 2018 Trail of Bits, Inc.
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

#include "astvisitor.h"
#include <clang/AST/Type.h>
#include <iostream>

namespace trailofbits {
namespace {
SourceCodeLocation GetSourceCodeLocation(clang::ASTContext *ast_context,
                                         clang::SourceManager &source_manager,
                                         clang::Decl *declaration) {
  auto start_location = declaration->getLocStart();
  auto full_start_location = ast_context->getFullLoc(start_location);

#if LLVM_MAJOR_VERSION > 5
  static_cast<void>(source_manager);
  const auto file_entry = full_start_location.getFileEntry();
#else
  auto file_id = full_start_location.getFileID();
  const auto file_entry = source_manager.getFileEntryForID(file_id);
#endif

  SourceCodeLocation output;
  if (file_entry != nullptr) {
    output.file_path = file_entry->getName();
  }

  if (output.file_path.empty()) {
    output.file_path = "memory:///main_source_file";
  }

  output.line = full_start_location.getSpellingLineNumber();
  output.column = full_start_location.getSpellingColumnNumber();

  return output;
}

bool isFunctionPointer(clang::ASTContext *ast_context, clang::QualType type) {
  class TypeVisitor final : public clang::RecursiveASTVisitor<TypeVisitor> {
    bool &is_function_ptr;
    clang::ASTContext *ast_context;

   public:
    TypeVisitor(bool &is_function_ptr, clang::ASTContext *ast_context)
        : is_function_ptr(is_function_ptr), ast_context(ast_context) {}

    virtual ~TypeVisitor() = default;

    virtual bool VisitType(clang::Type *type) {
      if (type->isFunctionType()) {
        is_function_ptr = true;
      } else if (type->isFunctionPointerType()) {
        is_function_ptr = true;
      } else if (type->isPointerType()) {
        if (isFunctionPointer(ast_context, type->getPointeeType())) {
          is_function_ptr = true;
        }
      }

      return true;
    }
  };

  bool is_function_ptr = false;
  TypeVisitor recursive_visitor(is_function_ptr, ast_context);
  recursive_visitor.TraverseType(type);

  return is_function_ptr;
}
}  // namespace

ASTVisitor::ASTVisitor(TranslationUnitData &data,
                       clang::ASTContext *ast_context,
                       clang::SourceManager &source_manager)
    : data(data), ast_context(ast_context), source_manager(source_manager) {}

bool ASTVisitor::VisitRecordDecl(clang::RecordDecl *declaration) {
  clang::PrintingPolicy printing_policy(ast_context->getLangOpts());

  RecordData record_data;
  for (auto it = declaration->field_begin(); it != declaration->field_end();
       it++) {
    auto field_name = it->getNameAsString();

    Type field_type = {};
    auto base_type_id = it->getType().getBaseTypeIdentifier();
    if (base_type_id != nullptr) {
      field_type.name = base_type_id->getName().data();
    }

    if (field_type.name.empty()) {
#if LLVM_MAJOR_VERSION > 5
      field_type.name = clang::QualType::getAsString(
          it->getType().getNonReferenceType().split(), printing_policy);
#else
      field_type.name = clang::QualType::getAsString(
          it->getType().getNonReferenceType().split());
#endif
    }

    field_type.is_function_pointer =
        isFunctionPointer(ast_context, it->getType());
    record_data.members.insert({field_name, field_type});
  }

  TypeDescriptor type_desc;
  type_desc.type = TypeDescriptor::DescriptorType::Record;
  type_desc.data = record_data;
  type_desc.name = declaration->getNameAsString();
  type_desc.location =
      GetSourceCodeLocation(ast_context, source_manager, declaration);

  data.type_index[type_desc.name].insert(type_desc.location);
  data.types.insert({type_desc.location, std::move(type_desc)});
  return true;
}

/*
bool ASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *declaration) {
return true;
}
*/

bool ASTVisitor::VisitTypedefNameDecl(clang::TypedefNameDecl *declaration) {
  TypeAliasData type_data;

  auto qual_type = ast_context->getTypeDeclType(declaration);
  type_data.is_function_pointer = isFunctionPointer(ast_context, qual_type);

  TypeDescriptor type_desc;
  type_desc.type = TypeDescriptor::DescriptorType::TypeAlias;
  type_desc.data = type_data;
  type_desc.name = declaration->getNameAsString();
  type_desc.location =
      GetSourceCodeLocation(ast_context, source_manager, declaration);

  data.type_index[type_desc.name].insert(type_desc.location);
  data.types.insert({type_desc.location, std::move(type_desc)});
  return true;
}

bool ASTVisitor::VisitTypeAliasDecl(clang::TypeAliasDecl *declaration) {
  TypeAliasData type_data;

  auto qual_type = ast_context->getTypeDeclType(declaration);
  type_data.is_function_pointer = isFunctionPointer(ast_context, qual_type);

  TypeDescriptor type_desc;
  type_desc.type = TypeDescriptor::DescriptorType::TypeAlias;
  type_desc.data = type_data;
  type_desc.name = declaration->getNameAsString();
  type_desc.location =
      GetSourceCodeLocation(ast_context, source_manager, declaration);

  data.type_index[type_desc.name].insert(type_desc.location);
  data.types.insert({type_desc.location, std::move(type_desc)});
  return true;
}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *declaration) {
  FunctionDescriptor new_function;
  new_function.name = declaration->getNameAsString();
  new_function.location =
      GetSourceCodeLocation(ast_context, source_manager, declaration);
  new_function.is_variadic = declaration->isVariadic();

  Type field_type = {};
  field_type.name = declaration->getReturnType().getAsString();

  new_function.return_type = field_type;

  clang::PrintingPolicy printing_policy(ast_context->getLangOpts());
  std::size_t unnamed_variable_counter = 0U;

  for (const auto &parameter : declaration->parameters()) {
    Type parameter_type = {};
    auto base_type_id = parameter->getType().getBaseTypeIdentifier();
    if (base_type_id != nullptr) {
      parameter_type.name = base_type_id->getName().data();
    }

    if (parameter_type.name.empty()) {
#if LLVM_MAJOR_VERSION > 5
      parameter_type.name = clang::QualType::getAsString(
          parameter->getType().getNonReferenceType().split(), printing_policy);
#else
      parameter_type.name = clang::QualType::getAsString(
          parameter->getType().getNonReferenceType().split());
#endif
    }

    std::string parameter_name = parameter->getNameAsString().data();
    if (parameter_name.empty()) {
      std::stringstream string_helper;
      string_helper << "unnamed" << unnamed_variable_counter;
      unnamed_variable_counter++;

      parameter_name = string_helper.str();
    }

    parameter_type.is_function_pointer =
        isFunctionPointer(ast_context, parameter->getType());

    new_function.parameters[parameter_name] = parameter_type;
  }

  data.function_index[new_function.name].insert(new_function.location);
  data.functions.insert({new_function.location, std::move(new_function)});
  return true;
}
}  // namespace trailofbits
