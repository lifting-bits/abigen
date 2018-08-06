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

#include <trailofbits/srcparser/isourcecodeparser.h>
#include <trailofbits/srcparser/macros.h>

#include <clang/Frontend/CompilerInstance.h>

namespace trailofbits {
class SourceCodeParser final : public ISourceCodeParser {
 public:
  SourceCodeParser();
  virtual ~SourceCodeParser();

  virtual Status processFile(
      std::unordered_map<std::string, FunctionType> &functions,
      std::unordered_set<std::string> &blacklisted_functions,
      const std::string &path,
      const SourceCodeParserSettings &settings) const override;

  virtual Status processBuffer(
      std::unordered_map<std::string, FunctionType> &functions,
      std::unordered_set<std::string> &blacklisted_functions,
      const std::string &buffer,
      const SourceCodeParserSettings &settings) const override;

 private:
  Status createCompilerInstance(
      std::unique_ptr<clang::CompilerInstance> &compiler,
      const SourceCodeParserSettings &settings) const;

  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  SourceCodeParser &operator=(const SourceCodeParser &other) = delete;
  SourceCodeParser(const SourceCodeParser &other) = delete;
};

SRCPARSER_PUBLICSYMBOL ISourceCodeParser::Status CreateSourceCodeParser(
    std::unique_ptr<ISourceCodeParser> &obj);
}  // namespace trailofbits
