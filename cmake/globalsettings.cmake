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

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

add_library(globalsettings INTERFACE)

target_compile_options(globalsettings INTERFACE
  -fvisibility=hidden -fvisibility-inlines-hidden
  -Werror -Wall -Wextra -Wconversion
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  if(UNIX)
    target_compile_options(globalsettings INTERFACE
      -gdwarf-2 -g3)
    endif()

  set(CMAKE_VERBOSE_MAKEFILE True)
endif()
