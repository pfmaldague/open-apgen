bin_PROGRAMS = apgen

apgen_SOURCES = apgen.c

AUTOMAKE_OPTIONS = subdir-objects

#
# Disable apcore_server for now - reinstated 12/2/2019 PFM
# Add tpf (Terse Plan Format) directory 3/31/2020 PFM
#
if BUILD_EVERYTHING
SUBDIRS= xmlrpc apcore tpf apcore_server apgenGUI .
else
SUBDIRS= xmlrpc apcore apcore_server apgenGUI .
endif

if JSON_INSTALLED
nobase_include_HEADERS = \
	apgenlib/AAFlibrary.H \
	apgenlib/aafReader.H \
	apgenlib/AbstractResource.H \
	apgenlib/accessorkey.H \
	apgenlib/ACT_exec.H \
	apgenlib/action_request_client.H \
	apgenlib/action_request.H \
	apgenlib/act_plan_parser.H \
	apgenlib/ActivityInstance.H \
 	apgenlib/ACT_sys.H \
	apgenlib/alphastring.H \
	apgenlib/alphatime.H \
	apgenlib/APbasic.H \
	apgenlib/APcolors.H \
	apgenlib/apcore_api.H \
	apgenlib/apcore_intfc.H \
	apgenlib/APcoreServer.H \
	apgenlib/apcoreWaiter.H \
	apgenlib/APsax.H \
	apgenlib/AP_cxx_intfc.H \
	apgenlib/APdata.H \
	apgenlib/apDEBUG.H \
	apgenlib/APerrors.H \
	apgenlib/AP_exp_eval.H \
	apgenlib/apgen_editor.H \
	apgenlib/apgen_globwin.H \
	apgenlib/apgen_mainwin.H \
	apgenlib/apgen_xmlwin.H \
	apgenlib/APmodel.H \
	apgenlib/APrequest_handler.H \
	apgenlib/array_key.H \
	apgenlib/backpointer.H \
	apgenlib/base64.H \
	apgenlib/BehavingElement.H \
	apgenlib/C_global.H \
	apgenlib/C_list.H \
	apgenlib/client_errors.H \
	apgenlib/CMD_exec.H \
	apgenlib/CompilerIntfc.H \
	apgenlib/concat_util.h \
	apgenlib/config.h \
	apgenlib/Constraint.H \
	apgenlib/CON_sys.H \
	apgenlib/c_parser_bridge.h \
	apgenlib/C_string.H \
	apgenlib/DB.H \
	apgenlib/doublelist.H \
	apgenlib/DSsource.H \
	apgenlib/EventRegistry.H \
	apgenlib/ExecStack.H \
	apgenlib/ExecutionContext.H \
	apgenlib/fileReader.H \
	apgenlib/flexval.H \
	apgenlib/gen-parsedExp.H \
	apgenlib/GLOBdata.H \
	apgenlib/gmsec_api.H \
	apgenlib/gramgen.H \
	apgenlib/gtk_bridge.H \
	apgenlib/instruction_node.H \
	apgenlib/EventBase.H \
	apgenlib/EventImpl.H \
	apgenlib/EventLoop.H \
	apgenlib/IO_activitylayout.H \
	apgenlib/IO_ApgenData.H \
	apgenlib/IO_ApgenDataOptions.H \
	apgenlib/IO_ApgenEnums.h \
	apgenlib/IO_plot.H \
	apgenlib/IO_plot_intfc.H \
	apgenlib/IO_plot_util.H \
	apgenlib/IO_SASFWrite.H \
	apgenlib/IO_SASFWriteImpl.H \
	apgenlib/IO_write.H \
	apgenlib/jpl.H \
	apgenlib/Legends.H \
	apgenlib/lex_intfc.H \
	apgenlib/libcrest_dto.h \
	apgenlib/libcrest.h \
	apgenlib/linkers.H \
	apgenlib/Miterator.H \
	apgenlib/multilist.H \
	apgenlib/multitemplate.H \
	apgenlib/Multiterator.H \
	apgenlib/mw_intfc.H \
	apgenlib/panel.H \
	apgenlib/ParsedExpressionSystem.H \
	apgenlib/perf_util.H \
	apgenlib/planning-time.H \
	apgenlib/Prefs.H \
	apgenlib/RD_sys.H \
	apgenlib/RES_def.H \
	apgenlib/RES_eval.H \
	apgenlib/RES_exceptions.H \
	apgenlib/RES_exec.H \
	apgenlib/ValueNode.H \
	apgenlib/Rsource.H \
	apgenlib/res_multiterator.H \
	apgenlib/Scheduler.H \
	apgenlib/Strategy.H \
	apgenlib/slist.H \
	apgenlib/smartptr.H \
	apgenlib/srComboBox.h \
	apgenlib/srComboBoxP.h \
	apgenlib/tag_tree.h \
	apgenlib/templates.H \
	apgenlib/ThreadIntfc.H \
	apgenlib/timeline_api.H \
	apgenlib/timeptr.H \
	apgenlib/TOL_write.H \
	apgenlib/toolbar.H \
	apgenlib/tooltip.h \
	apgenlib/treeDir.H \
	apgenlib/TypedValue.H \
	apgenlib/UI_abspanzoom.H \
	apgenlib/UI_activitydisplay.H \
	apgenlib/UIComponent.H \
	apgenlib/UI_defs.H \
	apgenlib/UI_dsconfig.h \
	apgenlib/UI_ds_draw.H \
	apgenlib/UI_ds_timeline.H \
	apgenlib/UI_exec.H \
	apgenlib/UI_GeneralMotifInclude.H \
	apgenlib/UI_mainwindowbx.H \
	apgenlib/UI_mainwindow.H \
	apgenlib/UI_messagewarndialog.H \
	apgenlib/UI_MotifPreview.H \
	apgenlib/UI_motif_widget.H \
	apgenlib/UI_mw_draw.H \
	apgenlib/UI_mw_timeline.H \
	apgenlib/UI_openfsd.H \
	apgenlib/UI_patterns.H \
	apgenlib/UI_popups.H \
	apgenlib/UI_printdialog.H \
	apgenlib/UI_resabsscrollzoombx.H \
	apgenlib/UI_resabsscrollzoom.H \
	apgenlib/UI_resourcedisplay.H \
	apgenlib/UI_utility.H \
	apgenlib/UTL_defs.H \
	apgenlib/UTL_stl.H \
	apgenlib/UTL_time_base.H \
	apgenlib/UTL_time.H \
	apgenlib/v_list.H \
	apgenlib/XM.h \
	apgenlib/XmlRpcClient.H \
	apgenlib/XmlRpcCompress.H \
	apgenlib/XmlRpcDispatch.H \
	apgenlib/XmlRpcException.H \
	apgenlib/XmlRpc.H \
	apgenlib/xmlrpc_intfc.H \
	apgenlib/XmlRpcServerConnection.H \
	apgenlib/XmlRpcServer.H \
	apgenlib/XmlRpcServerMethod.H \
	apgenlib/XmlRpcSocket.H \
	apgenlib/XmlRpcSource.H \
	apgenlib/XmlRpcUtil.H \
	apgenlib/XmlRpcValue.H \
	apgenlib/xml_act_parser.H \
	apgenlib/xml_incon_parser.H \
	apgenlib/xml_sax_parser.H

else 

nobase_include_HEADERS = \
	apgenlib/AAFlibrary.H \
	apgenlib/aafReader.H \
	apgenlib/AbstractResource.H \
	apgenlib/accessorkey.H \
	apgenlib/ACT_exec.H \
	apgenlib/action_request_client.H \
	apgenlib/action_request.H \
	apgenlib/act_plan_parser.H \
	apgenlib/ActivityInstance.H \
 	apgenlib/ACT_sys.H \
	apgenlib/alphastring.H \
	apgenlib/alphatime.H \
	apgenlib/APbasic.H \
	apgenlib/APcolors.H \
	apgenlib/apcore_api.H \
	apgenlib/apcore_intfc.H \
	apgenlib/APcoreServer.H \
	apgenlib/apcoreWaiter.H \
	apgenlib/APsax.H \
	apgenlib/AP_cxx_intfc.H \
	apgenlib/APdata.H \
	apgenlib/apDEBUG.H \
	apgenlib/APerrors.H \
	apgenlib/AP_exp_eval.H \
	apgenlib/apgen_editor.H \
	apgenlib/apgen_globwin.H \
	apgenlib/apgen_mainwin.H \
	apgenlib/apgen_xmlwin.H \
	apgenlib/APmodel.H \
	apgenlib/APrequest_handler.H \
	apgenlib/array_key.H \
	apgenlib/backpointer.H \
	apgenlib/base64.H \
	apgenlib/BehavingElement.H \
	apgenlib/C_global.H \
	apgenlib/client_errors.H \
	apgenlib/C_list.H \
	apgenlib/CMD_exec.H \
	apgenlib/CompilerIntfc.H \
	apgenlib/concat_util.h \
	apgenlib/config.h \
	apgenlib/Constraint.H \
	apgenlib/CON_sys.H \
	apgenlib/c_parser_bridge.h \
	apgenlib/C_string.H \
	apgenlib/DB.H \
	apgenlib/doublelist.H \
	apgenlib/DSsource.H \
	apgenlib/EventRegistry.H \
	apgenlib/ExecStack.H \
	apgenlib/ExecutionContext.H \
	apgenlib/fileReader.H \
	apgenlib/flexval.H \
	apgenlib/gen-parsedExp.H \
	apgenlib/GLOBdata.H \
	apgenlib/gmsec_api.H \
	apgenlib/gtk_bridge.H \
	apgenlib/instruction_node.H \
	apgenlib/EventBase.H \
	apgenlib/EventImpl.H \
	apgenlib/EventLoop.H \
	apgenlib/IO_activitylayout.H \
	apgenlib/IO_ApgenData.H \
	apgenlib/IO_ApgenDataOptions.H \
	apgenlib/IO_ApgenEnums.h \
	apgenlib/IO_plot.H \
	apgenlib/IO_plot_intfc.H \
	apgenlib/IO_plot_util.H \
	apgenlib/IO_SASFWrite.H \
	apgenlib/IO_SASFWriteImpl.H \
	apgenlib/IO_write.H \
	apgenlib/jpl.H \
	apgenlib/Legends.H \
	apgenlib/libcrest_dto.h \
	apgenlib/libcrest.h \
	apgenlib/linkers.H \
	apgenlib/Miterator.H \
	apgenlib/multilist.H \
	apgenlib/multitemplate.H \
	apgenlib/Multiterator.H \
	apgenlib/mw_intfc.H \
	apgenlib/panel.H \
	apgenlib/ParsedExpressionSystem.H \
	apgenlib/perf_util.H \
	apgenlib/planning-time.H \
	apgenlib/Prefs.H \
	apgenlib/RD_sys.H \
	apgenlib/RES_def.H \
	apgenlib/RES_eval.H \
	apgenlib/RES_exceptions.H \
	apgenlib/RES_exec.H \
	apgenlib/ValueNode.H \
	apgenlib/Rsource.H \
	apgenlib/res_multiterator.H \
	apgenlib/Scheduler.H \
	apgenlib/Strategy.H \
	apgenlib/slist.H \
	apgenlib/smartptr.H \
	apgenlib/srComboBox.h \
	apgenlib/srComboBoxP.h \
	apgenlib/tag_tree.h \
	apgenlib/templates.H \
	apgenlib/ThreadIntfc.H \
	apgenlib/timeline_api.H \
	apgenlib/timeptr.H \
	apgenlib/TOL_write.H \
	apgenlib/toolbar.H \
	apgenlib/tooltip.h \
	apgenlib/treeDir.H \
	apgenlib/TypedValue.H \
	apgenlib/UI_abspanzoom.H \
	apgenlib/UI_activitydisplay.H \
	apgenlib/UIComponent.H \
	apgenlib/UI_defs.H \
	apgenlib/UI_dsconfig.h \
	apgenlib/UI_ds_draw.H \
	apgenlib/UI_ds_timeline.H \
	apgenlib/UI_exec.H \
	apgenlib/UI_GeneralMotifInclude.H \
	apgenlib/UI_mainwindowbx.H \
	apgenlib/UI_mainwindow.H \
	apgenlib/UI_messagewarndialog.H \
	apgenlib/UI_MotifPreview.H \
	apgenlib/UI_motif_widget.H \
	apgenlib/UI_mw_draw.H \
	apgenlib/UI_mw_timeline.H \
	apgenlib/UI_openfsd.H \
	apgenlib/UI_patterns.H \
	apgenlib/UI_popups.H \
	apgenlib/UI_printdialog.H \
	apgenlib/UI_resabsscrollzoombx.H \
	apgenlib/UI_resabsscrollzoom.H \
	apgenlib/UI_resourcedisplay.H \
	apgenlib/UI_utility.H \
	apgenlib/UTL_defs.H \
	apgenlib/UTL_stl.H \
	apgenlib/UTL_time_base.H \
	apgenlib/UTL_time.H \
	apgenlib/v_list.H \
	apgenlib/XM.h \
	apgenlib/XmlRpcClient.H \
	apgenlib/XmlRpcCompress.H \
	apgenlib/XmlRpcDispatch.H \
	apgenlib/XmlRpcException.H \
	apgenlib/XmlRpc.H \
	apgenlib/xmlrpc_intfc.H \
	apgenlib/XmlRpcServerConnection.H \
	apgenlib/XmlRpcServer.H \
	apgenlib/XmlRpcServerMethod.H \
	apgenlib/XmlRpcSocket.H \
	apgenlib/XmlRpcSource.H \
	apgenlib/XmlRpcUtil.H \
	apgenlib/XmlRpcValue.H \
	apgenlib/xml_act_parser.H \
	apgenlib/xml_incon_parser.H \
	apgenlib/xml_sax_parser.H \
	apgenlib/arraylist.h \
	apgenlib/json.h \
	apgenlib/json_inttypes.h \
	apgenlib/json_object_private.h \
	apgenlib/linkhash.h \
	apgenlib/bits.h \
	apgenlib/json_c_version.h \
	apgenlib/json_object.h \
	apgenlib/json_tokener.h \
	apgenlib/printbuf.h \
	apgenlib/debug.h \
	apgenlib/json_config.h \
	apgenlib/json_object_iterator.h \
	apgenlib/json_util.h \
	apgenlib/random_seed.h

endif

#	apgenlib/IO_protocols.H
#	apgenlib/TMS_intfc.H

EXTRA_DIST = apgenlib config bootstrap mrproper \
	build.sh
