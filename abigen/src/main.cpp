#include <trailofbits/srcparser/isourcecodeparser.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <json11.hpp>

struct Profile final {
  std::string name;
  std::string root_path;

  std::string resource_dir;
  std::vector<std::string> internal_isystem;
  std::vector<std::string> internal_externc_isystem;
};

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

trailofbits::SourceCodeParserSettings getSourceCodeParserSettings(
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

std::string generateSourceBuffer(const std::vector<std::string> &include_list) {
  std::stringstream buffer;
  for (const auto &include : include_list) {
    buffer << "#include <" << include << ">\n";
  }

  return buffer.str();
}

int main(int argc, char *argv[], char *envp[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  static_cast<void>(envp);

  std::unordered_map<std::string, Profile> profile_list;
  if (!enumerateProfiles(profile_list)) {
    std::cerr << "Failed to load the profiles\n";
    return false;
  }

  if (profile_list.empty()) {
    std::cerr << "No profile found!\n";
    return 1;
  }

  /// \todo Parse the user input
  std::string profile_name = "Ubuntu 16.04.4 LTS";
  bool is_cpp = true;
  bool enable_gnu_extensions = true;
  std::size_t language_standard = 11;
  std::vector<std::string> additional_include_folders = {
      "/home/alessandro/Downloads/apr-1.6.3/include"};
  std::vector<std::string> headers = {
      "apr_allocator.h",    "apr_atomic.h",
      "apr_cstr.h",         "apr_dso.h",
      "apr_env.h",          "apr_errno.h",
      "apr_escape.h",       "apr_file_info.h",
      "apr_file_io.h",      "apr_fnmatch.h",
      "apr_general.h",      "apr_getopt.h",
      "apr_global_mutex.h", "apr.h",
      "apr_hash.h",         "apr_inherit.h",
      "apr_lib.h",          "apr_mmap.h",
      "apr_network_io.h",   "apr_perms_set.h",
      "apr_poll.h",         "apr_pools.h",
      "apr_portable.h",     "apr_proc_mutex.h",
      "apr_random.h",       "apr_ring.h",
      "apr_shm.h",          "apr_signal.h",
      "apr_skiplist.h",     "apr_strings.h",
      "apr_support.h",      "apr_tables.h",
      "apr_thread_cond.h",  "apr_thread_mutex.h",
      "apr_thread_proc.h",  "apr_thread_rwlock.h",
      "apr_time.h",         "apr_user.h",
      "apr_version.h",      "apr_want.h"};

  auto profile_it = profile_list.find(profile_name);
  if (profile_it == profile_list.end()) {
    std::cerr << "The specified profile was not found\n";
    return 1;
  }

  auto parser_settings = getSourceCodeParserSettings(profile_it->second);
  parser_settings.cpp = is_cpp;
  parser_settings.enable_gnu_extensions = enable_gnu_extensions;
  parser_settings.standard = language_standard;
  parser_settings.additional_include_folders = additional_include_folders;

  std::unique_ptr<trailofbits::ISourceCodeParser> parser;
  auto status = trailofbits::CreateSourceCodeParser(parser);
  if (!status.succeeded()) {
    std::cerr << status.toString() << "\n";
    return 1;
  }

  std::vector<std::string> current_include_list;
  std::list<trailofbits::StructureType> structure_type_list;
  std::list<trailofbits::FunctionType> function_type_list;

  std::cerr << "Attempting to add the following headers: ";
  for (const auto &header : headers) {
    std::cerr << header << ", ";
  }
  std::cerr << "\n";

  while (true) {
    bool header_added = false;

    for (auto header_it = headers.begin(); header_it != headers.end();) {
      const auto &current_header = *header_it;

      auto temp_include_list = current_include_list;
      temp_include_list.push_back(current_header);

      auto source_buffer = generateSourceBuffer(temp_include_list);

      std::list<trailofbits::StructureType> new_structures;
      std::list<trailofbits::FunctionType> new_functions;

      status = parser->processBuffer(new_structures, new_functions,
                                     source_buffer, parser_settings);
      if (!status.succeeded()) {
        header_it++;
        continue;
      }

      structure_type_list.insert(structure_type_list.end(),
                                 new_structures.begin(), new_structures.end());

      function_type_list.insert(function_type_list.end(), new_functions.begin(),
                                new_functions.end());

      header_it = headers.erase(header_it);
      current_include_list.push_back(current_header);

      header_added = true;
    }

    if (!header_added) {
      break;
    }
  }

  std::cerr << "Successfully added " << current_include_list.size()
            << " headers\n";

  if (!headers.empty()) {
    std::cerr << "The following headers could not be imported:\n";
    for (const auto &header : headers) {
      std::cerr << " > " << header << "\n";
    }
  }

  return 0;
}
