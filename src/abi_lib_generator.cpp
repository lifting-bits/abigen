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

#include "abi_lib_generator.h"
#include "std_filesystem.h"

#include <fstream>
#include <sstream>

namespace {
// clang-format off
const std::string kCopyrightHeader = "/*\n\
 * Copyright (c) 2018-present, Trail of Bits, Inc.\n\
 *\n\
 * Licensed under the Apache License, Version 2.0 (the \"License\");\n\
 * you may not use this file except in compliance with the License.\n\
 * You may obtain a copy of the License at\n\
 *\n\
 *     http://www.apache.org/licenses/LICENSE-2.0\n\
 *\n\
 * Unless required by applicable law or agreed to in writing, software\n\
 * distributed under the License is distributed on an \"AS IS\" BASIS,\n\
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n\
 * See the License for the specific language governing permissions and\n\
 * limitations under the License.\n\
 */\n";
// clang-format on

void generateAbigenHeader(std::ostream &stream, const Profile &profile) {
  stream << kCopyrightHeader << "\n";

  // clang-format off
  stream << "/*\n"
         << "\n"
         << "  Auto-generated using abigen (https://github.com/trailofbits/abigen)\n"
         << "\n"
         << "  Version information: " << ABIGEN_COMMIT_HASH << "@" << ABIGEN_BRANCH_NAME << " (" << ABIGEN_COMMIT_DESCRIPTION << ")\n"
         << "\n"
         << "  Profile: " << profile.name << "\n"
         << "    Root path: " << profile.root_path << "\n"
         << "    Resource directory: " << profile.resource_dir << "\n\n";
  // clang-format on

  stream << "    Internal isystem:\n";
  for (const auto &p : profile.internal_isystem) {
    const auto &language = p.first;
    const auto &path_list = p.second;

    stream << "      " << language << "\n";

    for (const auto &path : path_list) {
      stream << "        " << path << "\n";
    }
  }
  stream << "\n";

  stream << "    Internal externc isystem:\n";
  for (const auto &p : profile.internal_externc_isystem) {
    const auto &language = p.first;
    const auto &path_list = p.second;

    stream << "      " << language << "\n";

    for (const auto &path : path_list) {
      stream << "        " << path << "\n";
    }
  }
  stream << "\n";

  stream << "*/\n\n";
}

std::ostream &operator<<(std::ostream &stream,
                         const SourceCodeLocation &location) {
  stream << location.file_path << "@" << location.line << ":"
         << location.column;
  return stream;
}

std::ostream &operator<<(std::ostream &stream,
                         BlacklistedFunction::Reason blacklist_reason) {
  switch (blacklist_reason) {
    case BlacklistedFunction::Reason::FunctionPointer: {
      stream << "FunctionPointer";
      break;
    }

    case BlacklistedFunction::Reason::DuplicateName: {
      stream << "DuplicateName";
      break;
    }

    case BlacklistedFunction::Reason::Variadic: {
      stream << "Variadic";
      break;
    }

    default: {
      stream << "Unknown";
      break;
    }
  }

  return stream;
}
}  // namespace

ABILibGeneratorStatus generateABILibrary(
    const CommandLineOptions &cmdline_options,
    const ABILibraryState &abi_lib_state, const Profile &profile) {
  // Open the destination files
  auto header_file_path = cmdline_options.output + ".h";
  auto cpp_file_path = cmdline_options.output + ".cpp";
  auto header_file_name = stdfs::path(header_file_path).filename().string();

  std::fstream header_file(header_file_path, std::fstream::out);
  if (!header_file) {
    return ABILibGeneratorStatus(false, ABILibGeneratorError::IOError,
                                 "Failed to create the header file");
  }

  std::fstream implementation_file(cpp_file_path, std::fstream::out);
  if (!implementation_file) {
    return ABILibGeneratorStatus(false, ABILibGeneratorError::IOError,
                                 "Failed to create the implementation file");
  }

  // Generate the header file
  generateAbigenHeader(header_file, profile);

  if (!abi_lib_state.blacklisted_function_list.empty()) {
    header_file << "/*\n\n";
    header_file << "  Blacklisted functions\n\n";

    header_file
        << "  The following is a list of functions that have not been "
           "included\n"
        << "  in the library and reason why they have been blacklisted\n\n";

    header_file << std::setiosflags(std::ios::left);

    for (const auto &p : abi_lib_state.blacklisted_function_list) {
      const auto &function = p.second;
      header_file << "    " << std::setfill(' ') << std::setw(20)
                  << function.reason << function.name << "\n";

      header_file << "    " << std::setfill(' ') << std::setw(20) << " "
                  << function.location << "\n\n";
    }

    header_file << std::resetiosflags(std::ios::adjustfield);
    header_file << "*/\n\n";
  }

  header_file << "#pragma once\n\n";

  if (!cmdline_options.base_includes.empty()) {
    header_file << "// Base includes\n";
    for (const auto &base_include : cmdline_options.base_includes) {
      header_file << "#include <" << base_include << ">\n";
    }

    header_file << "\n";
  }

  header_file << "// Discovered headers\n";
  for (const auto &header : abi_lib_state.header_list) {
    header_file << "#include \"" << header << "\"\n";
  }

  // Generate the implementation file
  generateAbigenHeader(implementation_file, profile);
  implementation_file << "#include \"" << header_file_name << "\"\n\n";

  implementation_file << "__attribute__((used))\n";
  implementation_file << "void *__mcsema_externs[] = {\n";

  for (auto it = abi_lib_state.whitelisted_function_list.begin();
       it != abi_lib_state.whitelisted_function_list.end(); it++) {
    const auto &function_name = it->first;
    const auto &function_descriptor = it->second;

    implementation_file << "  // Location: " << function_descriptor.location
                        << "\n";

    implementation_file << "  (void *)(" << function_name << ")";
    if (std::next(it, 1) != abi_lib_state.whitelisted_function_list.end()) {
      implementation_file << ",\n";
    }

    implementation_file << "\n";
  }

  implementation_file << "};\n";

  return ABILibGeneratorStatus(true);
}
