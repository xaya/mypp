cmake_minimum_required(VERSION 3.14)
project(mypp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Werror -pedantic -Wno-deprecated)
add_compile_definitions(GLOG_NO_ABBREVIATED_SEVERITIES)

find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED IMPORTED_TARGET mariadb)
pkg_check_modules(GLOG   REQUIRED IMPORTED_TARGET libglog)
pkg_check_modules(GTEST  REQUIRED IMPORTED_TARGET gtest_main)

include(GNUInstallDirs)
enable_testing()

configure_file(mypp.pc.in ${CMAKE_CURRENT_BINARY_DIR}/mypp.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/mypp.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

add_subdirectory(src)
