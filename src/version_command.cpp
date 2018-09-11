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

#include "cmdline.h"

bool versionCommandHandler(ProfileManagerRef &profile_manager,
                           const LanguageManager &language_manager,
                           const CommandLineOptions &cmdline_options) {
  static_cast<void>(profile_manager);
  static_cast<void>(language_manager);
  static_cast<void>(cmdline_options);

  static const std::string commit_description = ABIGEN_COMMIT_DESCRIPTION;
  static const std::string commit_hash = ABIGEN_COMMIT_HASH;
  static const std::string branch_name = ABIGEN_BRANCH_NAME;

  std::cerr << "abigen ";
  std::cerr << commit_hash << "@" << branch_name;

  if (commit_description != commit_hash) {
    std::cerr << " (" << commit_description << ")";
  }

  std::cerr << "\nCopyright (C) 2018 Trail of Bits, Inc.\n";
  std::cerr << "Licensed under the Apache License, Version 2.0\n";

  return true;
}
