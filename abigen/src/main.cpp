#include <trailofbits/srcparser/isourcecodeparser.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

/*
#include <algorithm>
#include <filesystem>
void scanRootIncludeDirectory(std::vector<std::string> &folders,
std::vector<std::string>& headers, const std::string &root_include_folder) {
  folders.clear();
  headers.clear();

  std::filesystem::recursive_directory_iterator
root_include_dir(root_include_folder); for (const auto& p: root_include_dir) {
    auto current_path = p.path().string();

    if (p.is_directory()) {
      folders.push_back(current_path);
    } else if (p.is_regular_file()) {
      headers.push_back(current_path);
    }
  }

  std::sort(folders.begin(), folders.end());
  std::sort(headers.begin(), headers.end());
}*/

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

  std::unique_ptr<trailofbits::ISourceCodeParser> parser;
  auto status = trailofbits::CreateSourceCodeParser(parser);

  if (!status.succeeded()) {
    std::cerr << status.toString() << "\n";
    return 1;
  }

  trailofbits::SourceCodeParserSettings settings;
  settings.cxx_system = {
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/8.1.0/../../../../include/c++/"
      "8.1.0",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/8.1.0/../../../../include/c++/"
      "8.1.0/x86_64-pc-linux-gnu",
      "/usr/bin/../lib64/gcc/x86_64-pc-linux-gnu/8.1.0/../../../../include/c++/"
      "8.1.0/backward",
      "/usr/local/include", "/usr/lib/clang/6.0.0/include"};

  settings.c_system = {"/include", "/usr/include"};

  settings.resource_dir = "/usr/lib/clang/6.0.0";
  settings.additional_include_folders = {
      "/home/alessandro/Downloads/apr-1.6.3/include"};

  std::vector<std::string> current_include_list;
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

  std::list<trailofbits::StructureType> structure_type_list;
  std::list<trailofbits::FunctionType> function_type_list;

  std::cout << "Attempting to add the following headers: ";
  for (const auto &header : headers) {
    std::cout << header << ", ";
  }
  std::cout << "\n";

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
                                     source_buffer, settings);
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

  std::cout << "Successfully added " << current_include_list.size()
            << " headers\n";

  std::cout << "The following headers could not be imported:\n";
  for (const auto &header : headers) {
    std::cout << " > " << header << "\n";
  }

  std::cout << "The following structures have been found:\n";
  for (const auto &structure : structure_type_list) {
    std::cout << "  " << structure.name << "\n";
    for (const auto &p : structure.fields) {
      std::cout << "    " << p.second << " " << p.first << "\n";
    }

    std::cout << "\n\n";
  }

  std::cout << "The following functions have been found:\n";
  for (const auto &function : function_type_list) {
    std::cout << "  " << function.return_type << " " << function.name << "(";

    for (const auto &p : function.parameters) {
      std::cout << p.first << " " << p.second << ", ";
    }

    std::cout << ")\n";
  }

  return 0;
}
