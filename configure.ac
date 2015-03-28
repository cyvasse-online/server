AC_INIT([cyvasse-server],[0.1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign])
AM_SILENT_RULES([yes])

AC_PROG_CXX
AC_LANG(C++)
AX_CHECK_COMPILE_FLAG([-std=c++14], [CXXFLAGS="$CXXFLAGS -std=c++14"],
	AX_CHECK_COMPILE_FLAG([-std=c++1y], [CXXFLAGS="$CXXFLAGS -std=c++1y"],
		AC_MSG_ERROR([Compiler doesn't support C++14])
	)
)
AX_CHECK_COMPILE_FLAG([-Wall], [CXXFLAGS="$CXXFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra], [CXXFLAGS="$CXXFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-pedantic], [CXXFLAGS="$CXXFLAGS -pedantic"])

AC_CHECK_HEADER([websocketpp/version.hpp], [], AC_MSG_ERROR([websocket++ headers not found]))
AC_CHECK_HEADER([yaml-cpp/yaml.h], [], AC_MSG_ERROR([yaml-cpp headers not found]))

AC_CHECK_FUNCS([sigaction])

PKG_CHECK_MODULES([JSONCPP], [jsoncpp])

AC_CONFIG_SUBDIRS([cyvasse-common libb64])
AC_CONFIG_FILES([
	Makefile
	config.yml
])
AC_OUTPUT
