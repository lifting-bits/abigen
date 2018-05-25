cmake_minimum_required(VERSION 3.10)

    set(CMAKE_EXPORT_COMPILE_COMMANDS True)

        add_library(globalsettings INTERFACE)

            target_compile_options(globalsettings INTERFACE - fvisibility =
                                       hidden - fvisibility - inlines - hidden -
                                       Werror - Wall - Wextra - Wconversion)

                if (CMAKE_BUILD_TYPE STREQUAL
                    "Debug" OR CMAKE_BUILD_TYPE STREQUAL
                    "RelWithDebInfo") if (UNIX)
                    target_compile_options(globalsettings INTERFACE - gdwarf -
                                           2 - g3) endif()

                        set(CMAKE_VERBOSE_MAKEFILE True) endif()
