AC_INIT([cyvasse-server],[0.1],[mail@jonasplatte.de])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([1.11 foreign])
AM_SILENT_RULES([yes])
AC_PROG_CXX
AC_LANG(C++)

AC_CHECK_HEADER([jsoncpp/json/json.h], [], AC_MSG_ERROR([jsoncpp headers not found]))
AC_CHECK_HEADER([tntdb/connection.h], [], AC_MSG_ERROR([tntdb headers not found]))
AC_CHECK_HEADER([websocketpp/version.hpp], [], AC_MSG_ERROR([websocket++ headers not found]))

AC_CHECK_FUNCS([sigaction])

AC_CONFIG_SUBDIRS([cyvmath libb64])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
