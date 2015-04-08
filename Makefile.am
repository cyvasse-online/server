SUBDIRS = cyvasse-common libb64 .

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cyvasse-server

cyvasse_server_SOURCES = \
	src/cyvasse_server.cpp \
	src/main.cpp \
	src/shared_server_data.cpp \
	src/worker.cpp

cyvasse_server_CPPFLAGS = \
	-I$(top_srcdir)/cyvasse-common/include \
	-I$(top_srcdir)/libb64/include

cyvasse_server_CXXFLAGS = \
	$(JSONCPP_CFLAGS) \
	-std=c++11 \
	-pthread

cyvasse_server_LDFLAGS = \
	-pthread

cyvasse_server_LDADD = \
	$(JSONCPP_LIBS) \
	$(top_builddir)/cyvasse-common/libcyvasse.a \
	$(top_builddir)/cyvasse-common/libcyvdb.a \
	$(top_builddir)/cyvasse-common/libcyvws.a \
	$(top_builddir)/libb64/src/libb64.a \
	-lboost_system \
	-lcxxtools \
	-ltntdb \
	-lyaml-cpp
