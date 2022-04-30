
lib_LTLIBRARIES = libAPGENdata.la

## bin_PROGRAMS = testtemplates

libAPGENdata_includedir = $(includedir)

if USE_LIBXMLCXX

if JSON_INSTALLED
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags json-c`
AM_LDFLAGS = `pkg-config libxml++-3.0 --libs json-c`
else
AM_CPPFLAGS = `pkg-config libxml++-3.0 --cflags` \
	-I../../json-c/include \
	-pthread
AM_LDFLAGS = `pkg-config libxml++-3.0 --libs`
endif

else
if JSON_INSTALLED
AM_CPPFLAGS = `pkg-config --cflags json-c`
AM_LDFLAGS = `pkg-config --libs json-c`
else
AM_CPPFLAGS = \
	-I../../json-c/include \
	-pthread
AM_LDFLAGS =
endif
endif

libAPGENdata_la_LIBADD =
## CHECK IF GOOGLE TEST LIBRARY EXISTS
if USE_GOOGLETEST

AM_CPPFLAGS += -isystem ../../../../googletest/googletest/include -pthread ## PATH TO googletest\include
libAPGENdata_la_LIBADD += ../../../../libgtest.a ## PATH TO built libgtest.a

endif
# END NEW

if !JSON_INSTALLED
libAPGENdata_la_LIBADD += ../../json-c/lib/libjson-c.a
endif

## testtemplates_SOURCES = test.C

## testtemplates_LDADD = libAPGENdata.la \
## 	../../libUtils.la

## testtemplates_LDFLAGS = `pkg-config gdome2 --libs`

libAPGENdata_la_SOURCES = APGEN_Title.C \
	aostring.C \
	cstring.C \
	C_list.C \
	UTL_time.C \
	validate.C \
	v_list.C \
	server.c \
	blist.C \
	APdata.C \
	tag_tree.C \
	ValueNode.C \
	Multiterator.C \
	xmlrpc_api.C \
	test.C \
	test_time.C \
	test_multi.C \
	test_typed_value.C \
	test_json.C \
	test_json_parser.C \
	test_xml2.C \
	test_xml_parsing.C \
	test-xslt.C \
	test_json_resource.C \
	test_double_list.C \
	time.C \
	flexval.C \
	sax.C \
	test_regex.C \
	test_interval_logic.C

## CHECK IF GOOGLE TEST LIBRARY EXISTS, IF NOT, DON'T BUILD GOOGLE TEST SCRIPTS
if USE_GOOGLETEST

libAPGENdata_la_SOURCES += test_gtest.cc \
	gtest_slist.cc \
	gtest_tlist_parameterized.cc \
	gtest_multi.cc \
	gtest_multi_repeated.cc

endif
## END NEW
.PHONY: apgen_title

apgen_title:
	echo  ' char APGEN_build_date[] = "' `date` '"; ' >> $@.tmp
	echo  ' char APGEN_build_id[] = "'@BUILDTAG@'"; ' >> $@.tmp
	mv $@.tmp APGEN_Title.C

APGEN_Title.C APGEN_Title.o: apgen_title
