cmake_minimum_required(VERSION 3.10)
project(abigen)

function(main)
  set(PROJECT_SOURCEFILES
    src/main.cpp
  )

  add_executable("${PROJECT_NAME}" ${PROJECT_SOURCEFILES})

  set_target_properties("${PROJECT_NAME}" PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
  )

  if(TARGET globalsettings)
    message(STATUS "Importing settings from the globalsettings interface library")
    target_link_libraries("${PROJECT_NAME}" PUBLIC globalsettings)
  endif()

  target_link_libraries("${PROJECT_NAME}" PRIVATE srcparser)
endfunction()

main()
