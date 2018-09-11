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

#include <string>
#include <unordered_map>
#include <vector>

/// A simple string list
using StringList = std::vector<std::string>;

/// This structure is used to hold a location within the source code
struct SourceCodeLocation final {
  /// The absolute file path, except for the main file (the generated buffer)
  std::string file_path;

  /// The line
  std::uint32_t line;

  /// The column
  std::uint32_t column;
};

/// Describes a blacklisted function
struct BlacklistedFunction final {
  /// The location for this symbol
  SourceCodeLocation location;

  /// All the possible reasons why a function is blacklisted
  enum class Reason { Variadic, FunctionPointer, DuplicateName };

  /// Function name
  std::string name;

  /// Blacklist reason
  Reason reason;
};

/// Describes a whitelisted function
struct WhitelistedFunction final {
  /// The location for this symbol
  SourceCodeLocation location;

  /// Function name
  std::string name;
};

/// State data for the ABI library
struct ABILibraryState final {
  /// Functions that have been blacklisted
  std::unordered_map<std::string, BlacklistedFunction>
      blacklisted_function_list;

  /// Functions that will appear in the final ABI library
  std::unordered_map<std::string, WhitelistedFunction>
      whitelisted_function_list;

  /// Headers that have been successfully included
  StringList header_list;
};
