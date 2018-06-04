#include <trailofbits/srcparser/isourcecodeparser.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <CLI/CLI.hpp>
#include <json11.hpp>

struct Profile final {
  std::string name;
  std::string root_path;

  std::string resource_dir;
  std::vector<std::string> internal_isystem;
  std::vector<std::string> internal_externc_isystem;
};

struct LanguageDescriptor final {
  enum class Type { C, CXX };

  Type type;
  int standard;
};

struct HeaderDescriptor final {
  std::string name;
  std::vector<std::string> possible_prefixes;
};

std::unordered_map<std::string, Profile> profile_descriptors;

const std::map<std::string, LanguageDescriptor> language_descriptors = {
    {"c89", {LanguageDescriptor::Type::C, 89}},
    {"c94", {LanguageDescriptor::Type::C, 94}},
    {"c98", {LanguageDescriptor::Type::C, 98}},
    {"c11", {LanguageDescriptor::Type::C, 11}},
    {"c17", {LanguageDescriptor::Type::C, 17}},

    {"cxx98", {LanguageDescriptor::Type::CXX, 98}},
    {"cxx11", {LanguageDescriptor::Type::CXX, 11}},
    {"cxx14", {LanguageDescriptor::Type::CXX, 14}},
    {"cxx17", {LanguageDescriptor::Type::CXX, 17}}};

bool loadProfile(Profile &profile, const std::filesystem::path &path) {
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

  if (!json["internal-isystem"].is_array()) {
    return false;
  }

  for (const auto &item : json["internal-isystem"].array_items()) {
    if (!item.is_string()) {
      return false;
    }

    profile.internal_isystem.push_back(item.string_value());
  }

  if (!json["internal-externc-isystem"].is_array()) {
    return false;
  }

  for (const auto &item : json["internal-externc-isystem"].array_items()) {
    if (!item.is_string()) {
      return false;
    }

    profile.internal_externc_isystem.push_back(item.string_value());
  }

  return true;
}

bool enumerateProfiles(std::unordered_map<std::string, Profile> &profile_list) {
  profile_list = {};

  try {
    std::unordered_map<std::string, Profile> output;

    auto profiles_root = std::filesystem::current_path() / "data" / "platforms";

    std::filesystem::recursive_directory_iterator it(profiles_root.string());
    for (const auto &p : it) {
      if (!p.is_regular_file() ||
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

trailofbits::SourceCodeParserSettings getParserSettingsFromFile(
    const Profile &profile) {
  trailofbits::SourceCodeParserSettings settings = {};

  auto L_appendPaths = [](std::vector<std::string> &destination,
                          const std::vector<std::string> &source,
                          const std::string &base_path) -> void {
    for (const auto &raw_path : source) {
      auto path = std::filesystem::path(base_path) / raw_path;
      destination.push_back(path.string());
    }
  };

  L_appendPaths(settings.cxx_system, profile.internal_isystem,
                profile.root_path);

  L_appendPaths(settings.c_system, profile.internal_externc_isystem,
                profile.root_path);

  settings.resource_dir = profile.resource_dir;

  return settings;
}

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

bool enumerateIncludeFiles(std::vector<HeaderDescriptor> &header_files,
                           const std::string &header_folder) {
  const static std::vector<std::string> valid_extensions = {".h", ".hh", ".hp",
                                                            ".hpp", ".hxx"};

  auto root_header_folder = std::filesystem::absolute(header_folder);

  try {
    std::filesystem::recursive_directory_iterator it(root_header_folder);
    for (const auto &directory_entry : it) {
      if (!directory_entry.is_regular_file()) {
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

void generateABILibrary(
    std::string &header, std::string &implementation,
    const std::vector<std::string> &include_list,
    const std::list<trailofbits::FunctionType> &function_type_list,
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

  /// \todo skip overloaded functions, skip functions with callbacks, skip
  /// varargs
  for (auto function_it = function_type_list.begin();
       function_it != function_type_list.end(); function_it++) {
    const auto &function = *function_it;

    output << "  (void *)(" << function.name << ")";
    if (std::next(function_it, 1) != function_type_list.end()) {
      output << ",";
    }

    output << "\n";
  }

  output << "};";
  implementation = output.str();
}

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
      for (const auto &path : description.internal_externc_isystem) {
        std::cout << "      " << path << "\n";
      }
      std::cout << "\n";

      std::cout << "    isystem\n";
      for (const auto &path : description.internal_isystem) {
        std::cout << "      " << path << "\n";
      }
      std::cout << "\n\n";
    }

  } else {
    for (const auto &p : profile_descriptors) {
      std::cout << "  " << p.second.name << "\n";
    }
  }
}

void listLanguagesCommandHandler() {
  std::cout << "Supported languages\n";

  for (const auto &p : language_descriptors) {
    const auto &language = p.first;
    std::cout << "  " << language << "\n";
  }
}

int generateCommandHandler(
    const std::string &profile, const std::string &language,
    bool enable_gnu_extensions,
    const std::vector<std::string> &additional_include_folders,
    const std::vector<std::string> &header_folders,
    const std::vector<std::string> &base_includes,
    const std::string &output_file_path) {
  std::cerr << "Enumerating the include files...\n";
  std::vector<HeaderDescriptor> header_files;
  enumerateIncludeFiles(header_files, header_folders);

  auto parser_settings =
      getParserSettingsFromFile(profile_descriptors.at(profile));

  parseLanguageName(parser_settings.cpp, parser_settings.standard, language);
  parser_settings.enable_gnu_extensions = enable_gnu_extensions;
  parser_settings.additional_include_folders = additional_include_folders;

  std::unique_ptr<trailofbits::ISourceCodeParser> parser;
  auto status = trailofbits::CreateSourceCodeParser(parser);
  if (!status.succeeded()) {
    std::cerr << status.toString() << "\n";
    return 1;
  }

  std::cerr << "Processing the include headers...\n";
  std::vector<std::string> current_include_list;
  std::list<trailofbits::FunctionType> function_type_list;

  auto str_header_count = std::to_string(header_files.size());
  auto header_counter_size = static_cast<int>(str_header_count.size());

  while (true) {
    bool new_headers_added = false;

    for (auto header_it = header_files.begin();
         header_it != header_files.end();) {
      const auto &current_header_desc = *header_it;

      std::vector<std::string> include_directives = {current_header_desc.name};
      for (const auto &include_prefix : current_header_desc.possible_prefixes) {
        auto relative_path =
            std::filesystem::path(include_prefix) / current_header_desc.name;

        include_directives.push_back(relative_path);
      }

      bool current_header_added = false;

      for (const auto &current_include_directive : include_directives) {
        auto temp_include_list = current_include_list;
        temp_include_list.push_back(current_include_directive);

        auto source_buffer =
            generateSourceBuffer(temp_include_list, base_includes);

        std::list<trailofbits::FunctionType> new_functions;
        status = parser->processBuffer(new_functions, source_buffer,
                                       parser_settings);

        if (status.succeeded()) {
          function_type_list.insert(function_type_list.end(),
                                    new_functions.begin(), new_functions.end());

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

  auto header_file_path = output_file_path + ".h";
  auto implementation_file_path = output_file_path + ".cpp";

  std::string abi_lib_header;
  std::string abi_lib_implementation;
  generateABILibrary(abi_lib_header, abi_lib_implementation,
                     current_include_list, function_type_list,
                     std::filesystem::path(header_file_path).filename());

  {
    std::ofstream header_file(header_file_path);
    header_file << abi_lib_header << "\n";

    std::ofstream implementation_file(implementation_file_path);
    implementation_file << abi_lib_implementation << "\n";
  }

  return 0;
}

int main(int argc, char *argv[], char *envp[]) {
  static_cast<void>(envp);

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
