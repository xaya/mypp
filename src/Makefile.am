lib_LTLIBRARIES = libmypp.la
myppdir = $(includedir)/mypp

libmypp_la_CXXFLAGS = \
  $(MARIADB_CFLAGS) $(GLOG_CFLAGS)
libmypp_la_LIBADD = \
  $(MARIADB_LIBS) $(GLOG_LIBS)
libmypp_la_SOURCES = \
  connection.cpp \
  statement.cpp \
  tempdb.cpp \
  url.cpp
mypp_HEADERS = \
  connection.hpp \
  error.hpp \
  statement.hpp \
  tempdb.hpp \
  url.hpp

check_PROGRAMS = tests
TESTS = tests

tests_CXXFLAGS = \
  $(MARIADB_CFLAGS) $(GLOG_CFLAGS) \
  $(GTEST_CFLAGS)
tests_LDADD = $(builddir)/libmypp.la \
  $(MARIADB_LIBS) $(GLOG_LIBS) \
  $(GTEST_LIBS)
tests_SOURCES = \
  statement_tests.cpp \
  url_tests.cpp
