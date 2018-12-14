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

#include <iostream>

int main(int argc, char *argv[], char *envp[]) {
  static_cast<void>(envp);

  ProfileManagerRef profile_manager;
  auto status = ProfileManager::create(profile_manager);
  if (!status.succeeded()) {
    std::cerr << status.toString();
    return 1;
  }

  LanguageManager language_manager;

  CLI::App cmdline_parser{"McSema ABI library generator"};
  CommandLineOptions cmdline_options;

  CommandMap command_map;
  initializeCommandLineParser(cmdline_parser, cmdline_options, profile_manager,
                              language_manager, command_map);

  CLI11_PARSE(cmdline_parser, argc, argv);

  for (const auto &p : command_map) {
    const auto &subcommand = p.first;
    const auto &callback = p.second;

    if (cmdline_parser.got_subcommand(subcommand)) {
      auto succeeded =
          callback(profile_manager, language_manager, cmdline_options);
      return (succeeded ? 0 : 1);
    }
  }

  return 1;
}
