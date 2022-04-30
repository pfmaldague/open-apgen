lib_LTLIBRARIES = libModeler.la 


libModeler_la_SOURCES = \
		EventLoop.C \
		Execute.C \
		Events.C \
		Constraint.C

if JSON_INSTALLED

AM_CPPFLAGS = `pkg-config --cflags json-c`
AM_LDFLAGS = `pkg-config --libs json-c`

else

AM_CPPFLAGS = -I../../json-c/include

endif
