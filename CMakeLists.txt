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

  src/astvisitor.h
  src/astvisitor.cpp
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

  importJson11()
  importCli11()
  importLLVM()

  target_link_libraries("${PROJECT_NAME}" PRIVATE json11 cli11 llvm_libraries)

  generateMcsemaTestTargets()
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

function(generateMcsemaTestTargets)
  if(NOT "$ENV{ABIGEN_SKIP_MCSEMA_TESTS}" STREQUAL "")
    set(ABIGEN_SKIP_MCSEMA_TESTS "$ENV{ABIGEN_SKIP_MCSEMA_TESTS}")
  endif()

  if(DEFINED ABIGEN_SKIP_MCSEMA_TESTS)
    message(STATUS "Skipping abigen's mcsema tests (ABIGEN_SKIP_MCSEMA_TESTS has been set)")
    return()
  else()
    message(STATUS "Attempting to enable abigen's mcsema tests. You can disable them by defining or exporting ABIGEN_SKIP_MCSEMA_TESTS")
  endif()

  if(NOT "${CMAKE_CXX_COMPILER}" MATCHES "clang")
    message(STATUS "The CMake compiler should be set to clang when trying to run the tests. Skipping...")
    return()
  endif()

  # Attempt to grab the paths from the environment variables
  if(NOT "$ENV{MCSEMA_LIFT_PATH}" STREQUAL "")
    set(MCSEMA_LIFT_PATH "$ENV{MCSEMA_LIFT_PATH}")
  endif()

  if(NOT "$ENV{MCSEMA_DISASS_PATH}" STREQUAL "")
    set(MCSEMA_DISASS_PATH "$ENV{MCSEMA_DISASS_PATH}")
  endif()

  if(NOT "$ENV{REMILL_CLANG_PATH}" STREQUAL "")
    set(REMILL_CLANG_PATH "$ENV{REMILL_CLANG_PATH}")
  endif()

  if(NOT "$ENV{IDAT64_PATH}" STREQUAL "")
    set(IDAT64_PATH "$ENV{IDAT64_PATH}")
  endif()

  if(NOT "$ENV{MCSEMA_RT_PATH}" STREQUAL "")
    set(MCSEMA_RT_PATH "$ENV{MCSEMA_RT_PATH}")
  endif()

  # Attempt to find the missing tools from the PATH
  if(NOT DEFINED MCSEMA_LIFT_PATH)
    find_program(lift_tool_path "mcsema-lift-${LLVM_MAJOR_VERSION}.${LLVM_MINOR_VERSION}")
    if(NOT "${lift_tool_path}" STREQUAL "lift_tool_path-NOTFOUND")
      set(MCSEMA_LIFT_PATH "${lift_tool_path}")
    endif()
  endif()

  if(NOT DEFINED MCSEMA_DISASS_PATH)
    find_program(disass_tool_path "mcsema-disass")
    if(NOT "${disass_tool_path}" STREQUAL "disass_tool_path-NOTFOUND")
      set(MCSEMA_DISASS_PATH "${disass_tool_path}")
    endif()
  endif()

  if(NOT DEFINED REMILL_CLANG_PATH)
    find_program(remill_clang_tool_path "remill-clang-${LLVM_MAJOR_VERSION}.${LLVM_MINOR_VERSION}")
    if(NOT "${remill_clang_tool_path}" STREQUAL "remill_clang_tool_path-NOTFOUND")
      set(REMILL_CLANG_PATH "${remill_clang_tool_path}")
    endif()
  endif()

  if(NOT DEFINED IDAT64_PATH)
    find_program(ida_tool_path "idat64")
    if(NOT "${ida_tool_path}" STREQUAL "ida_tool_path-NOTFOUND")
      set(IDAT64_PATH "${ida_tool_path}")
    endif()
  endif()

  if(NOT DEFINED MCSEMA_RT_PATH)
    set(mcsema_rt_system_path "/usr/local/lib/libmcsema_rt64-${LLVM_MAJOR_VERSION}.${LLVM_MINOR_VERSION}.a")
    if(EXISTS "${mcsema_rt_system_path}")
      set(MCSEMA_RT_PATH "${mcsema_rt_system_path}")
    endif()
  endif()

  # Make sure we have everything we need
  if(NOT EXISTS "${MCSEMA_LIFT_PATH}")
    message(SEND_ERROR "The mcsema-lift executable was not found. Either define MCSEMA_LIFT_PATH or make sure the tool is in PATH")
    return()
  endif()

  if(NOT EXISTS "${MCSEMA_DISASS_PATH}")
    message(SEND_ERROR "The mcsema-disass executable was not found. Either define MCSEMA_DISASS_PATH or make sure the tool is in PATH")
    return()
  endif()

  if(NOT EXISTS "${IDAT64_PATH}")
    message(SEND_ERROR "The idat64 executable was not found. Either define IDAT64_PATH or make sure the tool is in PATH")
    return()
  endif()

  if(NOT EXISTS "${REMILL_CLANG_PATH}")
    message(SEND_ERROR "The remill-clang executable was not found. Either define REMILL_CLANG_PATH or make sure the tool is in PATH")
    return()
  endif()

  if(NOT EXISTS "${MCSEMA_RT_PATH}")
    message(SEND_ERROR "The libmcsema_rt64 library was not found. Either define MCSEMA_RT_PATH or install the library")
    return()
  endif()
  
  message(STATUS "Using mcsema-lift: ${MCSEMA_LIFT_PATH}")
  message(STATUS "Using mcsema-disass: ${MCSEMA_DISASS_PATH}")
  message(STATUS "Using idat64: ${IDAT64_PATH}")
  message(STATUS "Using remill-clang: ${REMILL_CLANG_PATH}")
  message(STATUS "Using libmcsema_rt64: ${MCSEMA_RT_PATH}")

  add_custom_target(mcsema_tests)
  add_subdirectory("mcsema_tests")

  message(STATUS "Tests can be run with `make mcsema_tests`")
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

function(importLLVM)
  add_library(llvm_libraries INTERFACE)

  if(DEFINED LIBRARY_REPOSITORY_ROOT)
    set(llvm_find_package_hints HINTS "${LIBRARY_REPOSITORY_ROOT}/llvm")
  endif()

  find_package(LLVM REQUIRED CONFIG ${llvm_find_package_hints})
  target_include_directories(llvm_libraries SYSTEM INTERFACE "${LLVM_INCLUDE_DIRS}")
  target_compile_definitions(llvm_libraries INTERFACE ${LLVM_DEFINITIONS})

  string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_PACKAGE_VERSION})
  list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VERSION)
  list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VERSION)

  target_compile_definitions(llvm_libraries INTERFACE
    LLVM_MINOR_VERSION=${LLVM_MINOR_VERSION}
    LLVM_MAJOR_VERSION=${LLVM_MAJOR_VERSION}
  )

  set(LLVM_MINOR_VERSION ${LLVM_MINOR_VERSION} PARENT_SCOPE)
  set(LLVM_MAJOR_VERSION ${LLVM_MAJOR_VERSION} PARENT_SCOPE)

  # It is important to match the RTTI setting used to compile the library
  detectLLVMrttiSetting()
  if(NOT "${detectLLVMrttiSetting_OUTPUT}")
    target_compile_options(llvm_libraries INTERFACE "-fno-rtti")
  endif()

  # This would be the preferred way, but we are required to support old LLVM/Clang versions!
  #
  # llvm_map_components_to_libnames(llvm_libraries support)
  # target_link_libraries("${PROJECT_NAME}" PUBLIC "${llvm_libraries}")

  if(LLVM_MAJOR_VERSION GREATER 6)
    target_link_libraries(llvm_libraries INTERFACE "LLVM")
  endif()

  find_package(Clang REQUIRED ${llvm_find_package_hints})
  target_include_directories(llvm_libraries SYSTEM INTERFACE ${CLANG_INCLUDE_DIRS})

  target_link_libraries(llvm_libraries INTERFACE clangBasic clangAST clangTooling clangFrontend clangParse)
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
