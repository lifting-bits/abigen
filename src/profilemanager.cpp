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

#include "profilemanager.h"
#include "std_filesystem.h"

#include <fstream>

#include <json11.hpp>

namespace {
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

/// Loads the profile located at the given path
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

/// Enumerates all the profiles found in the specified data directory
bool enumerateProfiles(std::unordered_map<std::string, Profile> &profile_list,
                       const std::string &profile_root_folder) {
  profile_list = {};

  try {
    std::unordered_map<std::string, Profile> output;

    stdfs::recursive_directory_iterator it(profile_root_folder);
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
}  // namespace

/// Private class data for ProfileManager objects
struct ProfileManager::PrivateData final {
  // This is the folder containing the profiles; abigen will always prefer to
  // use the `data` folder in the working directory if possible, but will
  // default to the system-wide one if it is not found
  std::string profiles_root;

  /// This is the list of discovered profiles, built scanning the
  /// `profiles_root` folder
  ProfileMap profile_descriptors;
};

ProfileManager::ProfileManager() : d(new PrivateData) {
  if (!getProfilesRootPath(d->profiles_root)) {
    throw Status(false, StatusCode::MissingProfilesRoot,
                 "Failed to locate a suitable profile root folder");
  }

  if (!enumerateProfiles(d->profile_descriptors, d->profiles_root)) {
    throw Status(false, StatusCode::ProfileEnumerationError,
                 "Failed to locate a suitable profile root folder");
  }

  if (d->profile_descriptors.empty()) {
    throw Status(false, StatusCode::ProfilesMissing,
                 "No profile could be found");
  }
}

ProfileManager::Status ProfileManager::create(ProfileManagerRef &obj) {
  obj.reset();

  try {
    auto ptr = new ProfileManager();
    obj.reset(ptr);

    return Status(true);

  } catch (const std::bad_alloc &) {
    return Status(false, StatusCode::MemoryAllocationFailure,
                  "Failed to allocate the object");

  } catch (const Status &status) {
    return status;
  }
}

ProfileManager::~ProfileManager() {}

ProfileManager::Status ProfileManager::get(Profile &profile,
                                           const std::string &name) const {
  auto it = d->profile_descriptors.find(name);
  if (it == d->profile_descriptors.end()) {
    return Status(false, StatusCode::ProfileNotFound,
                  "The specified profile does not exists");
  }

  profile = it->second;
  return Status(true);
}

const ProfileMap &ProfileManager::profileMap() const {
  return d->profile_descriptors;
}
