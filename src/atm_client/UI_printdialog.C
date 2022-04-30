#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "UI_printdialog.H"
#include "IO_plot.H"
#include "UI_exec.H"	/* for changeCursor() */
#include "Prefs.H"

			// in main.C:
extern motif_widget*	MW ;

	                // in UI_motif_widget.C:
extern resource_data    ResourceData ;
			// in ACT_exec.C:
extern UI_exec*		UI_subsystem ;

#define SET_BACKGROUND_TO_GREY  XtSetArg(args[ac], XmNbackground, ResourceData.grey_color ) ; ac++;

//in IO_seqtalk.C
void bell();

UI_printdialog*			printDialog = NULL ;
UI_print_size_other_popup*	printSizePopup = NULL;

UI_printdialog::UI_printdialog(const char * name)
	: motif_widget ( name , xmDialogShellWidgetClass , MW , NULL , 0 , FALSE ),
	PageSizeIndex(0) { 
	create(name); }

UI_printdialog::~UI_printdialog() { }

void UI_printdialog::printCancelButtonActivate (Widget, void *, void *) {
	unmanage() ; }

void UI_printdialog::paperSizeSelectActivate(Widget w, callback_stuff* client_data, void* stuff)
{
  PageSizeIndex = (long) client_data->data;

  if(PageSizeIndex == 3)
  {
    if( ! printSizePopup )
      printSizePopup = new UI_print_size_other_popup("New Print Size", this, UI_print_size_other_popup::nlOKCallback, 
						     PageSizes[3].WidthInInches, PageSizes[3].HeightInInches, 
						     PageSizes[3].LeftMargin, PageSizes[3].TopMargin) ;

    printSizePopup->initialize() ;

    if ( ! (printSizePopup->_UI_print_size_other_popup->is_managed()) )
      printSizePopup->_UI_print_size_other_popup->manage() ; 
  }
  else
  {
    UpdatePageSizeDisplay();
  }
}

void
UI_printdialog::CancelOtherPrintPageSize()
{
  UpdatePageSizeDisplay();
}

void
UI_printdialog::SetOtherPrintPageSize(double width, double height, double leftMargin, double topMargin)
{
  PageSizes[3].WidthInInches = width;
  PageSizes[3].HeightInInches = height;
  PageSizes[3].LeftMargin = leftMargin;
  PageSizes[3].TopMargin = topMargin;

  UpdatePageSizeDisplay();
}

void
UI_printdialog::UpdatePageSizeDisplay()
{
  char buffer[1024];

  sprintf(buffer, "%'.2f x %'.2f, %'.2f %'.2f", PageSizes[PageSizeIndex].WidthInInches, PageSizes[PageSizeIndex].HeightInInches, PageSizes[PageSizeIndex].LeftMargin, PageSizes[PageSizeIndex].TopMargin);
  *thePrintPageSizeDisplay = buffer;
}

void UI_printdialog::printPrintButtonActivate (Widget, void *, void *) {
#ifdef GUI
	if( GUI_Flag ) {
	int		pagesPerDisplayWidth;
	IO_plotter	plotter;
	char		*title;
	char		*subtitle;

	UI_subsystem->changeCursor( 0 ) ;
	unmanage () ;
	XmScaleGetValue ( thePrintPPDWScale->widget , & pagesPerDisplayWidth ) ;
	XtVaGetValues ( thePrintTitleTF->widget , XmNvalue , & title , NULL ) ;
	XtVaGetValues ( thePrintSubtitleTF->widget , XmNvalue , & subtitle , NULL ) ;
	//Top selected AD, its timezone, and its selected RDs, are now determined
	//  here, so that changes to these selections while the Print Dialog is in
	//  front of the user, are now reflected in the output Plot.

	//Save preferences here
	Preferences().SetPreference("PrintTitle", title);
	Preferences().SetPreference("PrintSubTitle", subtitle);
	
	if(thePrintSelADsAndRDsTB->get_text() == "SET")
	  Preferences().SetPreference("PrintSource", "Plus");
	else
	  Preferences().SetPreference("PrintSource", "Top");

	Preferences().SetPreference("PrintFileName", *(fileNameWidget->get_text()));

	if(thePrintLegAllButton->get_text() == "SET")
	  Preferences().SetPreference("PrintLegendsSource", "All");
	else
	  Preferences().SetPreference("PrintLegendsSource", "Selected");
	    
	char numBuf[64];
	sprintf(numBuf, "%d", pagesPerDisplayWidth);

	Preferences().SetPreference("PrintPagesPerDisplayWidth", numBuf);

	string value = "Other";
	if(PageSizeIndex == 0)
	  value = "Letter";
	else if(PageSizeIndex == 1)
	  value = "Legal";
	else if(PageSizeIndex == 2)
	  value = "A4";
	
	Preferences().SetPreference("PrintPageSize", value);

	sprintf(numBuf, "%lf", PageSizes[3].HeightInInches);

	Preferences().SetPreference("PrintPageSizeOtherHeight", numBuf);

	sprintf(numBuf, "%lf", PageSizes[3].WidthInInches);

	Preferences().SetPreference("PrintPageSizeOtherWidth", numBuf);

	sprintf(numBuf, "%lf", PageSizes[3].LeftMargin);

	Preferences().SetPreference("PrintPageSizeOtherLeftMargin", numBuf);

	sprintf(numBuf, "%lf", PageSizes[3].TopMargin);

	Preferences().SetPreference("PrintPageSizeOtherTopMargin", numBuf);


	if(IncludeSummaryPage->get_text() == "SET")
	  Preferences().SetPreference("PrintIncludeSummaryPage", "TRUE");
	else
	  Preferences().SetPreference("PrintIncludeSummaryPage", "FALSE");

	//Save the preferences
	Preferences().SavePreferences();

	plotter.setPlotPageDimensions(PageSizes[PageSizeIndex].WidthInInches, 
				      PageSizes[PageSizeIndex].HeightInInches, 
				      PageSizes[PageSizeIndex].LeftMargin, 
				      PageSizes[PageSizeIndex].TopMargin);

	if(IncludeSummaryPage->get_text() == "SET")
	  plotter.showPlotSummary(true);
	else
	  plotter.showPlotSummary(false);

	if ( UI_subsystem->printButtonUpdateSelections() == apgen::RETURN_STATUS::SUCCESS )
		plotter.generate_plot (
			fileNameWidget->get_text() ,
			activitySystem ,
			resourceSystem ,
			managedSelectedResourceSystemList , //may be length 0;can't be NULL
			timezone ,
			( thePrintSelADsAndRDsTB->get_text() == "SET" ? TRUE : FALSE ) ,
			( thePrintLegAllButton->get_text() == "SET" ? TRUE : FALSE ) ,
			FALSE ,
			pagesPerDisplayWidth ,
			0 ,
			title ,
			subtitle ) ;
	// PFM
	XtFree( title ) ;
	XtFree( subtitle ) ;
	UI_subsystem->changeCursor( -1 ) ;
	}
#endif
}

void UI_printdialog::initialize(ACT_sys *as, RD_sys *rs, List &rl, TIMEZONE tz) {
	printDialog->activitySystem = as;
	printDialog->resourceSystem = rs;
	printDialog->managedSelectedResourceSystemList = rl;
	printDialog->timezone = tz; }

void UI_printdialog::create(const char *name ) {
#ifdef GUI
	if( GUI_Flag ) {
	Arg      	args[256];
	Cardinal	ac = 0;
	Boolean		argok = False;
	motif_widget	*fileNameLabel ;

	XtSetArg ( args[ac], XmNfractionBase, 39 ); ac++;
	thePrintDialog = new motif_widget(
		name,
		xmFormWidgetClass ,	this ,
		args,			ac ,
		FALSE ) ;

	thePrintRightForm = new motif_widget  ( "printRightForm",
		xmFormWidgetClass,	thePrintDialog, 
		NULL,			0 ,
		TRUE );

	theLeftForm = new motif_widget( "printLeftForm", xmFormWidgetClass, thePrintDialog , NULL, 0 , TRUE );

	thePrintSeparator = new motif_widget( "printSeparator",
		xmSeparatorWidgetClass,	thePrintDialog , 
   		NULL ,			0 ,
		TRUE );

	thePrintRightForm->add_property( CHILD_OF_A_FORM ) ;
	theLeftForm->add_property( CHILD_OF_A_FORM ) ;
	*thePrintDialog ^ 10 ^ *theLeftForm ^ 10 ^ * thePrintSeparator ^ endform ;
	*thePrintDialog ^ 10 ^ *thePrintRightForm ^ 10 ^ *thePrintSeparator ^ endform ;
	*thePrintDialog < *theLeftForm < form_position( 19 ) < endform ;
	form_position( 20 ) < *thePrintRightForm < *thePrintDialog < endform ;
	thePrintRightForm->add_property( FORM_PARENT ) ;
	theLeftForm->add_property( FORM_PARENT ) ;

	ac = 0;
	XtSetArg ( args[ac], XmNborderWidth, 0 ); ac++;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintPageFormatLabel = new motif_widget(
		"printPlotFormatLabel",
		xmLabelWidgetClass, thePrintRightForm, 
		args, ac ,
		TRUE );
	*thePrintPageFormatLabel = "Page Format:" ;
	*thePrintRightForm < *thePrintPageFormatLabel < *thePrintRightForm < endform ;

	theSeparator9 = new motif_widget( "separator9",
		xmSeparatorWidgetClass, thePrintRightForm ,
		args, ac ,
		TRUE ) ;
	*thePrintRightForm < *theSeparator9 < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintLegendsLabel = new motif_widget(
		"printLegendsLabel",
		xmLabelWidgetClass, thePrintRightForm, 
		args, ac ,
		TRUE );
	*thePrintLegendsLabel = "Legends:" ;

	*thePrintRightForm < *thePrintLegendsLabel < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNnavigationType, XmNONE ); ac++;
	XtSetArg ( args[ac], XmNmarginHeight, 0 ); ac++;
	thePrintLegendsRB = new motif_widget(
		"printLegendsRB",
		xmRadioBoxWidgetClass ,		thePrintRightForm, 
		args,				ac ,
		TRUE ) ;
	*thePrintRightForm < *thePrintLegendsRB < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNtraversalOn, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	thePrintLegAllButton = new motif_widget(
		"All Activity Display Legends",		// "printLegAllButton"
		xmToggleButtonWidgetClass,	thePrintLegendsRB, 
		args,				ac ,
		TRUE ) ;

	ac = 0;
	XtSetArg ( args[ac], XmNtraversalOn, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	thePrintLegSelButton = new motif_widget  (
		"Selected Activity Display Legends",	// "printLegSelButton"
		xmToggleButtonWidgetClass, thePrintLegendsRB, 
		args, ac ,
		TRUE );

	if(Preferences().GetSetPreference("PrintLegendsSource", "All") == "All")
	  *thePrintLegAllButton = "1";
	else
	  *thePrintLegSelButton = "1";

	theSeparator11 = new motif_widget( "separator11",
	            xmSeparatorWidgetClass,	thePrintRightForm , 
	            args, ac ,
		TRUE ) ;
	*thePrintRightForm < *theSeparator11 < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintPPDWLabel = new motif_widget(
		"printPPDWLabel",
		xmLabelWidgetClass,	thePrintRightForm, 
		args,			ac ,
		TRUE );
	*thePrintPPDWLabel = "Pages Per Display Width:" ;
	*thePrintRightForm < *thePrintPPDWLabel < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNmaximum, 20 ); ac++;
	XtSetArg ( args[ac], XmNminimum, 1 ); ac++;
	XtSetArg ( args[ac], XmNheight, 40 ); ac++;
	XtSetArg ( args[ac], XmNorientation, XmHORIZONTAL ); ac++;
	XtSetArg ( args[ac], XmNshowValue, True ); ac++;
	thePrintPPDWScale = new motif_widget(
		"printPPDWScale",
		xmScaleWidgetClass,	thePrintRightForm, 
		args,			ac ,
		TRUE );
	*thePrintRightForm < *thePrintPPDWScale < *thePrintRightForm < endform ;

	int numPages = atoi((Preferences().GetSetPreference("PrintPagesPerDisplayWidth", "1")).c_str());

	XmScaleSetValue(thePrintPPDWScale->widget, numPages);

	ac = 0;
	theSeparator12 = new motif_widget( "separator12",
	            xmSeparatorWidgetClass,	thePrintRightForm , 
	            args, ac ,
		TRUE ) ;
	*thePrintRightForm < *theSeparator12 < *thePrintRightForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintPageSizeLabel = new motif_widget(
		"printPageSizeLabel",
		xmLabelWidgetClass,	thePrintRightForm, 
		args,			ac ,
		TRUE );
	*thePrintPageSizeLabel = "Page Size:" ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintPageSizeDisplay = new motif_widget(
		"printPageSizeDisplay",
		xmLabelWidgetClass,	thePrintRightForm, 
		args,			ac ,
		TRUE );
	
	*thePrintRightForm < *thePrintPageSizeLabel < *thePrintPageSizeDisplay < *thePrintRightForm < endform;

	//	*thePrintRightForm < *thePrintPageSizeLabel < *thePrintRightForm < endform ;

	thePrintPageSizePM = new motif_widget("pageSizePM", xmPulldownMenuWidgetClass, thePrintRightForm, NULL, 0, TRUE);

	//Letter
	PageSizes[0].WidthInInches = 8.5;
	PageSizes[0].HeightInInches = 11.0;
	PageSizes[0].LeftMargin = 0.5;
	PageSizes[0].TopMargin = 0.5;
	PageSizes[0].ButtonWidget = new motif_widget("plotSizeLetter", xmPushButtonWidgetClass, thePrintPageSizePM, NULL, 0, TRUE);
	*(PageSizes[0].ButtonWidget) = "Letter";
	PageSizes[0].ButtonWidget->add_callback(UI_printdialog::paperSizeSelectActivateCallback, XmNactivateCallback, (void*) 0);

	//Legal
	PageSizes[1].WidthInInches = 8.5;
	PageSizes[1].HeightInInches = 14.0;
	PageSizes[1].LeftMargin = 0.5;
	PageSizes[1].TopMargin = 0.5;
	PageSizes[1].ButtonWidget = new motif_widget("plotSizeLegal", xmPushButtonWidgetClass, thePrintPageSizePM, NULL, 0, TRUE);
	*(PageSizes[1].ButtonWidget) = "Legal";
	PageSizes[1].ButtonWidget->add_callback(UI_printdialog::paperSizeSelectActivateCallback, XmNactivateCallback, (void*) 1);

	//A4
	PageSizes[2].WidthInInches = 8.27;
	PageSizes[2].HeightInInches = 11.69;
	PageSizes[2].LeftMargin = 0.5;
	PageSizes[2].TopMargin = 0.5;
	PageSizes[2].ButtonWidget = new motif_widget("plotSizeA4", xmPushButtonWidgetClass, thePrintPageSizePM, NULL, 0, TRUE);
	*(PageSizes[2].ButtonWidget) = "A4";
	PageSizes[2].ButtonWidget->add_callback(UI_printdialog::paperSizeSelectActivateCallback, XmNactivateCallback, (void*) 2);

	//Other
	PageSizes[3].WidthInInches = 0.0;
	PageSizes[3].HeightInInches = 0.0;
	PageSizes[3].LeftMargin = 0.5;
	PageSizes[3].TopMargin = 0.5;
	PageSizes[3].ButtonWidget = new motif_widget("plotSizeOther", xmPushButtonWidgetClass, thePrintPageSizePM, NULL, 0, TRUE);
	*(PageSizes[3].ButtonWidget) = "Other";
	PageSizes[3].ButtonWidget->add_callback(UI_printdialog::paperSizeSelectActivateCallback, XmNactivateCallback, (void*) 3);

	ac = 0;
	XtSetArg(args[ac], XmNalignment, XmALIGNMENT_BEGINNING); ac++;

	thePrintPageSizeOM = new motif_widget("plotSizeOM", xmOptionMenuWidgetClass, thePrintRightForm, args, ac, TRUE);
	XtVaSetValues(thePrintPageSizeOM->widget, 
		      XmNnavigationType, XmTAB_GROUP,
		      XmNsubMenuId, thePrintPageSizePM->widget,
		      XmNorientation, XmHORIZONTAL,
		      NULL);

	IncludeSummaryPage = new motif_widget( "Include Summary Page" , xmToggleButtonWidgetClass , thePrintRightForm) ;

	if(Preferences().GetSetPreference("PrintIncludeSummaryPage", "TRUE") == "TRUE")
	  *IncludeSummaryPage = "1";

	*thePrintRightForm < *thePrintPageSizeOM < *IncludeSummaryPage < *thePrintRightForm < endform ;

	string currentPageSize = Preferences().GetSetPreference("PrintPageSize", "Letter");

	if(currentPageSize == "Letter")
	  PageSizeIndex = 0;
	else if(currentPageSize == "Legal")
	  PageSizeIndex = 1;
	else if(currentPageSize == "A4")
	  PageSizeIndex = 2;
	else
	  PageSizeIndex = 3;

	XtVaSetValues(thePrintPageSizeOM->widget , XmNmenuHistory , PageSizes[PageSizeIndex].ButtonWidget->widget , NULL ) ;

	string numString = Preferences().GetSetPreference("PrintPageSizeOtherWidth", "8.5");
	sscanf(numString.c_str(), "%lf" , &(PageSizes[3].WidthInInches));

	numString = Preferences().GetSetPreference("PrintPageSizeOtherHeight", "11.0");
	sscanf(numString.c_str(), "%lf", &(PageSizes[3].HeightInInches));

	numString = Preferences().GetSetPreference("PrintPageSizeOtherLeftMargin", "0.5");
	sscanf(numString.c_str(), "%lf", &(PageSizes[3].LeftMargin));

	numString = Preferences().GetSetPreference("PrintPageSizeOtherTopMargin", "0.5");
	sscanf(numString.c_str(), "%lf", &(PageSizes[3].TopMargin));

	UpdatePageSizeDisplay();

	ac = 0;
	XtSetArg ( args[ac], XmNnavigationType, XmNONE ); ac++;
	XtSetArg ( args[ac], XmNmarginHeight, 0 ); ac++;
	thePrintPageFormatRB = new motif_widget(
						"printPageFormatRB",
						xmRadioBoxWidgetClass , thePrintRightForm, 
						args, ac ,
						TRUE );

	*thePrintRightForm < *thePrintPageFormatRB < *thePrintRightForm < endform ;

	*thePrintRightForm ^ *thePrintPageFormatLabel ^ *thePrintPageFormatRB ^
	*theSeparator9 ^ *thePrintLegendsLabel ^ *thePrintLegendsRB ^
	*theSeparator11 ^ *thePrintPPDWLabel ^ *thePrintPPDWScale ^ *theSeparator12 ^ 
	*thePrintPageSizeLabel ^ *thePrintPageSizeOM ^ *thePrintRightForm ^ endform ;

	*thePrintRightForm ^ *thePrintPageFormatLabel ^ *thePrintPageFormatRB ^
	*theSeparator9 ^ *thePrintLegendsLabel ^ *thePrintLegendsRB ^
	*theSeparator11 ^ *thePrintPPDWLabel ^ *thePrintPPDWScale ^ *theSeparator12 ^ 
	*thePrintPageSizeDisplay ^ *IncludeSummaryPage ^ *thePrintRightForm ^ endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNtraversalOn, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	XtSetArg ( args[ac], XmNset, True ); ac++;
	thePrintLegEachButton = new motif_widget(
	"Legends Each Page",		// "printLegEachButton"
	xmToggleButtonWidgetClass, thePrintPageFormatRB, 
	args, ac ,
	TRUE );

	ac = 0;
	XtSetArg ( args[ac], XmNsensitive, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	thePrintLegFirstLastButton = new motif_widget(
	"Legends First and Last Pages",	// "printLegFirstLastButton"
	xmToggleButtonWidgetClass, thePrintPageFormatRB, 
	args, ac ,
	TRUE );

	ac = 0;
	XtSetArg ( args[ac], XmNsensitive, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	thePrintLegFirstButton = new motif_widget(
	"Legends First Page Only",	// "printLegFirstButton"
	xmToggleButtonWidgetClass, thePrintPageFormatRB, 
	args, ac ,
	TRUE );

	ac = 0;
	XtSetArg ( args[ac], XmNfractionBase, 39 ); ac++;
	XtSetArg(args[ac] , XmNautoUnmanage ,   False ) ; ac++ ;
	thePrintButtonForm = new motif_widget  (
		"printButtonForm",
		xmFormWidgetClass,	thePrintDialog , 
		args,			ac ,
		TRUE );

	ac = 0;
	thePrintPrintButton = new motif_widget(
		"printPrintButton",
		xmPushButtonWidgetClass,	thePrintButtonForm, 
		args,				ac ,
		TRUE ) ;
	*thePrintPrintButton = "Print" ;

	thePrintPrintButton->add_callback(
	                printPrintButtonActivateCallback ,
	                XmNactivateCallback ,
	                NULL ) ;
	ac = 0;
	XtSetArg ( args[ac], XmNborderWidth, 0 ); ac++;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	thePrintCancelButton = new motif_widget( "printCancelButton",
		xmPushButtonWidgetClass,	thePrintButtonForm, 
		args,				ac ,
		TRUE ) ;
	*thePrintCancelButton = "Cancel" ;

	thePrintCancelButton->add_callback(
	                printCancelButtonActivateCallback,
	                XmNactivateCallback,
	                NULL ) ;

	form_position(  5 ) < *thePrintPrintButton  < form_position( 14 ) < endform ;
	form_position( 25 ) < *thePrintCancelButton < form_position( 34 ) < endform ;

	// *thePrintDialog ^ 10 ^ *thePrintRightForm ^ * thePrintSeparator ^ endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintTitleLabel = new motif_widget( "printTitleLabel" ,
		xmLabelWidgetClass ,	theLeftForm ,
		args ,			ac ,
		TRUE ) ;
	*thePrintTitleLabel = "Title:" ;

	thePrintTitleTF = new single_line_text_widget( "printTitleTF",
	theLeftForm, EDITABLE , TRUE ) ;
	*theLeftForm < *thePrintTitleTF < *theLeftForm < endform ;
	*thePrintTitleTF = Preferences().GetSetPreference("PrintTitle", "");


	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintSubtitleLabel = new motif_widget( "printSubtitleLabel",
		xmLabelWidgetClass , theLeftForm,
		args, ac , TRUE ) ;
	*thePrintSubtitleLabel = "Subtitle:" ;

	ac = 0;
	SET_BACKGROUND_TO_GREY ;
	thePrintSubtitleTF = new single_line_text_widget( "printSubtitleTF",
		theLeftForm, EDITABLE , TRUE ) ;
	*theLeftForm < *thePrintSubtitleTF < *theLeftForm < endform ;

	*thePrintSubtitleTF = Preferences().GetSetPreference("PrintSubTitle", "");

	ac = 0;
	theSeparator10 = new motif_widget( "separator10",
		xmSeparatorWidgetClass, theLeftForm , 
		args, ac ,
		TRUE) ;
	*theLeftForm < *theSeparator10 < *theLeftForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintSourceLabel = new motif_widget( "printSourceLabel",
		xmLabelWidgetClass, theLeftForm, 
		args, ac , TRUE ) ;
	*thePrintSourceLabel = "Source:" ;

	thePrintSourceRB = new motif_widget(
		"printSourceRB",
		xmRadioBoxWidgetClass ,	theLeftForm, 
		NULL, 0 , TRUE ) ;

	ac = 0;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	thePrintSelADsTB = new motif_widget(
		//"Selected Activity Display(s)",
		"Top Selected Activity Display",		// "printSelADsTB"
		xmToggleButtonWidgetClass, thePrintSourceRB, 
		args, ac ,
		TRUE ) ;


	ac = 0;

	thePrintSelADsAndRDsTB = new motif_widget(
		//"Selected Activity and Resource Displays",
		"... plus its Selected Resource Displays",	// "printSelADsAndRDsTB"
		xmToggleButtonWidgetClass, thePrintSourceRB, 
		args, 
		ac , TRUE ) ;

	if(Preferences().GetSetPreference("PrintSource", "Plus") == "Top")
	  *thePrintSelADsTB = "1";
	else
	  *thePrintSelADsAndRDsTB = "1";

	ac = 0;
	theSeparator5 = new motif_widget( "separator5",
		xmSeparatorWidgetClass, theLeftForm ,
		args, ac ,
		TRUE ) ;

	*theLeftForm < *theSeparator5  < *theLeftForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNalignment, XmALIGNMENT_BEGINNING ); ac++;
	thePrintDestinationLabel = new motif_widget  ( "printDestinationLabel",
		xmLabelWidgetClass, theLeftForm, 
		args, ac , TRUE ) ;
	*thePrintDestinationLabel = "Destination:" ;
	*theLeftForm < *thePrintDestinationLabel < *theLeftForm < endform ;

	ac = 0;
	XtSetArg ( args[ac], XmNnavigationType, XmNONE ); ac++;
	XtSetArg ( args[ac], XmNmarginHeight, 0 ); ac++;
	thePrintDestinationRB = new motif_widget(
		"printDestinationRB",
		xmRadioBoxWidgetClass , theLeftForm, 
		args, ac , TRUE  );

	ac = 0;
	XtSetArg ( args[ac], XmNtraversalOn, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	XtSetArg ( args[ac], XmNset, True ); ac++;
	theToFileTB = new motif_widget(
		"To File",	// "toFileTB"
		xmToggleButtonWidgetClass, thePrintDestinationRB, 
		args, ac , TRUE );


	ac = 0;
	XtSetArg ( args[ac], XmNsensitive, False ); ac++;
	XtSetArg ( args[ac], XmNhighlightThickness, 0 ); ac++;
	// XtSetArg ( args[ac], XmNrecomputeSize, False ); ac++;
	theToPrinterTB = new motif_widget(
		"To Printer",			// "toPrinterTB"
		xmToggleButtonWidgetClass, thePrintDestinationRB, 
		args , ac , TRUE  );

	fileNameLabel = new motif_widget( "File Name: " ,
		xmLabelWidgetClass ,		theLeftForm ,
		NULL ,				0 ,
		TRUE ) ;
	fileNameWidget = new single_line_text_widget( "thefilename" ,
		theLeftForm , EDITABLE , TRUE ) ;
	*fileNameWidget = Preferences().GetSetPreference("PrintFileName", "apgen.ps") ;

	*theLeftForm < *thePrintDestinationRB < *theLeftForm < endform ;
	*theLeftForm < *fileNameLabel < *fileNameWidget < *theLeftForm < endform ;
	*thePrintDestinationRB ^ 4 ^ *fileNameLabel ^ endform ;
	*theLeftForm  ^ *thePrintTitleLabel ^ *thePrintTitleTF ^ *thePrintSubtitleLabel ^ *thePrintSubtitleTF
		^ *theSeparator10 ^ *thePrintSourceLabel ^ *thePrintSourceRB
		^ *theSeparator5 ^ *thePrintDestinationLabel ^ *thePrintDestinationRB
		^ *fileNameWidget ^ *theLeftForm ^ endform ;

	thePrintButtonForm->add_property( CHILD_OF_A_FORM ) ;
	*thePrintSeparator ^ 1 ^ *thePrintButtonForm ^ *thePrintDialog ^ endform ;
	*thePrintDialog < 10 < *thePrintSeparator < 10 < *thePrintDialog < endform ;
	*thePrintDialog < *thePrintButtonForm < *thePrintDialog < endform ;
	thePrintButtonForm->add_property( FORM_PARENT ) ;

	thePrintDialog->manage() ;
	}
#endif
}

void UI_printdialog::printCancelButtonActivateCallback (Widget, callback_stuff *, void *) { 
	printDialog->printCancelButtonActivate(NULL, NULL, NULL) ; }

void UI_printdialog::printPrintButtonActivateCallback (Widget, callback_stuff *, void *) {
	printDialog->printPrintButtonActivate(NULL, NULL, NULL) ; }

void UI_printdialog::paperSizeSelectActivateCallback(Widget w, callback_stuff *stuff, void* otherStuff)
{
  printDialog->paperSizeSelectActivate(w, stuff, otherStuff);
}


UI_print_size_other_popup::UI_print_size_other_popup(const Cstring& name, motif_widget	*parent_widget ,callback_proc proc_to_call, 
						     double width, double height, double leftMargin, double topMargin )
	: motif_widget(name, xmDialogShellWidgetClass, parent_widget,
			NULL, 0, FALSE) ,
	  data_for_window_closing(NULL, nlCancelButtonActivate, NULL, get_this_widget() ) ,
	  ok_proc( proc_to_call ) {
#ifdef GUI
	if( GUI_Flag ) {
	AllowWindowClosingFromMotifMenu(widget, nlCancelButtonActivate, &data_for_window_closing) ;

	int n = 0;
	Arg args[256];
	XtSetArg(args[n], XmNwidth, 300); n++;
	_UI_print_size_other_popup = new motif_widget(
		"Other Paper Size" ,
		xmFormWidgetClass,	this,
		args,			n ,
		FALSE ) ;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	PaperWidthLabel = new motif_widget(
		"Plot Width: (inches)",
		xmLabelWidgetClass, _UI_print_size_other_popup , 
		args, n ,
		TRUE );

	PaperWidthTextField = new single_line_text_widget( "nlPaperTF", _UI_print_size_other_popup, EDITABLE , TRUE ) ;


	char numBuf[64];
	sprintf(numBuf, "%'.2f", width);
	*PaperWidthTextField = numBuf;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	PaperHeightLabel = new motif_widget(
		"Plot Height (inches):" ,
		xmLabelWidgetClass,	_UI_print_size_other_popup , 
		args,			n ,
		TRUE ) ;

	PaperHeightTextField = new single_line_text_widget( "PaperHeightTextField", _UI_print_size_other_popup, EDITABLE , TRUE ) ;

	sprintf(numBuf, "%'.2f", height);
	*PaperHeightTextField = numBuf;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	LeftMarginLabel = new motif_widget(
		"Left Margin: (inches)",
		xmLabelWidgetClass, _UI_print_size_other_popup , 
		args, n ,
		TRUE );

	LeftMarginTextField = new single_line_text_widget( "PaperLeftMargin", _UI_print_size_other_popup, EDITABLE , TRUE ) ;

	sprintf(numBuf, "%'.2f", leftMargin);
	*LeftMarginTextField = numBuf;

	n = 0;
	XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
	TopMarginLabel = new motif_widget(
		"Top Margin: (inches)",
		xmLabelWidgetClass, _UI_print_size_other_popup , 
		args, n ,
		TRUE );

	TopMarginTextField = new single_line_text_widget( "PaperTopMargin", _UI_print_size_other_popup, EDITABLE , TRUE ) ;

	sprintf(numBuf, "%'.2f", topMargin);
	*TopMarginTextField = numBuf;

	n = 0;
	_nlSeparator = new motif_widget(
		"nlSeparator",
		xmSeparatorWidgetClass , _UI_print_size_other_popup ,
		args, n ,
		TRUE );

	*_UI_print_size_other_popup < 10 < *PaperWidthLabel			< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *PaperWidthTextField		< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *PaperHeightLabel	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *PaperHeightTextField	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *LeftMarginLabel	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *LeftMarginTextField	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *TopMarginLabel	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *TopMarginTextField	< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup < 10 < *_nlSeparator		< 10 < *_UI_print_size_other_popup < endform ;
	*_UI_print_size_other_popup ^ 10 ^ *PaperWidthLabel ^ *PaperWidthTextField ^ 10 ^ 
	  *PaperHeightLabel ^ *PaperHeightTextField ^ 10 ^ 
	  *LeftMarginLabel ^ *LeftMarginTextField ^ 10 ^ 
	  *TopMarginLabel ^ *TopMarginTextField ^ 5 ^ 
	  *_nlSeparator ^ endform ;

	n = 0;
	XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
	XtSetArg(args[n], XmNfractionBase, 59); n++;
	_nlButtonForm = new motif_widget(
		"nlButtonForm",
		xmFormWidgetClass , _UI_print_size_other_popup ,
		args, n ,
		TRUE );

	_nlButtonForm->add_property( CHILD_OF_A_FORM ) ;
	*_UI_print_size_other_popup < 10 < *_nlButtonForm < 10 < *_UI_print_size_other_popup < endform ;
	*_nlSeparator ^ 5 ^ *_nlButtonForm ^ 10 ^ *_UI_print_size_other_popup ^ endform ;
	_nlButtonForm->add_property( FORM_PARENT ) ;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_nlOKButton = new motif_widget(
		"nlOKButton",
		xmPushButtonWidgetClass , _nlButtonForm,
		args, n ,
		TRUE );
	*_nlOKButton = "OK" ;
	_nlOKButton->add_callback( UI_print_size_other_popup::nlOKButtonActivate , XmNactivateCallback , this ) ;

	form_position( 0 ) < *_nlOKButton < form_position( 19 ) < endform ;
	*_nlButtonForm ^ *_nlOKButton ^ *_nlButtonForm ^ endform ;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;

	n = 0;
	XtSetArg(args[n], XmNnavigationType, XmTAB_GROUP); n++;
	_nlCancelButton = new motif_widget(
		"nlCancelButton",
		xmPushButtonWidgetClass , _nlButtonForm,
		args, n ,
		TRUE );
	*_nlCancelButton = "Cancel" ;
	_nlCancelButton->add_callback( UI_print_size_other_popup::nlCancelButtonActivate, XmNactivateCallback, this ) ;

	*_nlButtonForm ^ *_nlCancelButton ^ *_nlButtonForm ^ endform ;
	form_position( 40 ) < *_nlCancelButton < form_position( 59 ) < endform ;
	}
#endif
	}

void UI_print_size_other_popup::initialize () {
	; }	//nothing to do

void UI_print_size_other_popup::nlApplyButtonActivate (Widget, callback_stuff * clientData , void * ) {
	UI_print_size_other_popup* eobject = (UI_print_size_other_popup*) clientData->data ;

	eobject->applyChanges() ; }

void UI_print_size_other_popup::nlCancelButtonActivate (Widget, callback_stuff * clientData , void * ) {
	UI_print_size_other_popup* eobject = (UI_print_size_other_popup*) clientData->data ;

	printDialog->CancelOtherPrintPageSize();
	eobject->_UI_print_size_other_popup->unmanage() ; }

void UI_print_size_other_popup::nlOKButtonActivate (Widget, callback_stuff * clientData , void * ) {
	UI_print_size_other_popup		*eobject = (UI_print_size_other_popup*) clientData->data ;

	if ( eobject->applyChanges() == apgen::RETURN_STATUS::SUCCESS ) {
		eobject->_UI_print_size_other_popup->unmanage() ;
		eobject->ok_proc( NULL , clientData, NULL ) ; } }

void UI_print_size_other_popup::nlOKCallback (Widget, callback_stuff * , void * ) {
	; }	//nothing to do

apgen::RETURN_STATUS UI_print_size_other_popup::applyChanges () 
{
  Cstring paperWidth(PaperWidthTextField->get_text()) ;
  Cstring paperHeight( PaperHeightTextField->get_text() ) ;
  Cstring paperLeftMargin(LeftMarginTextField->get_text());
  Cstring paperTopMargin(TopMarginTextField->get_text());

  double height;
  double width;
  double leftMargin;
  double topMargin;

  int success = sscanf(*paperWidth , "%lf" , &width) ;
  success = (success && sscanf(*paperHeight, "%lf", &height));
  success = (success && sscanf(*paperLeftMargin, "%lf", &leftMargin));
  success = (success && sscanf(*paperTopMargin, "%lf", &topMargin));
  

  if(success)
  {
    printDialog->SetOtherPrintPageSize(width, height, leftMargin, topMargin);
    return apgen::RETURN_STATUS::SUCCESS;
  }
  else
  {
    printDialog->CancelOtherPrintPageSize();
    return apgen::RETURN_STATUS::FAIL;
  }
}
