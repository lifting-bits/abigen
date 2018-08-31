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

#pragma once

#include "istatus.h"
#include "macros.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace trailofbits {
struct SourceCodeLocation final {
  std::string file_path;
  std::uint32_t line;
  std::uint32_t column;

  bool operator<(const SourceCodeLocation &other) const;
};

struct Type final {
  std::string name;
  bool is_function_pointer{false};
};

struct TypeAliasData final {
  bool is_function_pointer{false};
};

struct RecordData {
  std::map<std::string, Type> members;
};

struct CXXRecordData final : public RecordData {};

using TypeDescriptorData =
    std::variant<RecordData, CXXRecordData, TypeAliasData>;

struct TypeDescriptor final {
  enum class DescriptorType { Record, CXXRecord, TypeAlias };

  DescriptorType type;
  TypeDescriptorData data;

  std::string name;
  SourceCodeLocation location;
};

struct FunctionDescriptor final {
  SourceCodeLocation location;

  Type return_type;
  std::string name;
  std::map<std::string, Type> parameters;

  bool is_variadic{false};
};

using TypeDescriptorMap = std::map<SourceCodeLocation, TypeDescriptor>;
using FunctionDescriptorMap = std::map<SourceCodeLocation, FunctionDescriptor>;

using NameIndex = std::unordered_map<std::string, std::set<SourceCodeLocation>>;

struct TranslationUnitData final {
  TypeDescriptorMap types;
  FunctionDescriptorMap functions;

  NameIndex type_index;
  NameIndex function_index;
};

struct SourceCodeParserSettings final {
  bool cpp{true};
  std::size_t standard{11};
  bool enable_gnu_extensions{true};

  std::vector<std::string> internal_isystem;
  std::vector<std::string> internal_externc_isystem;
  std::string resource_dir;

  std::vector<std::string> additional_include_folders;
};

class SRCPARSER_PUBLICSYMBOL ISourceCodeParser {
 public:
  enum class StatusCode {
    OpenFailed,
    MemoryAllocationFailure,
    ParsingError,
    SucceededWithWarnings,
    InvalidLanguageStandard,
    Unknown
  };

  using Status = IStatus<StatusCode>;

  virtual ~ISourceCodeParser() = default;

  virtual Status processFile(
      TranslationUnitData &data, const std::string &path,
      const SourceCodeParserSettings &settings) const = 0;

  virtual Status processBuffer(
      TranslationUnitData &data, const std::string &buffer,
      const SourceCodeParserSettings &settings) const = 0;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj);
}  // namespace trailofbits
