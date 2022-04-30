#ifndef UI_resabsscrollzoombx_H
#define UI_resabsscrollzoombx_H
#include "UIComponent.H"

///
class UI_resabsscrollzoombx : public UIComponent {

  public:

    ///
    UI_resabsscrollzoombx(const char *, Widget);
    ///
    UI_resabsscrollzoombx(const char *);
    ///
    virtual ~UI_resabsscrollzoombx();
    ///
    virtual void create(Widget);
    ///
    const char *const  className();
    
    // Begin user code block <public> 
    // End user code block <public> 
  protected:
    // Classes created by this class
    
    /// Widgets created by this class
    Widget _UI_resabsscrollzoombx;
    ///
    Widget _raszButtonForm;
    ///
    Widget _raszOKButton;
    ///
    Widget _raszApplyButton;
    ///
    Widget _raszCancelButton;
    ///
    Widget _raszSeparator;
    ///
    Widget _raszTimeSpanTF;
    ///
    Widget _raszTimeSpanLabel;
    ///
    Widget _raszStartTimeTF;
    ///
    Widget _raszStartTimeLabel;
    
    
    ///
    virtual void raszOKButtonActivate(Widget, XtPointer, XtPointer);
    ///
    virtual void raszApplyButtonActivate(Widget, XtPointer, XtPointer);
    ///
    virtual void raszCancelButtonActivate(Widget, XtPointer, XtPointer);
    
  private: 
    
    ///
    UICallbackStruct  *_clientDataStructs;
    
    ///
    static void raszOKButtonActivateCallback(Widget, XtPointer, XtPointer);
    ///
    static void raszApplyButtonActivateCallback(Widget, XtPointer, XtPointer);
    ///
    static void raszCancelButtonActivateCallback(Widget, XtPointer, XtPointer);
    
};

// Begin user code block <tail> 
// End user code block <tail> 
#endif
