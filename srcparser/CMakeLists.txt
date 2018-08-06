# Copyright (c) 2018 Trail of Bits, Inc.
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
project(srcparser)

function(main)
  set(source_files
    src/llvm_compatibility.h

    src/sourcecodeparser.h
    src/sourcecodeparser.cpp

    src/asttypecollector.h
    src/asttypecollector.cpp

    src/astvisitor.h
    src/astvisitor.cpp
  )

  set(public_include_headers
    include/trailofbits/srcparser/isourcecodeparser.h
    include/trailofbits/srcparser/macros.h
    include/trailofbits/srcparser/istatus.h
  )

  add_library(
    "${PROJECT_NAME}"
    ${source_files} ${public_include_headers}
  )

  if(TARGET globalsettings)
    message(STATUS "Importing settings from the globalsettings interface library")
    target_link_libraries("${PROJECT_NAME}" PUBLIC globalsettings)
  endif()

  target_include_directories("${PROJECT_NAME}" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
  target_compile_definitions("${PROJECT_NAME}" PRIVATE SRCPARSER_EXPORTSYMBOLS)

  set_target_properties("${PROJECT_NAME}" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
  )

  find_package(LLVM REQUIRED CONFIG)
  target_include_directories("${PROJECT_NAME}" SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
  target_compile_definitions("${PROJECT_NAME}" PUBLIC ${LLVM_DEFINITIONS})

  string(REPLACE "." ";" LLVM_VERSION_LIST ${LLVM_PACKAGE_VERSION})
  list(GET LLVM_VERSION_LIST 1 LLVM_MINOR_VERSION)
  list(GET LLVM_VERSION_LIST 0 LLVM_MAJOR_VERSION)

  target_compile_definitions("${PROJECT_NAME}" PRIVATE
    LLVM_MINOR_VERSION=${LLVM_MINOR_VERSION}
    LLVM_MAJOR_VERSION=${LLVM_MAJOR_VERSION}
  )

  detectLLVMrttiSetting()
  if(NOT "${detectLLVMrttiSetting_OUTPUT}")
    target_compile_options("${PROJECT_NAME}" PRIVATE "-fno-rtti")
  endif()

  # This would be the preferred way, but we are required to support old LLVM/Clang versions!
  #
  # llvm_map_components_to_libnames(llvm_libraries support)
  # target_link_libraries("${PROJECT_NAME}" PUBLIC "${llvm_libraries}")

  if(LLVM_MAJOR_VERSION GREATER_EQUAL 5)
    target_link_libraries("${PROJECT_NAME}" PUBLIC "LLVM")
  endif()

  find_package(Clang REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PUBLIC clangBasic clangAST clangTooling clangFrontend clangParse)
endfunction()

function(detectLLVMrttiSetting)
  if(DEFINED ENABLE_RTTI)
    message(STATUS "Forcing RTTI to ${ENABLE_RTTI}")
    set(detectLLVMrttiSetting_OUTPUT "${ENABLE_RTTI}" PARENT_SCOPE)
    return()
  endif()

  message(STATUS "Detecting RTTI setting.. (override it with -DENABLE_RTTI=YES/NO)")

  execute_process(
    COMMAND llvm-config --has-rtti
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

main()
