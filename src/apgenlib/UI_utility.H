#ifndef UI_UTILITY_H
#define UI_UTILITY_H

#ifdef GUI
#	include <Xm/Xm.h>
#else
#	include "UI_MotifPreview.H"
#endif

#define NUM_OF_TIMEZONES 8

///
void setWidgetColors( Widget w , char *colorName , int flag = 0 ) ;

#endif
