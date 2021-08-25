/* vim: set noet: */

#ifndef WMCOMMONW_WMDEF_H
#define WMCOMMONW_WMDEF_H

/* mask indices */
#define	M_BEINGMOVED	(1 << 1)
#define	M_NEVERFOCUS	(1 << 2)
#define	M_OLDSTATE		(1 << 3)
#define M_CENTER		(1 << 4)
#define M_EXPOSED		(1 << 5)
#define M_FIXED			(1 << 6)
#define M_FLOATING		(1 << 7)
#define M_FULLSCREEN	(1 << 8)
#define M_NOSWALLOW		(1 << 9)
#define M_STEAM			(1 << 10)
#define M_TERMINAL		(1 << 11)
#define M_URGENT		(1 << 12)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeHigh }; /* color schemes */
enum { NetSupported, NetWMDemandsAttention, NetWMName, NetWMState, NetWMCheck,
	   NetWMActionClose, NetWMActionMinimize, NetWMAction, NetWMMoveResize,
	   NetWMMaximizedVert, NetWMMaximizedHorz,
	   NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation, NetSystemTrayOrientationHorz,
	   NetWMFullscreen, NetActiveWindow, NetWMWindowType, NetWMWindowTypeDock, NetWMDesktop,
	   NetWMWindowTypeDesktop, NetWMWindowTypeDialog, NetClientList, NetClientListStacking,
	   NetDesktopNames, NetDesktopViewport, NetNumberOfDesktops,
	   NetCurrentDesktop, NetLast, }; /* EWMH atoms */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMChangeState,
	   WMWindowRole, WMLast }; /* default atoms */


/* variables */
static Atom wmatom[WMLast], netatom[NetLast], xatom[XLast], motifatom;


#endif
