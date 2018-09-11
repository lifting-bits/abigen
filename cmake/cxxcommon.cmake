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

if(DEFINED ENV{TRAILOFBITS_LIBRARIES})
  set(LIBRARY_REPOSITORY_ROOT $ENV{TRAILOFBITS_LIBRARIES}
    CACHE PATH "Location of cxx-common libraries."
  )
endif()

if(DEFINED LIBRARY_REPOSITORY_ROOT)
  set(CXXCOMMON_INCLUDE "${LIBRARY_REPOSITORY_ROOT}/cmake_modules/repository.cmake")
  if(NOT EXISTS "${CXXCOMMON_INCLUDE}")
    message(FATAL_ERROR "Failed to locate the cxx-common include file")
  endif()

  include("${LIBRARY_REPOSITORY_ROOT}/cmake_modules/repository.cmake")
  message(STATUS "Using cxx-common")
else()
  message(STATUS "Using system libraries")
endif()
