cmake_minimum_required(VERSION 3.9.3)
project(abigen)

if(WIN32)
  set(WINDOWS_INSTALL_ROOT "abigen")
  set(PROFILE_INSTALL_FOLDER "${WINDOWS_INSTALL_ROOT}/data")
  set(BINARY_INSTALL_FOLDER "${WINDOWS_INSTALL_ROOT}")
else()
  set(PROFILE_INSTALL_FOLDER "share/abigen")
  set(BINARY_INSTALL_FOLDER "bin")
endif()

function(main)
  set(source_files
    src/main.cpp
  )

  add_executable("${PROJECT_NAME}" ${source_files})
  target_compile_definitions("${PROJECT_NAME}" PRIVATE PROFILE_INSTALL_FOLDER="${CMAKE_INSTALL_PREFIX}/${PROFILE_INSTALL_FOLDER}")

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

  generateInstallTargets()
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

main()
