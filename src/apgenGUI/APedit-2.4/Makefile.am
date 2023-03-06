## noinst_LIBRARIES = libAPedit.a
lib_LTLIBRARIES = libAPedit.la

libAPedit_la_SOURCES = \
	../../APedit-2.4/gtk_editwin.C \
	../../APedit-2.4/gtk_xmlwin.C \
	../../APedit-2.4/gtk_globwin.C \
	../../APedit-2.4/gtk_interface.C \
	../../APedit-2.4/gtk_mainwin.C

LIBS = `pkg-config gtkmm-2.4 --libs` \
	`pkg-config libxml++-3.0 --libs`

AM_CPPFLAGS = `pkg-config gtkmm-2.4 --cflags` \
	      `pkg-config libxml++-3.0 --cflags`

