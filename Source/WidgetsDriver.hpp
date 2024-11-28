/////////////////////////////////////////////////////////////////////////
//
// Widget Driver
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////


#ifndef _WidgetsDriver
#define _WidgetsDriver

#include "lib.hpp"

// Holder for Hidden types

typedef void _Window;  // Holds window stuff, implementation dependent
typedef void _Texture;  // Holds a Texture for each container (optional?)
typedef void _Bitmap;   // Holds off screen Image
typedef long int _WindowID;   // ID of window associated with an _Event

// Places and Regions

struct _Point
  {
    int x;
    int y;
  };

struct _Rect
  {
    int x, y;
    int Width, Height;
  };

// WINDOW ATTRIBUTES
#define waResizable 0x01
#define waBorderless 0x02
#define waAlwaysOnTop 0x04
#define waFullScreen 0x08
#define waModal 0x10

// Keys
#define KeyArrows 0x80
#define KeyUp 0x80
#define KeyDown 0x81
#define KeyLeft 0x82
#define KeyRight 0x83

#define KeyHome 0x84
#define KeyEnd 0x87
#define KeyIns 0x85
#define KeyDel 0x86
#define KeyPageUp 0x88
#define KeyPageDown 0x89
#define KeyF1 0x90

#define KeyBackSpace 0x08
#define KeyEnter 0x0D

#define KeyMax 0xFF   // Mask for Key Codes WITHOUT SHIFT BITS

#define KeyShift 0x100
#define KeyCntl 0x200
#define KeyAlt 0x400

#define KeyMouseLeft 1
#define KeyMouseMiddle 2
#define KeyMouseRight 3
#define KeyMouseWheelUp 4
#define KeyMouseWheelDown 5

// Events

typedef enum
  {
    etNone,   // No event
    etNext,   // Pass onto next Container
    etKeyDown, etKeyUp,   // Keyboard event: Key = ASCII+ | Mask for Cntr, Shift, Alt
    etMouseDown, etMouseUp, etMouseMove,   // Mouse event: Button in Key, Coordinates at X, Y
    etWindowResize, etWindowFocus, etWindowRedraw, etWindowClose,   // Window event
    etQuit
  } _EventType;

typedef struct
  {
    _EventType Type;   // What was it?
    _WindowID WindowID;
    int Key;   // Key code, Mouse button number ...
    int X, Y;   // Coordinates (Mouse)
    word MouseKeys;   // State of all Mouse Keys: Use Keys & Bit [KeyMouse* - 1]
    int ShiftState;   // State of Keyboard Shift Keys. See Key*
  } _Event;

inline bool IsEventMouse (_Event *Event)
  {
    if (Event->Type >= etMouseDown && Event->Type <= etMouseMove)
      return true;
    return false;
  }

extern bool ForceSoftwareRendering;

extern bool WidgetsInit (void);
extern bool WidgetsUninit (void);

extern void MouseCursor (_Window *Window, bool Show);

extern _Window *WindowCreate (char *Title, int x, int y, int SizeX, int SizeY, byte WindowAttributes = 0);  // Size*=0 => Full Screen
extern void WindowDestroy (_Window *Window);
extern void WindowGetPosSize (_Window *Window, int *x, int *y, int *Width, int *Height);
extern bool WindowSetPosSize (_Window *Window, int x, int y, int Width, int Height);
//extern void WindowRaise (_Window *Window);
extern bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap);

extern bool RenderTarget (_Window *Window, _Texture *Texture, int Colour, _Rect Clip);   // Specify where to draw (and hint background colour)
extern void RenderPresent (_Window *Window, byte Alpha);   // Put up on screen

extern _Texture *TextureCreate (_Window *Window, int Width, int Height);
extern void TextureDestroy (_Window *Window, _Texture *Texture);
//extern void TextureGetSize (_Texture *Texture, int *Width, int *Height);

extern _Bitmap *BitmapCreate (_Window *Window, int Width, int Height);
extern void BitmapDestroy (_Bitmap *Bitmap);
//extern _Bitmap* BitmapLoad (_Window *Window, char *Filename);
//extern bool BitmapSave (_Window *Window, _Bitmap* Bitmap, char *Filename);
extern bool BitmapGetSize (_Bitmap *Bitmap, int *Width, int *Height);
extern int BitmapGetPixel (_Bitmap *Bitmap, int x, int y);
extern bool BitmapSetPixel (_Bitmap *Bitmap, int x, int y, int Colour);
extern _Bitmap* BitmapFromTexture (_Window *Window, _Texture *Texture, int Width, int Height);

// Render Functions
extern void RenderDrawPoint (_Window *Window, int Colour, int x, int y);
extern void RenderDrawLine (_Window *Window, int Colour, int x1, int y1, int x2, int y2);
extern void RenderFillRect (_Window *Window, _Rect Rect, int Colour);
extern void RenderFillTransparent (_Window *Window);
extern bool RenderTexture (_Window *Window, _Texture *Texture, _Rect RecSource, int DestX, int DestY, byte AlphaOffset);
extern bool RenderBitmap (_Window *Window, _Bitmap *Bitmap, _Rect RecSource, _Rect RecDest, bool Transparent);   // Allow resizing

extern bool TextureToFile (_Window *Window, _Texture *Texture, int Width, int Height, char *Filename);

extern void EventPoll (_Event *Event);
extern bool WindowIDMatch (_WindowID WindowID, _Window *Window);

// Clipboard
extern char* ClipboardGet (void);   // Read clipboard contents. User must free result
extern bool ClipboardSet (char *Text);

// Start Thread
typedef void* (_ThreadFunction) (void *arg);
bool StartThread (_ThreadFunction ThreadFunction, void *Param);

// Main
extern int main_ (int argc, char* args []);

#endif
