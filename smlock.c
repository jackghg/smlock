// smlock
#define _XOPEN_SOURCE 500
#if HAVE_SHADOW_H
#include <shadow.h>
#endif
#include <ctype.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <crypt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <fcntl.h> 
#include <errno.h> 
#include <termios.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/dpms.h>
#include <time.h>
#if HAVE_BSD_AUTH
#include <login_cap.h>
#include <bsd_auth.h>
#endif

bool fst = 1;
int cs = 0;
bool icmc = 0;
bool ecmc = 0;
char icmd[58];
char ecmd[58];

static void
die(const char *errstr, ...) {
    va_list ap;
    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

#ifndef HAVE_BSD_AUTH
static const char *
get_password() { /* only run as root */
    const char *rval;
    struct passwd *pw;
    if(geteuid() != 0)
        die("cannot retrieve password entry (make sure to suid smlock)\n");
    pw = getpwuid(getuid());
    endpwent();
    rval =  pw->pw_passwd;

#if HAVE_SHADOW_H
    {
        struct spwd *sp;
        sp = getspnam(getenv("USER"));
        endspent();
        rval = sp->sp_pwdp;
    }
#endif
    /* drop privileges temporarily */
    if (setreuid(0, pw->pw_uid) == -1)
        die("cannot drop privileges\n");
    return rval;
}
#endif

int main(int argc, char **argv) {
    char curs[] = {0, 0, 0, 0, 0, 0, 0, 0};
    char buf[32], passwd[256], passdisp[256];
    int num, screen, width, height, term, pid, x, y, dir, ascent, descent;

#ifndef HAVE_BSD_AUTH
    const char *pws;
#endif
    unsigned int len;
    bool running = True;
    Cursor invisible;
    Display *dpy;
    KeySym ksym;
    Pixmap pmap;
    Window root, w;
    XColor black, red, dummy;
    XEvent ev;
    XSetWindowAttributes wa;
    XFontStruct* font;
    XFontStruct* fontt;
    GC gc; 
    XGCValues values;
    XCharStruct overall;

    // defaults
    char* passchar = "*";
    char* fontname = "-*-dejavu sans-bold-r-*-*-*-420-100-100-*-*-iso8859-1";
    char* fontnamet = "-*-dejavu sans-medium-r-*-*-*-900-100-100-*-*-iso8859-1";
    char* tfor = "%I:%M";
    char nnt[7];
    char* username = ""; 
    char* banner = "";
    bool showline = 1;
    bool custombanner = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-c")) {
            if (i + 1 < argc) 
                passchar = argv[i + 1];
            else
                die("error: no password character given.\n");
        }
        else if (!strcmp(argv[i], "-f")) {
                if (i + 1 < argc) 
                    fontname = argv[i + 1];
                else
                    die("error: font not specified.\n");
        }
        else if (!strcmp(argv[i], "-g")) {
                if (i + 1 < argc) 
                    fontnamet = argv[i + 1];
                else
                    die("error: font not specified.\n");
        }
        else if (!strcmp(argv[i], "-b")) {
                custombanner = 1;
                banner = argv[i + 1];
                i++;
        }
        else if (!strcmp(argv[i], "-i")) {
                if (i + 1 < argc) {
                    cs = strlen(argv[i + 1]);
                    if ( cs < 55 ) {
                    strcpy( icmd, argv[i + 1] );
                    strcat( icmd, " &");
                    icmc = 1; } }
                else
                    die("error: command not specified.\n");
        }
        else if (!strcmp(argv[i], "-e")) {
                if (i + 1 < argc) {
                    cs = strlen(argv[i + 1]);
                    if ( cs < 55 ) {
                    strcpy( ecmd, argv[i + 1] );
                    strcat( ecmd, " &");
                    ecmc = 1; } }
                else
                    die("error: command not specified.\n");
        }
        else if (!strcmp(argv[i], "-h"))
                    showline = 0;
        else if (!strcmp(argv[i], "-?")) {
                    die("\nsmlock 0.3\n\nOPTIONS:\n-c:	Password mask char/s\n-f:	Name/msg/pass font\n-g:	Clock font\n-b:	Message to replace the name\n-i:	Command at startup\n-e:	Command on exit\n-h:	Hide name/msg\n\n");
                    exit(0); }
    }

    // fill with password characters
    for (int i = 0; i < sizeof passdisp; i+= strlen(passchar)) 
        for (int j = 0; j < strlen(passchar) && i + j < sizeof passdisp; j++)
            passdisp[i + j] = passchar[j];

    /* disable tty switching */
    if ((term = open("/dev/console", O_RDWR)) == -1) {
        perror("error opening console");
    }

    if ((ioctl(term, VT_LOCKSWITCH)) == -1) {
        perror("error locking console"); 
    }
    /* initial command */
    if (icmc) {
       system(icmd);
    }
    /* deamonize */
    pid = fork();
    if (pid < 0) 
        die("Could not fork smlock.");
    if (pid > 0) 
        exit(0); // exit parent 
#ifndef HAVE_BSD_AUTH
    pws = get_password();
    username = getpwuid(geteuid())->pw_name;
#else
    username = getlogin();
#endif
    if (!custombanner)
        banner = username;

    if(!(dpy = XOpenDisplay(0)))
        die("cannot open dpy\n");

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    width = DisplayWidth(dpy, screen);
    height = DisplayHeight(dpy, screen);
    wa.override_redirect = 1;
    wa.background_pixel = XBlackPixel(dpy, screen);
    w = XCreateWindow(dpy, root, 0, 0, width, height, 0, DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen), CWOverrideRedirect | CWBackPixel, &wa);

    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "orange red", &red, &dummy);
    XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "black", &black, &dummy);
    pmap = XCreateBitmapFromData(dpy, w, curs, 8, 8);
    invisible = XCreatePixmapCursor(dpy, pmap, pmap, &black, &black, 0, 0);
    XDefineCursor(dpy, w, invisible);
    XMapRaised(dpy, w);

    font = XLoadQueryFont(dpy, fontname);
    fontt = XLoadQueryFont(dpy, fontnamet);
    if (font == 0 || fontt == 0 ) { die("error: could not find font. Try using a full description.\n"); }
    time_t nd = time(0);
    strftime(nnt, 7, tfor, localtime(&nd));
    gc = XCreateGC(dpy, w, (unsigned long)0, &values);
    XSetFont(dpy, gc, font->fid);
    int blen = strlen(banner);
    XTextExtents(font, banner, blen, &dir, &ascent, &descent, &overall);
    y = (height + ascent - descent) / 2;
    int xposb = (width - XTextWidth(font, banner, blen)) / 2;
    int yposb = y - ascent - 20;
    XSetFont(dpy, gc, fontt->fid);
    XTextExtents(fontt, "0", 1, &dir, &ascent, &descent, &overall);
    y = ( height / 2 ) + ( ascent / 2 );
    int tlen = strlen(nnt);
    int xpost = (width - XTextWidth(fontt, nnt, tlen)) / 2;
    int ypost = (y / 2) ;

    XSetForeground(dpy, gc, XWhitePixel(dpy, screen));

    if((running = running)) {
        for(len = 1000; len; len--) {
            if(XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync, CurrentTime)
                    == GrabSuccess)
                break;
            usleep(1000);
        }
        running = (len > 0);
    }

    len = 0;
    XSync(dpy, False);
    bool update = 1;
    bool sleepmode = 0;

    /* main event loop */
    while(running && !XNextEvent(dpy, &ev)) {
        if (sleepmode) {
            DPMSEnable(dpy);
            DPMSForceLevel(dpy, DPMSModeOff);
            XFlush(dpy);
        }

        if (update) {
            nd = time(0);
            strftime(nnt, 7, tfor, localtime(&nd));
            XTextExtents(font, passdisp, len, &dir, &ascent, &descent, &overall);
            x = (width - overall.width) / 2;
            y = (height + ascent - descent) / 2;
            XSetFont(dpy, gc, fontt->fid);
            XClearWindow(dpy, w);
            XDrawString(dpy,w,gc, xpost, ypost, nnt, tlen);
            XSetFont(dpy, gc, font->fid);
            if (showline) {
            XDrawString(dpy,w,gc, xposb, yposb, banner, blen);
            XDrawLine(dpy, w, gc, width * 3 / 8, y - ascent - 10, width * 5 / 8, y - ascent - 10); }
            XDrawString(dpy,w,gc, x, y, passdisp, len);
            update = False;
        }

        if(ev.type == KeyPress) {
            sleepmode = False;
            buf[0] = 0;
            num = XLookupString(&ev.xkey, buf, sizeof buf, &ksym, 0);
            if(IsKeypadKey(ksym)) {
                if(ksym == XK_KP_Enter)
                    ksym = XK_Return;
                else if(ksym >= XK_KP_0 && ksym <= XK_KP_9)
                    ksym = (ksym - XK_KP_0) + XK_0;
            }
            if(IsFunctionKey(ksym) || IsKeypadKey(ksym)
                    || IsMiscFunctionKey(ksym) || IsPFKey(ksym)
                    || IsPrivateKeypadKey(ksym))
                continue;

            switch(ksym) {
                case XK_Return:
                    passwd[len] = 0;
#ifdef HAVE_BSD_AUTH
                    running = !auth_userokay(getlogin(), NULL, "auth-xlock", passwd);
#else
                    running = strcmp(crypt(passwd, pws), pws);
#endif
                    if (running != 0)
                        // change background on wrong password
                        XSetWindowBackground(dpy, w, red.pixel);
                    len = 0;
                    break;
                case XK_Escape:
                    len = 0;
                    if (DPMSCapable(dpy)) {
                        sleepmode = True;
                    }
                    break;
                case XK_BackSpace:
                    if(len)
                        --len;
                    break;
                default:
                    if(num && !iscntrl((int) buf[0]) && (len + num < sizeof passwd)) { 
                        memcpy(passwd + len, buf, num);
                        len += num;
                    }
                    break;
            }
            update = True; // show changes
        }
    }

    /* free and unlock */
    setreuid(geteuid(), 0);
    if ((ioctl(term, VT_UNLOCKSWITCH)) == -1) {
        perror("error unlocking console"); 
    }

    close(term);
    setuid(getuid()); // drop rights permanently
	/* final command */
    if (ecmc) {       
    system(ecmd);
    }
    XFreePixmap(dpy, pmap);
    XFreeFont(dpy, font);
    XFreeFont(dpy, fontt);
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, w);
    XCloseDisplay(dpy);
    return 0;
}
