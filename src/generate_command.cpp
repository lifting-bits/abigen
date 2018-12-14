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

#include "generate_command.h"
#include "abi_lib_generator.h"
#include "astvisitor.h"
#include "generate_utils.h"

/// Handler for the 'generate' command
bool generateCommandHandler(ProfileManagerRef &profile_manager,
                            const LanguageManager &language_manager,
                            const CommandLineOptions &cmdline_options) {
  // Start by enumerating all the include files
  std::vector<HeaderDescriptor> header_files;
  if (!enumerateIncludeFiles(header_files, cmdline_options.header_folders)) {
    return false;
  }

  // Allocate a new compiler instance
  CompilerInstanceRef compiler;
  if (!createCompilerInstance(compiler, profile_manager, language_manager,
                              cmdline_options)) {
    return false;
  }

  // Attempt to include as many headers as possible; stop when we can no longer
  // add new ones to the list of active ones. We do not care about the AST right
  // now! Just try to pass the compilation
  std::cerr << "Processed headers\n\n";
  std::string total_header_count_str = std::to_string(header_files.size());
  auto header_counter_digits = static_cast<int>(total_header_count_str.size());

  StringList active_include_headers;

  while (true) {
    auto previous_active_header_count = active_include_headers.size();
    for (auto header_desc_it = header_files.begin();
         header_desc_it != header_files.end();) {
      auto possible_include_directives =
          generateIncludeDirectives(*header_desc_it);

      bool include_succeeded = false;
      for (const auto &include_directive : possible_include_directives) {
        auto new_include_headers = active_include_headers;
        new_include_headers.push_back(include_directive);

        auto source_buffer = generateSourceBuffer(
            new_include_headers, cmdline_options.base_includes);

        auto compiler_status = compiler->processAST(source_buffer);
        include_succeeded = compiler_status.succeeded();

        if (include_succeeded) {
          active_include_headers.push_back(include_directive);

          std::cerr << "  [" << std::setfill('0')
                    << std::setw(header_counter_digits)
                    << active_include_headers.size();

          std::cerr << "/" << total_header_count_str << "] "
                    << include_directive << "\n";

          break;
        }
      }

      if (include_succeeded) {
        header_desc_it = header_files.erase(header_desc_it);
      } else {
        header_desc_it++;
      }
    }

    if (previous_active_header_count == active_include_headers.size()) {
      break;
    }
  }

  std::cerr << "\n";

  // Print a list of the headers we couldn't import
  if (!header_files.empty()) {
    std::cerr << "Discarded headers\n\n";
    for (const auto &header : header_files) {
      std::cerr << "  {\"";
      for (auto it = header.possible_prefixes.begin();
           it != header.possible_prefixes.end(); it++) {
        std::cerr << (*it);
        if (std::next(it, 1) != header.possible_prefixes.end()) {
          std::cerr << ", ";
        }
      }
      std::cerr << "\"} " << header.name << "\n";
    }
    std::cerr << "\n";
  }

  // We now have a list of includes that work fine; enable the AST callbacks
  IASTVisitorRef visitor_ref;
  auto visitor_status = ASTVisitor::create(visitor_ref);
  if (!visitor_status.succeeded()) {
    std::cerr << "Failed to create the ASTVisitor object: "
              << visitor_status.toString() << "\n";
    return false;
  }

  // Compile the source buffer one last time with our ASTVisitor enabled
  auto source_buffer = generateSourceBuffer(active_include_headers,
                                            cmdline_options.base_includes);

  auto compiler_status = compiler->processAST(source_buffer, visitor_ref);
  if (!compiler_status.succeeded()) {
    std::cerr << compiler_status.toString() << "\n";
    return false;
  }

  // Render the ABI library
  Profile profile;
  auto prof_mgr_status =
      profile_manager->get(profile, cmdline_options.profile_name);

  assert(prof_mgr_status.succeeded());

  ABILibrary abi_library;
  abi_library.blacklisted_function_list = visitor_ref->blacklistedFunctions();
  abi_library.whitelisted_function_list = visitor_ref->whitelistedFunctions();
  abi_library.header_list = active_include_headers;

  auto status = generateABILibrary(cmdline_options, abi_library, profile);
  if (!status.succeeded()) {
    std::cerr << status.message() << "\n";
    return false;
  }

  return true;
}
