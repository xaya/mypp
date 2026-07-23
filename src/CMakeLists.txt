add_library(mypp SHARED
  connection.cpp
  statement.cpp
  tempdb.cpp
  url.cpp
)

target_link_libraries(mypp
  PUBLIC  PkgConfig::MARIADB
  PRIVATE PkgConfig::GLOG
)

target_include_directories(mypp PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mypp>
)

set_target_properties(mypp PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS mypp
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
  connection.hpp
  error.hpp
  statement.hpp
  tempdb.hpp
  url.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mypp
)

add_executable(tests
  statement_tests.cpp
  url_tests.cpp
)

target_link_libraries(tests PRIVATE
  mypp
  PkgConfig::GTEST
  PkgConfig::GLOG
)

include(GoogleTest)
gtest_discover_tests(tests
  PROPERTIES RUN_SERIAL TRUE
)
