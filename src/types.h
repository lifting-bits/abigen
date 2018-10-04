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

#include <map>
#include <string>
#include <unordered_map>
#include <variant>
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
  /// All the possible reasons why a function is blacklisted
  enum class Reason { Variadic, FunctionPointer, DuplicateName, Templated };

  /// If this function was blacklisted due to name duplication, this type
  /// can be used to get the location of the other functions
  using DuplicateFunctionLocations = std::vector<SourceCodeLocation>;

  /// If this function was blacklisted due to function pointer usage, this
  /// structure track the location of all the conflicting types
  using FunctionPointerLocations =
      std::vector<std::pair<SourceCodeLocation, std::string>>;

  /// The location for this symbol
  SourceCodeLocation location;

  /// Friendly function name
  std::string friendly_name;

  /// Mangled function name
  std::string mangled_name;

  /// Blacklist reason
  Reason reason;

  /// Additional reason information
  std::variant<DuplicateFunctionLocations, FunctionPointerLocations>
      reason_data;
};

/// List of the blacklisted functions
using BlacklistedFunctionList = std::vector<BlacklistedFunction>;

/// Describes a whitelisted function
struct WhitelistedFunction final {
  /// The location for this symbol
  SourceCodeLocation location;

  /// Friendly function name
  std::string friendly_name;

  /// Mangled function name
  std::string mangled_name;
};

/// List of whitelisted functions
using WhitelistedFunctionList = std::vector<WhitelistedFunction>;

/// ABI library contents
struct ABILibrary final {
  /// Functions that have been blacklisted
  BlacklistedFunctionList blacklisted_function_list;

  /// Functions that will appear in the final ABI library
  WhitelistedFunctionList whitelisted_function_list;

  /// Headers that have been successfully included
  StringList header_list;
};
