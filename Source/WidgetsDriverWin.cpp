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
extern void DebugAddInt (const char *St, int n);   // "
extern void DebugAddHex (const char *St, int p);
extern void DebugAddP (const char *St, _Point pnt);
//extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddR (const char *St, _Rect r);

char *ClipboardData = NULL;
HWND ClipboardWinID = NULL;
//bool ClipboardReady = false;

void ShowWindowsError (void)
  {
    DebugAddInt ("*** WINDOWS ERROR: ", GetLastError ());
  }

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

char ClassName [] = "Widget Window PiMicros";

int WindowNumber = 0;

_Window *WindowCreate (const char *Title, int x, int y, int SizeX, int SizeY, byte WindowAttributes)
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

HICON CreateIconFromPixels (__Bitmap *Bitmap) //unsigned int* buffer, int width, int height)
  {
    HDC dc = GetDC (NULL);
    HDC dc_andmask = CreateCompatibleDC (dc);
    HDC dc_xormask = CreateCompatibleDC (dc);
    //
    HBITMAP hBmpAnd = CreateCompatibleBitmap (dc, Bitmap->Width, Bitmap->Height);
    HBITMAP hBmpXor = CreateCompatibleBitmap (dc, Bitmap->Width, Bitmap->Height);
    //
    HBITMAP hOldAnd = (HBITMAP) SelectObject (dc_andmask, hBmpAnd);
    HBITMAP hOldXor = (HBITMAP) SelectObject (dc_xormask, hBmpXor);
    //
    int cTrans = Bitmap->Pixels [0];
    //
    for (int y = 0; y < Bitmap->Height; ++y)
      {
        for (int x = 0; x < Bitmap->Width; ++x)
          {
            unsigned int currpix = Bitmap->Pixels [y * Bitmap->Width + x];   //BitmapGetPixel (Bitmap, x, y);
            // Check alpha channel (0xRRGGBBAA or 0xAARRGGBB depending on format)
            // Assuming 0xAARRGGBB format based on search context
            if (currpix == cTrans) //if ((currpix & 0xFF000000) == 0)   // Transparent
              {
                SetPixel (dc_andmask, x, y, RGB (255, 255, 255));
                SetPixel (dc_xormask, x, y, RGB (0, 0, 0));
              }
            else   // Opaque
              {
                SetPixel (dc_andmask, x, y, RGB (0, 0, 0));
                int r = (currpix >> 16) & 0xFF;
                int g = (currpix >> 8) & 0xFF;
                int b = currpix & 0xFF;
                SetPixel (dc_xormask, x, y, RGB (r, g, b));
              }
          }
      }
    // Cleanup DCs and bitmaps
    SelectObject (dc_andmask, hOldAnd);
    SelectObject (dc_xormask, hOldXor);
    DeleteDC (dc_andmask);
    DeleteDC (dc_xormask);
    ReleaseDC (NULL, dc);
    //
    ICONINFO iconInfo = {FALSE, Bitmap->Width / 8, Bitmap->Height, hBmpAnd, hBmpXor};
    // Note: If using 32-bit DIB sections directly, you can skip SetPixel loops
    // and use CreateDIBSection for better performance, but SetPixel works for small icons.
    return CreateIconIndirect (&iconInfo);
  }

bool WindowSetTitle (_Window * Window, const char *Title)
  {
    __Window *Window_;
    //
    SetConsoleTitleA (Title);
    Window_ = (__Window *) Window;
    return SetWindowTextA (Window_->hWindow, Title);
  }

bool WindowSetIcon (_Window *Window_, _Bitmap *Bitmap_)
  {
    __Window *Window;
    __Bitmap *Bitmap;
    //
    Window = (__Window *) Window_;
    Bitmap = (__Bitmap *) Bitmap_;
    if (!Bitmap || !Window)
      return false;
    wc.hIcon = CreateIconFromPixels (Bitmap);
    SendMessage (Window->hWindow, WM_SETICON, ICON_SMALL, (LPARAM) wc.hIcon);
    SendMessage (Window->hWindow, WM_SETICON, ICON_BIG, (LPARAM) wc.hIcon);
    return true;
  }

// Put up on screen after all the Rendering
void RenderPresent (_Window *Window_, _Bitmap *Bitmap_, _Rect Source, _Point Dest, byte Alpha)
  {
    __Window *Window;
    __Bitmap *Bitmap;
    //
    Window = (__Window *) Window_;
    Bitmap = (__Bitmap *) Bitmap_;
    ///* works
    if (StretchDIBits (Window->WinDC, Dest.x, Dest.y + Source.Height - 1, Source.Width, -Source.Height, Source.x, Source.y, Source.Width, Source.Height, Bitmap->Pixels, &Bitmap->BitmapInfo, DIB_RGB_COLORS, SRCCOPY) == 0)
      DebugAdd ("Render Bitmap: StretchDIBBits returned 0");
    //*/
    //BitBlt (Window->WinDC, Dest.x, Dest.y, Source.Width, Source.Height, Bitmap->BitmapInfo.bmiHeader.
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
    Res->BitmapInfo.bmiColors [0].rgbBlue = 0xFF;
    Res->BitmapInfo.bmiColors [0].rgbGreen = 0xFF00;
    Res->BitmapInfo.bmiColors [0].rgbRed = 0xFF0000;
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
    {VK_RETURN, '\r'},
    {VK_BACK, '\b'},
    {VK_TAB, '\t'},
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
    //{VK_LSHIFT, KeyShift},
    //{VK_RSHIFT, KeyShift},
    //{VK_LCONTROL, KeyCntl},
    //{VK_RCONTROL, KeyCntl},
    {VK_LMENU, KeyAlt},
    {VK_RMENU, KeyAlt},
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

const char *ShiftPairs = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:\'\",<.>/?";


/////////////////////////////////////////////////////////////////////////
//
// MESSAGE PROCESSING

MSG Messages [256];
int MessagesA = 0;
int MessagesB = 0;

bool WMSetFocus = false;

// the Window Procedure
LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
  {
    //DebugAddHex ("WndProc", msg); // ####
    if (msg == WM_SETFOCUS)
      WMSetFocus = true;
    if (msg == WM_DESTROY || msg == WM_SIZE || msg == WM_CLOSE)
    //if (msg == WM_PAINT || /*msg == WM_CLOSE ||* / msg == WM_DESTROY || msg == WM_SIZE)
      {
        Messages [MessagesB].hwnd = hwnd;
        Messages [MessagesB].message = msg;
        Messages [MessagesB].wParam = wParam;
        Messages [MessagesB].lParam = lParam;
        MessagesB = (MessagesB + 1) % SIZEARRAY (Messages);
        //return DefWindowProc (hwnd, msg, wParam, lParam);//%%%%
        return 0;
      };
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
        default:* /
//          return DefWindowProc (hwnd, msg, wParam, lParam);
//      }
//    return 1;*/
  }

void EventPoll (_Event *Event)
  {
    MSG Msg;
    bool go;
    static word ShiftState = 0;
    static int x = -1, y = -1;
    char *pc;
    //
    /*if (ClipboardReady)
      {
        ClipboardReady = false;
        Event->Type = etClipboard;
        Event->WindowID = ClipboardWinID;
        return;
      }*/
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
        go = true;
        TranslateMessage (&Msg);
        DispatchMessage (&Msg);
      }
    if (go)
      {
        //DebugAddHex ("Win Msg ", Msg.message); //####
        Event->WindowID = Msg.hwnd;
        //if (Msg.message == WM_SETFOCUS || Msg.message == WM_KILLFOCUS)
        if (WMSetFocus)
          {
            ShiftState &= ~KeyAlt;   // Clear Alt State
            WMSetFocus = false;
          }
        else if (Msg.message == WM_SYSKEYDOWN)
          ShiftState |= KeyAlt; // gets here but we don't get Alt key events
        else if (Msg.message == WM_SYSKEYUP)
          ShiftState &= ~KeyAlt;
        else if (Msg.message == WM_KEYDOWN || Msg.message == WM_KEYUP)
          {
            if (Msg.message == WM_KEYDOWN)
              Event->Type = etKeyDown;
            else
              Event->Type = etKeyUp;
            //if (Event->Type == etKeyDown) // ####
            //  DebugAddHex ("  Keycode", Msg.wParam); // ####
            Event->Key = CodeToKey (Msg.wParam);
            if (HIWORD (Msg.lParam) & KF_ALTDOWN)
              ShiftState = KeyAlt;
            byte KbdState [256];
            wchar_t UTF16 [4], *u16;
            GetKeyboardState (KbdState);
            int n = ToUnicode (Msg.wParam, MapVirtualKey (Msg.wParam, 0), KbdState, UTF16, 4, 0);//Msg.lParam
            //DebugAddInt ("ToUnicode", n);//####
            if (n > 0)
              {
                u16 = (wchar_t *) UTF16;
                int u32 = UTF16Read ((word **) &u16);
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
            else
              {
                Event->Key |= ShiftState;
                if ((Event->Key & KeyMax) < 0x80)
                  {
                    if (IsAlpha (Event->Key & KeyMax))   // "Alpha" keys
                      if (ShiftState & KeyAlt)   // Alt alpha keys are always uppercase
                        Event->Key &= ~0x20;
                      else if (ShiftState & KeyCntl)   // Control alphas
                        Event->Key &= 0x1F;
                      else if (ShiftState & KeyShift)
                        Event->Key &= KeyMax;
                      else
                        Event->Key = (Event->Key & KeyMax) | 0x20;
                    //else   // Digits / Symbols ...
                    if (ShiftState & KeyShift)
                      {
                        pc = StrPos ((char *) ShiftPairs, (Event->Key & KeyMax));
                        if (pc)
                          Event->Key = pc [1];
                      }
                  }
              }
            //if (Event->Type == etKeyDown) // ####
            //  DebugAddHex ("  Key", Event->Key);//####
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
            /*if ( Msg.message != WM_MOUSEMOVE) // ####
              {
                DebugAddHex ("MouseEvent", Msg.message); //####
                DebugAddHex ("MouseEvent:wParam", Msg.wParam); //####
              }*/
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
                case WM_MOUSEHWHEEL: Event->Type = etMouseDown;
                                     if (Msg.wParam & 0x80000000)   // Right Tilt
                                       Event->Key = KeyMouseWheelLeft;
                                     else
                                       Event->Key = KeyMouseWheelRight;
                                     break;
              }
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
      }
    //if (Event->Type != etNone)//####
    //  DebugAddR ("EventPoll", {Event->Type, Event->Key, Event->X, Event->Y});//####
  }

bool WindowIDMatch (_WindowID WindowID, _Window *Window)
  {
    return ((__Window *) Window)->hWindow == WindowID;
  }


/////////////////////////////////////////////////////////////////////////
//
// CLIPBOARD

bool ClipboardGet (_Window *Window_)
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
        //ClipboardReady = true;
        // Unlock the global memory.
        GlobalUnlock (hClipboardData);
        // Finally, when finished I simply close the Clipboard
        // which has the effect of unlocking it so that other
        // applications can examine or modify its contents.
        CloseClipboard ();
        return true;
      }
    return false;
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
        if (Text)
          {
            hClipboardData = GlobalAlloc (GMEM_DDESHARE, StrLen (Text) + 1);
            pchData = (char*) GlobalLock (hClipboardData);
            strcpy (pchData, LPCSTR (Text));
            GlobalUnlock (hClipboardData);
            SetClipboardData (CF_TEXT,hClipboardData);
          }
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
    wc.lpfnWndProc   = WndProc; // %%%%DefWindowProc
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
    PathStart = GetCurrentPath ();
    char *p = StrFindFilename (__argv [0]);
    PathExe = StrMake (__argv [0], p - __argv [0]);
    res = main_ (__argc, __argv); //(0, nullptr);
    DebugAdd ("Widgets - Exit");
    if (ClipboardData)
      free (ClipboardData);
    free (PathStart);
    free (PathExe);
    return res;
  }


////////////////////////////////////////////////////////////////////////////
//
// Sound

int SoundPlaying = 0;

typedef struct
  {
    short *Buff;
    bool Free;
    HWAVEOUT hWaveOut;   // Wave ouptput device handle: constant throughout
    WAVEHDR header;
  } _Wave;

_Wave Waves [16];

int SoundPlayThread (void *arg)
  {
    _Wave *Wave;
    ////////////////////////////////////////////
    Wave = (_Wave *) arg;
    while (true)
      {
        Sleep (1000);   // sleep for 1 second
        if (waveOutUnprepareHeader (Wave->hWaveOut, &Wave->header, sizeof (WAVEHDR)) != WAVERR_STILLPLAYING)   // Finished
          {
            waveOutClose (Wave->hWaveOut);
            if (Wave->Free)
              free (Wave->Buff);   // Free buffer memeory
            Wave->Buff = NULL;
            SoundPlaying--;
            break;
          }
      }
    return 0;
  }

bool SoundPlay (short *Buffer, int Size, bool Free)
  {
    WAVEFORMATEX wfx;
    int i;
    ///////////////////////////////////////////////////////////////////
    i = 0;
    while (true)
      {
        if (i == 16)
          break;
        if (Waves [i].Buff == NULL)
          {
            // first we need to set up the WAVEFORMATEX structure.
            // the structure describes the format of the audio.
            memset (&wfx, 0, sizeof (wfx));
            wfx.nSamplesPerSec = 44100; /* sample rate */
            wfx.wBitsPerSample = 16; /* sample size */
            wfx.nChannels = 2; /* channels*/
            // WAVEFORMATEX also has other fields which need filling.
            // as long as the three fields above are filled this should
            // work for any PCM (pulse code modulation) format.
            wfx.cbSize = 0; /* size of _extra_ info */
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nBlockAlign = (wfx.wBitsPerSample >> 3) * wfx.nChannels;
            wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
            // try to open the default wave device. WAVE_MAPPER is
            // a constant defined in mmsystem.h, it always points to the
            // default wave device on the system (some people have 2 or
            // more sound cards).
            if (waveOutOpen (&Waves [i].hWaveOut, WAVE_MAPPER, &wfx, (DWORD) 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR)
              {
                // device is now open and then close the device again.
                // initialise the block header with the size and pointer.
                ZeroMemory (&Waves [i].header, sizeof (WAVEHDR));
                Waves [i].header.lpData = (char *) Buffer;
                Waves [i].header.dwBufferLength = Size * 2;
                // prepare the block for playback
                waveOutPrepareHeader (Waves [i].hWaveOut, &Waves [i].header, sizeof (WAVEHDR));
                // write the block to the device. waveOutWrite returns immediately
                // unless a synchronous driver is used (not often).
                if (waveOutWrite (Waves [i].hWaveOut, &Waves [i].header, sizeof (WAVEHDR)) == MMSYSERR_NOERROR)
                  {
                    Waves [i].Buff = Buffer;
                    Waves [i].Free = Free;
                    SoundPlaying++;
                    StartThread (SoundPlayThread, &Waves [i]);
                    return true;
                  }
              }
            break;
          }
        i++;
      }
    if (Free)
      free (Buffer);
    return false;
  }

void SoundPause (void)
  {
    int i;
    ////////////////////////////////////////////
    i = 0;
    while (i < 16)
      {
        if (Waves [i].Buff)
          waveOutPause (Waves [i].hWaveOut);
        i++;
      }
  }

void SoundResume (void)
  {
    int i;
    ////////////////////////////////////////////
    i = 0;
    while (i < 16)
      {
        if (Waves [i].Buff)
          waveOutRestart (Waves [i].hWaveOut);
        i++;
      }
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
            waveOutPause (Waves [i].hWaveOut);
            waveOutClose (Waves [i].hWaveOut);
            free (Waves [i].Buff);   // Free buffer memeory
            Waves [i].Buff = NULL;
          }
        i++;
      }
    SoundPlaying = 0;
  }

int SoundNumOutputDevices (void)
  {
    return waveOutGetNumDevs ();
  }

