///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS: IMAGE SUPPORT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////

//#define ImageSave

#include "WidgetsDriver.hpp"

#include <stdlib.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"
#ifdef ImageSave
  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include "../stb/stb_image_write.h"
#endif // ImageSave

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAddInt (const char *St, int n);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect r);

bool DataExpect (byte **Data, const int *Target)
  {
    while (*Target >= 0)
      if (*(*Data)++ != *Target++)
        return false;
    return true;
  }

int DataGetWord (byte **Data, bool MSBFirst)
  {
    int Res;
    byte a, b;
    //
    a = *(*Data)++;
    b = *(*Data)++;
    if (MSBFirst)
      Res = a << 8 | b;
    else
      Res = a | b << 8;
    return Res;
  }

int DataGetLongWord (byte **Data, bool MSBFirst)
  {
    int Res;
    word a, b;
    //
    a = DataGetWord (Data, MSBFirst);
    b = DataGetWord (Data, MSBFirst);
    if (MSBFirst)
      Res = a << 16 | b;
    else
      Res = a | b << 16;
    return Res;
  }

const int JpegSOIAPP1Markers [] = {0xFF, 0xD8, 0xFF, 0xE1, -1};

bool SearchMarker (byte **Data, word Marker)
  {
    int m, sz;
    //
    while (true)
      {
        m = DataGetWord (Data, true);
        if ((m & ~0xFF) == 0xFF00)   // valid Marker
          {
            if (m == Marker)   // Found
              return true;
            sz = DataGetWord (Data, true);
            if (sz < 256)  // arbitary range check
              *Data += sz - 2;
            else
              break;
          }
        else
          break;
      }
    DebugAdd ("JPEG: Invalid Marker / Size");
    return false;
  }

const int JpegExifHeader [] = {'E', 'x', 'i', 'f', 0, 0, -1};

int JpegOrientation (byte *Data, int Size)
  {
    int Res;
    byte *DataP;
    int Offset, NTags;
    bool MSBFirst;   // Byte order. Equ Motorola order
    int i;
    //
    Res = -1;
    DataP = Data;
    if (Size > 100)
      if (DataGetWord (&DataP, true) == 0xFFD8)   // Image Marker
        if (SearchMarker (&DataP, 0xFFE1))
          {
            Size = DataGetWord (&DataP, true);
            if (DataExpect (&DataP, JpegExifHeader))
              {
                i = DataGetWord (&DataP, true);
                if (i == 0x4949 || i == 0x4D4D)
                  {
                    MSBFirst = (i == 0x4D4D);
                    DataP += 2;
                    //Offset = DataGetWord (&DataP, MSBFirst);
                    Offset = DataGetLongWord (&DataP, MSBFirst);
                    DataP = DataP + Offset - 8;
                    NTags = DataGetWord (&DataP, MSBFirst);
                    i = 0;
                    while (true)
                      {
                        if (i >= NTags)
                          break;
                        if (DataGetWord (&DataP, MSBFirst) == 0x0112)   // Tag == Orientation
                          {
                            DataP += 6;
                            Res = DataGetWord (&DataP, MSBFirst);
                            break;
                          }
                        DataP += 10;   // step past the rest of the Tag data
                        i++;
                      }
                  }
              }
        }
    return Res;
  }

_Bitmap* BitmapLoad (_Window *Window, char *Filename)
  {
    _Bitmap *Res;
    int File;
    int Width, Height, Channels, Orient;
    int Size;
    byte *DataRaw;
    byte *Data, *d;
    int x, y;
    int Pixel;
    //
    DebugAddS ("Loading Image", Filename);
    Res = NULL;
    File = FileOpen_ (Filename, foRead);
    if (File >= 0)
      {
        Size = FileSize (File);//  Width * Height * Channels;   // allocate for RGBA
        DataRaw = (byte *) malloc (Size);
        if (FileRead (File, DataRaw, Size) == Size)
          {
            Orient = JpegOrientation (DataRaw, Size);
            Data = stbi_load_from_memory (DataRaw, Size, &Width, &Height, &Channels, 0);
            DebugAddR ("  ", {Width, Height, Channels, Orient});
            if (Data)
              {
                if (Orient <= 4)   // Normal aspect
                  Res = BitmapCreate (Window, Width, Height);
                else
                  Res = BitmapCreate (Window, Height, Width);   // Sideways
                d = Data;
                for (y = 0; y < Height; y++)
                  for (x = 0; x < Width; x++)
                    {
                      if (Channels == 1)   // grey
                        Pixel = *d * 0x010101 | 0xFF000000;
                      else if (Channels == 2)   // grey, alpha
                        Pixel = (d [0] * 0x010101) | (d [1] << 24);
                      else if (Channels == 3)   // rgb
                        Pixel = *((int *) d) | 0xFF000000;
                      else if (Channels == 4)   // rgba
                        Pixel = *((int *) d);
                      d += Channels;
                      switch (Orient)
                        {
                          case -1:
                          case  0:
                          case  1: BitmapSetPixel (Res, x, y, Pixel); break;
                          case  2: BitmapSetPixel (Res, Width - 1 - x, y, Pixel); break;
                          case  3: BitmapSetPixel (Res, Width - 1 - x, Height - 1 - y, Pixel); break;
                          case  4: BitmapSetPixel (Res, x, Height - 1 - y, Pixel); break;
                          case  5: BitmapSetPixel (Res, y, x, Pixel); break;
                          case  6: BitmapSetPixel (Res, Height - 1 - y, x, Pixel); break;
                          case  7: BitmapSetPixel (Res, Height - 1 - y, Width - 1 - x, Pixel); break;
                          case  8: BitmapSetPixel (Res, y, Width - 1 - x, Pixel); break;
                        }
                      //BitmapSetPixel (Res, x, y, Pixel);
                    }
                stbi_image_free (Data);
              }
          }
        free (DataRaw);
        FileClose (File);
      }
    return Res;
  }

/*_Bitmap* BitmapLoadResource (_Window *Window, char *Filename)
  {
    char *Filename_;
    _Bitmap *Res;
    //
    Filename_ = ResourcePathAdd (Filename);
    Res = BitmapLoad (Window, Filename_);
    free (Filename_);
    return Res;
  }*/

#ifdef ImageSave

bool  BitmapSave (_Window *Window, _Bitmap* Bitmap, char *Filename, int Quality)
  {
    bool OK;
    int Width, Height;
    byte *Data, *d;
    int x, y;
    //
    DebugAddS ("Saving Image", Filename);
    OK = false;
    BitmapGetSize (Bitmap, &Width, &Height);
    Data = (byte *) malloc (Width * Height * 4);
    d = Data;
    if (Data)
      {
        for (y = 0; y < Height; y++)
          for (x = 0; x < Width; x++)
            {
              *((int *) d) = BitmapGetPixel (Bitmap, x, y);
              d += 4;
            }
        if (StrSame_ (StrFindFileExtension (Filename), ".bmp"))
          OK = stbi_write_bmp (Filename, Width, Height, 4, Data);
        else if (StrSame_ (StrFindFileExtension (Filename), ".jpg") || StrSame_ (StrFindFileExtension (Filename), ".jpeg"))
          OK = stbi_write_jpg (Filename, Width, Height, 4, Data, Quality);
        if (StrSame_ (StrFindFileExtension (Filename), ".png"))
          OK = stbi_write_png (Filename, Width, Height, 4, Data, 0);
        free (Data);
      }
    return OK;
  }

#endif // ImageSave


///////////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

void BitmapCopy (_Bitmap *Source, _Bitmap *Target, _Rect RectSource, _Point Dest)
  {
    int x, y;
    int p;
    //
    for (x = 0; x < RectSource.Width; x++)
      for (y = 0; y < RectSource.Height; y++)
        {
          p = BitmapGetPixel (Source, RectSource.x + x, RectSource.y + y);
          BitmapSetPixel (Target, Dest.x + x, Dest.y + y, p);
        }
  }

void BitmapFill (_Bitmap *Target, _Rect Dest, int Colour)
  {
    int x, y;
    //
    for (x = 0; x < Dest.Width; x++)
      for (y = 0; y < Dest.Height; y++)
        BitmapSetPixel (Target, Dest.x + x, Dest.y + y, Colour);
  }

_Point SizeStretch (_Point Size, _Point Space)   // Stretch keeping aspect ratio
  {
    _Point Res;
    //
    Res = Space;
    if (Size.x * Space.y > Size.y * Space.x)   // Width controls size
      Res.y = (Size.y * Space.x) / Size.x;
    else   // Height controls size
      Res.x = (Size.x * Space.y) / Size.y;
    return Res;
  }
