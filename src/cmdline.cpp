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

void initializeCommandLineParser(CLI::App &cmdline_parser,
                                 CommandLineOptions &cmdline_options,
                                 ProfileManagerRef &profile_manager,
                                 LanguageManager &language_manager,
                                 CommandMap &command_map) {
  cmdline_parser.require_subcommand();

  //
  // Initialize the 'version' command
  //

  auto version_cmd =
      cmdline_parser.add_subcommand("version", "Prints the abigen version");

  command_map.insert({version_cmd, versionCommandHandler});

  //
  // Initialize the 'generate' command
  //

  auto generate_cmd =
      cmdline_parser.add_subcommand("generate", "Generate an ABI library");

  // The profile determines the options and include folders we will use when
  // parsing the include headers
  auto profile_option =
      generate_cmd->add_option("-p,--profile", cmdline_options.profile_name,
                               "Profile name; use the list_profiles command to "
                               "list the available options");

  profile_option->required(true)->take_last();

  // clang-format off
  profile_option->check(
      [&profile_manager](const std::string &profile_name) -> std::string {
        Profile profile;
        auto status = profile_manager->get(profile, profile_name);
        if (!status.succeeded()) {
          return status.message();
        }

        return "";
      }
  );
  // clang-format on

  /// The language used to parse the include headers
  auto language_option =
      generate_cmd->add_option("-l,--language", cmdline_options.language,
                               "Language name; use the list_languages command "
                               "to list the available options");

  language_option->required(true)->take_last();

  // clang-format off
  language_option->check(
      [&language_manager](const std::string &definition) -> std::string {
        Language language;
        int standard;
        if (!language_manager.parseLanguageDefinition(language, standard, definition)) {
          return "Invalid language";
        }

        return "";
      }
  );
  // clang-format on

  generate_cmd
      ->add_flag("-x,--enable-gnu-extensions",
                 cmdline_options.enable_gnu_extensions, "Enable GNU extensions")
      ->take_last();

  generate_cmd
      ->add_flag("-z,--use-visual-cxx-mangling",
                 cmdline_options.use_visual_cxx_mangling,
                 "Use Visual C++ name mangling")
      ->take_last();

  generate_cmd->add_option("-i,--include-search-paths",
                           cmdline_options.additional_include_folders,
                           "Additional include folders");

  // The folder to scan for include files
  generate_cmd
      ->add_option("-f,--header-folders", cmdline_options.header_folders,
                   "Header folders")
      ->required();

  // Include files that will always be added inside the ABI library
  generate_cmd->add_option(
      "-b,--base-includes", cmdline_options.base_includes,
      "Includes that should always be present in the ABI header");

  // Where the output should be saved
  generate_cmd
      ->add_option("-o,--output", cmdline_options.output,
                   "Output path, including the file name without the extension")
      ->required();

  command_map.insert({generate_cmd, generateCommandHandler});

  //
  // Initialize the 'list_profiles' command
  //

  auto list_profiles_cmd = cmdline_parser.add_subcommand(
      "list_profiles", "List the available profiles");

  list_profiles_cmd->add_flag("-v,--verbose",
                              cmdline_options.verbose_profile_list,
                              "Show a more verbose profile list");

  command_map.insert({list_profiles_cmd, listProfilesCommandHandler});

  //
  // Initialize the 'list_languages' command
  //

  auto list_languages_cmd = cmdline_parser.add_subcommand(
      "list_languages", "List the available languages");

  command_map.insert({list_languages_cmd, listLanguagesCommandHandler});
}
