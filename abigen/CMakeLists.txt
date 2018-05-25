cmake_minimum_required(VERSION 3.10) project(abigen)

    function(main) set(PROJECT_SOURCEFILES src / main.cpp)

        add_executable("${PROJECT_NAME}" ${PROJECT_SOURCEFILES})

            set_target_properties(
                "${PROJECT_NAME}" PROPERTIES
                    CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)

                if (TARGET globalsettings) message(
                    STATUS
                    "Importing settings from the globalsettings interface "
                    "library") target_link_libraries("${PROJECT_NAME}" PUBLIC
                                                         globalsettings) endif()

                    importJson11() target_link_libraries(
                        "${PROJECT_NAME}" PRIVATE json11 srcparser
                            stdc++ fs) endfunction()

                        function(importJson11) add_library(json11 json11 /
                                                           json11.hpp json11 /
                                                           json11.cpp)

                            set_target_properties(
                                json11 PROPERTIES
                                    CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES
                                        CXX_EXTENSIONS NO)

                                target_include_directories(json11 PUBLIC json11)
                                    endfunction()

                                        main()
