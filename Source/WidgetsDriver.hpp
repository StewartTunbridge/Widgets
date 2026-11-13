/////////////////////////////////////////////////////////////////////////
//
// Widget Driver
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024,2025 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////


#ifndef _WidgetsDriver
#define _WidgetsDriver

#include "lib.hpp"

// Holder for Hidden types

typedef void _Window;  // Holds window stuff, implementation dependent
typedef void _Bitmap;   // Holds off screen Images
//typedef long int _WindowID;   // ID of window associated with an _Event
typedef void* _WindowID;   // ID of window associated with an _Event

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
#define KeyNL 0x8A

#define KeyF1 0x90

#define KeyUNICODE 0xFF

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
#define KeyMouseWheelLeft 6
#define KeyMouseWheelRight 7

// Events

typedef enum
  {
    etNone,   // No event
    etNext, etPrev,   // Pass onto next / prev Container
    etKeyDown, etKeyUp,   // Keyboard event: Key = ASCII+ | Mask for Cntr, Shift, Alt
    etMouseDown, etMouseUp, etMouseMove,   // Mouse event: Button in Key, Coordinates at X, Y
    etWindowResize, etWindowFocus, etWindowRedraw, etWindowClose,   // Window event
    etQuit
    //etClipboard
  } _EventType;

typedef struct
  {
    _EventType Type;   // What was it?
    _WindowID WindowID;
    int Key;   // Key code, Mouse button number ...
    int X, Y;   // Coordinates (Mouse)
    word MouseKeys;   // State of all Mouse Keys: Use MouseKeys & Bit [KeyMouse* - 1]
    int ShiftState;   // State of Keyboard Shift Keys. See Key*
    int UTF;   // Unicode input
  } _Event;

extern bool WidgetsInit (void);
extern bool WidgetsUninit (void);

// Mouse
extern _Point MousePos (void);
extern void MouseCursor (_Window *Window, bool Show);

extern _Window *WindowCreate (const char *Title, int x, int y, int SizeX, int SizeY, byte WindowAttributes = 0);  // Size*=0 => Full Screen
extern void WindowDestroy (_Window *Window);
extern void WindowGetPosSize (_Window *Window, int *x, int *y, int *Width, int *Height);
extern bool WindowSetPosSize (_Window *Window, int x, int y, int Width, int Height);
//extern void WindowRaise (_Window *Window);
extern bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap);
extern bool WindowSetTitle (_Window * Window, const char *Title);

extern void RenderPresent (_Window *Window_, _Bitmap *Bitmap_, _Rect Source, _Point Dest, byte Alpha);   // Put up on screen after all the Rendering

extern _Bitmap *BitmapCreate (_Window *Window, int Width, int Height);
extern void BitmapDestroy (_Bitmap *Bitmap);
extern bool BitmapGetSize (_Bitmap *Bitmap, int *Width, int *Height);
extern int BitmapGetPixel (_Bitmap *Bitmap, int x, int y);
extern bool BitmapSetPixel (_Bitmap *Bitmap, int x, int y, int Colour);
extern void BitmapClear (_Bitmap *Bitmap);

extern void EventPoll (_Event *Event);
extern bool WindowIDMatch (_WindowID WindowID, _Window *Window);

// Clipboard
extern char *ClipboardData;   // Result of Clipboard read
extern bool ClipboardGet (_Window *Window);   // Initiate Clipboard Read. Result in ClipboardData (above)
extern bool ClipboardSet (_Window *Window_, char *Text);

// Start Thread
typedef int (*_ThreadFunction) (void *arg);
bool StartThread (_ThreadFunction ThreadFunction, void *Param);

void Beep (void);   // Make a beep, why not?

extern int SoundPlaying;
extern bool SoundPlay (short *Buffer, int BufferSize, bool Free);   // Stereo: signed 16 bit samples X 2
extern void SoundStop (void);

// Main
extern int main_ (int argc, char* args []);
// Main programme provided by the application
// The WidgetsDriver must set and free StartPath

#endif
