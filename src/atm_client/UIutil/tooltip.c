/* file tooltip.c Version 0.4 

Implement tooltip for Motif 1.2, 2.0 and 2.1 and Xaw*

Author: Jean-Jacques Sarton jj.sarton@t-online.de
Date:   February 2002

Changes:
  0.4:  - little bug corrected
  0.3:  - bugs removed, better handling for pulldown menues
  0.2:  - new resources
        - tooltip widgets nor more destroyed and created again
	- removed main(), put this to mtiptoll.c
	- better handling of Menu (Motif)
----------------------------------------------------------------
This is a lean version which can be integrated in an existing
application.

The developer need only to add event handler with the function
xmAddTooltip() for each widget which is relevant or call after the
windows was build the function xmAddtooltipGlobal(). In this case
event handlers will be installed for each pushbutton and togglebuton.

Note that xmAddtooltipGlobal() is not available now for Xaw.

The last work is to fill the resource file with the wanted tooltip textes.
and a few values for the tooltip behaviour.

Widgets:
The tooltip widgets consist of the following widgets:
- shell:      tooltip_shell  (XmMenuShell)
- row_column  tooltip_rc     (XmRowColum)
- label       tooltip_label  (XmLabel)

Resources:
The resource for above widgets are applicable
exected the labelString resoource.

New resources:
tooltipPost	  The delay in millisecond for displaying the tooltip
 		  after a widget was entered (value < 20 -> delay set
		  to 20 ms (avod a few problems)

tooltipDuraction  The time for which the tooltip will be displayed in
 		  case the pointer is still abovce the widget

tooltipX	  X offset relatif to left border of "parent" widget.

tooltipY	  Y offset relatif to botom border of "parent" widget.

tooltip 	  The text which is to be displayed.
 		  This is a new resource and correspond to the labelString
 		  resource.

tooltipBackground background for the tooltip
tooltipForeground foreground for the tooltip
resource file example:

Refer to the included resource file Test.

Using within own projects:
Copy the file tooltip.c to your project source. If you use XAW*
define TIPTOOL_USE_XAW in order to generate the Xaw object file.

*/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#ifndef TOOLTIP_USE_XAW
#include <Xm/XmAll.h>
#include <Xm/Xm.h>
#include <Xm/ToggleBG.h>
#include <X11/StringDefs.h>
#include <Xm/XmStrDefs.h>
#else
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#endif
#include <tooltip.h>

/* prototypes */
static void  showTooltip(Widget w, XtPointer client_data, XEvent *event);
static void  unmapTooltip(XtPointer closure, XtIntervalId *id);
static void  deleteTooltip(Widget w, XtPointer client_data, XEvent *event);
static void  postTooltip(XtPointer closure, XtIntervalId *id);

/* private variables */
static XtIntervalId postId;
static XtIntervalId durationId;
static XtAppContext app;
static int          init;
static long         postDelay;
static long         duration;
Position            xOffset = 1;
Position            yOffset = 1;
static int          enabled = 1;
static Widget       popup_shell;
static Widget       popup_label      = NULL;
static int          pixelInitialized = 0;
static char        *Lang             = NULL;

#define BG_INIT     1
#define FG_INIT     2

 
/*********************************************************************
 * Function: xmTooltipSetLang
 *
 * Set the language name (name of the top form)
 *
 * Input: char *lang
 *
 * Return: -
 *
 ********************************************************************/ 

void xmTooltipSetLang(char *lang)
{
   if ( lang )
      Lang = strdup(lang);
}

/*********************************************************************
 * Function: xmAddTooltip
 *
 * Add the tooltip event handler for the given widget
 *
 * Input: Widget w
 *
 * Return: -
 *
 ********************************************************************/
 
void xmAddTooltip(Widget w)
{
   XtAddEventHandler(w, EnterWindowMask, False,
		               (XtEventHandler) showTooltip, NULL);    
   XtAddEventHandler(w, LeaveWindowMask, False,
		               (XtEventHandler) deleteTooltip, NULL);    
}

/*********************************************************************
 * Function: xmSetPostDelay
 *
 * Set the delay for posting of tooltip. if the value is 0, no delay
 * will be taken in account. The time is in milli second
 *
 * Input: long delay
 *
 * Return: -
 *
 ********************************************************************/ 

void xmSetPostDelay(long delay)
{
   postDelay = delay;
}

/*********************************************************************
 * Function: xmSetDuration
 *
 * Set the delay for unposting of tooltip. if the value is 0, no delay
 * will be taken in account. The time is in milli second
 *
 * Input: long delay
 *
 * Return: -
 *
 ********************************************************************/ 

void xmSetDuration(long delay)
{
   duration = delay;
}

/*********************************************************************
 * Function: xmEnableTooltip
 *
 * Set the enable flag. If the value is 0 tooltips are disabled. 
 * Other values enable displayin of tooltips.
 *
 * Input: int enable 
 *
 * Return: -
 *
 ********************************************************************/ 

void xmEnableTooltip(int enable)
{
   enabled = enable;
}

/*********************************************************************
 * Function: xmSetXOffset
 *
 * Set the x offset where the tooltip will be displayed
 * 
 *
 * Input: int offset 
 *
 * Return: -
 *
 ********************************************************************/ 

void xmSetXOffset(int offset)
{
   xOffset = offset;
   if ( xOffset < 0 ) xOffset = 0;
}

/*********************************************************************
 * Function: xmSetYOffset
 *
 * Set the x offset where the tooltip will be displayed
 * 
 *
 * Input: int offset 
 *
 * Return: -
 *
 ********************************************************************/ 

void xmSetYOffset(int offset)
{
   yOffset = offset;
   if ( yOffset < 0 ) yOffset = 0;
}

#ifndef TOOLTIP_USE_XAW
/*********************************************************************
 * Function: xmAddtooltipGlobal
 *
 * Add the tooltip event handler for all widget bellow the the given
 * topwidget. 
 *
 * Input: Widget top
 *
 * Return: -
 *
 ********************************************************************/ 

void xmAddtooltipGlobal(Widget top)
{
   /* get a list of all widgets */
   WidgetList  children = NULL;
   int         numChildren = 0;
   int         i;
   
   if ( ! XtIsComposite(top) )
   {
      return;
   }

   XtVaGetValues(top,
                 XmNchildren, &children,
                 XmNnumChildren, &numChildren,
                 NULL);
   for (i=0; i < numChildren; i++ )
   {
      /* check for pushbutton and togglebutton */
      if ( XmIsPushButton(children[i])         ||
           XmIsToggleButton(children[i])
         )
      {
         xmAddTooltip(children[i]);
      }
      else
      {
         xmAddtooltipGlobal(children[i]);
      }
   }
#ifndef XmVERSION
   /* with Motif we will have a crash !!!! */
   if ( numChildren )
   {
      free(children);
   }
#endif
}
#endif

/*********************************************************************
 * Function: postTooltip
 *
 * Manage the tooltip if the postdelay timeout was encountoured
 *
 * Input: not relevant
 *
 * Return: -
 *
 ********************************************************************/ 

static void postTooltip(XtPointer closure, XtIntervalId *id)
{
   XtManageChild(popup_shell);
   XRaiseWindow(XtDisplay(popup_shell),XtWindow(popup_shell));

   if ( duration > 0 )
   {
      durationId = XtAppAddTimeOut(app, duration, unmapTooltip, NULL );
   }
   postId = 0;
}

/*********************************************************************
 * Function: unmapTooltip
 *
 * destroy the tooltip if the durcation timeout was encountoured
 *
 * Input: not relevant
 *
 * Return: -
 *
 ********************************************************************/ 

static void unmapTooltip(XtPointer closure, XtIntervalId *id)
{
   if ( popup_shell )
      XtUnmanageChild(popup_shell);
   if (postId)
      XtRemoveTimeOut(postId);
   if ( durationId )
      XtRemoveTimeOut(durationId);
   postId = durationId = 0;
}

/*********************************************************************
 * Function: showTooltip
 *
 * Build the tooltip and display it if the post delay is <= 0.
 * If the post delay is > 0 arme a timer which will manage the tooltip
 * later.
 *
 * Input: Widget w,
 *        XtPointer  client_data
 *        XEvent    *event
 *
 * Return: -
 *
 ********************************************************************/ 

static void showTooltip(Widget w, XtPointer client_data, XEvent *event)
{
   Widget    popup_rc;
   Position  x, y;
   Dimension h;
   Dimension ph, pw;
#ifndef TOOLTIP_USE_XAW
   XmString  xms;
   int	     isMenu = False;
#endif
   char     *s;
   Widget    top    = w;
   Pixel     bg;
   Pixel     fg; 

   if(event->xany.type != EnterNotify)
      return;

   if ( ! enabled )
      return;

//popup tooltips whether the button is sensitive or not
   if ( ! XtIsSensitive(w) )
       return;
   
   /* get the shell */
   while ( !XtIsShell(top) )
      top = XtParent(top);

   if ( ! init )
   {
      /* read the values for tooltip post delay and duration */

      if ((s = xmGetResource(top, "tooltipPost"))) 
      {
         postDelay = atoi(s);
      }

      if ((s = xmGetResource(top, "tooltipDuration"))) 
      {
         duration = atoi(s);
      }

      if ((s = xmGetResource(top, "tooltipX"))) 
      {
         xOffset = atoi(s);
	 if ( xOffset < 0 ) xOffset = 0;
      }

      if ((s = xmGetResource(top, "tooltipY"))) 
      {
         yOffset = atoi(s);
	 if ( yOffset < 0 ) yOffset = 0;
      }

      /* some application will replace all background and */
      /* foreground resources to a given values, so we    */
      /* have our own resource foir this                  */

      if ((s = xmGetResource(top, "tooltipBackground"))) 
      {
         XColor col, col2;
         Colormap cmap = DefaultColormap(XtDisplay(top),
                                         DefaultScreen(XtDisplay(top)));
         if ( XAllocNamedColor(XtDisplay(top),cmap,s,&col,&col2) )
         {
            bg = col.pixel;
            pixelInitialized = BG_INIT;
         }
      }

      if ((s = xmGetResource(top, "tooltipForeground"))) 
      {
         XColor col, col2;
         Colormap cmap = DefaultColormap(XtDisplay(top),
                                         DefaultScreen(XtDisplay(top)));
         if ( XAllocNamedColor(XtDisplay(top),cmap,s,&col,&col2) )
         {
            fg = col.pixel;
            pixelInitialized |= FG_INIT;
         }
      }

      /* and get the application context, for timers */
      app = XtDisplayToApplicationContext(XtDisplay(w));

      popup_shell = XtVaCreatePopupShell("tooltip_shell",
        				 topLevelShellWidgetClass,
        				 top,
        				 XtNoverrideRedirect,
        				 True,
        				 XtNallowShellResize,
        				 True,
        				 XtNmappedWhenManaged, False,
        				 XtNwidth,	       1,
        				 XtNheight,	       1,
        				 XtNborderWidth,       0,
        				 NULL);
                  
      XtManageChild(popup_shell);
      XMoveWindow(XtDisplay(popup_shell), XtWindow(popup_shell), x, y);

      /* the container, The class must be according to the toolkit */
      popup_rc = XtVaCreateWidget(Lang?Lang:"tooltip_form",
#ifndef TOOLTIP_USE_XAW
   				  xmFormWidgetClass,
#else
   				  boxWidgetClass,
#endif
   				  popup_shell,
#ifdef TOOLTIP_USE_XAW
   				  XtNhSpace,	  0,
   				  XtNvSpace,	  0,
   				  XtNborderWidth, 0,
#endif
   				  NULL);
      XtManageChild(popup_rc);

      /* the label, class according to toolkit */
#ifndef TOOLTIP_USE_XAW
      popup_label = XtVaCreateWidget("tooltip_label",
   				     xmLabelWidgetClass,
   				     popup_rc,
   				     NULL);
#else
      popup_label = XtVaCreateWidget("tooltip_label",
   				     labelWidgetClass,
   				     popup_rc,
   				     NULL);
#endif
      if ( pixelInitialized & BG_INIT )
      {
         XtVaSetValues(popup_label, XtNbackground, bg, NULL);
      }
      if ( pixelInitialized & FG_INIT )
      {
         XtVaSetValues(popup_label, XtNforeground, fg, NULL);
      }
      XtManageChild(popup_label);

      XtUnmanageChild(popup_shell);
      XtVaSetValues(popup_shell, XtNmappedWhenManaged, True,NULL);
      init++;
   }

   /* get the resource */
   if ( (s = xmGetResource(w, "tooltip")) )
   {
#ifndef TOOLTIP_USE_XAW
# if XmVERSION == 1
      xms = XmStringCreateLtoR( s, XmFONTLIST_DEFAULT_TAG);
# else
      xms = XmStringGenerate(s,
                             XmFONTLIST_DEFAULT_TAG,
                             XmCHARSET_TEXT,
                             NULL);
# endif			     
      XtVaSetValues(popup_label,
                    XmNlabelString, xms,
		    NULL);
#else
      XtVaSetValues(popup_label,
                    XtNlabel, s,
		    NULL);
#endif
   }
   else
   {
      return;
   }

   /* get position and dimensions */
   
   XtVaGetValues(w, XtNheight, &h, NULL);
   XtTranslateCoords(w, 0, 0, &x, &y);

   XtVaGetValues(popup_shell, XtNheight, &ph, XtNwidth, &pw, NULL);

   /* calculate the x and y position fot popup */

#ifndef TOOLTIP_USE_XAW
   if ( XtParent(w) )
   {
      isMenu = XmIsMenuShell(XtParent(w));
      if ( isMenu == False && XtParent(XtParent(w)) )
      {
         isMenu = XmIsMenuShell(XtParent(XtParent(w)));
      }
   }

   if ( isMenu )
   {
       Dimension wi;
       XtVaGetValues(XtParent(w),
                     XmNwidth, &wi,
                     NULL);
       if ( (x + pw + wi ) > WidthOfScreen(XtScreen(w)) )
          x -= pw;
       else
          x += wi;
   }
   else
   {
#endif
     if ( (h + y + ph + yOffset ) < HeightOfScreen(XtScreen(w)) )
     {
     	y = h + y + yOffset;
     }
     else
     {
     	y = y - ph - yOffset;
     }

     if ( ( x + pw + xOffset ) > WidthOfScreen(XtScreen(w)) )
     {
     	x = WidthOfScreen(XtScreen(w)) - pw;
     }
     else
     {
     	x += xOffset;
     }
#ifndef TOOLTIP_USE_XAW
   }
#endif 
   XtVaSetValues(popup_shell, XtNx, x, XtNy, y,NULL);
   postId = XtAppAddTimeOut(app, postDelay, postTooltip, NULL );

#ifndef TOOLTIP_USE_XAW
   XmStringFree(xms);
#endif
}

/*********************************************************************
 * Function: deleteTooltip
 *
 * unmap the tooltip
 *
 * Input: Widget w,
 *        XtPointer  client_data
 *        XEvent    *event
 *
 * Return: -
 *
 ********************************************************************/ 

static void deleteTooltip(Widget w, XtPointer client_data, XEvent
		   *event)
{
   XLeaveWindowEvent *cevent = (XLeaveWindowEvent *) event;
   if(cevent->type != LeaveNotify)
      return;

   if ( popup_shell )
      XtUnmanageChild(popup_shell);
   if ( durationId )
      XtRemoveTimeOut(durationId);
   durationId = 0;
   if ( postId )
      XtRemoveTimeOut(postId);
   postId = 0;
}

/*********************************************************************
 * Function: xmGetResource
 *
 * read the resource
 *
 * Input: Widget  w
 *        char   *resource
 *
 * Return: char *resource
 *
 ********************************************************************/
 
char *xmGetResource(Widget w, char *resource)
{
    XrmDatabase	 database;
    char        *type_return;
    XrmValue     value_return;
    Widget       top = w;
    char        *tmp;
    char        *first;
    char        *last = strdup(resource);
    
    /* build the full resource name */
    while( !XtIsShell(top) )
    {
       tmp = XtName(top);
       first = (char*)calloc(strlen(tmp)+strlen(last)+2,1);
       sprintf(first, "%s.%s",tmp,last);
       free(last);
       last = first;
       top = XtParent(top);
    }

    /* get the class name */
    XtGetApplicationNameAndClass(XtDisplay(w), &type_return, &tmp);

    /* and complete the name */
    first = (char*)calloc(strlen(tmp)+strlen(last)+2,1);
    sprintf(first, "%s.%s",tmp,last);
    free(last);

    /* get Database */
    database = XtDatabase(XtDisplay(w));

    /* get value from the resource database,the class ist not */
    /* is not important here                                  */
    XrmGetResource(database, first,
             "String", &type_return, &value_return);

#if 0
    printf("%s: <%s>\n",first,value_return.addr?value_return.addr:"");
#endif
    free(first);
    return value_return.addr;
}
