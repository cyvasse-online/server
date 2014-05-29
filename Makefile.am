AUTOMAKE_OPTIONS = subdir-objects

AM_CXXFLAGS = \
	-std=c++11

bin_PROGRAMS = cyvasse-server

cyvasse_server_SOURCES = \
	src/cyvasse_server.cpp \
	src/cyvasse_server.hpp \
	src/main.cpp

cyvasse_server_LDADD = \
	-lboost_system
