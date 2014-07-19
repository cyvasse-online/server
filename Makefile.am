SUBDIRS = cyvmath libb64 .

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = cyvasse-server

cyvasse_server_SOURCES = \
	src/b64.hpp \
	src/client_data.cpp \
	src/client_data.hpp \
	src/cyvasse_server.cpp \
	src/cyvasse_server.hpp \
	src/job_handler.cpp \
	src/job_handler.hpp \
	src/main.cpp \
	src/match_data.cpp \
	src/match_data.hpp \
	src/db_config.hpp \
	src/db/match.hpp \
	src/db/match_manager.cpp \
	src/db/match_manager.hpp \
	src/db/player.hpp \
	src/db/player_manager.cpp \
	src/db/player_manager.hpp

cyvasse_server_CPPFLAGS = \
	-I$(top_srcdir)/cyvmath/include \
	-I$(top_srcdir)/libb64/include

cyvasse_server_CXXFLAGS = \
	-std=c++11 \
	-pthread

cyvasse_server_LDFLAGS = \
	-pthread

cyvasse_server_LDADD = \
	$(top_builddir)/cyvmath/libcyvmath.a \
	$(top_builddir)/libb64/src/libb64.a \
	-lboost_system \
	-lcxxtools \
	-ljsoncpp \
	-ltntdb
