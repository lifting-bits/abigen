cmake_minimum_required(VERSION 3.10)
project(abigen)

function(main)
  set(PROJECT_SOURCEFILES
    src/main.cpp
  )

  add_executable("${PROJECT_NAME}" ${PROJECT_SOURCEFILES})

  set_target_properties(
    "${PROJECT_NAME}" PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
  )

  if(TARGET globalsettings)
    message(STATUS "Importing settings from the globalsettings interface library")
    target_link_libraries("${PROJECT_NAME}" PUBLIC globalsettings)
  endif()

  importJson11()
  importCli11()

  target_link_libraries("${PROJECT_NAME}" PRIVATE json11 cli11 srcparser stdc++fs)
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
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libraries/cli11/include/CLI")
    message(SEND_ERROR "The CLI11 git submodule has not been initialized")
  endif()

  add_library(cli11 INTERFACE)
  target_include_directories(cli11 INTERFACE libraries/cli11/include)
endfunction()

main()
