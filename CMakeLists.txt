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

include(cmake/globalsettings.cmake)

add_subdirectory(srcparser)
add_subdirectory(abigen)
