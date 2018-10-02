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

#include "languagemanager.h"
#include "profilemanager.h"

#include <unordered_set>

#include <CLI/CLI.hpp>

/// Command line options
struct CommandLineOptions final {
  /// The profile to use when generating the ABI library
  std::string profile_name;

  /// The language used to parse the include headers
  std::string language;

  /// If true, gnu extensions will be enabled when parsing the include files
  bool enable_gnu_extensions{false};

  /// The primary folder that will be scanned for include files
  std::vector<std::string> header_folders;

  /// Include files that should always be added at the top of the ABI library
  std::vector<std::string> base_includes;

  /// The ABI library will be saved with this name, adding .cpp and .h at the
  /// end of the file name
  std::string output;

  /// If true, show a verbose list when printing the profile list
  bool verbose_profile_list{false};

  /// Additional include directories
  std::vector<std::string> additional_include_folders;

  /// If true, name mangling will follow the Microsoft Visual C++ convention
  /// instead of the standard one
  bool use_visual_cxx_mangling{false};
};

/// Command handler
using CommandHandler = bool (*)(ProfileManagerRef &profile_manager,
                                const LanguageManager &language_manager,
                                const CommandLineOptions &cmdline_options);

/// The command map is used to dispatch the user command to the right callback
using CommandMap = std::unordered_map<CLI::App *, CommandHandler>;

/// Handler for the 'version" command
bool versionCommandHandler(ProfileManagerRef &profile_manager,
                           const LanguageManager &language_manager,
                           const CommandLineOptions &cmdline_options);

/// Handler for the 'generate' command
bool generateCommandHandler(ProfileManagerRef &profile_manager,
                            const LanguageManager &language_manager,
                            const CommandLineOptions &cmdline_options);

/// Handler for the 'list_profiles' command
bool listProfilesCommandHandler(ProfileManagerRef &profile_manager,
                                const LanguageManager &language_manager,
                                const CommandLineOptions &cmdline_options);

/// Handler for the 'list_languages" command
bool listLanguagesCommandHandler(ProfileManagerRef &profile_manager,
                                 const LanguageManager &language_manager,
                                 const CommandLineOptions &cmdline_options);

/// Initializes the command line parser
void initializeCommandLineParser(CLI::App &cmdline_parser,
                                 CommandLineOptions &cmdline_options,
                                 ProfileManagerRef &profile_manager,
                                 LanguageManager &language_manager,
                                 CommandMap &command_map);
