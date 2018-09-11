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
#include "generate_command.h"
#include "generate_utils.h"

#include <iostream>

extern const std::string kTestBuffer;

const std::vector<std::string> kExpectedBlacklist = {
    "my_callback_function_array_union_ptr",
    "my_callback_function_array_union",
    "my_callback_function_union",
    "my_callback_function_struct",
    "my_callback_function_array_struct_ptr",
    "my_callback_struct_recursive",
    "my_callback_struct_recursive2",
    "my_callback_function_array_struct",
    "my_callback_function_array",
    "my_callback_function_union_ptr",
    "my_callback_function_struct_ptr",
    "my_varargs_function",
    "my_callback_function",
    "my_callback_function_array_fixed"};

const std::vector<std::string> kExpectedWhitelist = {
    "my_function", "no_callback_function_array_struct",
    "no_callback_function_array_struct_ptr", "no_callback_function_struct",
    "no_callback_function_struct_ptr"};

int main(int argc, char *argv[], char *envp[]) {
  static_cast<void>(argc);
  static_cast<void>(argv);
  static_cast<void>(envp);

  CompilerInstanceSettings compiler_settings;
  compiler_settings.enable_gnu_extensions = true;

  CompilerInstanceRef compiler;
  auto compiler_status = CompilerInstance::create(compiler, compiler_settings);
  if (!compiler_status.succeeded()) {
    std::cerr << compiler_status.toString() << "\n";
    return 1;
  }

  ABILibraryState abi_lib_state;
  compiler->setASTCallbackParameter(&abi_lib_state);
  compiler->registerASTCallback(CompilerInstance::ASTCallbackType::Function,
                                astFunctionCallback);

  // Compile the source buffer one last time, now that we enabled our AST
  // callbacks
  compiler_status = compiler->processBuffer(kTestBuffer);
  if (!compiler_status.succeeded()) {
    std::cerr << compiler_status.toString() << "\n";
    return 1;
  }

  auto missing_from_blacklist = kExpectedBlacklist;
  StringList wrongly_blacklisted;

  for (const auto &p : abi_lib_state.blacklisted_function_list) {
    const auto &function = p.second;

    auto list_it = std::find(missing_from_blacklist.begin(),
                             missing_from_blacklist.end(), function.name);
    if (list_it != missing_from_blacklist.end()) {
      missing_from_blacklist.erase(list_it);
    } else {
      wrongly_blacklisted.push_back(function.name);
    }
  }

  auto missing_from_whitelist = kExpectedWhitelist;
  StringList wrongly_whitelisted;

  for (const auto &p : abi_lib_state.whitelisted_function_list) {
    const auto &function_name = p.first;

    auto list_it = std::find(missing_from_whitelist.begin(),
                             missing_from_whitelist.end(), function_name);

    if (list_it != missing_from_whitelist.end()) {
      missing_from_whitelist.erase(list_it);
    } else {
      wrongly_whitelisted.push_back(function_name);
    }
  }

  bool succeeded = true;

  if (!wrongly_blacklisted.empty()) {
    succeeded = false;

    std::cerr << "Wrongly blacklisted:\n\n";
    for (const auto &function : wrongly_blacklisted) {
      std::cout << function << "\n";
    }

    std::cout << "\n";
  }

  if (!missing_from_blacklist.empty()) {
    succeeded = false;

    std::cerr << "Missing from blacklist:\n\n";
    for (const auto &function : missing_from_blacklist) {
      std::cout << function << "\n";
    }

    std::cout << "\n";
  }

  if (!wrongly_whitelisted.empty()) {
    succeeded = false;

    std::cerr << "Wrongly whitelisted:\n\n";
    for (const auto &function : wrongly_whitelisted) {
      std::cout << function << "\n";
    }

    std::cout << "\n";
  }

  if (!missing_from_whitelist.empty()) {
    succeeded = false;

    std::cerr << "Missing from whitelist:\n\n";
    for (const auto &function : missing_from_whitelist) {
      std::cout << function << "\n";
    }

    std::cout << "\n";
  }

  return (succeeded ? 0 : 1);
}

// clang-format off
const std::string kTestBuffer = "int my_function(int a);\
int my_varargs_function(int a, ...);\
typedef void (*callback)();\
struct bar {\
  int x;\
  float y;\
  callback z;\
};\
\
union baz {\
  struct bar b;\
  void *q;\
};\
\
struct nocall {\
  int a;\
  int b;\
  void *c;\
  float *d;\
  char arr[500];\
};\
\
struct X;\
typedef struct X Y;\
struct X {\
  Y *next;\
  void (*foo)(void);\
};\
void my_callback_struct_recursive(struct X *z);\
void my_callback_struct_recursive2(Y *z);\
\
int my_callback_function(int a, callback c);\
int my_callback_function_array(int a, callback c[]);\
int my_callback_function_array_fixed(int a, callback c[5]);\
int my_callback_function_struct(int a, struct bar b);\
int my_callback_function_struct_ptr(int a, struct bar *b);\
int my_callback_function_union(int a, union baz b);\
int my_callback_function_union_ptr(int a, union baz *b);\
int my_callback_function_array_union(int a, union baz b[]);\
int my_callback_function_array_union_ptr(int a, union baz *b[20]);\
int my_callback_function_array_struct(int a, struct bar b[]);\
int my_callback_function_array_struct_ptr(int a, struct bar *b[20]);\
\
int no_callback_function_array_struct(int a, struct nocall b[]);\
int no_callback_function_array_struct_ptr(int a, struct nocall *b[20]);\
int no_callback_function_struct(int a, struct nocall b);\
int no_callback_function_struct_ptr(int a, struct nocall *b);";
// clang-format on
