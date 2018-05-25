cmake_minimum_required(VERSION 3.10)
project(srcparser)

function(main)
  set(PROJECT_SOURCEFILES
    src/sourcecodeparser.h
    src/sourcecodeparser.cpp

    src/asttypecollector.h
    src/asttypecollector.cpp

    src/astvisitor.h
    src/astvisitor.cpp
  )

  set(PROJECT_INCLUDEHEADERS
    include/trailofbits/srcparser/isourcecodeparser.h
    include/trailofbits/srcparser/macros.h
    include/trailofbits/srcparser/istatus.h
  )

  add_library(
    "${PROJECT_NAME}"
    ${PROJECT_SOURCEFILES} ${PROJECT_INCLUDEHEADERS}
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

  #llvm_map_components_to_libnames(llvm_libraries support)
  #target_link_libraries("${PROJECT_NAME}" PUBLIC "${llvm_libraries}")
  target_link_libraries("${PROJECT_NAME}" PUBLIC "LLVM")

  find_package(Clang REQUIRED)
  target_link_libraries("${PROJECT_NAME}" PUBLIC clangBasic clangAST clangTooling clangFrontend clangParse)
endfunction()

main()
