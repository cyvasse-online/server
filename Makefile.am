SUBDIRS = cyvasse-common libb64 .

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cyvasse-server

cyvasse_server_SOURCES = \
	src/b64.hpp \
	src/client_data.hpp \
	src/cyvasse_server.cpp \
	src/cyvasse_server.hpp \
	src/job_handler.cpp \
	src/job_handler.hpp \
	src/main.cpp \
	src/match_data.hpp

cyvasse_server_CPPFLAGS = \
	-I$(top_srcdir)/cyvasse-common/include \
	-I$(top_srcdir)/libb64/include

cyvasse_server_CXXFLAGS = \
	-std=c++11 \
	-pthread

cyvasse_server_LDFLAGS = \
	-pthread

cyvasse_server_LDADD = \
	$(top_builddir)/cyvasse-common/libcyvdb.a \
	$(top_builddir)/cyvasse-common/libcyvmath.a \
	$(top_builddir)/libb64/src/libb64.a \
	-lboost_system \
	-lcxxtools \
	-ljsoncpp \
	-ltntdb \
	-lyaml-cpp
