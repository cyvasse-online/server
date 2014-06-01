SUBDIRS = libb64

AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = \
	-std=c++11

bin_PROGRAMS = cyvasse-server

cyvasse_server_SOURCES = \
	src/cyvasse_server.cpp \
	src/cyvasse_server.hpp \
	src/job_data.cpp \
	src/job_data.hpp \
	src/main.cpp

cyvasse_server_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-I $(top_srcdir)/libb64/include

cyvasse_server_LDADD = \
	$(top_builddir)/libb64/src/libb64.a \
	-lboost_system \
	-ljsoncpp
