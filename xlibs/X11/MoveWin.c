/* $Xorg: MoveWin.c,v 1.4 2001/02/09 02:03:34 xorgcvs Exp $ */
/*

Copyright 1986, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/* $XFree86: xc/lib/X11/MoveWin.c,v 1.3 2001/01/17 19:41:40 dawes Exp $ */

#include "Xlibint.h"

int
XMoveWindow (dpy, w, x, y)
    register Display *dpy;
    Window w;
    int x, y;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 8, req);

    req->window = w;
    req->mask = CWX | CWY;

#ifdef MUSTCOPY
    {
	long lx = (long) x, ly = (long) y;
	dpy->bufptr -= 8;
	Data32 (dpy, (long *) &lx, 4);	/* order dictated by CWX and CWY */
	Data32 (dpy, (long *) &ly, 4);
    }
#else
    {
	CARD32 *valuePtr = (CARD32 *) NEXTPTR(req,xConfigureWindowReq);
	*valuePtr++ = x;
	*valuePtr = y;
    }
#endif /* MUSTCOPY */
    UnlockDisplay(dpy);
    SyncHandle();
    return 1;
}

