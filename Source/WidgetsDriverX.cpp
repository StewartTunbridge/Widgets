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

#include "WidgetsDriver.hpp"

extern void DebugAdd (const char *St);   // defined in application
extern void DebugAddS (const char *s1, const char *s2);   // defined in Widgets.cpp
extern void DebugAdd (const char *St, int n);
extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddP (const char *St, _Point p);

extern bool RenderBitmapTransparentTL (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest);

#define TexturesEnabled

/*
GC gcCopy;
GC gcOr;
GC gcAnd;
GC gcAndInverted;
*/

typedef struct
  {
    int Width, Height;
    Pixmap Image;
    Pixmap Mask;
    //@@@@GC gc;
    //@@@@GC gcMask;
  } __Texture;

typedef struct
  {
    //Display *XDisplay;
    int XScreen;
    int XDepth;
    Visual *XVisual;
    Window XWindow;
    GC WinGCCopy;
    GC WinGCOr;
    GC WinGCAnd;
    GC WinGCAndInvert;
    //
    byte WindowAttributes;
    int SizeX, SizeY;
    //Pixmap Icon;
    __Texture *tIcon;
    Pixmap pIcon;
  } __Window;

bool ForceSoftwareRendering = false;

Display *XDisplay = NULL;
Window XWindow = 0;


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
// COLOUR TRANSLATION

int RGB (byte R, byte G, byte B)
  {
    return B << 16 | G << 8 | R;
  }

int ColourToXColour (int Colour)//, byte Alpha)
  {
    //return ColourR (Colour) << 16 | ColourG (Colour) << 8 | ColourB (Colour) | Alpha << 24;
    return ColourR (Colour) << 16 | ColourG (Colour) << 8 | ColourB (Colour);
  }

int XColourToColour (int XColour)
  {
    return ((XColour & 0xFF0000) >> 16) | (XColour & 0x00FF00) | ((XColour & 0x0000FF) << 16);
  }


/////////////////////////////////////////////////////////////////////////
//
// KEYCODE TRANSLATION

typedef struct
  {
    int XCode;
    byte Key;
  } _XCodeToKey;

const _XCodeToKey XCodeToKeys [] =
  {
    {XK_Escape, esc},
    {XK_Return, cr},
    {XK_BackSpace, bs},
    {XK_Tab, tab},
    //
    {XK_Up, KeyUp},
    {XK_Down, KeyDown},
    {XK_Left, KeyLeft},
    {XK_Right, KeyRight},
    {XK_Insert, KeyIns},
    {XK_Delete, KeyDel},
    {XK_Home, KeyHome},
    {XK_End, KeyEnd},
    {XK_Page_Up, KeyPageUp},
    {XK_Page_Down, KeyPageDown},
    {XK_F1, KeyF1+0},
    {XK_F2, KeyF1+1},
    {XK_F3, KeyF1+2},
    {XK_F4, KeyF1+3},
    {XK_F5, KeyF1+4},
    {XK_F6, KeyF1+5},
    {XK_F7, KeyF1+6},
    {XK_F8, KeyF1+7},
    {XK_F9, KeyF1+8},
    {XK_F10, KeyF1+9},
    {XK_F11, KeyF1+10},
    {XK_F12, KeyF1+11}
  };

int XkeysymToChar (int keysym)
  {
    int Res;
    int i;
    //
    i = 0;
    Res = -1;
    while (true)
      {
        if (i >= SIZEARRAY (XCodeToKeys))
          {
            Res = keysym;
            //if ((keysym & 0xFF00) == 0xFF00)
              Res = keysym & 0x00FF;
            break;
          }
        if (XCodeToKeys [i].XCode == keysym)
          {
            Res = XCodeToKeys [i].Key;
            break;
          }
        i++;
      }
    return Res;
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
    //return {50, 50}; //####
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int mask;
    //XQueryPointer (XDisplay, DefaultRootWindow (XDisplay), &root, &child, &rootX, &rootY, &winX, &winY, &mask);
    XQueryPointer (XDisplay, XWindow, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
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
	  // slows things if true XSynchronize (XDisplay, false);   // make operations asynchronous
	  return true;
  }

bool WidgetsUninit (void)
  {
    if (XDisplay)
      XCloseDisplay (XDisplay);
    return true;
  }

void SetGCFunction (__Window *Window, GC gc, int Function)
  {
    XGCValues gcv;
    //
    //XGetGCValues (XDisplay, gc, GCFunction, &gcv);
    gcv.function = Function;
    XChangeGC (XDisplay, gc, GCFunction, &gcv);
  }

// Copy pixmap area using a mask to a Drawable
// Mask is Black & White. Areas in Source corresponding White areas in Mask are drawn on Dest
bool CopyArea (__Window *Window, Drawable Source, Drawable Dest, Drawable SourceMask, Drawable DestMask, int x, int y, int Width, int Height, int DestX, int DestY)
  {
    Pixmap Mask_;
    __Window * Window_;
    //
    Window_ = Window;
    if (SourceMask == 0)
      {
        XCopyArea (XDisplay, Source, Dest, Window_->WinGCCopy, x, y, Width, Height, DestX, DestY);
        if (DestMask)
          {
            XSetForeground (XDisplay, Window_->WinGCCopy, 0x00FFFFFF);
            XFillRectangle (XDisplay, DestMask, Window_->WinGCCopy, DestX, DestY, Width, Height);
          }
      }
    else   // #### Try XSetClipMask instead of the following logic, may be faster
      {
        Mask_ = XCreatePixmap (XDisplay, Dest, Width, Height, Window->XDepth);
        // Clear effected pixels in Dest
        //@@@@SetGCFunction (Window, gc, GXandInverted);
        XCopyArea (XDisplay, SourceMask, Dest, Window_->WinGCAndInvert, x, y, Width, Height, DestX, DestY);
        // Generate coloured Mask to OR into Dest
        //@@@@SetGCFunction (Window, gc, GXcopy);
        XCopyArea (XDisplay, SourceMask, Mask_, Window_->WinGCCopy, x, y, Width, Height, 0, 0);
        //@@@@SetGCFunction (Window, gc, GXand);
        XCopyArea (XDisplay, Source, Mask_, Window_->WinGCAnd, x, y, Width, Height, 0, 0);
        // Copy new coloured Mask into Dest
        //@@@@SetGCFunction (Window, gc, GXor);
        XCopyArea (XDisplay, Mask_, Dest, Window_->WinGCOr, 0, 0, Width, Height, DestX, DestY);
        // Modify DestMask
        if (DestMask)
          XCopyArea (XDisplay, SourceMask, DestMask, Window_->WinGCOr, 0, 0, Width, Height, DestX, DestY);
        // Restore gc & free new Mask
        //@@@@SetGCFunction (Window, gc, GXcopy);
        XFreePixmap (XDisplay, Mask_);
      }
    return true;
  }

void ApplyAlpha (_Window *Window, Drawable Image, int Width, int Height, byte Alpha)
  {
    __Window *Window_;
    int Alpha_;
    //
    Window_ = (__Window *) Window;
    //
    Alpha_ = Alpha | Alpha << 8 | Alpha << 16 | Alpha << 24;
    if (Alpha == 0xFF)
      XDeleteProperty (XDisplay, Window_->XWindow, XInternAtom (XDisplay, "_NET_WM_WINDOW_OPACITY", 0));
    else
      XChangeProperty (XDisplay, Window_->XWindow, XInternAtom (XDisplay, "_NET_WM_WINDOW_OPACITY", 0),
                       XA_CARDINAL, 32, PropModeReplace, (byte *) &Alpha_, 1);
  }


/*
//////////////////////////////////////////////////////////////////////////////
// ICON

#define icon_width 16
#define icon_height 16

const byte icon_bits [] =
  {
    0xff, 0xff, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80,
    0xc1, 0x83, 0x41, 0x82, 0x41, 0x82, 0xc1, 0x83, 0x01, 0x80, 0x01, 0x80,
    0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0xff, 0xff
  };
*/


//////////////////////////////////////////////////////////////////////////////
// WINDOW

XVisualInfo vinfo;
XSetWindowAttributes attr;

_Window *WindowCreate (char *Title, int x, int y, int SizeX_, int SizeY_, byte WindowAttributes)
  {
    __Window *Res;
    bool OK;
    //XSizeHints *sh;
    int Width, Height;
    Atom window_type;
    long value;
    //
    OK = false;
    Res = (__Window *) malloc (sizeof (__Window));
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
        Res->XWindow = XCreateSimpleWindow (XDisplay, RootWindow (XDisplay, Res->XScreen), x, y, SizeX_, SizeY_, 0, BlackPixel (XDisplay, Res->XScreen), WhitePixel (XDisplay, Res->XScreen));
        if (Res->XWindow)
          {
            if (XWindow == 0)
              XWindow = Res->XWindow;   // keep for MousePos ()
            XMapWindow (XDisplay, Res->XWindow);   // needed for XMoveWindow ()
            XMoveWindow (XDisplay, Res->XWindow, x, y);
            Res->SizeX = SizeX_;
            Res->SizeY = SizeY_;
            Res->WinGCCopy = XCreateGC (XDisplay, Res->XWindow, 0, NULL);
            SetGCFunction (Res, Res->WinGCCopy, GXcopy);
            Res->WinGCOr = XCreateGC (XDisplay, Res->XWindow, 0, NULL);
            SetGCFunction (Res, Res->WinGCOr, GXor);
            Res->WinGCAnd = XCreateGC (XDisplay, Res->XWindow, 0, NULL);
            SetGCFunction (Res, Res->WinGCAnd, GXand);
            Res->WinGCAndInvert = XCreateGC (XDisplay, Res->XWindow, 0, NULL);
            SetGCFunction (Res, Res->WinGCAndInvert, GXandInverted);
            Res->tIcon = NULL;
            Res->pIcon = 0;
            //
            // Trap window close messages
            Atom wm_delete_window = XInternAtom (XDisplay, "WM_DELETE_WINDOW", False);
            XSetWMProtocols (XDisplay, Res->XWindow, &wm_delete_window, 1);
            //
            if (~WindowAttributes & waResizable)
              {
                XSizeHints *size_hints = XAllocSizeHints ();
                size_hints->flags = PMinSize | PMaxSize;
                size_hints->min_width = size_hints->max_width = SizeX_;
                size_hints->min_height = size_hints->max_height = SizeY_;
                XSetWMNormalHints (XDisplay, Res->XWindow, size_hints);
                XFree (size_hints);
                //attr.override_redirect = 1; BLOCKS EVERY WINDOW MANAGER TASK
                //XChangeWindowAttributes (Res->XDisplay, Res->XWindow, CWOverrideRedirect, &attr);
                window_type = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE", false);
                value = XInternAtom (XDisplay, "_NET_WM_WINDOW_TYPE_MENU", false);
                XChangeProperty (XDisplay, Res->XWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
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
              PointerMotionMask | //SubstructureNotifyMask | //####
              StructureNotifyMask);   // Specify what Events we get
            //XMapWindow (XDisplay, Res->XWindow);   // Display Window
            if (Title)
              XStoreName (XDisplay, Res->XWindow, Title);   // Set Title Name
            //XSetInputFocus (Res->XDisplay, Res->XWindow, RevertToNone, CurrentTime);
            XFlush (XDisplay);
            XRaiseWindow (XDisplay, Res->XWindow);
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

bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap)
  {
    __Window *Window_;
    XImage *Img;
    XWMHints *wm_hints;
    //
    Window_ = (__Window *) Window;
    if (Window_->tIcon)
      TextureDestroy (Window, Window_->tIcon);
    if (Window_->pIcon)
      XFreePixmap (XDisplay, Window_->pIcon);
    Window_->tIcon = NULL;
    Window_->pIcon = 0;
    if (Bitmap)
      {
        wm_hints = XAllocWMHints ();
        Img = (XImage *) Bitmap;
        Window_->tIcon = (__Texture *) TextureCreate (Window, Img->width, Img->height);
        if (Window_->tIcon)
          {
            RenderTarget (Window, Window_->tIcon, 0, {0, 0, Img->width, Img->height});
            RenderFillTransparent (Window);
            RenderBitmapTransparentTL (Window, Bitmap, {0, 0, Img->width, Img->height}, {0, 0, Img->width, Img->height});
            wm_hints->icon_pixmap = Window_->tIcon->Image;
            wm_hints->icon_mask = Window_->tIcon->Mask;
            wm_hints->flags = IconPixmapHint | IconMaskHint;
          }
        else
          {
            Window_->pIcon = XCreatePixmap (XDisplay, Window_->XWindow, Img->width, Img->height, Window_->XDepth);
            XPutImage (XDisplay, Window_->pIcon, Window_->WinGCCopy, Img, 0, 0, 0, 0, Img->width, Img->height);
            wm_hints->icon_pixmap = Window_->pIcon;
            wm_hints->flags = IconPixmapHint;  // StateHint | IconPixmapHint | InputHint;
          }
        XSetWMHints (XDisplay, Window_->XWindow, wm_hints);
        XFree (wm_hints);
      }
    return true;
  }

bool WindowSetPosSize (_Window *Window, int x, int y, int Width, int Height)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (x >= 0 && y >= 0)
      XMoveWindow (XDisplay, Window_->XWindow, x, y);
    if (Width >= 0 && Height >= 0)
      XResizeWindow (XDisplay, Window_->XWindow, Width, Height);
    //XMoveResizeWindow (Window_->XDisplay, Window_->XWindow, x, y, Width, Height);
    //XResizeWindow (XDisplay, Window_, Width, Height);
    return true;
  }

void WindowGetPosSize (_Window *Window, int *x, int *y, int *Width, int *Height)
  {
    __Window *Window_;
    XWindowAttributes  wa;
    //
    Window_ = (__Window *) Window;
    XGetWindowAttributes (XDisplay, Window_->XWindow, &wa);
    if (x)
      *x = wa.x;
    if (y)
      *y = wa.y;
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
    if (Window_->tIcon)
      TextureDestroy (XDisplay, Window_->tIcon);
    if (Window_->pIcon)
      XFreePixmap (XDisplay, Window_->pIcon);
    if (MouseCursorPixmap)
      XFreePixmap (XDisplay, MouseCursorPixmap);
    XFreeGC (XDisplay, Window_->WinGCCopy);
    XFreeGC (XDisplay, Window_->WinGCOr);
    XFreeGC (XDisplay, Window_->WinGCAnd);
    XFreeGC (XDisplay, Window_->WinGCAndInvert);
    XDestroyWindow (XDisplay, Window_->XWindow);
    //if (Window_->XDisplay)
    //  XCloseDisplay (Window_->XDisplay);
    free (Window);
  }


////////////////////////////////////////////////////////////////////////////
//
// RENDER FUNCTIONS

__Texture *RenderTargetTexture;
__Texture WindowTexture;

int TargetColour;
int TargetColourBgnd;
XRectangle XTargetClip;

bool RenderTarget (_Window *Window, _Texture *Texture, int Colour, _Rect Clip)   // Specify where to draw
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (Texture && ((__Texture *) Texture)->Image)
      RenderTargetTexture = (__Texture *) Texture;
    else
      {
        //WindowTexture.Width = 0;
        //WindowTexture.Height = 0;
        WindowTexture.Image = Window_->XWindow;
        WindowTexture.Mask = 0;
        //@@@@WindowTexture.gc = Window_->WinGC; //DefaultGC (Window_->XDisplay, Window_->XScreen);
        //WindowTexture.gc = DefaultGC (Window_->XDisplay, Window_->XScreen);
        //@@@@WindowTexture.gcMask = NULL;
        RenderTargetTexture = &WindowTexture;
      }
    TargetColour = -1;
    TargetColourBgnd = Colour;
    if (Texture == NULL)
      {
        XTargetClip.x = Clip.x;
        XTargetClip.y = Clip.y;
        XTargetClip.width = Clip.Width;
        XTargetClip.height = Clip.Height;
        //@@@@XSetClipRectangles (XDisplay, RenderTargetTexture->gc, 0, 0, &XTargetClip, 1, Unsorted);
        XSetClipRectangles (XDisplay, Window_->WinGCCopy, 0, 0, &XTargetClip, 1, Unsorted);
        //XSetClipRectangles (Window_->XDisplay, RenderTargetTexture->gc, Clip.x, Clip.y, &XTargetClip, 1, Unsorted);
      }
    return true;
  }

void RenderPresent (_Window *Window, byte Alpha)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (Alpha != 0xFF)
      ApplyAlpha (Window, Window_->XWindow, Window_->SizeX, Window_->SizeY, Alpha);
  }

void SetRenderDrawColor (_Window *Window, int Colour)//, byte Alpha = 0xFF)
  {
    __Window *Window_;
    int Col;
    //
    Window_ = (__Window *) Window;
    Col = ColourToXColour (Colour);//, Alpha);
    if (Col != TargetColour)
      {
        //@@@@XSetForeground (XDisplay, RenderTargetTexture->gc, Col);
        XSetForeground (XDisplay, Window_->WinGCCopy, Col);
        TargetColour = Col;
      }
  }

void RenderDrawPoint (_Window *Window, int Colour, int x, int y)//, byte Alpha = 0xFF)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    SetRenderDrawColor (Window, Colour);//, Alpha);
    //@@@@XDrawPoint (XDisplay, RenderTargetTexture->Image, RenderTargetTexture->gc, x, y);
    XDrawPoint (XDisplay, RenderTargetTexture->Image, Window_->WinGCCopy, x, y);
    if (RenderTargetTexture->Mask)
      //@@@@XDrawPoint (XDisplay, RenderTargetTexture->Mask, RenderTargetTexture->gcMask, x, y);
      {
        XSetForeground (XDisplay, Window_->WinGCOr, 0xFFFFFF);
        XDrawPoint (XDisplay, RenderTargetTexture->Mask, Window_->WinGCOr, x, y);
      }
  }

void RenderDrawLine (_Window *Window, int Colour, int x1, int y1, int x2, int y2)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (x1 == x2 && y1 == y2)
      RenderDrawPoint (Window, Colour, x1, y1);//, 0xFF);
    else
      {
        SetRenderDrawColor (Window, Colour);
        //@@@@XDrawLine (XDisplay, RenderTargetTexture->Image, RenderTargetTexture->gc, x1, y1, x2, y2);
        XDrawLine (XDisplay, RenderTargetTexture->Image, Window_->WinGCCopy, x1, y1, x2, y2);
        if (RenderTargetTexture->Mask)
          //@@@@XDrawLine (XDisplay, RenderTargetTexture->Mask, RenderTargetTexture->gcMask, x1, y1, x2, y2);
          {
            XSetForeground (XDisplay, Window_->WinGCOr, 0xFFFFFF);
            XDrawLine (XDisplay, RenderTargetTexture->Mask, Window_->WinGCOr, x1, y1, x2, y2);
          }
      }
  }

void RenderFillRect (_Window *Window, _Rect Rect, int Colour)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    SetRenderDrawColor (Window, Colour);
    //@@@@XFillRectangle (XDisplay, RenderTargetTexture->Image, RenderTargetTexture->gc, Rect.x, Rect.y, Rect.Width, Rect.Height);
    XFillRectangle (XDisplay, RenderTargetTexture->Image, Window_->WinGCCopy, Rect.x, Rect.y, Rect.Width, Rect.Height);
    if (RenderTargetTexture->Mask)
      //@@@@XFillRectangle (XDisplay, RenderTargetTexture->Mask, RenderTargetTexture->gcMask, Rect.x, Rect.y, Rect.Width, Rect.Height);
      XFillRectangle (XDisplay, RenderTargetTexture->Mask, Window_->WinGCOr, Rect.x, Rect.y, Rect.Width, Rect.Height);
  }

void RenderFillTransparent (_Window *Window)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    if (RenderTargetTexture->Mask == 0)   // no Mask so make one
      RenderTargetTexture->Mask = XCreatePixmap (XDisplay, Window_->XWindow, RenderTargetTexture->Width, RenderTargetTexture->Height, Window_->XDepth);
    //@@@@if (RenderTargetTexture->gcMask == NULL)   // no GC so make one
    //@@  RenderTargetTexture->gcMask = XCreateGC (XDisplay, RenderTargetTexture->Mask, 0, NULL);
    // Reset the Mask to Black, but ready for White
    //@@@@XSetForeground (XDisplay, RenderTargetTexture->gcMask, 0x000000);   // Fill Mask with Black
    XSetForeground (XDisplay, Window_->WinGCCopy, 0x000000);   // Fill Mask with Black
    //@@@@XFillRectangle (XDisplay, RenderTargetTexture->Mask, RenderTargetTexture->gcMask, 0, 0, RenderTargetTexture->Width, RenderTargetTexture->Height);
    XFillRectangle (XDisplay, RenderTargetTexture->Mask, Window_->WinGCCopy, 0, 0, RenderTargetTexture->Width, RenderTargetTexture->Height);
    //@@@@XSetForeground (XDisplay, RenderTargetTexture->gcMask, 0xFFFFFFFF);   // Restore White
    //@@@@XSetForeground (XDisplay, Window_->WinGCMask, 0xFFFFFFFF);   // Restore White
  }

void EventPoll (_Event *Event)
  {
    //__Window *Window_;
    XEvent e;
    //int Len;
    KeySym ks;
    char s [16];
    static int ShiftState = 0;
    int ShiftState_;
    //
    Event->Type = etNone;
    while (XPending (XDisplay) > 0 && Event->Type == etNone)
      {
        XNextEvent (XDisplay, &e);
        Event->WindowID = (_WindowID) e.xany.window;   // same as XWindow
        //
        //#define DebugShowXEvents
        #ifdef DebugShowXEvents
        DebugAdd ("XEvent: ", e.type);
        #endif // DebugShowXEvents
        if (e.type == Expose)
          Event->Type = etWindowRedraw;
        else if (e.type == ConfigureNotify)
          {
            Event->Type = etWindowResize;
            Event->X = e.xconfigure.width;
            Event->Y = e.xconfigure.height;
          }
        else if (e.type == ClientMessage)   // Close Window?
          Event->Type = etWindowClose;
        else if (e.type == KeyPress || e.type == KeyRelease)
          {
            /*Len =*/ XLookupString (&e.xkey, s, sizeof (s), &ks, NULL);
            //if (Len != 1)  When Shift/Alt/Cntrl pressed, Len == 0
            //  DebugAdd ("X: EventPoll: Len != 1");
            ShiftState_ = 0;
            if (ks == XK_Shift_L || ks == XK_Shift_R)
              ShiftState_ = KeyShift;
            else if (ks == XK_Control_L || ks == XK_Control_R)
              ShiftState_ = KeyCntl;
            else if (ks == XK_Alt_L || ks == XK_Alt_R)
              ShiftState_ = KeyAlt;
            else if (ks == XK_Meta_L || ks == XK_Meta_R)
              ShiftState_ = KeyShift | KeyAlt;
            if (ShiftState_)
              {
                Event->Key = ShiftState ^ ShiftState_;
                if (e.type == KeyPress)
                  ShiftState |= ShiftState_;
                else
                  ShiftState &= ~ShiftState_;
              }
            else
              {
                Event->Key = XkeysymToChar (ks);
                if (Event->Key >= 0x80)
                  Event->Key |= ShiftState;
                else
                  {
                    if (Event->Key >= '@')   // "Alpha" keys
                      if (ShiftState & KeyAlt)   // Alt alpha keys are always uppercase
                        Event->Key = (Event->Key & ~0x20) | ShiftState;
                      else if (ShiftState & KeyCntl)   // Control alphas
                        Event->Key &= 0x1F;
                      else if (ShiftState & KeyShift)
                        Event->Key &= ~0x20;
                    else
                      Event->Key |= ShiftState;
                  }
              }
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
// Texture Functions

bool RenderTexture (_Window *Window, _Texture *Texture, _Rect RecSource, int DestX, int DestY, byte AlphaOffset)
  {
    __Window *Window_;
    __Texture *Texture_;
    //
    Window_ = (__Window *) Window;
    Texture_ = (__Texture *) Texture;
    if (Texture)
      return CopyArea (Window_, Texture_->Image, RenderTargetTexture->Image,
                       //@@@@Texture_->Mask, RenderTargetTexture->Mask, RenderTargetTexture->gc,
                       Texture_->Mask, RenderTargetTexture->Mask,
                       RecSource.x, RecSource.y, RecSource.Width, RecSource.Height, DestX, DestY);
    return false;
  }


_Texture *TextureCreate (_Window *Window, int Width, int Height)
  {
    __Window *Window_;
    __Texture *Texture;
    //
    // Allocate and initialize
    Texture = NULL;
    #ifdef TexturesEnabled
    Window_ = (__Window *) Window;
    Texture = (__Texture *) malloc (sizeof (__Texture));
    Texture->Width = Width;
    Texture->Height = Height;
    Texture->Image = XCreatePixmap (XDisplay, Window_->XWindow, Width, Height, Window_->XDepth);
    //@@@@Texture->gc = XCreateGC (XDisplay, Texture->Image, 0, NULL);
    Texture->Mask = 0;
    #endif // TexturesEnabled
    return Texture;
  }

void TextureDestroy (_Window *Window, _Texture *Texture)
  {
    __Texture *Texture_;
    //
    if (Texture)
      {
        Texture_ = (__Texture *) Texture;
        if (Texture_->Image)
          XFreePixmap (XDisplay, Texture_->Image);
        if (Texture_->Mask)
          XFreePixmap (XDisplay, Texture_->Mask);
        free (Texture_);
      }
  }

_Bitmap *BitmapCreate (_Window *Window, int Width, int Height)
  {
    __Window *Window_;
    int ImgDataSize;
    byte *ImgData;
    //
    Window_ = (__Window *) Window;
    ImgDataSize = (Width + 1) * (Height + 1) * 4 + 8;
    ImgData = (byte *) malloc (ImgDataSize);
    return (_Bitmap *) XCreateImage (XDisplay, Window_->XVisual, Window_->XDepth, ZPixmap, 0, (char *) ImgData, Width, Height, 8, 0);
  }

void BitmapDestroy (_Bitmap *Bitmap)
  {
    if (Bitmap)
      XDestroyImage ((XImage *) Bitmap);
  }

bool BitmapGetSize (_Bitmap *Bitmap, int *Width, int *Height)
  {
    if (Bitmap)
      {
        if (Width)
          *Width = ((XImage *) Bitmap)->width;
        if (Height)
          *Height = ((XImage *) Bitmap)->height;
        return true;
      }
    return false;
  }

int BitmapGetPixel (_Bitmap *Bitmap, int x, int y)
  {
    return XColourToColour (XGetPixel ((XImage *) Bitmap, x, y));
  }

bool BitmapSetPixel (_Bitmap *Bitmap, int x, int y, int Colour)
  {
    if (Bitmap)
      {
        XPutPixel ((XImage *) Bitmap, x, y, ColourToXColour (Colour));
        return true;
      }
    return false;
  }

extern int Time1, Time2, Time3;

bool RenderBitmap (__Window *Window, XImage *Img, _Rect RecSource, int x, int y, int ColTransparent)
  {
    __Window *Window_;
    XImage *Img_, *ImgMask;
    int x1, y1;
    int Pixel;
    //
    Window_ = (__Window *) Window;
    if (Img)
      if ((RecSource.Width > 0) && (RecSource.Height > 0))
        {
          int Tick1 = ClockuS ();//####
          // If Transparent, create a transparency Mask
          if (ColTransparent >= 0)
            {
              ColTransparent = ColourToXColour (ColTransparent);
              Img_ = (XImage *) BitmapCreate (Window, RecSource.Width, RecSource.Height);
              int Tick2 = ClockuS ();//####
              ImgMask = (XImage *) BitmapCreate (Window, RecSource.Width, RecSource.Height);
              for (y1 = 0; y1 < RecSource.Height; y1++)
                for (x1 = 0; x1 < RecSource.Width; x1++)
                  {
                    Pixel = XGetPixel (Img, RecSource.x + x1, RecSource.y + y1);
                    if (Pixel == ColTransparent)
                      {
                        XPutPixel (Img_, x1, y1, 0x000000);
                        XPutPixel (ImgMask, x1, y1, 0x000000);
                      }
                    else
                      {
                        XPutPixel (Img_, x1, y1, Pixel);
                        XPutPixel (ImgMask, x1, y1, 0xFFFFFF);
                      }
                  }
              int Tick3 = ClockuS (); //####
              Time2 += Tick3 -Tick2;//####
              // Now put on "screen"
              // Draw the unmasked bits in Target
              // Clear effected pixels in Dest (using AND NOT)
              XPutImage (XDisplay, RenderTargetTexture->Image, Window_->WinGCAndInvert, ImgMask,//
                         0, 0, x, y, RecSource.Width, RecSource.Height);
              // Copy new coloured Mask into Dest (using OR)
              XPutImage (XDisplay, RenderTargetTexture->Image, Window_->WinGCOr, Img_,//
                         0, 0, x, y, RecSource.Width, RecSource.Height);
              // Modify DestMask (using OR)
              if (RenderTargetTexture->Mask && ImgMask)
                XPutImage (XDisplay, RenderTargetTexture->Mask, Window_->WinGCOr, ImgMask,//
                           0, 0, x, y, RecSource.Width, RecSource.Height);
              Time3 += ClockuS () - Tick3; //####
              // free stuff
              BitmapDestroy (Img_);
              BitmapDestroy (ImgMask);
            }
          else
            {
              // Put on "screen"
              int Tick3 = ClockuS ();//####
              //SetGCFunction (Window, RenderTargetTexture->gc, GXcopy);  // not needed
              //@@@@XPutImage (XDisplay, RenderTargetTexture->Image, RenderTargetTexture->gc, Img,
              XPutImage (XDisplay, RenderTargetTexture->Image, Window_->WinGCCopy, Img,
                         RecSource.x, RecSource.y, x, y, RecSource.Width, RecSource.Height);
              // Modify DestMask (using OR)
              if (RenderTargetTexture->Mask)
                //@@@@XFillRectangle (XDisplay, RenderTargetTexture->Mask, RenderTargetTexture->gcMask,
                XFillRectangle (XDisplay, RenderTargetTexture->Mask, Window_->WinGCOr,
                                x, y, RecSource.Width, RecSource.Height);
              Time3 += ClockuS () - Tick3;//####
            }
          Time1 += ClockuS () - Tick1;//####
          return true;
        }
    return false;
  }

bool RenderBitmap (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest, int ColTransparent)
  {
    __Window *Window_;
    XImage *Img1, *Img2;
    int xs, ys, xd, yd;
    int a;
    bool get, put;
    int pixel;
    bool Res;
    //
    Res = false;
    Window_ = (__Window *) Window;
    Img1 = (XImage *) Bitmap;
    Img2 = NULL;
    if (Bitmap)
      if ((RecDest.Width > 0) && (RecDest.Height > 0))
        {
          if (RecSource.Width == RecDest.Width && RecSource.Height == RecDest.Height)   // Same size
            Res = RenderBitmap (Window_, Img1, RecSource, RecDest.x, RecDest.y, ColTransparent);
          else   // Bitmap needs resizing
            {
              // Generate Scaled Image
              Img2 = (XImage *) BitmapCreate (Window, RecDest.Width, RecDest.Height);
              xs = ys = 0;
              xd = yd = 0;
              get = put = true;
              while (true)
                {
                  if (get & put)
                    {
                      get = false;
                      pixel = XGetPixel (Img1, RecSource.x + xs, RecSource.y + ys);
                    }
                  if (put)
                    {
                      put = false;
                      XPutPixel (Img2, xd, yd, pixel);
                    }
                  a = (xs+1) * RecDest.Width - (xd+1) * RecSource.Width;
                  if (a <= 0)
                    {
                      xs++;
                      get = true;
                    }
                  if (a >= 0)
                    {
                      xd++;
                      put = true;
                    }
                  if (xs >= RecSource.Width && get)
                    {
                      get = put = true;
                      xs = 0;
                      xd = 0;
                      a = (ys+1) * RecDest.Height - (yd+1) * RecSource.Height;
                      if (a <= 0)
                        {
                          ys++;
                          get = true;
                        }
                      if (a >= 0)
                        {
                          yd++;
                          put = true;
                        }
                      if (ys >= RecSource.Height && get)
                        break;
                    }
                }
              // Now put on "screen"
              Res = RenderBitmap (Window_, Img2, {0, 0, RecDest.Width, RecDest.Height}, RecDest.x, RecDest.y, ColTransparent);
            }
          // Free stuff
          if (Img2)
            BitmapDestroy (Img2);
        }
    return Res;
  }

_Bitmap* BitmapFromTexture (_Window *Window, _Texture *Texture, int Width, int Height)
  {
    __Texture *Texture_;
    //
    Texture_ = (__Texture *) Texture;
    if (Texture_ && Texture_->Image)
      return (_Bitmap *) XGetImage (XDisplay, Texture_->Image, 0, 0, Width, Height, 0xFFFFFF, ZPixmap);
    return NULL;
  }


////////////////////////////////////////////////////////////////////////////
//
// Clipboard

void ClipboardFilename (char *Filename)
  {
    StrPathHome (&Filename, ".clipboard");
    *Filename = 0;
  }

char* ClipboardGet (void)
  {
    char Filename [MaxPath];
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
    return nullptr;
  }

bool ClipboardSet (char *Text)
  {
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
    //pthread_attr_t ThreadAttr;
    int err;
    char St [64], *s;
    //
    Thread = 0;
    ThreadFunction_ = ThreadFunction;
    err = pthread_create (&Thread, NULL, ThreadWrapper, Params);//ThreadFunction Param
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
    return err == 0;
  }


////////////////////////////////////////////////////////////////////////////
//
// Main Programme

int main (int argc, char* args [])
  {
    return main_ (argc, args);
  }
