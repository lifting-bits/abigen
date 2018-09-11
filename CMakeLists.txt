# Copyright (c) 2018-present, Trail of Bits, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.9.3)
project(abigen)

include(cmake/cxxcommon.cmake)
include(cmake/globalsettings.cmake)

if(WIN32)
  set(WINDOWS_INSTALL_ROOT "abigen")
  set(PROFILE_INSTALL_FOLDER "${WINDOWS_INSTALL_ROOT}/data")
  set(BINARY_INSTALL_FOLDER "${WINDOWS_INSTALL_ROOT}")
else()
  set(PROFILE_INSTALL_FOLDER "share/abigen")
  set(BINARY_INSTALL_FOLDER "bin")
endif()

set(COMMON_SOURCE_FILES
  src/istatus.h
  src/std_filesystem.h
  src/types.h

  src/profilemanager.h
  src/profilemanager.cpp

  src/languagemanager.h
  src/languagemanager.cpp

  src/cmdline.h
  src/cmdline.cpp

  src/list_commands.cpp
  src/version_command.cpp

  src/generate_command.h
  src/generate_command.cpp

  src/generate_utils.h
  src/generate_utils.cpp

  src/compilerinstance.h
  src/compilerinstance.cpp

  src/abi_lib_generator.h
  src/abi_lib_generator.cpp
)

function(abigen)
  fetchAbigenVersionInformation()

  set(source_files
    ${COMMON_SOURCE_FILES}
    src/main.cpp
  )

  add_executable("${PROJECT_NAME}" ${source_files})
  target_link_libraries("${PROJECT_NAME}" PRIVATE globalsettings stdc++fs)

  target_compile_definitions("${PROJECT_NAME}" PRIVATE
    PROFILE_INSTALL_FOLDER="${CMAKE_INSTALL_PREFIX}/${PROFILE_INSTALL_FOLDER}"
    ABIGEN_COMMIT_DESCRIPTION="${ABIGEN_COMMIT_DESCRIPTION}"
    ABIGEN_BRANCH_NAME="${ABIGEN_BRANCH_NAME}"
    ABIGEN_COMMIT_HASH="${ABIGEN_COMMIT_HASH}"
  )

  generateInstallTargets()

  #
  # Built-in depedencies
  #

  importJson11()
  importCli11()

  target_link_libraries("${PROJECT_NAME}" PRIVATE json11 cli11)

  #
  # External dependencies
  #

  # LLVM and Clang
  find_package(LLVM REQUIRED CONFIG)

  target_include_directories("${PROJECT_NAME}" SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
  target_compile_definitions("${PROJECT_NAME}" PRIVATE ${LLVM_DEFINITIONS})

  string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_PACKAGE_VERSION})
  list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VERSION)
  list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VERSION)

  target_compile_definitions("${PROJECT_NAME}" PRIVATE
    LLVM_MINOR_VERSION=${LLVM_MINOR_VERSION}
    LLVM_MAJOR_VERSION=${LLVM_MAJOR_VERSION}
  )

  # It is important to match the RTTI setting used to compile the library
  detectLLVMrttiSetting()
  if(NOT "${detectLLVMrttiSetting_OUTPUT}")
    target_compile_options("${PROJECT_NAME}" PRIVATE "-fno-rtti")
  endif()

  # This would be the preferred way, but we are required to support old LLVM/Clang versions!
  #
  # llvm_map_components_to_libnames(llvm_libraries support)
  # target_link_libraries("${PROJECT_NAME}" PUBLIC "${llvm_libraries}")

  if(LLVM_MAJOR_VERSION GREATER_EQUAL 6)
    target_link_libraries("${PROJECT_NAME}" PRIVATE "LLVM")
  endif()

  find_package(Clang REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PRIVATE clangBasic clangAST clangTooling clangFrontend clangParse)

  # Generate the tests, if they have not been disabled
  generateTestTargets()
endfunction()

function(fetchAbigenVersionInformation)
  message(STATUS "Fetching version information from git...")

  execute_process(
    COMMAND git fetch --tags --all
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_QUIET
    ERROR_QUIET
    TIMEOUT 15
  )

  execute_process(
    COMMAND git describe --tags --always --abbrev=0
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE abigen_commit_description
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(ABIGEN_COMMIT_DESCRIPTION "${abigen_commit_description}" PARENT_SCOPE)

  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE abigen_commit_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(ABIGEN_COMMIT_HASH "${abigen_commit_hash}" PARENT_SCOPE)

  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE abigen_branch_name
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  set(ABIGEN_BRANCH_NAME "${abigen_branch_name}" PARENT_SCOPE)
endfunction()

function(generateTestTargets)
  # Allow the user to skip the tests
  if(DEFINED ABIGEN_SKIP_TESTS)
    message(STATUS "Skipping test generation")
    return()
  endif()

  file(GLOB test_source_list
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
  )

  get_property(library_list TARGET "${PROJECT_NAME}" PROPERTY LINK_LIBRARIES)
  get_property(compile_definition_list TARGET "${PROJECT_NAME}" PROPERTY COMPILE_DEFINITIONS)

  message(STATUS "Generating tests")
  foreach(test_source_file ${test_source_list})
    get_filename_component(test_name "${test_source_file}" NAME_WE)
    message(STATUS "> ${test_name}")

    add_executable("${test_name}"
      ${COMMON_SOURCE_FILES}
      "tests/${test_source_file}"
    )

    target_link_libraries("${test_name}" PRIVATE ${library_list})
    target_compile_definitions("${test_name}" PRIVATE ${compile_definition_list})
    target_include_directories("${test_name}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  endforeach()
endfunction()

function(importJson11)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/json11/json11.cpp")
    message(SEND_ERROR "The Json11 git submodule has not been initialized")
  endif()

  add_library(json11
    libraries/json11/json11.hpp
    libraries/json11/json11.cpp
  )

  set_target_properties(json11 PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
  )

  target_include_directories(json11 PUBLIC libraries/json11)
endfunction()

function(importCli11)
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/CLI11/include/CLI")
    message(SEND_ERROR "The CLI11 git submodule has not been initialized")
  endif()

  add_library(cli11 INTERFACE)
  target_include_directories(cli11 INTERFACE libraries/CLI11/include)
endfunction()

function(generateInstallTargets)
  install(TARGETS "${PROJECT_NAME}" DESTINATION "${BINARY_INSTALL_FOLDER}")
  install(DIRECTORY "data" DESTINATION "${PROFILE_INSTALL_FOLDER}")

  if(WIN32)
    list(APPEND remove_commands
      COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_INSTALL_PREFIX}/${WINDOWS_INSTALL_ROOT}"
    )
  else()
    list(APPEND remove_commands
      COMMAND "${CMAKE_COMMAND}" -E remove "${CMAKE_INSTALL_PREFIX}/${BINARY_INSTALL_FOLDER}/abigen"
      COMMAND "${CMAKE_COMMAND}" -E remove_directory "${CMAKE_INSTALL_PREFIX}/${PROFILE_INSTALL_FOLDER}"
    )
  endif()

  add_custom_target(uninstall
    ${remove_commands}
    COMMENT "Uninstalling abigen..."
    VERBATIM
  )
endfunction()

function(detectLLVMrttiSetting)
  # Allow the user to force this setting
  if(DEFINED ABIGEN_ENABLE_RTTI)
    message(STATUS "Forcing RTTI to ${ENABLE_RTTI}")
    set(detectLLVMrttiSetting_OUTPUT "${ENABLE_RTTI}" PARENT_SCOPE)
    return()
  endif()

  # Detect the llvm-config path
  get_filename_component(llvm_config_path "${LLVM_LIBRARY_DIR}" DIRECTORY)
  set(llvm_config_path "${llvm_config_path}/bin/llvm-config")

  if(WIN32)
    set(llvm_config_path "${llvm_config_path}.exe")
  endif()

  message(STATUS "llvm-config path: ${llvm_config_path}")

  # Execute the llvm-config program and the get rtti setting
  message(STATUS "Detecting RTTI setting.. (override it with -DENABLE_RTTI=YES/NO)")

  execute_process(
    COMMAND "${llvm_config_path}" --has-rtti
    OUTPUT_VARIABLE llvm_config_output
  )

  string(STRIP "${llvm_config_output}" llvm_config_output)

  if("${llvm_config_output}" STREQUAL "YES")
    message(STATUS "RTTI appears to be enabled")
  elseif("${llvm_config_output}" STREQUAL "NO")
    message(STATUS "RTTI appears to be disabled")
  else()
    message(SEND_ERROR "Failed to determine the rtti setting")
    set(detectLLVMrttiSetting_OUTPUT "NO" PARENT_SCOPE)
  endif()

  set(detectLLVMrttiSetting_OUTPUT "${llvm_config_output}" PARENT_SCOPE)
endfunction()

abigen()
