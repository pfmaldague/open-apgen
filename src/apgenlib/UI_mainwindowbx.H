#ifndef UI_mainwindowbx_H
#define UI_mainwindowbx_H
#include "UI_motif_widget.H"

class UI_mainwindowbx : public motif_widget {
public:
	UI_mainwindowbx(const char *,  motif_widget * ) ;
	~UI_mainwindowbx();
#ifdef GUI

			// allow the constructor to call derived class method:
	virtual void	initialize() {}

	void		create( motif_widget * ) ;
			// make CC -O faster (create() is too long):
	void		create_part_2( motif_widget * ) ;
#endif
	motif_widget	*_panedWindow , *_toolbarForm, *_printToolbarButton, *_remodelToolbarButton,
			/* *_rdZoomOutToolbarButton, *_rdZoomInToolbarButton, */ *_closeSRDToolbarButton, *_newRDToolbarButton,
			*_zoomoutToolbarButton, *_zoominToolbarButton, *_closeSADToolbarButton, *_newADToolbarButton,
			*_editActToolbarButton, *_pasteToolbarButton, *_copyToolbarButton, *_cutToolbarButton,
			*_menuBar, *_activityDesignButton, *_giantButton, *_hopperButton, *_unfreezeButton;

protected:
	static motif_widget	*_statusBarLabel;
	motif_widget	*_fileButton,	*_fileButtonPM , *_integratePlanFilesButton,
			*_writeIntegratedPlanFileButton, *_exportDataButton, *_genSASFButton,
			*_genTOLButton, *_genXMLTOLButton, *_printButton,
			*_genCsourceButton , *_purgeButton,
			*_quitButton, *_fast_quitButton, *_opsrevButton, *_seqrButton,
			*_editButton,				*_editButtonPM,
			*_cutButton,				*_copyButton ,
			*_openButton ,				*_savePlanFilesButton ,
			*_pasteButton,				*_activityButton,
			*_activityButtonPM,			*_newActivityButton,
			*_newActivitiesButton,
			*_abstractActivityButton,		*_abstractAllButton,
			*_abstractAllRecursivelyButton,		*_fullyAbstractActivityButton,
			*_detailActivityButton,			*_detailAllButton,
			*_detailAllRecursivelyButton,		*_redetailAllRecursivelyButton,
			*_redetailActivityButton,		*_redetailAllButton,
			*_deleteAllDescButton,			*_editActivityButton,
			*_gtkEditActivityButton ,		*_fullyDetailActivityButton,
			*_activityDefinitionsButton ,		*_activityLockSelection ,
			*_activityUnlockSelection ,		*_preferences ,
			*_showActivityNameButton ,		*_showActivityTypeButton ,
			*_showActivityIDButton ,		*_showActivityLabelButton,
			*_groupActivitiesButton ,		*_ungroupActivitiesButton;
	motif_widget	*_resourceButton,			*_resourceButtonPM,
			*_addResourceButton,			*_removeResourceButton,
			*_remodelButton;

			/* *_remodelRealTimeButton , *_remodelStopButton , *_remodelOneStepAtATimeButton */

	motif_widget	*actDisplayButton ,
			*resDisplayButton ,			*actDisplayButtonPM ,
			*resDisplayButtonPM ,			*_newActivityDisplayButton,
			*_newResourceDisplayButton,		*_newLegendButton,
			*_closeSelectedADsButton,		*_closeSelectedRDsButton,
			*_deleteSelectedLegendsButton ,		*_squishSelectedLegendsButton ,
			*_flattenSelectedLegendsButton ,
			*_timeSystemButton ,			*_unsquishSelectedLegendsButton ,
			*_expandSelectedLegendsButton ,		*_unexpandSelectedLegendsButton ,
			*_unselectAllLegendsButton ,
			*_zoominButton ,			*_zoomoutButton ,
			*_absolutePanZoomButton ,		*_resOptionsButton ,
			*_resZoominButton ,			*_resZoomoutButton ,
			*_resAbsScrollZoomButton ,		*_helpButton,
			*_helpButtonPM ,			*_apgenHelpButton,
			*_searchForHelpOnButton ,		*_aboutApgenButton, *_componentsButton;
	motif_widget	*_schedulingButton ,			*_schedulingButtonPM ,
			*_scheduleAllButton ,			*_unscheduleButton ,
			*_scheduleRealTimeButton ,		*_scheduleOneStepAtATimeButton ;
	motif_widget	*_unscheduleAllButton ,			*_scheduleSelectionButton ;
#ifdef HAVE_EUROPA
public:
	motif_widget	*_planButton, *_planButtonPM, *_planInitButton, *_planAllButton,
		*_planGoalButton,  *_unplanGoalButton,*_fixResourcesButton, *_displayParentsButton,
		*_plannerOnButton,
		*_plannerOffButton, *_undoPlanningButton, *_placeSelectedGoalButton,
		*_setRelaxedModeButton, *_setFullModeButton, *_clearConstraintsButton,
		*_pinSelectedButton, *_unpinSelectedButton, *_unpinAllButton, *_selectPlannedActivitiesButton,
	        *_addHeatingContraintsButton, *_removeHeatingContraintsButton, *_regenerateHeaterActivitiesButton,
	        *_updateSelectedHeaterActivitiesButton;

	//planner buttons
	motif_widget *_plannerStateToolbarButton, *_plannerModeToolbarButton;

	void SetPlannerButton(bool on);
	
	void SetRelaxedButton(bool on);
protected:
#endif
#ifdef GUI
	callback_stuff	data_for_window_closing ;
#endif
} ;

#endif
