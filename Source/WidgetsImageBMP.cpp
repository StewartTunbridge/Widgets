///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS: IMAGE SUPPORT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024,2026 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////

//#define ImageSave

#include "WidgetsDriver.hpp"

#include <stdlib.h>
#include <stdio.h>

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAddInt (const char *St, int n);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect r);
extern void DebugAddError (const char* Message);


///////////////////////////////////////////////////////////////////////////////
//
// LOAD BMP

_Bitmap* BitmapLoad (_Window *Window, char *Filename)
  {
    int file;
    longint size;
    byte *buf;
    //
    _Bitmap *Img;
    int pixels, header, palette;
    int sizeX, sizeY;
    int bpp, bytepp, bytepl, bytepad;
    int comp, ncols;  // %%%% currently ignored
    int x, y, b;
    int col;
    byte *ptr;
    byte *palitem;
    //
    DebugAddS ("Loading Image", Filename);
    Img = NULL;
    file = FileOpen_ (Filename, foRead);
    if (file > 0)
      {
        size = FileSize (file);
        if (size > 0x1E + sizeof (int))
          {
            buf = (byte *) malloc (Max (size, (longint) 512));
            if (buf)
              {
                if (FileRead (file, buf, size) == size)
                  {
                    pixels = *(int *) (buf + 0x0A);
                    header = *(int *) (buf + 0x0E);
                    palette = 0x0E + header; // + 24;
                    sizeX = *(int *) (buf + 0x12);
                    sizeY = *(int *) (buf + 0x16);
                    bpp = *(word *) (buf + 0x1C);   // bits / pixel
                    comp = *(int *) (buf + 0x1E);
                    ncols = *(int *) (buf + 0x2E);
                    bytepp = bpp >> 3;
                    bytepl = ((bpp * sizeX + 31) & ~31) >> 3;   // pixel lines are padded out to 4*n bytes
                    bytepad = bytepl - ((bpp * sizeX + 7) >> 3);
                    //####
                    //DebugAddS ("LoadBMP", Filename);
                    DebugAddInt ("  SizeX", sizeX);
                    DebugAddInt ("  SizeY", sizeY);
                    DebugAddInt ("  bits/pixel", bpp);
                    DebugAddInt ("  bytes/pixel", bytepp);
                    DebugAddInt ("  compression", comp);
                    DebugAddInt ("  Num Colours", ncols);
                    if (sizeX && sizeY && bpp && pixels < size /*&& comp == 0*/ && bpp >= 4)
                      {
                        Img = BitmapCreate (Window, sizeX, sizeY);
                        if (Img)
                          {
                            x = 0;
                            y = sizeY - 1;
                            b = 0;
                            ptr = &buf [pixels];
                            while (y >= 0)
                              {
                                if (bpp == 32)
                                  {
                                    col = ptr [0] << 16 | ptr [1] << 8 | ptr [2] | ptr [3] << 24;
                                    ptr += bytepp;
                                  }
                                else if (bpp == 24)
                                  {
                                    col = 0xFF000000 | ptr [0] << 16 | ptr [1] << 8 | ptr [2];
                                    ptr += bytepp;
                                  }
                                else if (bpp == 16)
                                  {
                                    col = 0xFF000000 | ptr [0] | ptr [1] << 8;
                                    ptr += 2;
                                  }
                                else if (bpp == 8)
                                  col = *ptr++;
                                else // bpp == 4, 2, 1)
                                  {
                                    col = (*ptr >> (8 - bpp - b)) & (Bit [bpp] - 1);
                                    b += bpp;
                                    if (b >= 8)
                                      {
                                        ptr++;
                                        b = 0;
                                      }
                                  }
                                if (bpp <= 8)
                                  if (palette + col * 4 < size)
                                    {
                                      palitem = &buf [palette + col * 4];
                                      col = 0xFF000000 | palitem [0] << 16 | palitem [1] << 8 | palitem [2];
                                    }
                                  else
                                    col = 0xFF000000 | (col * 0xFFFFFF / (Bit [bpp] - 1));
                                BitmapSetPixel (Img, x, y, col);
                                x++;
                                if (x >= sizeX)
                                  {
                                    x = 0;
                                    y--;
                                    if (b)   // part way thru last byte ...
                                      ptr++;   // ... so step to next
                                    b = 0;
                                    ptr += bytepad;
                                  }
                              }
                          }
                      }
                    else
                      DebugAddS ("Unsupported file type - ", Filename);
                  }
                free (buf);
              }
          }
        FileClose (file);
      }
    else
      DebugAddError ("could not open");
    return Img;
  }

/*
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
    File = FileOpen (Filename, foRead);
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

_Bitmap* BitmapLoadResource (_Window *Window, char *Filename)
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

#include <stdint.h>

// Define the BMP file header structure
typedef struct __attribute__ ((packed)) {
    char bfType [2]; // "BM"
    uint32_t bfSize; // file size
    uint16_t bfReserved1; // reserved
    uint16_t bfReserved2; // reserved
    uint32_t bfOffBits; // offset to pixel data
} BMPFileHeader;

// Define the BMP info header structure
typedef struct __attribute__ ((packed)) {
    uint32_t biSize; // size of this header
    int32_t biWidth; // image width
    int32_t biHeight; // image height
    uint16_t biPlanes; // number of color planes
    uint16_t biBitCount; // bits per pixel (24 for RGB)
    uint32_t biCompression; // compression type (0 for uncompressed)
    uint32_t biSizeImage; // size of pixel data
    int32_t biXPelsPerMeter; // horizontal resolution
    int32_t biYPelsPerMeter; // vertical resolution
    uint32_t biClrUsed; // number of colors used
    uint32_t biClrImportant; // number of important colors
} BMPInfoHeader;

bool  BitmapSave (_Window *Window, _Bitmap* Bitmap, char *Filename, int Quality)
//bool SaveBMP (_Window *Window, _Bitmap *Bitmap, char *Filename)
  {
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    int Width, Height;
    int x, y, Pixel;
    int SizeScan, SizePixelData;
    byte *Pixels, *p, *pp;
    int Index;
    int file;//FILE* fp;
    //
    if (!BitmapGetSize (Bitmap, &Width, &Height))
      return false;   // Can't get size
    SizeScan = (Width * (Quality ? 4 : 3) + 3) & ~3;
    SizePixelData = SizeScan * Height;
    // Initialize file header
    file_header.bfType [0] = 'B';
    file_header.bfType [1] = 'M';
    file_header.bfSize = sizeof (BMPFileHeader) + sizeof (BMPInfoHeader) + SizePixelData; // file size
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = sizeof (BMPFileHeader) + sizeof (BMPInfoHeader); // offset to pixel data
    // Initialize info header
    info_header.biSize = sizeof (BMPInfoHeader);
    info_header.biWidth = Width;
    info_header.biHeight = Height;
    info_header.biPlanes = 1;
    info_header.biBitCount = Quality ? 32 : 24; // 24/32-bit RGB/RGBA
    info_header.biCompression = 0; // uncompressed
    info_header.biSizeImage = SizePixelData; //Width * Height * 3; // size of pixel data
    info_header.biXPelsPerMeter = 0;
    info_header.biYPelsPerMeter = 0;
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;
    // Open file for writing
    file = FileOpen (Filename, foWrite);//fp = fopen (Filename, "wb");
    if (file < 0)//(!fp)
      return false;   // Can't open file
    // Write file header
    FileWrite (file, (byte *) &file_header, sizeof (BMPFileHeader)); //fwrite (&file_header, sizeof (BMPFileHeader), 1, fp);
    // Write info header
    FileWrite (file, (byte *) &info_header, sizeof (BMPInfoHeader)); //fwrite (&info_header, sizeof (BMPInfoHeader), 1, fp);
    // Write pixel data
    Pixels = (byte *) malloc (SizePixelData);
    if (!Pixels)
      return false;
    p = Pixels;
    for (y = Height - 1; y >= 0; y--)
      {
        Index = 0;
        for (x = 0; x < Width; x++)
          {
            Pixel = BitmapGetPixel (Bitmap, x, y);
            pp = (byte *) &Pixel;
            p [Index++] = pp [2];
            p [Index++] = pp [1];
            p [Index++] = pp [0];
            if (Quality)
              p [Index++] = pp [3];
          }
        p += SizeScan;
      }
    FileWrite (file, Pixels, SizePixelData); //fwrite (Pixels, Size, 1, fp);
    free (Pixels);
    // Close file
    FileClose (file);
    return true;
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
