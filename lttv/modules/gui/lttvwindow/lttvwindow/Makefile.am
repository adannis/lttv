## Process this file with automake to produce Makefile.in

include ../../common.am

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(DEFAULT_INCLUDES)

#libdir = ${lttvplugindir}

lib_LTLIBRARIES = liblttvwindow.la


liblttvwindow_la_SOURCES = \
	toolbar.c\
	menu.c\
	timebar.c\
	timeentry.c\
	lttvwindow.c \
	lttvwindowtraces.c \
	init_module.c \
	support.c \
	interface.c \
	callbacks.c \
	lttv_plugin_tab.c \
	lttv_plugin.c

noinst_HEADERS = \
	support.h \
	interface.h \
	callbacks.h\
	mainwindow-private.h

lttvwindowinclude_HEADERS = \
	lttvwindow.h\
	lttvwindowtraces.h\
	mainwindow.h\
	menu.h\
	toolbar.h\
	timebar.h\
	timeentry.h\
	lttv_plugin_tab.h \
	lttv_plugin.h

