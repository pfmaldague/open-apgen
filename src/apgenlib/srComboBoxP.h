#ifndef __srComboBoxWidgetP_h
#define __srComboBoxWidgetP_h

#include "srComboBox.h"
#include <Xm/XmP.h>
#include <Xm/ManagerP.h>

///
typedef struct _XmsrComboBoxClassPart {
    int Just_to_keep_the_compiler_happy; 
} ///
 XmsrComboBoxClassPart;
///
typedef struct _XmsrComboBoxClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
    XmsrComboBoxClassPart combobox_class;
} ///
 XmsrComboBoxClassRec;
///
extern XmsrComboBoxClassRec xmsrComboBoxClassRec;

///
typedef struct _XmsrComboBoxPart {
    Boolean	    Editable;		
    Boolean	    Sorted;	
    int		    VisibleItemCount;	
    XmFontList	    Font;	
    XtCallbackList  SelectionCBL;       
    XtCallbackList  DropDownCBL;       
    Boolean	    Persistent;	
    Boolean	    TwmHandlingOn;	
    Boolean	    ShowLabel;	
    Position	    DropDownOffset;	
    
    Widget	    EditCtrl;
    Widget	    ArrowCtrl;
    Widget	    LabelCtrl;  
    
    Widget	    PopupShell; 
    Widget	    ListCtrl;   

    Widget	    MyNextShell;
    
    Cursor          ArrowCursor;
    
    Boolean         ListVisible;
    Boolean	    IgnoreFocusOut;
    Boolean         PendingFocusOut;
    Boolean	    PendingOverrideInOut;
    XtWorkProcId    WorkProcID;
    XEvent          xevent;
    
    Boolean	    PassVerification;
    Boolean	    ConvertBitmapToPixmap, 
                    ConvertBitmapToPixmapInsensitive;
    Pixmap	    LabelPixmap, LabelInsensitivePixmap;
} ///
 XmsrComboBoxPart;

///
typedef struct _XmsrComboBoxRec {
    CorePart       core;
    CompositePart  composite;
    ConstraintPart constraint;
    XmManagerPart  manager;
    XmsrComboBoxPart combobox;
} ///
 XmsrComboBoxRec;

#endif
