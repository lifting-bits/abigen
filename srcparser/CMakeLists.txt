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

main()
