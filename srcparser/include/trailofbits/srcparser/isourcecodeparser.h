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
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace trailofbits {
struct SourceCodeParserSettings final {
  bool cpp{true};
  std::size_t standard{11};
  bool enable_gnu_extensions{true};

  std::vector<std::string> cxx_system;
  std::vector<std::string> c_system;
  std::string resource_dir;

  std::vector<std::string> additional_include_folders;
};

struct FunctionType final {
  std::string name;
  std::string return_type;
  std::map<std::string, std::string> parameters;
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
      std::unordered_map<std::string, FunctionType> &functions,
      std::unordered_set<std::string> &blacklisted_functions,
      const std::string &path,
      const SourceCodeParserSettings &settings) const = 0;

  virtual Status processBuffer(
      std::unordered_map<std::string, FunctionType> &functions,
      std::unordered_set<std::string> &blacklisted_functions,
      const std::string &buffer,
      const SourceCodeParserSettings &settings) const = 0;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj);
}  // namespace trailofbits
