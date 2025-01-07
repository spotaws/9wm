/*
 * Copyright multiple authors, see README for licence details
 */

/* for XShapeEvent */
#include <X11/extensions/shape.h>

#ifdef	DEBUG
#define	trace(s, c, e)	dotrace((s), (c), (e))
#else
#define	trace(s, c, e)
#endif

/* 9wm.c */
void	usage();
void	initscreen(ScreenInfo *, int);
ScreenInfo *getscreen(Window);
Time	timestamp();
void	sendcmessage(Window, Atom, long, int);
void	sendconfig(Client *);
void	sighandler(int);
void	getevent(XEvent *);
void	cleanup();

/* event.c */
void	mainloop(int);
void	configurereq(XConfigureRequestEvent *);
void	mapreq(XMapRequestEvent *);
void	circulatereq(XCirculateRequestEvent *);
void	unmap(XUnmapEvent *);
void	newwindow(XCreateWindowEvent *);
void	destroy(Window);
void	clientmesg(XClientMessageEvent *);
void	cmap(XColormapEvent *);
void	property(XPropertyEvent *);
#ifdef SHAPE
void	shapenotify(XShapeEvent *);
#endif
void	enter(XCrossingEvent *);
void	focusin(XFocusChangeEvent *);
void	reparent(XReparentEvent *);

/* manage.c */
int 	manage(Client *, int);
void	scanwins(ScreenInfo *);
void	setshape(Client *);
void	withdraw(Client *);
void	gravitate(Client *, int);
void	cmapfocus(Client *);
void	cmapnofocus(ScreenInfo *);
void	getcmaps(Client *);
int 	_getprop(Window, Atom, Atom, long, unsigned char **);
char	*getprop(Window, Atom);
Window	getwprop(Window, Atom);
int 	getiprop(Window, Atom);
int 	getwstate(Window, int *);
void	setwstate(Client *, int);
void	setlabel(Client *);
void	getproto(Client *);
void	gettrans(Client *);

/* menu.c */
void	button(XButtonEvent *);
void	spawn(ScreenInfo *, char *);
void	reshape(Client *);
void	move(Client *);
void	delete(Client *, int);
void	hide(Client *);
void	unhide(int, int);
void	unhidec(Client *, int);
void	renamec(Client *, char *);

/* client.c */
void	setactive(Client *, int);
void	draw_border(Client *, int);
void	active(Client *);
void	nofocus();
void	top(Client *);
Client	*getclient(Window, int);
void	rmclient(Client *);
void	dump_revert();
void	dump_clients();

/* grab.c */
int 	menuhit(XButtonEvent *, Menu *);
Client	*selectwin(int, int *, ScreenInfo *);
int 	sweep(Client *);
int 	drag(Client *);
void	getmouse(int *, int *, ScreenInfo *);
void	setmouse(int, int, ScreenInfo *);

/* error.c */
int 	handler(Display *, XErrorEvent *);
void	fatal(char *);
void	graberror(char *, int);
void	showhints();
void	dotrace(char *, Client *, XEvent *);

/* cursor.c */
void	initcurs(ScreenInfo * s);
