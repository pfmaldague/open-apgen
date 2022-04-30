#if HAVE_CONFIG_H
#include <config.h>
#endif
#include "UI_GeneralMotifInclude.H"

#include "UI_utility.H"
#define UI_utility_C

extern int		GUI_Flag ;

void setWidgetColors (Widget w,char *colorName,int flag) {
#ifdef GUI
	if( GUI_Flag ) {
	Colormap cmap;
	XColor color, unused;
	Pixel bgColor,topShadow,bottomShadow,fg,selectColor;

	XtVaGetValues (w,XmNcolormap,&cmap,NULL);
	XAllocNamedColor (XtDisplay (w),cmap,colorName,&color,&unused);
	bgColor = color.pixel;
	XmGetColors (XtScreen (w),cmap,bgColor,&fg,&topShadow,
		&bottomShadow,&selectColor);
	XtVaSetValues (w,
		XtVaTypedArg,XmNforeground,XmRString,"Black",
		    strlen ("Black") + 1,
		XmNbackground,		bgColor,
		XmNtopShadowColor,	topShadow,
		XmNbottomShadowColor,	bottomShadow,
		NULL);
	if (flag) XtVaSetValues (w,XmNtroughColor,selectColor,NULL);
	}
#endif
}
