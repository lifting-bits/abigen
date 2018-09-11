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

#include <iomanip>

/// Handler for the 'list_profiles' command
bool listProfilesCommandHandler(ProfileManagerRef &profile_manager,
                                const LanguageManager &language_manager,
                                const CommandLineOptions &cmdline_options) {
  static_cast<void>(language_manager);

  std::cout << "Profile list\n\n";

  profile_manager->enumerate<bool>(
      [](const Profile &profile, bool verbose) -> bool {
        if (verbose) {
          std::cout << "  Name: " << profile.name << "\n";
          std::cout << "    Root path: " << profile.root_path << "\n";
          std::cout << "    Resource directory: " << profile.resource_dir
                    << "\n\n";

          std::cout << "    externc-isystem\n";
          for (const auto &p : profile.internal_externc_isystem) {
            const auto &language = p.first;
            const auto &path_list = p.second;

            std::cout << "      " << language << "\n";
            for (const auto &path : path_list) {
              std::cout << "        " << path << "\n";
            }
          }
          std::cout << "\n";

          std::cout << "    isystem\n";
          for (const auto &p : profile.internal_isystem) {
            const auto &language = p.first;
            const auto &path_list = p.second;

            std::cout << "      " << language << "\n";
            for (const auto &path : path_list) {
              std::cout << "        " << path << "\n";
            }
          }

          std::cout << "\n\n";

        } else {
          std::cout << "  " << profile.name << "\n";
        }

        return true;
      },

      cmdline_options.verbose_profile_list);

  return true;
}

/// Handler for the 'list_languages' command
bool listLanguagesCommandHandler(ProfileManagerRef &profile_manager,
                                 const LanguageManager &language_manager,
                                 const CommandLineOptions &cmdline_options) {
  static_cast<void>(profile_manager);
  static_cast<void>(cmdline_options);

  std::cout << "Supported languages\n\n";

  language_manager.enumerate<void *>(
      [](const std::string &definition, Language language, int standard,
         void *) -> bool {
        std::cout << "  " << std::setw(3) << std::setfill(' ') << language
                  << " " << standard << " (" << definition << ")\n";
        return true;
      },

      nullptr);

  return true;
}
