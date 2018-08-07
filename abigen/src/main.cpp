/*
 * Copyright (c) 2018 Trail of Bits, Inc.
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

#include <trailofbits/srcparser/isourcecodeparser.h>

#include <algorithm>

#if __has_include(<filesystem>)
// A compiler released after C++17 has been finalized

#include <filesystem>
namespace stdfs = std::filesystem;

#elif __has_include(<experimental/filesystem>)
// A compiler released when C++17 was a draft

#include <experimental/filesystem>
namespace stdfs = std::experimental::filesystem;

#else  // __has_include(<filesystem>)
// A compiler without C++17
// which may have already failed at the #if __has_include(...) directive
#error An implementation of std::filesystem is required (are you using C++17?)
#endif

#include <iostream>
#include <unordered_map>

#include <CLI/CLI.hpp>
#include <json11.hpp>

/// A simple list of strings
using StringList = std::vector<std::string>;

/// Language type
enum class Language { C, CXX };

/// Prints the language name
std::ostream &operator<<(std::ostream &stream, const Language &language) {
  stream << (language == Language::C ? "C" : "C++");
  return stream;
}

/// A profile is a collection of headers and default compiler settings
/// taken from a standard OS installation
struct Profile final {
  /// This is usually the distribution name and version
  std::string name;

  /// Where the root folder is located, containing the default headers
  /// and settings
  std::string root_path;

  /// The location for the clang resource directory
  std::string resource_dir;

  /// Default isystem parameters
  std::unordered_map<Language, StringList> internal_isystem;

  /// Default externc_isystem parameters
  std::unordered_map<Language, StringList> internal_externc_isystem;
};

/// This structure is used to describe the list of supported
/// language/version pairs, such as C++11 or C89. It is used to map
/// the language name (like cxx14) to the correct settings that will
/// be passed to the libsourcecodeparser component
struct LanguageDescriptor final {
  /// Language (either C or C++)
  Language language;

  /// The language standard, like 11 or 14 for C++11 and C++14. Look
  /// at the language_descriptors variable for accepted types
  int standard;
};

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
  std::vector<std::string> possible_prefixes;
};

/// This is the list of discovered profiles, built scanning the `data`
/// folder. abigen will prefer to use the `data` folder in the working
/// directory if possible, but will default to the system-wide one if
/// it is not found
std::unordered_map<std::string, Profile> profile_descriptors;

/// The selected profiles root path
std::string profiles_root;

/// This map contains the language and version combinations supported
/// by abigen, and is also used to build the help message
const std::map<std::string, LanguageDescriptor> language_descriptors = {
    {"c89", {Language::C, 89}},     {"c94", {Language::C, 94}},
    {"c99", {Language::C, 99}},     {"c11", {Language::C, 11}},

    {"cxx98", {Language::CXX, 98}}, {"cxx11", {Language::CXX, 11}},
    {"cxx14", {Language::CXX, 14}}};

/// Loads the profile specified by the given path
bool loadProfile(Profile &profile, const stdfs::path &path) {
  profile = {};

  std::ifstream profile_file(path.string());
  std::string json_profile((std::istreambuf_iterator<char>(profile_file)),
                           std::istreambuf_iterator<char>());

  std::string error_messages;
  const auto json = json11::Json::parse(json_profile, error_messages);
  if (json == json11::Json()) {
    return false;
  }

  profile.root_path = path.parent_path();

  if (!json["name"].is_string()) {
    return false;
  }

  profile.name = json["name"].string_value();

  if (!json["resource-dir"].is_string()) {
    return false;
  }

  profile.resource_dir = json["resource-dir"].string_value();

  if (!json["c"].is_object()) {
    return false;
  }

  if (!json["c++"].is_object()) {
    return false;
  }

  auto L_getSettingsFromLanguageSection =
      [](StringList &internal_isystem, StringList &internal_externc_isystem,
         const json11::Json &object) -> bool {
    if (!object["internal-isystem"].is_array() ||
        !object["internal-externc-isystem"].is_array()) {
      return false;
    }

    for (const auto &item : object["internal-isystem"].array_items()) {
      if (!item.is_string()) {
        return false;
      }

      internal_isystem.push_back(item.string_value());
    }

    for (const auto &item : object["internal-externc-isystem"].array_items()) {
      if (!item.is_string()) {
        return false;
      }

      internal_externc_isystem.push_back(item.string_value());
    }

    return true;
  };

  // Load the C settings
  auto &c_settings = json["c"];

  StringList c_internal_isystem;
  StringList c_internal_externc_isystem;
  if (!L_getSettingsFromLanguageSection(
          c_internal_isystem, c_internal_externc_isystem, c_settings)) {
    return false;
  }

  profile.internal_isystem.insert({Language::C, c_internal_isystem});
  profile.internal_externc_isystem.insert(
      {Language::C, c_internal_externc_isystem});

  // Load the C++ settings
  auto &cpp_settings = json["c++"];

  StringList cpp_internal_isystem;
  StringList cpp_internal_externc_isystem;
  if (!L_getSettingsFromLanguageSection(
          cpp_internal_isystem, cpp_internal_externc_isystem, cpp_settings)) {
    return false;
  }

  profile.internal_isystem.insert({Language::CXX, cpp_internal_isystem});
  profile.internal_externc_isystem.insert(
      {Language::CXX, cpp_internal_externc_isystem});

  return true;
}

/// Locates the closest `data` folder (either at the current working directory
/// or at the system-wide install location)
bool getProfilesRootPath(std::string &path) {
  path.clear();

  auto profiles_root = stdfs::current_path() / "data" / "platforms";
  std::error_code error;
  if (stdfs::exists(profiles_root, error)) {
    path = profiles_root.string();
    return true;
  }

  profiles_root = stdfs::path(PROFILE_INSTALL_FOLDER) / "data" / "platforms";
  if (stdfs::exists(profiles_root, error)) {
    path = profiles_root.string();
    return true;
  }

  return false;
}

/// Enumerates all the profiles found in the data directory
bool enumerateProfiles(std::unordered_map<std::string, Profile> &profile_list) {
  profile_list = {};

  try {
    std::unordered_map<std::string, Profile> output;

    stdfs::recursive_directory_iterator it(profiles_root);
    for (const auto &p : it) {
      if (!stdfs::is_regular_file(p.path()) ||
          p.path().filename().string() != "profile.json") {
        continue;
      }

      auto current_path = p.path();

      Profile profile;
      if (!loadProfile(profile, current_path)) {
        continue;
      }

      if (profile_list.find(profile.name) != profile_list.end()) {
        return false;
      }

      output.insert({profile.name, profile});
    }

    profile_list = std::move(output);
    output.clear();

    return true;

  } catch (...) {
    return false;
  }
}

/// Loads all the profiles from the data directory
bool loadProfiles() {
  if (!enumerateProfiles(profile_descriptors)) {
    std::cerr << "Failed to load the profiles\n";
    return false;
  }

  if (profile_descriptors.empty()) {
    std::cerr << "No profile found!\n";
    return false;
  }

  return true;
}

/// Generates the libsourcecodeparser settings from the given profile
trailofbits::SourceCodeParserSettings getParserSettingsFromFile(
    const Profile &profile, Language language) {
  trailofbits::SourceCodeParserSettings settings = {};

  auto L_appendPaths = [](std::vector<std::string> &destination,
                          const std::vector<std::string> &source,
                          const std::string &base_path) -> void {
    for (const auto &raw_path : source) {
      auto path = stdfs::path(base_path) / raw_path;
      destination.push_back(path.string());
    }
  };

  auto it = profile.internal_isystem.find(language);
  if (it != profile.internal_isystem.end()) {
    const auto &internal_isystem = it->second;
    L_appendPaths(settings.internal_isystem, internal_isystem,
                  profile.root_path);
  }

  it = profile.internal_externc_isystem.find(language);
  if (it != profile.internal_externc_isystem.end()) {
    const auto &internal_externc_isystem = it->second;
    L_appendPaths(settings.internal_externc_isystem, internal_externc_isystem,
                  profile.root_path);
  }

  auto resource_directory =
      stdfs::path(profile.root_path) / profile.resource_dir;
  settings.resource_dir = resource_directory.string();

  return settings;
}

/// Generates a compilable source code buffer that includes all the
/// given headers
std::string generateSourceBuffer(
    const std::vector<std::string> &include_list,
    const std::vector<std::string> &base_includes) {
  std::stringstream buffer;
  for (const auto &include : base_includes) {
    buffer << "#include <" << include << ">\n";
  }

  for (const auto &include : include_list) {
    buffer << "#include <" << include << ">\n";
  }

  return buffer.str();
}

/// Recursively enumerates all the include files found in the given folder
bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const std::string &header_folder) {
  const static std::vector<std::string> valid_extensions = {".h", ".hh", ".hp",
                                                            ".hpp", ".hxx"};

  auto root_header_folder = stdfs::absolute(header_folder);

  try {
    stdfs::recursive_directory_iterator it(root_header_folder);
    for (const auto &directory_entry : it) {
      if (!stdfs::is_regular_file(directory_entry.path())) {
        continue;
      }

      auto path = directory_entry.path();

      const auto &ext = path.extension().string();
      if (ext.empty() ||
          std::find(valid_extensions.begin(), valid_extensions.end(), ext) ==
              valid_extensions.end()) {
        continue;
      }

      HeaderDescriptor header_desc = {};
      header_desc.name = path.filename();

      for (auto parent_path = path.parent_path();
           !parent_path.empty() && parent_path != parent_path.root_path();
           parent_path = parent_path.parent_path()) {
        if (parent_path == root_header_folder) {
          break;
        }

        auto current_relative_path =
            parent_path.filename() /
            (header_desc.possible_prefixes.empty()
                 ? ""
                 : header_desc.possible_prefixes.back());
        header_desc.possible_prefixes.push_back(current_relative_path.string());
      }

      header_files.push_back(header_desc);
    }

    return true;

  } catch (...) {
    return false;
  }
}

/// Recursively enumerates all the include files found in the given folder list
bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const std::vector<std::string> &header_folders) {
  header_files = {};

  for (const auto &folder : header_folders) {
    if (!enumerateIncludeFiles(header_files, folder)) {
      std::cerr << "Failed to enumerate the include files in the following "
                   "directory: "
                << folder << "\n";

      return false;
    }
  }

  return true;
}

/// Generates an ABI library (both the header and the implementation files)
/// using the given settings
void generateABILibrary(
    std::string &header, std::string &implementation,
    const std::vector<std::string> &include_list,
    const std::unordered_map<std::string, trailofbits::FunctionType> &functions,
    const std::string &abi_include_name) {
  std::stringstream output;

  // Generate the header
  for (const auto &header : include_list) {
    output << "#include <" << header << ">\n";
  }

  header = output.str();

  // Generate the implementation file
  output.str("");
  output << "#include \"" << abi_include_name << "\"\n\n";

  output << "__attribute__((used))\n";
  output << "void *__mcsema_externs[] = {\n";

  // The following functions will be skipped:
  // 1. Overloads
  // 2. Functions that accept parameters (as they could be callbacks)
  // 3. Functions that accept a variable amount of parameters (varargs)
  for (auto function_it = functions.begin(); function_it != functions.end();
       function_it++) {
    const auto &function = function_it->second;

    output << "  (void *)(" << function.name << ")";
    if (std::next(function_it, 1) != functions.end()) {
      output << ",";
    }

    output << "\n";
  }

  output << "};";
  implementation = output.str();
}

/// Validates and converts the language label shown by the --help message
void parseLanguageName(bool &is_cpp, std::size_t &standard,
                       const std::string &valid_language_name) {
  std::string standard_version;

  if (valid_language_name.find("cxx") == 0) {
    is_cpp = true;
    standard_version = valid_language_name.substr(3);
  } else {
    is_cpp = false;
    standard_version = valid_language_name.substr(1);
  }

  standard = static_cast<std::size_t>(
      std::strtoul(standard_version.data(), nullptr, 10));
}

/// Entry point for the 'list_profiles' command
void listProfilesCommandHandler(bool verbose) {
  std::cout << "Profile list\n";

  if (verbose) {
    std::cout << "\n";

    for (const auto &p : profile_descriptors) {
      const auto &description = p.second;

      std::cout << "  Name: " << description.name << "\n";
      std::cout << "    Root path: " << description.root_path << "\n";
      std::cout << "    Resource directory: " << description.resource_dir
                << "\n\n";

      std::cout << "    externc-isystem\n";
      for (const auto &p : description.internal_externc_isystem) {
        const auto &language = p.first;
        const auto &path_list = p.second;

        std::cout << "      " << language << "\n";
        for (const auto &path : path_list) {
          std::cout << "        " << path << "\n";
        }
      }
      std::cout << "\n";

      std::cout << "    isystem\n";
      for (const auto &p : description.internal_isystem) {
        const auto &language = p.first;
        const auto &path_list = p.second;

        std::cout << "      " << language << "\n";
        for (const auto &path : path_list) {
          std::cout << "        " << path << "\n";
        }
      }
      std::cout << "\n\n";
    }

  } else {
    for (const auto &p : profile_descriptors) {
      std::cout << "  " << p.second.name << "\n";
    }
  }
}

/// Entry point for the 'list_languages' command
void listLanguagesCommandHandler() {
  std::cout << "Supported languages\n";

  for (const auto &p : language_descriptors) {
    const auto &language = p.first;
    std::cout << "  " << language << "\n";
  }
}

/// This is the entry point for the 'generate' command
int generateCommandHandler(
    const std::string &profile, const std::string &language,
    bool enable_gnu_extensions,
    const std::vector<std::string> &additional_include_folders,
    const std::vector<std::string> &header_folders,
    const std::vector<std::string> &base_includes,
    const std::string &output_file_path) {
  // Generate the settings for libsourcecodeparser
  bool is_cpp = false;
  std::size_t language_standard;
  parseLanguageName(is_cpp, language_standard, language);

  auto parser_settings = getParserSettingsFromFile(
      profile_descriptors.at(profile), is_cpp ? Language::CXX : Language::C);

  parser_settings.standard = language_standard;
  parser_settings.cpp = is_cpp;
  parser_settings.enable_gnu_extensions = enable_gnu_extensions;
  parser_settings.additional_include_folders = additional_include_folders;

  // Print a summary of the base settings we loaded from the profile
  std::cout << "Profile settings\n";
  std::cout << "  Name: " << profile << "\n";
  std::cout << "  Language: " << (is_cpp ? "C++" : "C") << language_standard
            << "\n";

  std::cout << "  GNU extensions: "
            << (enable_gnu_extensions ? "enabled" : "disabled") << "\n\n";

  std::cout << "  internal-isystem\n";
  for (const auto &path : parser_settings.internal_isystem) {
    std::cout << "    " << path << "\n";
  }
  std::cout << "\n";

  std::cout << "  internal-externc-isystem\n";
  for (const auto &path : parser_settings.internal_externc_isystem) {
    std::cout << "    " << path << "\n";
  }
  std::cout << "\n";

  std::cout << "  resource-dir: " << parser_settings.resource_dir << "\n\n";

  std::unique_ptr<trailofbits::ISourceCodeParser> parser;
  auto status = trailofbits::CreateSourceCodeParser(parser);
  if (!status.succeeded()) {
    std::cerr << status.toString() << "\n";
    return 1;
  }

  std::cerr << "Enumerating the include files...\n";
  std::vector<HeaderDescriptor> header_files;
  enumerateIncludeFiles(header_files, header_folders);

  std::cerr << "Processing the include headers...\n";
  std::vector<std::string> current_include_list;
  std::unordered_map<std::string, trailofbits::FunctionType> functions;
  std::unordered_set<std::string> blacklisted_functions;

  auto str_header_count = std::to_string(header_files.size());
  auto header_counter_size = static_cast<int>(str_header_count.size());

  // Attempt to include as many headers as we can, also trying different
  // prefix combinations (i.e.: "clang/Frontend/Utils.h", "Frontend/Utils.h",
  // ...)
  while (true) {
    bool new_headers_added = false;

    for (auto header_it = header_files.begin();
         header_it != header_files.end();) {
      const auto &current_header_desc = *header_it;

      std::vector<std::string> include_directives = {current_header_desc.name};
      for (const auto &include_prefix : current_header_desc.possible_prefixes) {
        auto relative_path =
            stdfs::path(include_prefix) / current_header_desc.name;

        include_directives.push_back(relative_path);
      }

      bool current_header_added = false;

      for (const auto &current_include_directive : include_directives) {
        auto temp_include_list = current_include_list;
        temp_include_list.push_back(current_include_directive);

        auto source_buffer =
            generateSourceBuffer(temp_include_list, base_includes);

        auto new_functions = functions;
        auto new_blacklist = blacklisted_functions;
        status = parser->processBuffer(new_functions, new_blacklist,
                                       source_buffer, parser_settings);

        if (status.succeeded()) {
          functions = std::move(new_functions);
          new_functions.clear();

          blacklisted_functions = std::move(new_blacklist);
          new_blacklist.clear();

          current_header_added = true;
          current_include_list.push_back(current_include_directive);

          break;
        }
      }

      if (!current_header_added) {
        header_it++;
        continue;
      }

      std::cerr << "[" << std::setfill(' ') << std::setw(header_counter_size)
                << current_include_list.size() << "/" << str_header_count
                << "] " << current_include_list.back() << "\n";
      header_it = header_files.erase(header_it);
      new_headers_added = true;
    }

    if (!new_headers_added) {
      break;
    }
  }

  std::cerr << "\n";

  if (!current_include_list.empty()) {
    std::cerr << "Headers imported:\n";

    for (const auto &header : current_include_list) {
      std::cerr << "  " << header << "\n";
    }

    std::cerr << "\n";
  }

  if (!header_files.empty()) {
    std::cerr << "The following headers could not be imported:\n";

    for (auto header_it = header_files.begin(); header_it != header_files.end();
         header_it++) {
      const auto &header = *header_it;
      std::cerr << "  File name: " << header.name << "\n";
      std::cerr << "  Prefixes tried: { ";

      for (auto prefix_it = header.possible_prefixes.begin();
           prefix_it != header.possible_prefixes.end(); prefix_it++) {
        std::cerr << "'" << *prefix_it << "'";

        if (std::next(prefix_it, 1) != header.possible_prefixes.end()) {
          std::cerr << ", ";
        }
      }

      std::cerr << " }\n\n";
    };
  }

  if (!blacklisted_functions.empty()) {
    std::cerr << "The following functions have been blacklisted: ";

    for (auto it = blacklisted_functions.begin();
         it != blacklisted_functions.end(); it++) {
      const auto &name = *it;
      std::cerr << name;

      if (std::next(it, 1) != blacklisted_functions.end()) {
        std::cerr << ", ";
      }
    }

    std::cerr << "\n";
  }

  auto header_file_path = output_file_path + ".h";
  auto implementation_file_path = output_file_path + ".cpp";

  std::string abi_lib_header;
  std::string abi_lib_implementation;
  generateABILibrary(abi_lib_header, abi_lib_implementation,
                     current_include_list, functions,
                     stdfs::path(header_file_path).filename());

  {
    std::ofstream header_file(header_file_path);
    header_file << abi_lib_header << "\n";

    std::ofstream implementation_file(implementation_file_path);
    implementation_file << abi_lib_implementation << "\n";
  }

  return 0;
}

/// Entry point!
int main(int argc, char *argv[], char *envp[]) {
  static_cast<void>(envp);

  // Get the profiles root folder; use the current directory when possible, then
  // look for the system-wide install directory
  if (!getProfilesRootPath(profiles_root)) {
    std::cerr << "No profiles folder found.\n";
    return 1;
  }

  std::cerr << "Using profiles from " << profiles_root << "\n\n";

  // load the profiles as soon as we start so that the argument parser can use
  // the list to validate user input
  loadProfiles();

  CLI::App app{"McSema ABI library generator"};
  app.require_subcommand();

  auto generate_cmd = app.add_subcommand("generate", "Generate an ABI library");

  std::string profile;
  auto profile_option =
      generate_cmd->add_option("-p,--profile", profile,
                               "Profile name; use the list_profiles command to "
                               "list the available options");

  profile_option->required(true)->take_last();

  // clang-format off
  profile_option->check(
      [](const std::string &profile) -> std::string {
        if (profile_descriptors.find(profile) == profile_descriptors.end()) {
          return "The specified profile could not be found";
        }

        return "";
      }
  );
  // clang-format on

  std::string language;
  auto language_option =
      generate_cmd->add_option("-l,--language", language,
                               "Language name; use the list_languages command "
                               "to list the available options");
  language_option->required(true)->take_last();

  bool enable_gnu_extensions = false;
  generate_cmd
      ->add_flag("-x,--enable-gnu-extensions", enable_gnu_extensions,
                 "Enable GNU extensions")
      ->take_last();

  // clang-format off
  language_option->check(
    [](const std::string &language) -> std::string {
      if (language_descriptors.find(language) == language_descriptors.end()) {
        return "Invalid language";
      }

      return "";
    }
  );
  // clang-format on

  std::vector<std::string> additional_include_folders;
  generate_cmd->add_option("-i,--include-search-paths",
                           additional_include_folders,
                           "Additional include folders");

  std::vector<std::string> header_folders;
  generate_cmd
      ->add_option("-f,--header-folders", header_folders, "Header folders")
      ->required();

  std::vector<std::string> base_includes;
  generate_cmd->add_option(
      "-b,--base-includes", base_includes,
      "Includes that should always be present in the ABI header");

  std::string output;
  generate_cmd
      ->add_option("-o,--output", output,
                   "Output path, including the file name without the extension")
      ->required();

  auto list_profiles_cmd =
      app.add_subcommand("list_profiles", "List the available profiles");

  bool verbose_profile_list = false;
  list_profiles_cmd->add_flag("-v,--verbose", verbose_profile_list,
                              "Show a more verbose profile list");

  auto list_languages_cmd =
      app.add_subcommand("list_languages", "List the available languages");

  CLI11_PARSE(app, argc, argv);

  additional_include_folders.insert(additional_include_folders.end(),
                                    header_folders.begin(),
                                    header_folders.end());

  if (app.got_subcommand(list_profiles_cmd)) {
    listProfilesCommandHandler(verbose_profile_list);
    return 0;

  } else if (app.got_subcommand(list_languages_cmd)) {
    listLanguagesCommandHandler();
    return 0;

  } else if (app.got_subcommand(generate_cmd)) {
    return generateCommandHandler(profile, language, enable_gnu_extensions,
                                  additional_include_folders, header_folders,
                                  base_includes, output);
  }
}
