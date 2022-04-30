##This makefile.am is intended to include some User Interface utility files that are
##used to build user interfaces in different applications. (APGEN, seqtalk monitor)
## as more UI stuff is used by applications other that APGEN, it may make sense to 
##add more files to this library achase - 4/15/2002
##currently to use this library, I think you must also link with APutil

noinst_LIBRARIES = libUIutil.a


libUIutil_a_SOURCES = \
	../../../atm_client/UIutil/ComboBox.c \
	../../../atm_client/UIutil/Converter.c \
	../../../atm_client/UIutil/UI_motif_widget.C \
	../../../atm_client/UIutil/UI_utility.C \
	../../../atm_client/UIutil/UI_bxutil.C \
	../../../atm_client/UIutil/UIComponent.C \
	../../../atm_client/UIutil/tooltip.c

if USE_LIBXMLCXX

if JSON_INSTALLED
AM_CPPFLAGS = -DGUI @APGEN_X_INCL@ @APGEN_XM_INCL@ `pkg-config libxml++-3.0 --cflags json-c`
else
AM_CPPFLAGS = -DGUI @APGEN_X_INCL@ @APGEN_XM_INCL@ `pkg-config libxml++-3.0 --cflags` \
	-I../../../json-c/include
endif
else 
if JSON_INSTALLED
AM_CPPFLAGS = @APGEN_X_INCL@ @APGEN_XM_INCL@ -DGUI `pkg-config libxml++-3.0 --cflags json-c`
else
AM_CPPFLAGS = @APGEN_X_INCL@ @APGEN_XM_INCL@ -DGUI \
	-I../../../json-c/include
endif

endif
