lib_LTLIBRARIES = libAPxml.la
bin_PROGRAMS = autolisten

libAPxml_la_SOURCES = \
	XmlRpcClient.C \
	XmlRpcDispatch.C \
	XmlRpcServer.C \
	XmlRpcServerConnection.C \
	XmlRpcServerMethod.C \
	XmlRpcSocket.C \
	XmlRpcSource.C \
	XmlRpcUtil.C \
	XmlRpcValue.C \
	XmlRpcCompress.C

autolisten_SOURCES = auto_listen.C

autolisten_LDADD = ./libAPxml.la
