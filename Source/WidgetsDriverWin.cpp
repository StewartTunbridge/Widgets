/////////////////////////////////////////////////////////////////////////
//
// Widget Driver: Windows
//
/////////////////////////////////////////////////////////////////////////


#include <string.h>

#include "WidgetsDriver.hpp"

#include <windows.h>

extern void DebugAdd (const char *St);   // defined in application
extern void DebugAddS (const char *s1, const char *s2);   // defined in Widgets.cpp
extern void DebugAdd (const char *St, int n);
extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddP (const char *St, _Point pnt);
extern void DebugAddInt (const char *St, int n);   // "

char *ClipboardData = NULL;
HWND ClipboardWinID = NULL;
bool ClipboardReady = false;

WNDCLASS wc;

typedef struct
  {
    int Width, Height;
    unsigned int *Pixels;
    byte *Alpha;
    BITMAPINFO BitmapInfo;
  } __Bitmap;

typedef struct
  {
    HWND hWindow;
    HDC WinDC;
    char *ClassName;
    //
    byte WindowAttributes;
    int SizeX, SizeY;
    __Bitmap *tIcon;
  } __Window;

HINSTANCE WinhInstance = 0, WinhPrevInstance = 0;

void ShowWindowsError (void)
  {
    DebugAddInt ("*** WINDOWS ERROR: ", GetLastError ());
  }

int ColourToWinColour (int Colour)
  {
    //return (Colour & 0x00FF00) | (Colour >> 16) | ((Colour << 16) & 0xFF0000);
    _Colour Col;
    //
    Col.RGBA = Colour;
    Swap (Col.Prime [0], Col.Prime [2]);
    return Col.RGBA;
  }

int WinColourToColour (int Colour)
  {
    //return (Colour & 0x00FF00) | (Colour >> 16) | ((Colour << 16) & 0xFF0000);
    _Colour Col;
    //
    Col.RGBA = Colour;
    Swap (Col.Prime [0], Col.Prime [2]);
    return Col.RGBA;
  }

bool WidgetsInit (void)
  {
    return true;
  }

bool WidgetsUninit (void)
  {
    return true;
  }

POINT Mouse;

_Point MousePos (void)
  {
    return {Mouse.x, Mouse.y};
  }

void MouseCursor (_Window *Window, bool Show)
  {
    //####
  }

MSG Messages [256];
int MessagesA = 0;
int MessagesB = 0;

// the Window Procedure
LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    if (msg == WM_DESTROY || msg == WM_SIZE)
    //if (msg == WM_PAINT || /*msg == WM_CLOSE ||* / msg == WM_DESTROY || msg == WM_SIZE)
      {
        Messages [MessagesB].hwnd = hwnd;
        Messages [MessagesB].message = msg;
        Messages [MessagesB].wParam = wParam;
        Messages [MessagesB].lParam = lParam;
        MessagesB = (MessagesB + 1) % SIZEARRAY (Messages);
        return 0;
      }
    //else
      return DefWindowProc (hwnd, msg, wParam, lParam);
    //return 0;
    /*switch (msg)
      {
        case WM_CLOSE:
          //DestroyWindow (hwnd);
          break;
        case WM_DESTROY:
          //PostQuitMessage (0);
          break;
        case WM_SIZE:
          //Resizing = true;
          break;
        default:*/
//          return DefWindowProc (hwnd, msg, wParam, lParam);
//      }
//    return 1;
  }

char ClassName [] = "Widget Window PiMicros";

int WindowNumber = 0;

_Window *WindowCreate (char *Title, int x, int y, int SizeX, int SizeY, byte WindowAttributes)
  {
    RECT r;
    HWND hwnd;
    __Window *Res;
    //
    WindowNumber++;
    // Calculate the window area from the client area
    r.top = 0;
    r.bottom = SizeY;
    r.left = 0;
    r.right = SizeX;
    AdjustWindowRect (&r, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, FALSE);
    //
    // Step 2: Creating the Window
    //hwnd = CreateWindowEx (WS_EX_CLIENTEDGE, g_szClassName, Title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, NULL, NULL, WinhInstance, NULL);
    //hwnd = CreateWindow (wc.lpszClassName, Title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, x, y, SizeX, SizeY, NULL, NULL, WinhInstance, NULL);
    DWORD Style = WS_OVERLAPPED | WS_SYSMENU | WS_VISIBLE;
    if (WindowAttributes & waResizable)
      Style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    if (WindowAttributes & waBorderless)
      Style = WS_POPUP;
    if (WindowAttributes & waFullScreen)
      hwnd = CreateWindowEx (0, wc.lpszClassName, Title, WS_POPUP, 0, 0, GetSystemMetrics (SM_CXSCREEN), GetSystemMetrics (SM_CYSCREEN), NULL, NULL, WinhInstance, NULL);
    else
      hwnd = CreateWindowEx (0, wc.lpszClassName, Title, Style, x, y, r.right-r.left, r.bottom-r.top, NULL, NULL, WinhInstance, NULL);
    if (!hwnd)
      {
        ShowWindowsError ();
        return nullptr;
      }
    if (WindowAttributes & waAlwaysOnTop)
      SetWindowPos (hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE); //r.left, r.top, r.right - r.left, r.top - r.bottom, Flags);
    //UINT Flags = SWP_DRAWFRAME | SWP_NOSIZE;
    //if (WindowAttributes & waResizable)
    //  Flags = SWP_DRAWFRAME;
    //SetWindowPos (hwnd, hInsert, r.left, r.top, r.right - r.left, r.top - r.bottom, Flags);
    //SetWindowPos (hwnd, hInsert, x, y, r.right - r.left, r.bottom - r.top, Flags);
    ShowWindow (hwnd, SW_NORMAL);
    //if (WindowAttributes & waModal)
    //  EnableWindow (parent window handle, false);
    UpdateWindow (hwnd);
    Res = (__Window *) malloc (sizeof (__Window));
    Res->hWindow = hwnd;
    Res->WindowAttributes = WindowAttributes;
    Res->SizeX = SizeX;
    Res->SizeY = SizeY;
    Res->WinDC = GetDC (Res->hWindow);
    return Res;
  }

void WindowDestroy (_Window *Window)
  {
    __Window *Window_;
    //
    Window_ = (__Window *) Window;
    DestroyWindow (Window_->hWindow);
    free (Window_);
    WindowNumber--;
  }

void WindowGetPosSize (_Window *Window, int *x, int *y, int *Width, int *Height)
  {
    __Window *Window_;
    RECT Rect;
    //
    Window_ = (__Window *) Window;
    if (!GetClientRect (Window_->hWindow, &Rect))
      return;
    if (x)
      *x = Rect.left;
    if (y)
      *y = Rect.top;
    if (Width)
      *Width = Rect.right - Rect.left;
    if (Height)
      *Height = Rect.bottom - Rect.top;
  }

bool WindowSetPosSize (_Window *Window, int x, int y, int Width, int Height)
  {
    return false;
  }

bool WindowSetIcon (_Window *Window, _Bitmap *Bitmap)
  {
    //wc.hIcon = LoadIconA %%%%%
    return false;
  }

// Put up on screen after all the Rendering
void RenderPresent (_Window *Window_, _Bitmap *Bitmap_, _Rect Source, _Point Dest, byte Alpha)
  {
    __Window *Window;
    __Bitmap *Bitmap;
    //
    Window = Window_;
    Bitmap = Bitmap_;
    ///* works
    if (StretchDIBits (Window->WinDC, Dest.x, Dest.y + Source.Height, Source.Width, -Source.Height, Source.x, Source.y, Source.Width, Source.Height, Bitmap->Pixels, &Bitmap->BitmapInfo, DIB_RGB_COLORS, SRCCOPY) == 0)
                DebugAdd ("Render Bitmap: StretchDIBBits returned 0");
    //*/
    //BitBlt (Window->WinDC, Dest.x, Dest.y, Source.Width, Source.Height, Bitmap->BitmapInfo.???
  }

_Bitmap *BitmapCreate (_Window *Window, int Width, int Height)
  {
    __Bitmap *Res;
    //
    Res = (__Bitmap *) malloc (sizeof (__Bitmap));
    *Res = {0};
    Res->Width = Width;
    Res->Height = Height;
    Res->Pixels = (unsigned int *) malloc (Width * Height * sizeof (unsigned int));
    //memset (Res->Pixels, 0x80, Width * Height * sizeof (unsigned int));
    //
    Res->BitmapInfo.bmiHeader.biSize = sizeof (Res->BitmapInfo.bmiHeader);
    Res->BitmapInfo.bmiHeader.biWidth = Res->Width;
    Res->BitmapInfo.bmiHeader.biHeight = Res->Height;
    Res->BitmapInfo.bmiHeader.biPlanes = 1;
    Res->BitmapInfo.bmiHeader.biBitCount = 32;
    Res->BitmapInfo.bmiHeader.biCompression = BI_RGB;
    //Res->BitmapInfo.bmiColors [0].rgbBlue = 0xFF;
    //Res->BitmapInfo.bmiColors [0].rgbGreen = 0xFF00;
    //Res->BitmapInfo.bmiColors [0].rgbRed = 0xFF0000;
    return Res;
  }

void BitmapDestroy (_Bitmap *Bitmap)
  {
    __Bitmap *Bitmap_;
    //
    if (Bitmap)
      {
        Bitmap_ = (__Bitmap *) Bitmap;
        if (Bitmap_->Pixels)
          free (Bitmap_->Pixels);
        free (Bitmap_);
      }
  }

bool BitmapGetSize (_Bitmap *Bitmap, int *Width, int *Height)
  {
    __Bitmap *Bitmap_;
    //
    Bitmap_ = (__Bitmap *) Bitmap;
    if (Bitmap)
      {
        if (Width)
          *Width = Bitmap_->Width;
        if (Height)
          *Height = Bitmap_->Height;
        return true;
      }
    return false;
  }

int BitmapGetPixel (_Bitmap *Bitmap, int x, int y)
  {
    __Bitmap *Bitmap_;
    //
    if (Bitmap)
      {
        Bitmap_ = (__Bitmap *) Bitmap;
        if (x >= 0 & y >= 0 && x < Bitmap_->Width && y < Bitmap_->Height)
          return WinColourToColour (Bitmap_->Pixels [y * Bitmap_->Width + x]);
      }
    return 0;
  }

bool BitmapSetPixel (_Bitmap *Bitmap, int x, int y, int Colour)
  {
    __Bitmap *Bitmap_;
    //
    if (Bitmap)
      {
        Bitmap_ = (__Bitmap *) Bitmap;
        if (x >= 0 & y >= 0 && x < Bitmap_->Width && y < Bitmap_->Height)
          {
            Bitmap_->Pixels [y * Bitmap_->Width + x] = ColourToWinColour (Colour);
            return true;
          }
      }
    return false;
  }

void BitmapClear (_Bitmap *Bitmap)
  {
    __Bitmap *Bitmap_;
    int i;
    //
    if (Bitmap)
      {
        Bitmap_ = (__Bitmap *) Bitmap;
        i = Bitmap_->Width * Bitmap_->Height;
        memset (Bitmap_->Pixels, 0, i * sizeof (int));
      }
  }


/////////////////////////////////////////////////////////////////////////
//
// KEYCODE TRANSLATION

typedef struct
  {
    int Code;
    word Key;
  } _CodeToKey;

const _CodeToKey CodeToKeys [] =
  {
    {0xDE, '\''},
    {0xBA, ';'},
    {0xBD, '-'},
    {0xBB, '='},
    {0xBE, '.'},
    {0xBF, '/'},
    {0xC0, '`'},
    {0xBC, ','},
    {0xDB, '['},
    {0xDC, '\\'},
    {0xDD, ']'},
    //
    {VK_ESCAPE, esc},
    {VK_RETURN, cr},
    {VK_BACK, bs},
    {VK_TAB, tab},
    //
    {VK_UP, KeyUp},
    {VK_DOWN, KeyDown},
    {VK_LEFT, KeyLeft},
    {VK_RIGHT, KeyRight},
    {VK_INSERT, KeyIns},
    {VK_DELETE, KeyDel},
    {VK_HOME, KeyHome},
    {VK_END, KeyEnd},
    {VK_PRIOR, KeyPageUp},
    {VK_NEXT, KeyPageDown},
    {VK_F1, KeyF1+0},
    {VK_F2, KeyF1+1},
    {VK_F3, KeyF1+2},
    {VK_F4, KeyF1+3},
    {VK_F5, KeyF1+4},
    {VK_F6, KeyF1+5},
    {VK_F7, KeyF1+6},
    {VK_F8, KeyF1+7},
    {VK_F9, KeyF1+8},
    {VK_F10, KeyF1+9},
    {VK_F11, KeyF1+10},
    {VK_F12, KeyF1+11},
    //
    /*{VK_LSHIFT, KeyShift},
    {VK_RSHIFT, KeyShift},
    {VK_LCONTROL, KeyCntl},
    {VK_RCONTROL, KeyCntl},
    {VK_LMENU, KeyAlt},
    {VK_RMENU, KeyAlt}*/
    {VK_SHIFT, KeyShift},
    {VK_CONTROL, KeyCntl},
    {VK_MENU, KeyAlt}
  };

int CodeToKey (int Code)
  {
    int Res;
    int i;
    //
    i = 0;
    Res = -1;
    while (true)
      {
        if (i >= SIZEARRAY (CodeToKeys))
          {
            Res = Code & 0x00FF;
            break;
          }
        if (CodeToKeys [i].Code == Code)
          {
            Res = CodeToKeys [i].Key;
            break;
          }
        i++;
      }
    return Res;
  }

/*
    etKeyDown, etKeyUp,   // Keyboard event: Key = ASCII+ | Mask for Cntr, Shift, Alt
    etMouseDown, etMouseUp, etMouseMove,   // Mouse event: Button in Key, Coordinates at X, Y
    etWindowResize, etWindowFocus, etWindowRedraw, etWindowClose,   // Window event
    etQuit
*/

const char *ShiftPairs = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:\'\",<.>/?";

void EventPoll (_Event *Event)
  {
    MSG Msg;
    bool go;
    static word ShiftState = 0;
    static int x = -1, y = -1;
    char *pc;
    //
    //char s [80], *sp;
    //
    //DebugAdd ("EventPoll-In");//####
    if (ClipboardReady)
      {
        ClipboardReady = false;
        Event->Type = etClipboard;
        Event->WindowID = ClipboardWinID;
        return;
      }
    Event->Type = etNone;
    go = false;
    if (MessagesA != MessagesB)
      {
        Msg = Messages [MessagesA];
        go = true;
        MessagesA = (MessagesA + 1) % SIZEARRAY (Messages);
      }
    else if (PeekMessage (&Msg, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage (&Msg);
        DispatchMessage (&Msg);
        go = true;
      }
    if (go)
      {
        //sp = s;
        //StrCat (&sp, "Event ");
        //NumToHex (&sp, Msg.message, 0);
        //StrCat (&sp, ' ');
        Event->WindowID = Msg.hwnd;
        if (Msg.message == WM_KEYDOWN || Msg.message == WM_KEYUP)
          {
            if (Msg.message == WM_KEYDOWN)
              Event->Type = etKeyDown;
            else
              Event->Type = etKeyUp;
            Event->Key = CodeToKey (Msg.wParam);
            byte KbdState [256];
            wchar_t UTF16 [4], *u16;
            GetKeyboardState (KbdState);
            int n = ToUnicode (Msg.wParam, MapVirtualKey (Msg.wParam, 0), KbdState, UTF16, 4, 0);//Msg.lParam
            DebugAddInt ("ToUnicode", n);//####
            if (n > 0)
              {
                u16 = UTF16;
                int u32 = UTF16Read (&u16);
                if (u32 > KeyMax)
                  {
                    Event->UTF = u32;
                    Event->Key = KeyUNICODE;
                  }
              }
            if (Event->Key & 0xFF00)
              if (Msg.message == WM_KEYDOWN)
                ShiftState |= Event->Key;
              else
                ShiftState &= ~Event->Key;
              if (Event->Key >= 0x80)
                Event->Key |= ShiftState;
              else
                {
                  if (IsAlpha (Event->Key))   // "Alpha" keys
                    if (ShiftState & KeyAlt)   // Alt alpha keys are always uppercase
                      Event->Key = Event->Key & ~0x20 | ShiftState;
                    else if (ShiftState & KeyCntl)   // Control alphas
                      Event->Key &= 0x1F;
                    else if (ShiftState & KeyShift)
                      Event->Key &= ~0x20;
                    else
                      Event->Key |= 0x20;
                  else   // Digits / Symbols ...
                    if (ShiftState & KeyShift)
                      {
                        pc = StrPos ((char *) ShiftPairs, Event->Key);
                        if (pc)
                          Event->Key = pc [1];
                        else
                          Event->Key |= ShiftState;
                      }
                }
            //NumToHex (&sp, Event->Key);
            //StrCat (&sp, "  Shift=");
            //NumToHex (&sp, Event->ShiftState);
            //StrCat (&sp, "  wParam=");
            //NumToHex (&sp, Msg.wParam);
          }
        else if ((Msg.message & 0xFFFFFF00) == WM_MOUSEFIRST)
          {
            if (Msg.message == WM_MOUSEMOVE)
              {
                x = Msg.lParam & 0xFFFF;
                y = (Msg.lParam >> 16) & 0xFFFF;
                Mouse.x = x;
                Mouse.y = y;
                ClientToScreen (Msg.hwnd, &Mouse);
              }
            Event->X = x;
            Event->Y = y;
            switch (Msg.message)
              {
                case WM_MOUSEMOVE:   Event->Type = etMouseMove; break;
                case WM_LBUTTONDOWN: Event->Type = etMouseDown; Event->Key = KeyMouseLeft; break;
                case WM_LBUTTONUP:   Event->Type = etMouseUp;   Event->Key = KeyMouseLeft; break;
                case WM_RBUTTONDOWN: Event->Type = etMouseDown; Event->Key = KeyMouseRight; break;
                case WM_RBUTTONUP:   Event->Type = etMouseUp;   Event->Key = KeyMouseRight; break;
                case WM_MBUTTONDOWN: Event->Type = etMouseDown; Event->Key = KeyMouseMiddle; break;
                case WM_MBUTTONUP:   Event->Type = etMouseUp;   Event->Key = KeyMouseMiddle; break;
                case WM_MOUSEWHEEL:  Event->Type = etMouseDown;
                                      if (Msg.wParam & 0x80000000)
                                        Event->Key = KeyMouseWheelDown;
                                      else
                                        Event->Key = KeyMouseWheelUp;
                                      break;
              }
            //NumToHex (&sp, Msg.wParam, 8);
          }
        else if (Msg.message == WM_CLOSE || Msg.message == WM_DESTROY)
          Event->Type = etWindowClose;
        else if (Msg.message == WM_SIZE)
          {
            Event->Type = etWindowResize;
            Event->X = Msg.lParam & 0x0000FFFF;
            Event->Y = Msg.lParam >> 16;
          }
        else if (Msg.message == WM_PAINT)
          Event->Type = etWindowRedraw;
        Event->ShiftState = ShiftState;
        //*sp = 0;
        //DebugAdd (s);
      }
    //DebugAdd ("EventPoll-Out");//####
  }

bool WindowIDMatch (_WindowID WindowID, _Window *Window)
  {
    return ((__Window *) Window)->hWindow == WindowID;
  }


// Clipboard
void ClipboardGet (_Window *Window_)
  {
    __Window *Window;
    //
    Window = (__Window *) Window_;
    if (OpenClipboard (nullptr))
      {
        // Retrieve the Clipboard data (specifying that
        // we want ANSI text (via the CF_TEXT value).
        HANDLE hClipboardData = GetClipboardData (CF_TEXT);
        // Call GlobalLock so that to retrieve a pointer
        // to the data associated with the handle returned
        // from GetClipboardData.
        char *pchData = (char*) GlobalLock (hClipboardData);
        // Set a local CString variable to the data
        // copy to result
        StrAssignCopy (&ClipboardData, pchData);
        ClipboardWinID = Window->hWindow;
        ClipboardReady = true;
        // Unlock the global memory.
        GlobalUnlock (hClipboardData);
        // Finally, when finished I simply close the Clipboard
        // which has the effect of unlocking it so that other
        // applications can examine or modify its contents.
        CloseClipboard ();
      }
  }

bool ClipboardSet (_Window *Window, char *Text)
  {
    //char *Text_;
    HGLOBAL hClipboardData;
    char *pchData;
    //
    if (OpenClipboard (nullptr))
      {
        EmptyClipboard ();
        hClipboardData = GlobalAlloc (GMEM_DDESHARE, StrLen (Text) + 1);
        pchData = (char*) GlobalLock (hClipboardData);
        strcpy (pchData, LPCSTR (Text));
        GlobalUnlock (hClipboardData);
        SetClipboardData (CF_TEXT,hClipboardData);
        CloseClipboard ();
        return true;
      }
    return false;
  }


////////////////////////////////////////////////////////////////////////////
//
// Start Thread

//#include <pthread.h>

bool StartThread (_ThreadFunction ThreadFunction, void *Param)
  {
    HANDLE Thread;
    char St [64], *s;
    //
    Thread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE) ThreadFunction, Param, 0, NULL);
    s = St;
    if (Thread == NULL)
      StrCat (&s, "** StartThread ERROR");
    else
      {
        StrCat (&s, "New Thread 0x");
        NumToHex (&s, (long long int) Thread);
      }
    *s = 0;
    DebugAdd (St);
    return Thread != NULL;
    // Thread needs to be closed in Windows
  }


void Beep (void)
  {
    MessageBeep (MB_ICONEXCLAMATION); //();//(MB_ICONASTERISK);MB_OK
  }


////////////////////////////////////////////////////////////////////////////
//
// Main Programme

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
  {
    int res;
    //
    DebugAdd ("Widgets - Start");
    //
    WinhInstance = hInstance;
    WinhPrevInstance = hPrevInstance;
    //Step 1: Registering the Window Class
    wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = ClassName;
    wc.lpfnWndProc   = WndProc;
    //
    //wc.cbSize        = sizeof (WNDCLASSEX);
    //wc.style         = 0;
    //wc.lpfnWndProc   = WndProc;
    //wc.cbClsExtra    = 0;
    //wc.cbWndExtra    = 0;
    wc.hInstance     = WinhInstance;
    //wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
    //wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    //wc.lpszMenuName  = NULL;
    //wc.lpszClassName = g_szClassName;
    //wc.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);
    //*/
    //
    if (!RegisterClass (&wc))
      {
        ShowWindowsError ();
        return -1;
      }
    //MessageBox (NULL, "AAAHHHHHH!!!!!!", "Windows Sucks", MB_OK);
    res = main_ (__argc, __argv); //(0, nullptr);
    DebugAdd ("Widgets - Exit");
    if (ClipboardData)
      free (ClipboardData);
    return res;
  }
