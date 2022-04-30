/*	PROJECT		: APGEN
 *	SUBSYSTEM	: UI (non-BX Motif)
 *	FILE NAME	: UI_MotifPreview.H
 *
 *
 *	ABSTRACT:
 *	
 *      Motif Preview Header file (used by Pierre's Motif wrapper classes),
 *      adapted from SEQ_REVIEW file MotifPreview.H
 *
 */

#ifndef BATCH_GENERAL_INCLUDE_H
#define BATCH_GENERAL_INCLUDE_H

// general-purpose Motif "gut-less emulation" for X-less version of seq_review

#define True	1
// #define TRUE	1
// #define FALSE	0
#define False	0

class _WidgetRec ;
class _WidgetClassRec ;

typedef struct _WidgetRec* Widget ;
typedef struct _WidgetClassRec *WidgetClass;
typedef void * XtPointer ;
typedef unsigned short     Dimension ;

class callback_stuff ;
// class handler_stuff ;
// class kbd_and_mouse_handler_stuff ;

typedef void (* callback_proc ) ( Widget , callback_stuff * , void * ) ;
// typedef void (* event_handler ) ( Widget , handler_stuff * , void * ) ;
// typedef void (* kbd_and_mouse_event_handler ) ( Widget , kbd_and_mouse_handler_stuff * , void * ) ;

// The following classes don't exist in motif (only way to create the Widget is through a convenience routine):
extern WidgetClass xmScrolledListWidgetClass ;
extern WidgetClass xmScrolledTextWidgetClass ;
extern WidgetClass xmCheckBoxWidgetClass ;
extern WidgetClass xmRadioBoxWidgetClass ;
extern WidgetClass xmOptionMenuWidgetClass ;
extern WidgetClass xmPulldownMenuWidgetClass ;
extern WidgetClass xmTopLevelWidgetClass ;
extern WidgetClass xmFileSelectionDialogWidgetClass ;
extern WidgetClass xmPromptDialogWidgetClass ;
extern WidgetClass xmPopupShellWidgetClass ;
extern WidgetClass xmMenubarWidgetClass ;
extern WidgetClass xmPopupMenuWidgetClass ;
#ifndef GUI
	extern WidgetClass xmCascadeButtonWidgetClass ;
	extern WidgetClass xmDialogShellWidgetClass ;
	extern WidgetClass xmDrawingAreaWidgetClass ;
	extern WidgetClass xmFormWidgetClass ;
	extern WidgetClass xmFrameWidgetClass ;
	extern WidgetClass xmLabelWidgetClass ;
	extern WidgetClass xmPushButtonWidgetClass ;
	extern WidgetClass xmPanedWindowWidgetClass ;
	extern WidgetClass xmRowColumnWidgetClass ;
	extern WidgetClass xmScaleWidgetClass ;
	extern WidgetClass xmScrollBarWidgetClass ;
	extern WidgetClass xmScrolledWindowWidgetClass ;
	extern WidgetClass xmSeparatorWidgetClass ;
	extern WidgetClass xmTextWidgetClass ;
	extern WidgetClass xmToggleButtonWidgetClass ;
	extern WidgetClass xmMainWindowWidgetClass ;
	extern WidgetClass xmMenuShellWidgetClass ;

	// ...

	extern const char *XmNactivateCallback ;
	extern const char *XmNmenuHistory ;
	extern const char *XmNvalueChangedCallback ;
	extern const char *XmNsingleSelectionCallback ;
	extern const char *XmNlosingFocusCallback ;
	extern const char *XmNokCallback ;
	extern const char *XmNbrowseSelectionCallback ;
	extern const char *XmNdefaultActionCallback ;
	// ...
	extern char * XtName( Widget ) ;
	// ...
	extern long StructureNotifyMask ;
	extern long ExposureMask ;
	extern long  EnterWindowMask ;
	extern long LeaveWindowMask ;

	typedef void * Arg ;
	typedef int Cardinal ;

#endif

#endif
