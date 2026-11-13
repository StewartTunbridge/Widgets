/////////////////////////////////////////////////////////////////////////
//
// Widgets: X11/XLib Driver
//
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2020,2025 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////


#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
//#include <X11/XKBlib.h>
#include <xkbcommon/xkbcommon.h>
#include <limits.h>

#include "WidgetsDriver.hpp"

extern void DebugAdd (const char *St);   // defined in application
extern void DebugAddS (const char *s1, const char *s2);   // defined in Widgets.cpp
extern void DebugAddInt (const char *St, int n);
//extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddHex (const char *St, int p);

extern bool RenderBitmapTransparentTL (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest);

typedef struct
  {
    int Width, Height;
    int ImageDataSize;
    int *ImageData;
    XImage *Image;   // RGB normally
    byte *Alpha;   // Pixel Alpha mask: 0 => not to render.  0xFF => Full render
  } __Bitmap;

typedef struct
  {
    int XScreen;
    int XDepth;
    Visual *XVisual;
    Window XWindow;
    GC gc;
    //
    byte WindowAttributes;
    int SizeX, SizeY;
  } __Window;

Display *XDisplay = NULL;
Window WindowMain = 0;
Atom aDeleteWindow;   // Used to trap window close events

// Clipboard support
Atom aCLIPBOARD;
Atom aUTF8;
Atom aXSEL_DATA;
Atom aTARGETS;

Window ClipboardRequestWindow = 0;
char *ClipboardData = NULL;


/////////////////////////////////////////////////////////////////////////
//
// Error Handling

const char XError [] = "** X ERROR: ";

void DebugAddXError (char *St)
  {
    char *Message = (char *) malloc (StrLen (St) + StrLen (XError) + 32);
    char *x = Message;
    if (St)
      {
        StrCat (&x, XError);
        StrCat (&x, ": ");
        StrCat (&x, St);
        *x = 0;
        DebugAdd (Message);
      }
    free (Message);
  }

int ErrorHandler (Display *Dis, XErrorEvent *ErrorEvent)
  {
    char st [80];
    //
    XGetErrorText (Dis, ErrorEvent->error_code, st, sizeof (st));
    DebugAddXError (st);
    if (ErrorEvent->minor_code)
      {
        XGetErrorText (Dis, ErrorEvent->minor_code, st, sizeof (st));
        DebugAddXError (st);
      }
    return 0;
  }

void XImageDump (XImage *Image, char *Name)
  {
    int x, y;
    int pixel;
    char *s, *ps;
    //
    if (Image)
      {
        s = (char *) malloc (Max (128, Image->width + 8));
        ps = s;
        StrCat (&ps, "Image Dump ");
        if (Name)
          StrCat (&ps, Name);
        StrCat (&ps, ": ");
        for (x = 0; x < 8; x++)
          {
            NumToHex (&ps, XGetPixel (Image, x, 0));
            StrCat (&ps, ' ');
          }
        *ps = 0;
        DebugAdd (s);
        for (y = 0; y < Image->height; y++)
          {
            ps = s;
            for (x = 0; x < Image->width; x++)
              {
                pixel = XGetPixel (Image, x, y);
                StrCat (&ps, 'A' + (ColourR (pixel) + ColourG (pixel) + ColourB (pixel)) % 26);
              }
            *ps = 0;
            DebugAdd (s);
          }
        free (s);
      }
  }


/////////////////////////////////////////////////////////////////////////
//
// COLOUR TRANSLATION - and yes these are both the same

int ColourToXColour (int Colour)//, byte Alpha)
  {
    _Colour c, Res;
    //
    //return Colour;
    c.RGBA = Res.RGBA = Colour;
    Res.Prime [0] = c.Prime [2];
    Res.Prime [2] = c.Prime [0];
    return Res.RGBA;
    //return ColourR (Colour) << 16 | ColourG (Colour) << 8 | ColourB (Colour) | Alpha << 24;
    //return ColourR (Colour) << 16 | ColourG (Colour) << 8 | ColourB (Colour);
  }

int XColourToColour (int XColour)
  {
    _Colour c, Res;
    //
    //return XColour;
    c.RGBA = Res.RGBA = XColour;
    Res.Prime [0] = c.Prime [2];
    Res.Prime [2] = c.Prime [0];
    return Res.RGBA;
    //return ((XColour & 0xFF0000) >> 16) | (XColour & 0x00FF00) | ((XColour & 0x0000FF) << 16);
  }


/////////////////////////////////////////////////////////////////////////
//
// MOUSE

bool MouseCursorState = true;

Cursor MouseCursorMap = 0;
Pixmap MouseCursorPixmap = 0;
byte MouseCursorData [] = {0};
XColor MouseCursorCol = {0, 0, 0, 0, 0, 0};

_Point MousePos (void)
  {
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int mask;
    XQueryPointer (XDisplay, WindowMain, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
    return {rootX, rootY};
  }

void MouseCursor (_Window *Window, bool Show)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (Show != MouseCursorState)
      {
        XFreeCursor (XDisplay, MouseCursorMap);
        if (Show)
          MouseCursorMap = XCreateFontCursor (XDisplay, XC_arrow);
        else
          {
            if (!MouseCursorPixmap)
              MouseCursorPixmap = XCreateBitmapFromData (XDisplay, Window_->XWindow, (char *) MouseCursorData, 1, 1);
            MouseCursorMap = XCreatePixmapCursor (XDisplay, MouseCursorPixmap, MouseCursorPixmap, &MouseCursorCol, &MouseCursorCol, 0, 0);
          }
        XDefineCursor (XDisplay, Window_->XWindow, MouseCursorMap);
        MouseCursorState = Show;
      }
  }


/////////////////////////////////////////////////////////////////////////
//
// INITIALIZE ...

bool WidgetsInit (void)
  {
    XSetErrorHandler (ErrorHandler);
    XDisplay = XOpenDisplay (NULL);
    aDeleteWindow = XInternAtom (XDisplay, "WM_DELETE_WINDOW", False);
    aCLIPBOARD = XInternAtom (XDisplay, "CLIPBOARD", false);
    aUTF8 = XInternAtom (XDisplay, "UTF8_STRING", false);
    aXSEL_DATA = XInternAtom (XDisplay, "XSEL_DATA", false);
    aTARGETS = XInternAtom (XDisplay, "TARGETS", false);
    return true;
  }

bool WidgetsUninit (void)
  {
    if (XDisplay)
      XCloseDisplay (XDisplay);
    XDisplay = NULL;
    if (ClipboardData)
      free (ClipboardData);
    ClipboardData = NULL;
    return true;
  }

void SetGCFunction (__Window *Window, GC gc, int Function)
  {
    XGCValues gcv;
    //
    gcv.function = Function;
    XChangeGC (XDisplay, gc, GCFunction, &gcv);
  }

void ApplyAlpha (Window window, Drawable Image, int Width, int Height, byte Alpha)
  {
    int Alpha_;
    //
    Alpha_ = Alpha | Alpha << 8 | Alpha << 16 | Alpha << 24;
    if (Alpha == 0xFF)
      XDeleteProperty (XDisplay, window, XInternAtom (XDisplay, "_NET_WM_WINDOW_OPACITY", 0));
    else
      XChangeProperty (XDisplay, window, XInternAtom (XDisplay, "_NET_WM_WINDOW_OPACITY", 0),
                       XA_CARDINAL, 32, PropModeReplace, (byte *) &Alpha_, 1);
  }


//////////////////////////////////////////////////////////////////////////////
// WINDOW

XVisualInfo vinfo;
XSetWindowAttributes attr;

_Window *WindowCreate (const char *Title, int x, int y, int SizeX_, int SizeY_, byte WindowAttributes)
  {
    __Window *Res;
    bool OK;
    //int x_, y_;
    //Window w;
    int Width, Height;
    Atom window_type;
    long value;
    //
    OK = false;
    Res = (__Window *) malloc (sizeof (__Window));
    *Res = {0};
    if (XDisplay)
      {
        Res->WindowAttributes = WindowAttributes;
        Res->XScreen = DefaultScreen (XDisplay);
        /*
        XMatchVisualInfo (Res->XDisplay, Res->XScreen, 32, TrueColor, &vinfo);
        attr.colormap = XCreateColormap (Res->XDisplay, DefaultRootWindow (Res->XDisplay), vinfo.visual, AllocNone);
        attr.border_pixel = 0;
        attr.background_pixel = 0xFFFFFFFF; //0x00000000; //0xFFFFFFFF; //0x80808080; //0x80ffffff; //None
        Res->XVisual = vinfo.visual;
        Res->XDepth = vinfo.depth;// 32;
        */
        ///*
        Res->XDepth = DefaultDepth (XDisplay, Res->XScreen);
        Res->XVisual = DefaultVisual (XDisplay, Res->XScreen);
        //*/
        Width = XDisplayWidth (XDisplay, Res->XScreen);
        Height = XDisplayHeight (XDisplay, Res->XScreen);
        if (SizeX_ * SizeY_ == 0 || (WindowAttributes & waFullScreen))
          {
            x = 0;
            y = 0;
            SizeX_ = Width;
            SizeY_ = Height;
          }
        Res->XWindow = XCreateSimpleWindow (XDisplay, RootWindow (XDisplay, Res->XScreen), 0, 0, SizeX_, SizeY_, 0, BlackPixel (XDisplay, Res->XScreen), WhitePixel (XDisplay, Res->XScreen));
        if (Res->XWindow)
          {
            XSetWindowBackgroundPixmap (XDisplay, Res->XWindow, None);
            if (WindowMain == 0)
              WindowMain = Res->XWindow;   // First window, keep for MousePos () and Clipboard
            XMapWindow (XDisplay, Res->XWindow);   // needed for XMoveWindow ()
            XMoveWindow (XDisplay, Res->XWindow, x, y);
            Res->SizeX = SizeX_;
            Res->SizeY = SizeY_;
            Res->gc = XCreateGC (XDisplay, Res->XWindow, 0, NULL);
            SetGCFunction (Res, Res->gc, GXcopy);
            // Set Event traps
            XSetWMProtocols (XDisplay, Res->XWindow, &aDeleteWindow, 1);   // Trap window close messages
            //
            if (~WindowAttributes & waResizable)   // NOT Resizable
              {
                XSizeHints *size_hints = XAllocSizeHints ();
                size_hints->flags = PMinSize | PMaxSize;
                size_hints->min_width = size_hints->max_width = SizeX_;
                size_hints->min_height = size_hints->max_height = SizeY_;
                XSetWMNormalHints (XDisplay, Res->XWindow, size_hints);
                XFree (size_hints);
                //attr.override_redirect = 1; BLOCKS EVERY WINDOW MANAGER TASK
                //XChangeWindowAttributes (Res->XDisplay, Res->XWindow, CWOverrideRedirect, &attr);
                //window_type = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE", false);
                //value = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE_MENU", false);
                //XChangeProperty (XDisplay, Res->XWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
              }
            //
            if (WindowAttributes & waBorderless)
              {
                // Borderless
                window_type = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE", false);
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_DOCK", false);//Borderless, always on top, can't focus
                value = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE_TOOLBAR", false);//Borderless, behind init, can focus
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_MENU", false);//?Normal, init unfocussed
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_UTILITY", false);//Normal, behind
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_SPLASH", false);// Borberless, center screen
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_DIALOG", false);//Normal, center screen
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_POPUP_MENU", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_TOOLTIP", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_NOTIFICATION", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_COMBO", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_DND", false);//Normal
                //value = XInternAtom (Res->XDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", false);//Normal
                XChangeProperty (XDisplay, Res->XWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
                // Bring to front
                //???
              }
            if (WindowAttributes & waAlwaysOnTop)
              {
                //XChangeProperty (XDisplay, Res->XWindow, _NET_WM_STATE_ATOM, XA_ATOM, 32, PropModeReplace, (unsigned char *)&(_NET_WM_STATE_ABOVE), 1);
                window_type = XInternAtom (XDisplay, "_NET_WM_STATE", false);
                value = XInternAtom (XDisplay, "_NET_WM_STATE_ABOVE", false);
                XChangeProperty (XDisplay, Res->XWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
              }
            if (WindowAttributes & waModal)   // doesn't work
              {
                window_type = XInternAtom (XDisplay, "_NET_WM_STATE", false);
                value = XInternAtom (XDisplay, "_NET_WM_STATE_MODAL", true);//false
                XChangeProperty (XDisplay, Res->XWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
              }
            //
            XSelectInput (XDisplay, Res->XWindow,
              ExposureMask |
              KeyPressMask | KeyReleaseMask |
              ButtonPressMask | ButtonReleaseMask |
              PointerMotionMask |
              FocusChangeMask |   // Window gets / loses focus
              StructureNotifyMask);   // Specify what Events we get
            if (Title)
              XStoreName (XDisplay, Res->XWindow, Title);   // Set Title Name
            //XRaiseWindow (XDisplay, Res->XWindow);
            //Window w;
            //int r;
            //XGetInputFocus (XDisplay, &w, &r);
            //XGetInputFocus (XDisplay, &w, &r);
            XSetInputFocus (XDisplay, Res->XWindow, RevertToParent, CurrentTime);  // works but produces error
            //XFlush (XDisplay);
            OK = true;
          }
      }
    if (!OK)
      {
        DebugAddXError ("Window Create");
        free (Res);
        Res = NULL;
      }
    return Res;
  }

bool WindowSetTitle (_Window * Window, const char *Title)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    XStoreName (XDisplay, Window_->XWindow, Title);
    return true;
  }

bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap_)
  {
    __Window *Window_;
    __Bitmap *Bitmap;
    long *Pixels, *p;
    int Size;
    int Pix, TransPix;
    bool TransImg;
    int x, y;
    Atom net_wm_icon;
    //
    if (Bitmap_ == NULL)
      return false;
    Window_ = (__Window *) Window;
    Bitmap = (__Bitmap *) Bitmap_;
    // Convert Bitmap to Icon format: (long) w, h, p..p
    Size = 2 + Bitmap->Width * Bitmap->Height;
    Pixels = (long *) malloc (Size * sizeof (long));
    p = Pixels;
    *p++ = Bitmap->Width;
    *p++ = Bitmap->Height;
    TransPix = XGetPixel (Bitmap->Image, 0, 0);
    TransImg = Bitmap->Alpha && Bitmap->Alpha [0] != 0xFF;
    for (y = 0; y < Bitmap->Height; y++)
      for (x = 0; x < Bitmap->Width; x++)
        {
          Pix = ColourToXColour (BitmapGetPixel (Bitmap_, x, y));
          if (!TransImg)
            if (((Pix ^ TransPix) & 0xFFFFFF) == 0)   // transparent
              Pix = 0x00000000;
          *p++ = Pix;
        }
    net_wm_icon = XInternAtom (XDisplay, "_NET_WM_ICON", False);
    XChangeProperty (XDisplay, Window_->XWindow, net_wm_icon, XA_CARDINAL, 32, PropModeReplace, (byte *) Pixels, Size);//(unsigned char *) Img->data, Img->width * Img->height);
    free (Pixels);
    return true;
  }

bool WindowSetPosSize (_Window *Win, int x, int y, int Width, int Height)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Win;
    if (x >= 0 && y >= 0)
      XMoveWindow (XDisplay, Window_->XWindow, x, y);
    if (Width >= 0 && Height >= 0)
      XResizeWindow (XDisplay, Window_->XWindow, Width, Height);
    return true;
  }

void WindowGetPosSize (_Window *Win, int *x, int *y, int *Width, int *Height)
  {
    __Window *Window_;
    XWindowAttributes  wa;
    int x_, y_;
    Window w;
    //
    Window_ = (__Window *) Win;
    XTranslateCoordinates (XDisplay, Window_->XWindow, DefaultRootWindow (XDisplay), 0, 0, &x_, &y_, &w);
    XGetWindowAttributes (XDisplay, Window_->XWindow, &wa); //w
    if (x)
      *x = x_ - wa.x;
    if (y)
      *y = y_ - wa.y;
    if (Width)
      *Width = wa.width;
    if (Height)
      *Height = wa.height;
  }

void WindowRaise (_Window *Window)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    XRaiseWindow (XDisplay, Window_->XWindow);
  }

void WindowDestroy (_Window *Window)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    XFreeGC (XDisplay, Window_->gc);
    if (MouseCursorPixmap)
      XFreePixmap (XDisplay, MouseCursorPixmap);
    XDestroyWindow (XDisplay, Window_->XWindow);
    free (Window);
  }

void RenderPresent (_Window *Window_, _Bitmap *Bitmap_, _Rect Source, _Point Dest, byte Alpha)
  {
    __Window *Window;
    __Bitmap *Bitmap;
    //
    //DebugAdd ("Prensenting");   //%%%% Optimize
    Window = (__Window *) Window_;
    Bitmap = (__Bitmap *) Bitmap_;
    if (Alpha != 0xFF)
      ApplyAlpha (Window->XWindow, Window->XWindow, Window->SizeX, Window->SizeY, Alpha);
    XPutImage (XDisplay, Window->XWindow, Window->gc, Bitmap->Image, Source.x, Source.y, Dest.x, Dest.y, Source.Width, Source.Height);  //Window->SizeX, Window->SizeY);
  }

/*
typedef struct
  {
    int RangeA, RangeB;
    int Char;
    char *Chars;
  } _KeyTable;

byte KeyEdit [] = {KeyHome, KeyUp, KeyPageUp, KeyLeft, KeyRight, KeyEnd, KeyDown, KeyPageDown, KeyIns, KeyDel, '\0'};

_KeyTable KeyTable [] =
  {
    {65, 65, ' ',    NULL},
    {49, 49, '`',    NULL},
    {10, 22, 0,      "1234567890-=\x08"},
    {23, 35, 0,      "\x09qwertyuiop[]"},
    {51, 51, '\\',   NULL},
    {38, 48, 0,      "asdfghjkl;'"},
    {36, 36, '\x0d', NULL},   // Enter
    {52, 61, 0,      "zxcvbnm,./"},
    {9, 9, '\x1b',   NULL},   // esc
    //
    // Numeric Keypad
    {77, 77, KeyNL, NULL},   // Num Lock
    {106, 106, '/', NULL},
    {63, 63, '*', NULL},
    {82, 82, '-', NULL},
    {79, 81, 0, "789"},
    {83, 85, 0, "456"},
    {87, 89, 0, "123"},
    {90, 91, 0, "0."},
    {86, 86, '+', NULL},
    {104, 104, '\x0d', NULL},   // Enter
    //
    {67, 76, KeyF1, NULL},
    {95, 96, KeyF1+10, NULL},
    //
    {110, 119, 0, (char *) KeyEdit},
    //
    {50, 50, KeyShift, NULL},
    {62, 62, KeyShift, NULL},
    {37, 37, KeyCntl, NULL},
    {105, 105, KeyCntl, NULL},
    {64, 64, KeyAlt, NULL},
    {108, 108, KeyAlt, NULL},
    //
    {-1, -1, 0, NULL}
  };

char KeyShiftTable [] = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:'\",<.>/?";

int LookupKeyCode (bool Down, int keycode, int *ShiftState)
  {
    int Res;
    byte ResCh;
    bool ResAlpha;
    char *p;
    _KeyTable *kt;
    //
    Res = 0;
    kt = KeyTable;
    while (true)
      {
        if (kt->RangeA < 0)
          break;
        if (keycode >= kt->RangeA && keycode <= kt->RangeB)   // in range
          {
            if (kt->Char == 0)
              Res = (byte) kt->Chars [keycode - kt->RangeA];   // use the "string"
            else if (kt->Char < 0x100)   // non shift keys
              Res = kt->Char + (keycode - kt->RangeA);
            else   // Shift Keys
              if (Down)
                *ShiftState |= kt->Char;
              else
                *ShiftState &= ~kt->Char;
            break;
          }
        kt++;
      }
    ResCh = Res;   // low 8 bits
    ResAlpha = (ResCh >= 'a' && ResCh <= 'z');
    if (Res)
      {
        Res |= *ShiftState;
        if (ResCh && (ResCh < 0x80))
          if (*ShiftState & KeyAlt)   // Is Alt
            {
              if (ResAlpha)   // Alt capitals
                Res = Res - 'a' + 'A';
            }
          else
            if (*ShiftState & KeyCntl)   // Is Cntrl
              {
                if (ResCh >= '@')
                  ResCh = ResCh & 0x1F;
                Res = ResCh;
              }
            else if (*ShiftState & KeyShift)
              if (ResAlpha)
                Res = ResCh - 'a' + 'A';
              else
                {
                  p = StrPos (KeyShiftTable, (char) ResCh);
                  if (p)
                    Res = *(p + 1);
                }
      }
    return Res;
  }
*/

typedef struct
  {
    int XK0, XK1;
    int Key;
  } _TranslateTable;

_TranslateTable XKeyTable [] =
  {
    {XK_Shift_L, XK_Shift_R, KeyShift},
    {XK_Control_L, XK_Control_R, KeyCntl},
    {XK_Alt_L, XK_Alt_R, KeyAlt},

    {XK_Escape, XK_Escape, esc},
    {XK_Tab, XK_Tab, '\t'},
    {XK_BackSpace, XK_BackSpace, '\b'},
    {XK_Return, XK_Return, '\r'},

    {XK_Insert, XK_Insert, KeyIns},
    {XK_Delete, XK_Delete, KeyDel},
    {XK_Home, XK_Home, KeyHome},
    {XK_End, XK_End, KeyEnd},
    {XK_Page_Up, XK_Page_Up, KeyPageUp},
    {XK_Page_Down, XK_Page_Down, KeyPageDown},

    {XK_F1, XK_F12, KeyF1},
    {XK_Left, XK_Left, KeyLeft},
    {XK_Right, XK_Right, KeyRight},
    {XK_Up, XK_Up, KeyUp},
    {XK_Down, XK_Down, KeyDown},

    {XK_ISO_Left_Tab, XK_ISO_Left_Tab, '\t'},

    {XK_KP_Enter, XK_KP_Enter, KeyEnter},   // Num Pad Enter
    {XK_Num_Lock, XK_Num_Lock, 0},   // Num Lock (ignored)
    {XK_KP_Space, XK_KP_Divide, 0},   // Non numeric Num pad functions (ignored)

    {XK_Caps_Lock, XK_Caps_Lock, 0},   // ignore caps lock here

    {-1, -1, -1}
  };

bool EventPollKey (_Event *Event, XKeyEvent *XKeyEvent, int *ShiftState)
  {
    KeySym XKey;
    _TranslateTable *t;
    int Res;
    int Sh;
    char St [64];
    //
    XLookupString (XKeyEvent, St, sizeof (St), &XKey, NULL);
    //if (XKeyEvent->type == KeyPress) // ####
    //  DebugAddHex ("EventPollKey ", XKey); // ####
    Res = XKey;
    t = XKeyTable;
    while (true)
      {
        if (t->XK0 < 0)   // End of table
          {
            if (XKey >= 0x80)   // special printable character (UNICODE)
              Res = KeyUNICODE;
            break;
          }
        if (XKey >= t->XK0 && XKey <= t->XK1)
          {
            if (t->Key >= 0x100)   // shift key
              {
                if (XKeyEvent->type == KeyPress)
                  *ShiftState |= t->Key;
                else   // Up
                  *ShiftState &= ~t->Key;
                Res = 0;
                break;
              }
            if (t->Key == 0)
              Res = 0;
            else
              Res = t->Key + XKey - t->XK0;
            break;
          }
        t++;
      }
    if (Res == KeyUNICODE)
      Event->UTF = xkb_keysym_to_utf32 (XKey);
    else   // normal ASCII+ character.  Add/Apply Shift bits.
      {
        Sh = *ShiftState;
        if (Res)
          if (Res < 0x80)   // Normal character
            {
              if (Res >= 0x40)   // Alpha+ character
                {
                  if (Sh & KeyAlt)
                    Res = UpCase (Res);
                  else
                    {
                      if (Sh & KeyCntl)
                        Res &= 0x1F;
                      Sh = 0;//%%%%
                    }
                }
            }
        Res |= Sh;
      }
    Event->Key = Res;
    /*//####
    DebugAddHex ("Key:", Res);
    if (XKeyEvent->type == KeyPress)
      {
        char *s = St;
        StrCat (&s, "X: EventPoll:KeyPress keycode=");
        NumToHex (&s, XKeyEvent->keycode);
        StrCat (&s, " xk=");
        NumToHex (&s, XKey);
        StrCat (&s, " Key=");
        NumToHex (&s, Event->Key);
        //StrCat (&s, " Shift=");
        //NumToHex (&s, ShiftState);
        StrCat (&s, " UTF=");
        NumToHex (&s, Event->UTF);
        *s = 0;
        DebugAdd (St);
      }*/
    return true;
  }

bool EventPollClipboard (XEvent *e)
  {
    XSelectionRequestEvent request;
    XSelectionEvent sendEvent;
    XSelectionEvent selection;
    Atom possibleTargets [] = {aTARGETS, aUTF8, XA_STRING};
    Atom actualType;
    int actualFormat;
    unsigned char *data;
    unsigned long Count;
    //
    if (e->type == SelectionRequest)
      {
        //DebugAdd ("Selection Request"); //####
        request = e->xselectionrequest;
        // DebugAddS ("Selection Request", XGetAtomName (XDisplay, request.target)); free XGetAtomName result w XFree()
        if (XGetSelectionOwner (XDisplay, aCLIPBOARD) == WindowMain && request.property != None)
          {
            if (request.target == aTARGETS)
              XChangeProperty (request.display, request.requestor, request.property, XA_ATOM, 32, PropModeReplace,
                (unsigned char*) possibleTargets, SIZEARRAY (possibleTargets));
            else if (request.target == aUTF8 || request.target == XA_STRING)
              XChangeProperty (request.display, request.requestor, request.property,
                request.target, 8, PropModeReplace, (const byte *) ClipboardData, StrLen (ClipboardData));
            else   // unsupported Target
              request.property = None;
            // Send the Selection Notification
            sendEvent.type = SelectionNotify;
            sendEvent.serial = request.serial;
            sendEvent.send_event = request.send_event;
            sendEvent.display = request.display;
            sendEvent.requestor = request.requestor;
            sendEvent.selection = request.selection;
            sendEvent.target = request.target;
            sendEvent.property = request.property;
            sendEvent.time = request.time;
            XSendEvent (XDisplay, request.requestor, False, NoEventMask, (XEvent*) &sendEvent);
          }
      }
    else if (e->type == SelectionNotify)
      {
        DebugAdd ("Selection Notify"); // ####
        selection = e->xselection;
        //if (selection.property != None)
          {
            unsigned long int restail;
            if (XGetWindowProperty (XDisplay, ClipboardRequestWindow, aXSEL_DATA, 0, IntMax / 4, False, AnyPropertyType,
              &actualType, &actualFormat, &Count, &restail, &data) == Success)
              {
                if (actualType == aUTF8 || request.target == XA_STRING)
                  StrAssign (&ClipboardData, StrMake ((char *) data, Count));
                XFree (data);
              }
          }
      }
    else
      return false;
    return true;
	}

void EventPoll (_Event *Event)
  {
    static int ShiftState = 0;
    XEvent e;
    //
    Event->Type = etNone;
    while (XPending (XDisplay) > 0 && Event->Type == etNone)
      {
        e = {0};
        XNextEvent (XDisplay, &e);
        //DebugAddInt ("XEvent", e.type); //####
        Event->WindowID = (_WindowID) e.xany.window;   // same as XWindow
        //
        //DebugAddInt ("XEvent: ", e.type);
        if (e.type == FocusIn)  // ####
          ShiftState &= ~KeyAlt;   // Clear Alt State
          //DebugAdd ("FocusIn"); //####
        if (e.type == Expose)
          Event->Type = etWindowRedraw;
        else if (e.type == ConfigureNotify)
          {
            Event->Type = etWindowResize;
            Event->X = e.xconfigure.width;
            Event->Y = e.xconfigure.height;
            //DebugAddP ("Win Resize", {Event->X, Event->Y}); //####
          }
        else if (e.type == ClientMessage)   // Close Window?
          {
            if (e.xclient.data.l [0] == aDeleteWindow)
              Event->Type = etWindowClose;
          }
        else if (!EventPollClipboard (&e))
          if (e.type == KeyPress || e.type == KeyRelease)
            {
              EventPollKey (Event, &e.xkey, &ShiftState);
              // Alternative method
              //XLookupString (&e.xkey, St, sizeof (St), &ks, NULL);
              //Event->Key = KeyEventToKey (&Event, &e.type == KeyPress, ks, &ShiftState);
              //Event->UTF = xkb_keysym_to_utf32 (ks); // xkb_keysym_to_utf32 (ks);
              //ks = e.xkey.keycode;
              //Len = XkbTranslateKeySym (XDisplay, &ks, 0, s, sizeof (s), NULL);
              //Len = XmbLookupString (0, &e.xkey, s, sizeof (s), &ks, NULL);
              /*s [Len] = 0;
              if (e.type == KeyPress)  // When Shift/Alt/Cntrl pressed, Len == 0
                {
                  DebugAddInt ("X: EventPoll: XLookupString Len", Len);
                  DebugAddS ("X: EventPoll: XLookupString Buffer", s);
                  DebugAddHex ("X: EventPoll: XLookupString KeySym", ks);
                }*/
              //Event->Key = LookupKeyCode (e.type == KeyPress, e.xkey.keycode, &ShiftState);
              if (e.type == KeyPress)
                Event->Type = etKeyDown;
              else
                Event->Type = etKeyUp;
            }
          else if (e.type == ButtonPress || e.type == ButtonRelease)
            {
              if (e.type == ButtonPress)
                Event->Type = etMouseDown;
              else
                Event->Type = etMouseUp;
              Event->Key = e.xbutton.button;
              Event->X = e.xbutton.x;
              Event->Y = e.xbutton.y;
              //DebugAddInt ("Mouse Button", Event->Key); // ####
            }
          else if (e.type == MotionNotify)
            {
              Event->Type = etMouseMove;
              Event->X = e.xmotion.x;
              Event->Y = e.xmotion.y;
              //DebugAddP ("Mouse: ", {Event->X, Event->Y}); // only gives points in a window
            }
      }
    Event->ShiftState = ShiftState;
  }

bool WindowIDMatch (_WindowID WindowID, _Window *Window_)
  {
    return ((Window) WindowID) == ((__Window *) Window_)->XWindow;
  }

////////////////////////////////////////////////////////////////////////////
//
// Clipboard

void ProcessClipboardEvents (void)
  {
    int Tick;
    XEvent e;
    //
    Tick = ClockMS ();
    while (true)
      {
        if (ClockMS () - Tick > 10)
          break;
        if (XPending (XDisplay) > 0)
          {
            e = {0};
            XNextEvent (XDisplay, &e);
            if (EventPollClipboard (&e))
              Tick = ClockMS ();
          }
      }
  }

/*void ClipboardFilename (char *Filename)
  {
    StrPathHome (&Filename, ".clipboard");
    *Filename = 0;
  }*/

bool ClipboardGet (_Window *Window_)   // Reads / returns Clipboard
  {
    __Window *Window;
    //
    Window = (__Window *) Window_;
    ClipboardRequestWindow = Window->XWindow;
    XConvertSelection (XDisplay, aCLIPBOARD, aUTF8, aXSEL_DATA, Window->XWindow, CurrentTime);
    ProcessClipboardEvents ();
    return ClipboardData != NULL;
    /*char Filename [MaxPath];
    char *Res;
    int f;
    int sz;
    //
    ClipboardFilename (Filename);
    f = FileOpen (Filename, foRead);
    if (f)
      {
        sz = FileSize (f);
        Res = (char *) malloc (sz + 1);
        FileRead (f, (byte *) Res, sz);
        Res [sz] = 0;
        FileClose (f);
        return Res;
      }
    return nullptr;*/
  }

bool ClipboardSet (_Window *Window_, char *Text)
  {
    StrAssignCopy (&ClipboardData, Text);
    XSetSelectionOwner (XDisplay, aCLIPBOARD, WindowMain, CurrentTime);
    ProcessClipboardEvents ();
    return true;
    /*Tick = ClockMS ();
    while (true)
      {
        if (XPending (XDisplay) > 0)
          {
            e = {0};
            XNextEvent (XDisplay, &e);
            if (EventPollClipboard (&e, &Event))
              Tick = ClockMS ();
          }
        if (ClockMS () - Tick > 10)
          break;
      }
    return true;
    /*
    char Filename [MaxPath];
    int f;
    int sz;
    //
    ClipboardFilename (Filename);
    f = FileOpen (Filename, foWrite);
    if (f)
      {
        sz = StrLen (Text);
        FileWrite (f, (byte *) Text, sz);
        FileClose (f);
        return true;
      }
    return false;
    */
  }


////////////////////////////////////////////////////////////////////////////
//
// Bitmap Functions

_Bitmap *BitmapCreate (_Window *Window, int Width, int Height)
  {
    __Window *Window_;
    __Bitmap *Bitmap;
    //
    Window_ = (__Window *) Window;
    Bitmap = (__Bitmap *) malloc (sizeof (__Bitmap));
    Bitmap->Width = Width;
    Bitmap->Height = Height;
    Bitmap->ImageDataSize = (Width + 1) * (Height + 1) * 4 + 8;
    Bitmap->ImageData = (int *) malloc (Bitmap->ImageDataSize);
    Bitmap->Image = XCreateImage (XDisplay, Window_->XVisual, Window_->XDepth, ZPixmap, 0, (char *) Bitmap->ImageData, Width, Height, 32, 0);
    Bitmap->Alpha = (byte *) malloc (Width * Height);
    return (_Bitmap *) Bitmap;
  }

void BitmapDestroy (_Bitmap *Bitmap_)
  {
    __Bitmap *Bitmap;
    //
    if (Bitmap_)
      {
        Bitmap = (__Bitmap *) Bitmap_;
        if (Bitmap->Image)
          XDestroyImage (Bitmap->Image);
        if (Bitmap->Alpha)
          free (Bitmap->Alpha);
        free (Bitmap_);
      }
  }

bool BitmapGetSize (_Bitmap *Bitmap_, int *Width, int *Height)
  {
    __Bitmap *Bitmap;
    //
    if (Bitmap_)
      {
        Bitmap = (__Bitmap *) Bitmap_;
        if (Width)
          *Width = Bitmap->Width;
        if (Height)
          *Height = Bitmap->Height;
        return true;
      }
    return false;
  }

int BitmapGetPixel (_Bitmap *Bitmap_, int x, int y)
  {
    __Bitmap *Bitmap;
    //
    Bitmap = (__Bitmap *) Bitmap_;
    if (Bitmap)
      {
        if (x >= 0 && y >= 0 && x < Bitmap->Width && y < Bitmap->Height)
          return XColourToColour (XGetPixel (Bitmap->Image, x, y) | (Bitmap->Alpha [y * Bitmap->Width + x] << 24));
        //DebugAddP ("BitmapGetPixel invalid", {x, y});
      }
    return 0;
  }

bool BitmapSetPixel (_Bitmap *Bitmap_, int x, int y, int Colour)
  {
    __Bitmap *Bitmap;
    _Colour c;
    //
    Bitmap = (__Bitmap *) Bitmap_;
    if (Bitmap)
      if (x >= 0 && y >= 0 && x < Bitmap->Width && y < Bitmap->Height)
        {
          XPutPixel (Bitmap->Image, x, y, ColourToXColour (Colour) & 0xFFFFFF);
          c.RGBA =  Colour;
          Bitmap->Alpha [y * Bitmap->Width + x] = c.Prime [3];
          return true;
        }
      //else
      //  DebugAddP ("BitmapSetPixel invalid", {x, y});
    return false;
  }

void BitmapClear (_Bitmap *Bitmap_)
  {
    __Bitmap *Bitmap;
    //
    Bitmap = (__Bitmap *) Bitmap_;
    if (Bitmap)
      if (Bitmap->Alpha)
        memset (Bitmap->Alpha, 0, Bitmap->Width * Bitmap->Height);
  }


////////////////////////////////////////////////////////////////////////////
//
// Start Thread

#include <pthread.h>

_ThreadFunction ThreadFunction_;

void* ThreadWrapper (void *Params)
  {
    ThreadFunction_ (Params);
    return NULL;
  }


bool StartThread (_ThreadFunction ThreadFunction, void *Params)
  {
    pthread_t Thread;
    int err;
    char St [64], *s;
    //
    Thread = 0;
    ThreadFunction_ = ThreadFunction;
    err = pthread_create (&Thread, NULL /*&ThreadAttr*/, ThreadWrapper, Params);
    s = St;
    if (err)
      {
        StrCat (&s, "** StartThread ERROR ");
        NumToStr (&s, err);
      }
    else
      {
        StrCat (&s, "New Thread 0x");
        NumToHex (&s, Thread);
      }
    *s = 0;
    DebugAdd (St);
    pthread_detach (Thread);   // This frees up remaining resources for Thread
    return err == 0;
  }


////////////////////////////////////////////////////////////////////////////
//
// Main Programme

int main (int argc, char* argv [])
  {
    PathStart = GetCurrentPath ();
    char *p = StrFindFilename (argv [0]);
    PathExe = StrMake (argv [0], p - argv [0]);
    int Res = main_ (argc, argv);
    free (PathStart);
    free (PathExe);
    return Res;
  }


////////////////////////////////////////////////////////////////////////////
//
// Sound. Thanks to LinuxMint dropping Beep support

#define BeepASLA

#ifdef BeepASLA

#include <alsa/asoundlib.h>

void DebugAddErr (int Error)
  {
    if (Error)
      //DebugAddInt ("ALSA Error", Error);
      DebugAddS ("ALSA Error", snd_strerror (Error));
  }

const int SampleRate = 44100;

int SoundPlaying = 0;

typedef struct
  {
    snd_pcm_t *handle;
    short *Buff;
    int BuffSize;
    bool Free;
  } _Wave;

_Wave Waves [16] = {0};

int SoundPlayThread (void *Param)
  {
    _Wave *Wave;
    int Size, err;
    ////////////////////////////////////////////
    SoundPlaying++;
    Wave = (_Wave *) Param;
    // Open PCM device for playback
    err = snd_pcm_open (&Wave->handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    DebugAddErr (err);
    // Set Parameters
    err = snd_pcm_set_params (Wave->handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, SampleRate, 1, 50000);
    DebugAddErr (err);
    Size = snd_pcm_writei (Wave->handle, Wave->Buff, Wave->BuffSize / 2);
    DebugAddInt ("PCM Write Size", Size);
    err = snd_pcm_drain (Wave->handle);
    DebugAddErr (err);
    err = snd_pcm_close (Wave->handle);
    DebugAddErr (err);
    if (Wave->Free)
      free (Wave->Buff);   // Free buffer memeory
    Wave->Buff = NULL;
    SoundPlaying--;
    return 0;
  }

bool SoundPlay (short *Buffer, int BufferSize, bool Free)
  {
    int i;
    //
    i = 0;
    while (true)
      {
        if (i == 16)
          break;
        if (Waves [i].Buff == NULL)
          {
            Waves [i].Buff = Buffer;
            Waves [i].BuffSize = BufferSize;
            Waves [i].Free = Free;
            StartThread (SoundPlayThread, &Waves [i]);
            DebugAddInt ("Play Wave", i); //####
            return true;
          }
        i++;
      }
    if (Free)
      free (Buffer);
    return false;
  }

void SoundStop (void)
  {
    int i;
    ////////////////////////////////////////////
    i = 0;
    while (i < 16)
      {
        if (Waves [i].Buff)
          {
            snd_pcm_drop (Waves [i].handle);
            free (Waves [i].Buff);   // Free buffer memeory
            Waves [i].Buff = NULL;
          }
        i++;
      }
    SoundPlaying = 0;
  }

/*bool PlaySound (byte *Buffer, int BufferSize)
  {
    int err;
    snd_pcm_t *handle;
    // Open PCM device for playback
    err = snd_pcm_open (&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0)
      return false;
    // Set Parameters
    err = snd_pcm_set_params (handle, SND_PCM_FORMAT_U8, SND_PCM_ACCESS_RW_INTERLEAVED, 1, SampleRate, 1, 5000);
    if (err < 0)
      return false;
    //
    snd_pcm_writei (handle, Buffer, BufferSize);
    snd_pcm_drain (handle);
    snd_pcm_close (handle);
    return true;
  }

void SoundPlay (void *handle_, short *Buffer, int BufferSize)
  {
    snd_pcm_t *handle;
    int Size;
    int err;
    //
    handle = (snd_pcm_t *) handle_;
    Size = snd_pcm_writei (handle, Buffer, BufferSize / 2);
    DebugAddInt ("PCM Write Size", Size);
    err = snd_pcm_drain (handle);
    DebugAddErr (err);
    err = snd_pcm_close (handle);
    DebugAddErr (err);
  }

void SoundStop (void *handle_)
  {
    snd_pcm_t *handle;
    //
    handle = (snd_pcm_t *) handle_;
    snd_pcm_drop (handle);
  }*/

short BeepRaw [2 * SampleRate / 8];   // Buffer for 1/8 second of sound in stereo
bool BeepRawFilled = false;

void BeepRawFill (int Freq)
  {
    int p, f, i, j, k;
    //
    p = 2 * SampleRate / Freq;   // Period: samples per cycle
    // Fill BeepRaw with a sawtooth wave with a sawtooth envelope
    for (i = 0; i < SIZEARRAY (BeepRaw); i++)
      BeepRaw [i] = ((0x4000 * (i % p)) / p) * (SIZEARRAY (BeepRaw) - i) / SIZEARRAY (BeepRaw);
    // Filter: average 8 samples
    f = 32;
    for (i = 0; i < SIZEARRAY (BeepRaw); i++)
      {
        k = 0;
        for (j = 0; j < f; j++)
          if (i + j < SIZEARRAY (BeepRaw))
            k += BeepRaw [i + j];
        BeepRaw [i] = k / f;
      }
  }

void Beep (void)
  {
    if (!BeepRawFilled)
      BeepRawFill (440 * 4);   // A + 2 octaves
    BeepRawFilled = true;
    if (SoundPlaying < 4)
      SoundPlay (BeepRaw, sizeof (BeepRaw) / 2, false);
  }

#else

void Beep (void)
  {
    XBell (XDisplay, 100);   // no good on Linux Mint
  }

#endif
