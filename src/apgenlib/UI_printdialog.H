#ifndef UI_PRINTDIALOG_H
#define UI_PRINTDIALOG_H

#include "UI_motif_widget.H"
#include "UTL_time.H"
#include "UI_GeneralMotifInclude.H"
///#include "UI_defs.H"  // constant and pixmap descriptions

class ACT_sys;
class RD_sys ;

#define NUM_OF_PLOTSIZES 4

struct PageSize
{
  double WidthInInches;
  double HeightInInches;
  double LeftMargin;
  double TopMargin;
  motif_widget* ButtonWidget;
};

///
class UI_print_size_other_popup : public motif_widget {
protected:
  static void	nlApplyButtonActivate( Widget , callback_stuff * , void * );
  static void	nlCancelButtonActivate( Widget , callback_stuff * , void * );
  static void	nlOKButtonActivate( Widget , callback_stuff * , void * );

  apgen::RETURN_STATUS	applyChanges ();

  callback_stuff	data_for_window_closing ;
  callback_proc	ok_proc ;

public:
  UI_print_size_other_popup(
	       const Cstring	&name ,			//will be name of top widget
	       motif_widget	*parent_widget ,	//parent of this widget
	       callback_proc	proc_to_call,		//call when OK pushed
	       double width,
	       double height,
	       double leftMargin,
	       double rightMargin
	       ) ;
  ~UI_print_size_other_popup()	{}

  static void initialize () ;

  // widget hierarchy (indent 1 space per level of parentage,as in an outline)
  motif_widget		*_UI_print_size_other_popup ; ///DialogShell child;un/manage it
  motif_widget		*PaperWidthLabel ;
  motif_widget		*PaperWidthTextField ;
  motif_widget		*_nlSeparator ;
  motif_widget		*_nlButtonForm;
  motif_widget		*_nlCancelButton ;
  motif_widget		*_nlApplyButton ;
  motif_widget		*_nlOKButton ;
  motif_widget		*PaperHeightLabel;
  motif_widget          *PaperHeightTextField ;
  motif_widget		*LeftMarginLabel;
  motif_widget          *LeftMarginTextField ;
  motif_widget		*TopMarginLabel;
  motif_widget          *TopMarginTextField ;
  static void	nlOKCallback( Widget , callback_stuff * , void * ) ; 
} ;


class UI_printdialog : public motif_widget
{
protected:
    ///
    ACT_sys		* activitySystem ;
    ///
    RD_sys		* resourceSystem ;
    ///
    List		managedSelectedResourceSystemList ;
    ///
    TIMEZONE		timezone ;

///
	motif_widget	*  thePrintDialog ;
///
	motif_widget	* fileNameWidget ;
///
	motif_widget	*  thePrintButtonForm ;
///
	motif_widget	*  thePrintCancelButton ;
///
	motif_widget	*  thePrintDestinationLabel ;
///
	motif_widget	*  thePrintDestinationRB ;
///
	motif_widget	*  theLeftForm ;
///
	motif_widget	*  thePrintLegAllButton ;
///
	motif_widget	*  thePrintLegEachButton ;
///
	motif_widget	*  thePrintLegFirstButton ;
///
	motif_widget	*  thePrintLegFirstLastButton ;
///
	motif_widget	*  thePrintLegSelButton ;
///
	motif_widget	*  thePrintLegendsLabel ;
///
	motif_widget	*  thePrintLegendsRB ;
///
	motif_widget	*  thePrintPPDWLabel ;
///
	motif_widget	*  thePrintPageSizeLabel ;
///
	motif_widget	*  thePrintPageSizeDisplay ;
///
	motif_widget	*  thePrintPageSizePM ;
///
	motif_widget	*  thePrintPageSizeOM ;
///
	motif_widget	*  thePrintPPDWScale ;
///
	motif_widget	*  thePrintPageFormatLabel ;
///
	motif_widget	*  thePrintPageFormatRB ;
///
	motif_widget	*  thePrintPrintButton ;
///
	motif_widget	*  thePrintRightForm ;
///
	motif_widget	*  thePrintSelADsAndRDsTB ;
///
	motif_widget	*  thePrintSelADsTB ;
///
	motif_widget	*  thePrintSourceLabel ;
///
	motif_widget	*  thePrintSourceRB ;
///
	motif_widget	*  thePrintSubtitleLabel ;
///
	motif_widget	*  thePrintSubtitleTF ;
///
	motif_widget	*  thePrintTitleLabel ;
///
	motif_widget	*  thePrintTitleTF ;
///
	motif_widget	*  theToFileTB ;
///
	motif_widget	*  theToPrinterTB ;

///
	motif_widget	* thePrintSeparator ;
///
	motif_widget	* theSeparator10 ;
///
	motif_widget	* theSeparator11 ;
///
	motif_widget	* theSeparator5 ;
///
	motif_widget	* theSeparator9 ;
///
	motif_widget	* theSeparator12 ;
  ///
	motif_widget *IncludeSummaryPage;

  ///
	int PageSizeIndex;

  ///
	PageSize PageSizes[NUM_OF_PLOTSIZES];
  ///
	static void paperSizeSelectActivateCallback(Widget widge, callback_stuff* clientData, void *callData);
  ///
	void paperSizeSelectActivate(Widget w, callback_stuff* client_data, void* stuff);
///
	void			printCancelButtonActivate(Widget, void *, void *) ;
///
	void			printPrintButtonActivate(Widget, void *, void *) ;
///
	static void		printCancelButtonActivateCallback (Widget, callback_stuff *,  void * ) ;
///
	static void		printPrintButtonActivateCallback (Widget, callback_stuff *,  void * ) ;

  ///
	void UpdatePageSizeDisplay();
///
void		create(const char *) ;

public:

    ///
    UI_printdialog(const char *) ;
    ///
    ~UI_printdialog() ;

  ///
	void SetOtherPrintPageSize(double width, double height, double leftMargin, double rightMargin);
  ///
	void CancelOtherPrintPageSize();
  ///
    static void	initialize ( ACT_sys * as , RD_sys * rs , List & rl , TIMEZONE tz ) ;
} ;

#endif
