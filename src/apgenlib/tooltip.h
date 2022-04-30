#ifndef _TOOLTIP_H_
#define _TOOLTIP_H_

/* file tooltip.h

Implement tooltip for Motif 1.2, 2.0 and 2.1.
Work also for Motif 2.2 which allready include tooltips.

This can also be used for other toolbits which use the
Xt library. In this case modification of the tooltips.c
gile are necessary (allready probided for libXaw{3d}.

Author: Jean-Jacques Sarton jj.sarton@t-online.de
                            http://xwtools.automatix.de
Date:   February 2002

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
 
#ifdef __cplusplus
extern "C" {
#endif

extern void  xmAddTooltip(Widget w);
extern void  xmSetPostDelay(long delay);
extern void  xmSetDuration(long delay);
extern void  xmAddtooltipGlobal(Widget top); /* only for Motif */
extern void  xmEnableTooltip(int enable);
extern void  xmSetXOffset(int offset);
extern void  xmSetYOffset(int offset);

/* a nice convenience function */
extern char *xmGetResource(Widget w, char *resource);

#ifdef __cplusplus
}
#endif

#endif
