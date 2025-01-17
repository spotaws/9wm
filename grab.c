/*
 * Copyright multiple authors, see README for licence details
 */
#include <stdio.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "dat.h"
#include "fns.h"

int
nobuttons(XButtonEvent * e)
{
	int state;

	state = (e->state & AllButtonMask);
	return (e->type == ButtonRelease) && (state & (state - 1)) == 0;
}

int
grab(Window w, Window constrain, int mask, Cursor curs, int t)
{
	int status;

	if (t == 0)
		t = timestamp();
	status = XGrabPointer(dpy, w, False, mask, GrabModeAsync, GrabModeAsync, constrain, curs, t);
	return status;
}

void
ungrab(XButtonEvent * e)
{
	XEvent ev;

	if (!nobuttons(e))
		for (;;) {
			XMaskEvent(dpy, ButtonMask | ButtonMotionMask, &ev);
			if (ev.type == MotionNotify)
				continue;
			e = &ev.xbutton;
			if (nobuttons(e))
				break;
		}
	XUngrabPointer(dpy, e->time);
	curtime = e->time;
}

int
menuhit(XButtonEvent * e, Menu * m)
{
	XEvent ev;
	int i, n, cur, old, wide, high, status, drawn, warp;
	int x, y, dx, dy, xmax, ymax;
	int tx, ty;
	ScreenInfo *s;

	if (font == 0)
		return -1;
	s = getscreen(e->root);
	if (s == 0 || e->window == s->menuwin)	/* ugly event mangling */
		return -1;

	dx = 0;
	for (n = 0; m->item[n]; n++) {
		wide = XTextWidth(font, m->item[n], strlen(m->item[n])) + 4;
		if (wide > dx)
			dx = wide;
	}
	wide = dx;
	cur = m->lasthit;
	if (cur >= n)
		cur = n - 1;

	high = font->ascent + font->descent + 1;
	dy = n * high;
	x = e->x - wide / 2;
	y = e->y - cur * high - high / 2;
	warp = 0;
	xmax = DisplayWidth(dpy, s->num);
	ymax = DisplayHeight(dpy, s->num);
	if (x < 0) {
		e->x -= x;
		x = 0;
		warp++;
	}
	if (x + wide >= xmax) {
		e->x -= x + wide - xmax;
		x = xmax - wide;
		warp++;
	}
	if (y < 0) {
		e->y -= y;
		y = 0;
		warp++;
	}
	if (y + dy >= ymax) {
		e->y -= y + dy - ymax;
		y = ymax - dy;
		warp++;
	}
	if (warp)
		setmouse(e->x, e->y, s);
	XMoveResizeWindow(dpy, s->menuwin, x, y, dx, dy);
	XSelectInput(dpy, s->menuwin, MenuMask);
	XMapRaised(dpy, s->menuwin);
	status = grab(s->menuwin, None, MenuGrabMask, None, e->time);
	if (status != GrabSuccess) {
		/*
		 * graberror("menuhit", status); 
		 */
		XUnmapWindow(dpy, s->menuwin);
		return -1;
	}
	drawn = 0;
	for (;;) {
		XMaskEvent(dpy, MenuMask, &ev);
		switch (ev.type) {
		default:
			fprintf(stderr, "9wm: menuhit: unknown ev.type %d\n", ev.type);
			break;
		case ButtonPress:
			break;
		case ButtonRelease:
			if (ev.xbutton.button != e->button)
				break;
			x = ev.xbutton.x;
			y = ev.xbutton.y;
			i = y / high;
			if (cur >= 0 && y >= cur * high - 3 && y < (cur + 1) * high + 3)
				i = cur;
			if (x < 0 || x > wide || y < -3)
				i = -1;
			else if (i < 0 || i >= n)
				i = -1;
			else
				m->lasthit = i;
			if (!nobuttons(&ev.xbutton))
				i = -1;
			ungrab(&ev.xbutton);
			XUnmapWindow(dpy, s->menuwin);
			return i;
		case MotionNotify:
			if (!drawn)
				break;
			x = ev.xbutton.x;
			y = ev.xbutton.y;
			old = cur;
			cur = y / high;
			if (old >= 0 && y >= old * high - 3 && y < (old + 1) * high + 3)
				cur = old;
			if (x < 0 || x > wide || y < -3)
				cur = -1;
			else if (cur < 0 || cur >= n)
				cur = -1;
			if (cur == old)
				break;
			if (old >= 0 && old < n)
				XFillRectangle(dpy, s->menuwin, s->gc, 0, old * high, wide, high);
			if (cur >= 0 && cur < n)
				XFillRectangle(dpy, s->menuwin, s->gc, 0, cur * high, wide, high);
			break;
		case Expose:
			XClearWindow(dpy, s->menuwin);
			for (i = 0; i < n; i++) {
				char *item = m->item[i];

				if (i < 5) {
					tx = (wide - XTextWidth(font, item, strlen(item))) / 2;
				} else {
					tx = 1;
				}
				ty = i * high + font->ascent + 1;
				XDrawString(dpy, s->menuwin, s->gc, tx, ty, item, strlen(item));
			}
			if (cur >= 0 && cur < n)
				XFillRectangle(dpy, s->menuwin, s->gc, 0, cur * high, wide, high);
			drawn = 1;
		}
	}
}

Client *
selectwin(int release, int *shift, ScreenInfo * s)
{
	XEvent ev;
	XButtonEvent *e;
	int status;
	Window w;
	Client *c;

	status = grab(s->root, s->root, ButtonMask, s->target, 0);
	if (status != GrabSuccess) {
		graberror("selectwin", status);	/* */
		return 0;
	}
	w = None;
	for (;;) {
		XMaskEvent(dpy, ButtonMask, &ev);
		e = &ev.xbutton;
		switch (ev.type) {
		case ButtonPress:
			if (e->button != Button3) {
				ungrab(e);
				return 0;
			}
			w = e->subwindow;
			if (!release) {
				c = getclient(w, 0);
				if (c == 0)
					ungrab(e);
				if (shift != 0)
					*shift = (e->state & ShiftMask) != 0;
				return c;
			}
			break;
		case ButtonRelease:
			ungrab(e);
			if (e->button != Button3 || e->subwindow != w)
				return 0;
			if (shift != 0)
				*shift = (e->state & ShiftMask) != 0;
			return getclient(w, 0);
		}
	}
}

void
sweepcalc(Client * c, int x, int y)
{
	int dx, dy, sx, sy;

	dx = x - c->x;
	dy = y - c->y;
	sx = sy = 1;
	if (dx < 0) {
		dx = -dx;
		sx = -1;
	}
	if (dy < 0) {
		dy = -dy;
		sy = -1;
	}

	dx -= 2 * BORDER;
	dy -= 2 * BORDER;
	if (c->size.flags & PResizeInc) {
		dx = c->min_dx + (dx - c->min_dx) / c->size.width_inc * c->size.width_inc;
		dy = c->min_dy + (dy - c->min_dy) / c->size.height_inc * c->size.height_inc;
	}

	if (c->size.flags & PMaxSize) {
		if (dx > c->size.max_width)
			dx = c->size.max_width;
		if (dy > c->size.max_height)
			dy = c->size.max_height;
	}
	c->dx = sx * (dx + 2 * BORDER);
	c->dy = sy * (dy + 2 * BORDER);
}

void
dragcalc(Client * c, int x, int y)
{
	c->x = x;
	c->y = y;
}

void
drawbound(Client * c)
{
	int x, y, dx, dy;
	ScreenInfo *s;

	s = c->screen;
	x = c->x;
	y = c->y;
	dx = c->dx;
	dy = c->dy;
	if (dx < 0) {
		x += dx;
		dx = -dx;
	}
	if (dy < 0) {
		y += dy;
		dy = -dy;
	}
	if (dx <= 2 || dy <= 2)
		return;
	XDrawRectangle(dpy, s->root, s->gc, x, y, dx - 1, dy - 1);
	XDrawRectangle(dpy, s->root, s->gc, x + 1, y + 1, dx - 3, dy - 3);
}

void
misleep(int msec)
{
	struct timeval t;

	t.tv_sec = msec / 1000;
	t.tv_usec = (msec % 1000) * 1000;
	select(0, 0, 0, 0, &t);
}

int
sweepdrag(Client * c, XButtonEvent * e0, void (*recalc) (Client *, int, int))
{
	XEvent ev;
	int idle;
	int cx, cy, rx, ry;
	int ox, oy, odx, ody;
	XButtonEvent *e;

	ox = c->x;
	oy = c->y;
	odx = c->dx;
	ody = c->dy;
	c->x -= BORDER;
	c->y -= BORDER;
	c->dx += 2 * BORDER;
	c->dy += 2 * BORDER;
	if (e0) {
		c->x = cx = e0->x;
		c->y = cy = e0->y;
		recalc(c, e0->x, e0->y);
	} else
		getmouse(&cx, &cy, c->screen);
	XGrabServer(dpy);
	drawbound(c);
	idle = 0;
	for (;;) {
		if (XCheckMaskEvent(dpy, ButtonMask, &ev) == 0) {
			getmouse(&rx, &ry, c->screen);
			if (rx != cx || ry != cy || ++idle > 300) {
				drawbound(c);
				if (rx == cx && ry == cy) {
					XUngrabServer(dpy);
					XFlush(dpy);
					misleep(500);
					XGrabServer(dpy);
					idle = 0;
				}
				recalc(c, rx, ry);
				cx = rx;
				cy = ry;
				drawbound(c);
				XFlush(dpy);
			}
			misleep(50);
			continue;
		}
		e = &ev.xbutton;
		switch (ev.type) {
		case ButtonPress:
		case ButtonRelease:
			drawbound(c);
			ungrab(e);
			XUngrabServer(dpy);
			recalc(c, ev.xbutton.x, ev.xbutton.y);
			if (c->dx < 0) {
				c->x += c->dx;
				c->dx = -c->dx;
			}
			if (c->dy < 0) {
				c->y += c->dy;
				c->dy = -c->dy;
			}
			c->x += BORDER;
			c->y += BORDER;
			c->dx -= 2 * BORDER;
			c->dy -= 2 * BORDER;
			if (c->dx < 4 || c->dy < 4 || c->dx < c->min_dx || c->dy < c->min_dy)
				goto bad;
			return 1;
		}
	}
      bad:
	c->x = ox;
	c->y = oy;
	c->dx = odx;
	c->dy = ody;
	return 0;
}

int
sweep(Client * c)
{
	XEvent ev;
	int status;
	XButtonEvent *e;
	ScreenInfo *s;

	s = c->screen;
	status = grab(s->root, s->root, ButtonMask, s->sweep0, 0);
	if (status != GrabSuccess) {
		graberror("sweep", status);	/* */
		return 0;
	}

	XMaskEvent(dpy, ButtonMask, &ev);
	e = &ev.xbutton;
	if (e->button != Button3) {
		ungrab(e);
		return 0;
	}
	if (c->size.flags & (PMinSize | PBaseSize))
		setmouse(e->x + c->min_dx, e->y + c->min_dy, s);
	XChangeActivePointerGrab(dpy, ButtonMask, s->boxcurs, e->time);
	return sweepdrag(c, e, sweepcalc);
}

int
drag(Client * c)
{
	int status;
	ScreenInfo *s;

	s = c->screen;
	if (c->init)
		setmouse(c->x - BORDER, c->y - BORDER, s);
	else {
		getmouse(&c->x, &c->y, s);	/* start at current mouse pos */
		c->x += BORDER;
		c->y += BORDER;
	}
	status = grab(s->root, s->root, ButtonMask, s->boxcurs, 0);
	if (status != GrabSuccess) {
		graberror("drag", status);	/* */
		return 0;
	}
	return sweepdrag(c, 0, dragcalc);
}

void
getmouse(int *x, int *y, ScreenInfo * s)
{
	Window dw1, dw2;
	int t1, t2;
	unsigned int t3;

	XQueryPointer(dpy, s->root, &dw1, &dw2, x, y, &t1, &t2, &t3);
}

void
setmouse(int x, int y, ScreenInfo * s)
{
	XWarpPointer(dpy, None, s->root, None, None, None, None, x, y);
}
