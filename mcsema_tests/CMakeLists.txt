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
project(mcsema_tests)

function(mcsemaTests)
  # Build the sample binary
  add_executable(mcsema_tests_zlib
    mcsema_tests_zlib.c
  )

  target_link_libraries(mcsema_tests_zlib z)

  # Generate the abi library using abigen
  set(zlib_abi_library_output_path "${CMAKE_CURRENT_BINARY_DIR}")
  set(zlib_abi_library_name "zlib_abi_library")

  set(zlib_abi_library_header_path "${zlib_abi_library_output_path}/${zlib_abi_library_name}.h")
  set(zlib_abi_library_impl_path "${zlib_abi_library_output_path}/${zlib_abi_library_name}.cpp")

  set(zlib_include_folder "${CMAKE_CURRENT_SOURCE_DIR}/zlib_includes")

  add_custom_command(
    OUTPUT "${zlib_abi_library_header_path}"
           "${zlib_abi_library_impl_path}"

    DEPENDS abigen
    COMMAND "$<TARGET_FILE:abigen>" generate -p "Ubuntu 18.04.1 LTS" -l c11 -f "${zlib_include_folder}" -o "${zlib_abi_library_output_path}/${zlib_abi_library_name}" > /dev/null 2>&1
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating the zlib ABI library..."
  )

  add_custom_target(zlib_abi_library DEPENDS
    "${zlib_abi_library_header_path}"
    "${zlib_abi_library_impl_path}"
  )

  # Generate the bitcode file from the abi library
  set(zlib_abi_library_path "${CMAKE_CURRENT_BINARY_DIR}/zlib_abi_library.ll")

  add_custom_command(
    OUTPUT "${zlib_abi_library_path}"
    COMMAND "${CMAKE_CXX_COMPILER}" -S -emit-llvm "${zlib_abi_library_impl_path}" -o "${zlib_abi_library_path}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating the LLVM bitcode from the zlib ABI library..."
    DEPENDS zlib_abi_library
  )

  add_custom_target(mcsema_tests_zlib_abi_lib_compiler DEPENDS "${zlib_abi_library_path}")

  # Generate the cfg file with mcsema-disass and IDA Pro
  set(cfg_path "${CMAKE_CURRENT_BINARY_DIR}/mcsema_tests_zlib.cfg")
  set(log_path "${CMAKE_CURRENT_BINARY_DIR}/mcsema_tests_zlib.cfg.log")

  add_custom_command(
    OUTPUT "${cfg_path}"
    COMMAND "${MCSEMA_DISASS_PATH}" --disassembler "${IDAT64_PATH}" --arch amd64 --os linux --log_file "${log_path}" --output "${cfg_path}" --binary "$<TARGET_FILE:mcsema_tests_zlib>" --entrypoint main || cat "${log_path}"
    COMMENT "Disassembling..."
    VERBATIM
  )

  add_custom_target(mcsema_tests_zlib_disassembler DEPENDS "${cfg_path}")

  # Lift the code using mcsema-lift
  set(bc_path "${CMAKE_CURRENT_BINARY_DIR}/mcsema_tests_zlib.bc")

  add_custom_command(
    OUTPUT "${bc_path}"
    COMMAND "${MCSEMA_LIFT_PATH}" --output "${bc_path}" --arch amd64 --os linux --cfg "${cfg_path}" --abi_libraries "${zlib_abi_library_path}"
    DEPENDS mcsema_tests_zlib_disassembler mcsema_tests_zlib_abi_lib_compiler
    COMMENT "Lifting: ${cfg_path}"
    VERBATIM
  )

  add_custom_target(mcsema_tests_zlib_lifter DEPENDS "${bc_path}")

  # Compile the lifted code
  set(recompiled_binary_path "${CMAKE_CURRENT_BINARY_DIR}/mcsema_tests_zlib.out")

  add_custom_command(
    OUTPUT "${recompiled_binary_path}"
    COMMAND "${REMILL_CLANG_PATH}" -O3 -o "${recompiled_binary_path}" "${bc_path}" "${MCSEMA_RT_PATH}" -lz
    DEPENDS mcsema_tests_zlib_lifter
    COMMENT "Recompiling: ${bc_path}"
    VERBATIM
  )

  add_custom_target(mcsema_tests_zlib_recompiler DEPENDS "${recompiled_binary_path}")

  # Run the lifted code
  set(lifted_binary_output "${CMAKE_CURRENT_BINARY_DIR}/lifted_binary_output.txt")

  add_custom_command(
    OUTPUT "${lifted_binary_output}"
    COMMAND "${recompiled_binary_path}" > "${lifted_binary_output}"
    DEPENDS mcsema_tests_zlib_recompiler
    COMMENT "Running the recompiled binary..."
    VERBATIM
  )

  add_custom_target(mcsema_tests_recompiled_zlib_runner DEPENDS "${lifted_binary_output}")

  # Attach our test to the global test target
  add_dependencies(mcsema_tests mcsema_tests_recompiled_zlib_runner)
endfunction()

mcsemaTests()
