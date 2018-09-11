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

#include <iostream>
#include <map>
#include <string>

/// Language type
enum class Language { C, CXX };

/// This structure is used to describe the list of supported
/// language/version pairs, such as C++11 or C89. It is used to map
/// the language name (like cxx14) to the correct language and
/// standard combination
struct LanguageDescriptor final {
  /// Language (either C or C++)
  Language language;

  /// The language standard, like 11 or 14 for C++11 and C++14. Look
  /// at the language_descriptors variable for accepted types
  int standard;
};

/// Language map
using LanguageMap = std::map<std::string, LanguageDescriptor>;

/// Language manager
class LanguageManager final {
 public:
  /// Constructor
  LanguageManager() = default;

  /// Destructor
  ~LanguageManager() = default;

  /// Parses the given language definition, extracting the language type and
  /// standard
  bool parseLanguageDefinition(Language &language, int &standard,
                               const std::string &definition) const;

  /// Enumerates each supported language
  template <typename T>
  void enumerate(bool (*callback)(const std::string &definition,
                                  Language language, int standard, T),
                 T user_defined) const;

 private:
  /// Private accessor used by LanguageManager::enumerate
  const LanguageMap &languageMap() const;
};

template <typename T>
void LanguageManager::enumerate(bool (*callback)(const std::string &definition,
                                                 Language language,
                                                 int standard, T),
                                T user_defined) const {
  for (const auto &p : languageMap()) {
    const auto &definition = p.first;
    const auto &descriptor = p.second;
    if (!callback(definition, descriptor.language, descriptor.standard,
                  user_defined)) {
      break;
    }
  }
}

/// Prints the Language enum value to the specified stream
std::ostream &operator<<(std::ostream &stream, const Language &language);
