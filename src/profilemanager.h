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

#include "istatus.h"
#include "languagemanager.h"
#include "types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

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

/// A profile map
using ProfileMap = std::unordered_map<std::string, Profile>;

class ProfileManager;

/// A unique_ptr reference to a ProfileManager instance
using ProfileManagerRef = std::unique_ptr<ProfileManager>;

/// The ProfileManager is used to load profiles from disk
class ProfileManager final {
  struct PrivateData;
  std::unique_ptr<PrivateData> d;

  /// Constructor
  ProfileManager();

 public:
  /// Status code, used with ProfileManager::Status
  enum class StatusCode {
    MissingProfilesRoot,
    MemoryAllocationFailure,
    ProfileEnumerationError,
    ProfilesMissing,
    ProfileNotFound,
    Unknown
  };

  /// Status object
  using Status = IStatus<StatusCode>;

  /// Factory method for creating new ProfileManager objects
  static Status create(ProfileManagerRef &obj);

  /// Destructor
  ~ProfileManager();

  /// Returns the specified profile
  Status get(Profile &profile, const std::string &name) const;

  /// Enumerates each profile
  template <typename T>
  void enumerate(bool (*callback)(const Profile &profile, T user_defined),
                 T user_defined) const;

  /// Disable the copy constructor
  ProfileManager(const ProfileManager &other) = delete;

  /// Disable the assignment operator
  ProfileManager &operator=(const ProfileManager &other) = delete;

 private:
  /// Private accessor used by the ProfileManager::enumerate method
  const ProfileMap &profileMap() const;
};

template <typename T>
void ProfileManager::enumerate(bool (*callback)(const Profile &profile,
                                                T user_defined),
                               T user_defined) const {
  for (const auto &p : profileMap()) {
    const auto &profile = p.second;
    if (!callback(profile, user_defined)) {
      break;
    }
  }
}