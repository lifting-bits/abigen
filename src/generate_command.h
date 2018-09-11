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

#include "types.h"

/// This structure contains the header name and the possible prefixes
/// to use when including it
struct HeaderDescriptor final {
  /// The header name (i.e.: Utils.h)
  std::string name;

  /// The list of possible prefixes. Take for example clang/Frontend/Utils.h
  /// Possible prefixes are "clang/Frontend" and "Frontend". abigen will try
  /// to find a prefix that will not cause a compile-time error by attempting
  /// to include the Utils.h header using each possible prefix:
  ///
  ///   #include "clang/Frontend/Utils.h"
  ///   #include "Frontend/Utils.h"
  StringList possible_prefixes;
};
