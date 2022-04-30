#ifndef __srComboBoxWidget_h
#define __srComboBoxWidget_h

#include <Xm/Xm.h>

#if defined(__cplusplus) || defined(c_plusplus)
///
extern "C" {
#endif

#ifndef XmNselectionCallback
#define XmNselectionCallback		"selectionCallback"
#endif
#define XmCSelectionCallback		"SelectionCallback"
#define XmNdropDownCallback		"dropDownCallback"
#define XmCDropDownCallback		"DropDownCallback"

#define XmNdropDownCursor		"dropDownCursor"
#define XmCDropDownCursor		"DropDownCursor"

#define XmNsorted			"sorted"
#define XmCSorted			"Sorted"


#undef XmNpersistentDropDown	
#undef XmCPersistentDropDown
#undef XmNtwmHandlingOn	
#undef XmCTwmHandlingOn
#undef XmNshowLabel
#undef XmCShowLabel
#undef XmNdropDownOffset
#undef XmCDropDownOffset

#define XmNpersistentDropDown		"persistentDropDown"
#define XmCPersistentDropDown		"PersistentDropDown"
#define XmNtwmHandlingOn		"twmHandlingOn"
#define XmCTwmHandlingOn		"TwmHandlingOn"
#define XmNshowLabel			"showLabel"
#define XmCShowLabel			"ShowLabel"
#define XmNdropDownOffset		"dropDownOffset"
#define XmCDropDownOffset		"DropDownOffset"

#define XmNlabelMarginBottom		"labelMarginBottom"
#define XmCLabelMarginBottom		"LabelMarginBottom"
#define XmNlabelMarginHeight		"labelMarginHeight"
#define XmCLabelMarginHeight		"LabelMarginHeight"
#define XmNlabelMarginLeft		"labelMarginLeft"
#define XmCLabelMarginLeft		"LabelMarginLeft"
#define XmNlabelMarginRight		"labelMarginRight"
#define XmCLabelMarginRight		"LabelMarginRight"
#define XmNlabelMarginTop		"labelMarginTop"
#define XmCLabelMarginTop		"LabelMarginTop"
#define XmNlabelMarginWidth		"labelMarginWidth"
#define XmCLabelMarginWidth		"LabelMarginWidth"

/* Callback reasons: (part. predefined)
 *
 * XmCR_SINGLE_SELECT	    user selected item in the list
 */

typedef struct {
    int      reason;	
    XEvent   *event;
    XmString value;
    int      index;
} XmsrComboBoxSelectionCallbackStruct;
/* Callback reasons: new
 * 
 * XmCR_SHOW_LIST	    list is dropping down
 * XmCR_HIDE_LIST	    list is getting hidden
 */
#define XmCR_SHOW_LIST	4200	/* ten times "42", that should   */
#define XmCR_HIDE_LIST	4201	/* explain everything of live... */
typedef struct {
    int	     reason;
    XEvent   *event;
} XmsrComboBoxDropDownCallbackStruct;

extern void    XmsrComboBoxAddItem(Widget w, XmString item, int pos);
extern void    XmsrComboBoxAddItems(Widget w, XmString *items, int item_count, 
		int pos);
extern void    XmsrComboBoxAddItemUnselected(Widget w, XmString item, int pos);
extern void    XmsrComboBoxDeleteItem(Widget w, XmString item);
extern void    XmsrComboBoxDeleteItems(Widget w, XmString *items, int item_count);
extern void    XmsrComboBoxDeletePos(Widget w, int pos);
extern void    XmsrComboBoxDeleteItemsPos(Widget w, int item_count, int pos);
extern void    XmsrComboBoxDeleteAllItems(Widget w);
extern void    XmsrComboBoxReplaceItems(Widget w, XmString *old_items, 
		int item_count, XmString *new_items);
extern void    XmsrComboBoxReplaceItemsPos(Widget w, XmString *new_items, 
		int item_count, int position);
extern Boolean XmsrComboBoxItemExists(Widget w, XmString item);
extern int     XmsrComboBoxItemPos(Widget w, XmString item);
extern Boolean XmsrComboBoxGetMatchPos(Widget w, XmString item, int **pos_list, 
		int *pos_count);
extern void    XmsrComboBoxSelectPos(Widget w, int pos, Boolean notify);
extern void    XmsrComboBoxSelectItem(Widget w, XmString item, Boolean notify);
extern int     XmsrComboBoxGetSelectedPos(Widget w);
extern void    XmsrComboBoxClearSelection(Widget w, Time time);
extern Boolean XmsrComboBoxCopy(Widget w, Time time);
extern Boolean XmsrComboBoxCut(Widget w, Time time);
extern XmTextPosition XmsrComboBoxGetInsertionPosition(Widget w);
extern XmTextPosition XmsrComboBoxGetLastPosition(Widget w);
extern int     XmsrComboBoxGetMaxLength(Widget w);
extern char *  XmsrComboBoxGetSelection(Widget w);
extern Boolean XmsrComboBoxGetSelectionPosition(Widget w, XmTextPosition *left, 
                                               XmTextPosition *right);
extern char *  XmsrComboBoxGetString(Widget w);
extern void    XmsrComboBoxInsert(Widget w, XmTextPosition position, char *value);
extern Boolean XmsrComboBoxPaste(Widget w);
extern Boolean XmsrComboBoxRemove(Widget w);
extern void    XmsrComboBoxReplace(Widget w, XmTextPosition from_pos, 
                                 XmTextPosition to_pos, char *value);
extern void    XmsrComboBoxSetAddMode(Widget w, Boolean state);
extern void    XmsrComboBoxSetHighlight(Widget w, XmTextPosition left, 
                                      XmTextPosition right, XmHighlightMode mode);
extern void    XmsrComboBoxSetInsertionPosition(Widget w, XmTextPosition position);
extern void    XmsrComboBoxSetMaxLength(Widget w, int max_length);
extern void    XmsrComboBoxSetSelection(Widget w, XmTextPosition first, 
                                      XmTextPosition last, Time time);
extern void    XmsrComboBoxSetString(Widget w, char *value);
extern void    XmsrComboBoxShowPosition(Widget w, XmTextPosition position);

extern WidgetClass xmsrComboBoxWidgetClass; /* Die Klasse hoechstselbst */

#ifndef XmIssrComboBox
#define XmIssrComboBox(w)	    XtIsSubclass(w, xmsrComboBoxWidgetClass)
#endif /* XmIssrComboBox */

typedef struct _XmsrComboBoxClassRec *XmsrComboBoxWidgetClass;
typedef struct _XmsrComboBoxRec      *XmsrComboBoxWidget;

/* PLEASE do not use this functions if you really not need to do so !!! */
extern Widget XmsrComboBoxGetEditWidget(Widget w);
extern Widget XmsrComboBoxGetListWidget(Widget w);
extern Widget XmsrComboBoxGetLabelWidget(Widget w);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* __srComboBoxWidget_h */
