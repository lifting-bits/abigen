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

#include "languagemanager.h"
#include <string>

namespace {
// clang-format off
/// This map contains the language and version combinations supported
/// by abigen, and is also used to build the help message
const LanguageMap language_descriptors = {
    {"c89", {Language::C, 89}},
    {"c94", {Language::C, 94}},
    {"c99", {Language::C, 99}},
    {"c11", {Language::C, 11}}

    /*
      Keep C++ disabled until we actually add support for it

      {"cxx98", {Language::CXX, 98}},
      {"cxx11", {Language::CXX, 11}},
      {"cxx14", {Language::CXX, 14}}
    */
};
// clang-format on
}  // namespace

bool LanguageManager::parseLanguageDefinition(
    Language &language, int &standard, const std::string &definition) const {
  auto it = language_descriptors.find(definition);
  if (it == language_descriptors.end()) {
    return false;
  }

  const auto &descriptor = it->second;
  language = descriptor.language;
  standard = descriptor.standard;

  return true;
}

const LanguageMap &LanguageManager::languageMap() const {
  return language_descriptors;
}

std::ostream &operator<<(std::ostream &stream, const Language &language) {
  switch (language) {
    case Language::C: {
      stream << "C";
      break;
    }

    case Language::CXX: {
      stream << "C++";
      break;
    }
  }

  static_cast<void>(stream);
  static_cast<void>(language);

  return stream;
}
