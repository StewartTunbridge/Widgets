///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS: IMAGE SUPPORT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


#include "WidgetsDriver.hpp"

#define JPGPNG

#include <stdint.h>
#ifdef JPGPNG
#include <png.h>
#include <jpeglib.h>
#include <jerror.h>
#endif // JPGPNG

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAddInt (const char *St, int n);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect *r);


///////////////////////////////////////////////////////////////////////////////
//
// LOAD / SAVE BMP, PNG ...

_Bitmap *LoadBMP (_Window *Window, char *Filename)
  {
    int file;
    longint size;
    byte *buf;
    //
    _Bitmap *Img;
    int pixels, header, palette;
    int sizeX, sizeY;
    int bpp, bytepp, bytepl, bytepad;
    int comp;
    int x, y, b;
    int col;
    byte *ptr;
    byte *palitem;
    //
    Img = NULL;
    file = FileOpen (Filename, foRead);
    if (file > 0)
      {
        size = FileSize (file);
        if (size > 0x1E + sizeof (int))
          {
            buf = (byte *) malloc (Max (size, 512l));
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
                    bytepp = bpp >> 3;
                    bytepl = ((bpp * sizeX + 31) & ~31) >> 3;   // pixel lines are padded out to 4*n bytes
                    bytepad = bytepl - ((bpp * sizeX + 7) >> 3);
                    //####
                    DebugAddS ("LoadBMP", Filename);
                    DebugAddInt ("sizeX", sizeX);
                    DebugAddInt ("SizeY", sizeY);
                    DebugAddInt ("bits/pixel", bpp);
                    DebugAddInt ("bytes/pixel", bytepp);
                    if (sizeX && sizeY && bpp && pixels < size && comp == 0 && bpp >= 4)
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
                                if (bpp >= 24)
                                  {
                                    col = ptr [0] << 16 | ptr [1] << 8 | ptr [2];   // X Colour order
                                    ptr += bytepp;
                                  }
                                else if (bpp == 16)
                                  {
                                    col = ptr [0] | ptr [1] << 8;
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
                                      col = palitem [0] << 16 | palitem [1] << 8 | palitem [2];
                                    }
                                  else
                                    col = col * 0xFFFFFF / (Bit [bpp] - 1);
                                BitmapSetPixel (Img, x, y, col);// | 0xFF000000);
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
    return Img;
  }

_Bitmap *LoadPNG (_Window *Window, char *Filename)
  {
    #ifndef JPGPNG
    return nullptr;
    #else
    _Bitmap *Img;
    png_structp	png_ptr;
    png_infop info_ptr;
    FILE * fp;
    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    int interlace_method;
    int compression_method;
    int filter_method;
    int x, y, Index;
    png_bytepp rows;
    int bpp;
    int rowbytes;
    png_bytep row;
    int pixel;
    //
    Img = NULL;
    fp = fopen (Filename, "rb");
    if (fp)
      {
        png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr)
          {
            info_ptr = png_create_info_struct (png_ptr);
            if (info_ptr)
              {
                png_init_io (png_ptr, fp);
                png_read_png (png_ptr, info_ptr, 0, 0);
                png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_method, &compression_method, &filter_method);
                if (bit_depth != 8)
                  DebugAdd ("**** PNG: Only 8 bit/colour supported");
                rows = png_get_rows (png_ptr, info_ptr);
                rowbytes = png_get_rowbytes (png_ptr, info_ptr);
                bpp = rowbytes / width;
                Img = BitmapCreate (Window, width, height);
                for (y = 0; y < height; y++)
                  {
                    row = rows [y];
                    Index = 0;
                    for (x = 0; x < width; x++)
                      {
                        if (bpp == 1)
                          pixel = row [Index] * 0x010101;   // Assume gray scale
                        else if (bpp == 2)
                          pixel = row [Index+0] | row [Index+1] << 8;
                        else if (bpp == 3)
                          pixel = row [Index+0] | row [Index+1] << 8 | row [Index+2] << 16;
                        else //if (bpp == 4)
                          {
                            pixel = row [Index+0] | row [Index+1] << 8 | row [Index+2] << 16 | row [Index+3] << 24;
                            if ((pixel & 0xFF000000) == 0)   // transparent
                              pixel = 0xFF00FF;
                            pixel &= 0x00FFFFFF;
                          }
                        Index += bpp;
                        BitmapSetPixel (Img, x, y, pixel);
                      }
                  }
                png_destroy_read_struct (&png_ptr, &info_ptr, (png_infopp) NULL);
              }
          }
        fclose (fp);
      }
    return Img;
    #endif // _Windows
	}

// stolen from https://stackoverflow.com/questions/5616216/need-help-in-reading-jpeg-file-using-libjpeg

typedef enum {jfRGB, jfRGBA} _JpegFormat;

_Bitmap *LoadJPEG (_Window *Window, char* FileName)
  {
    #ifndef JPGPNG
    return nullptr;
    #else
    _Bitmap *Img;
    unsigned long x, y;
    unsigned long data_size;     // length of the file
    int channels;               //  3 =>RGB   4 =>RGBA
    unsigned char * rowptr [1];    // pointer to an array
    unsigned char * jdata;        // data for the image
    struct jpeg_decompress_struct info; //for our jpeg info
    struct jpeg_error_mgr err;          //the error handler
    _JpegFormat JpegFormat;
    FILE* file;
    //
    Img = NULL;
    //
    file = fopen (FileName, "rb");  //open the file
    if (file)
      {
        info.err = jpeg_std_error (& err);
        jpeg_create_decompress (& info);   //fills info structure
        jpeg_stdio_src (&info, file);
        jpeg_read_header (&info, TRUE);   // read jpeg file header
        jpeg_start_decompress (&info);    // decompress the file
        //set width and height
        x = info.output_width;
        y = info.output_height;
        channels = info.num_components;
        JpegFormat = jfRGB;
        if (channels == 4)
          JpegFormat = jfRGBA;
        data_size = x * y * 3;
        //--------------------------------------------
        // read scanlines one at a time & put bytes
        //    in jdata[] array. Assumes an RGB image
        //--------------------------------------------
        jdata = (byte *) malloc (data_size);
        while (info.output_scanline < info.output_height) // loop
          {
            // Enable jpeg_read_scanlines() to fill our jdata array
            rowptr[0] = (unsigned char *) jdata +  // secret to method
                    3 * info.output_width * info.output_scanline;
            jpeg_read_scanlines (&info, rowptr, 1);
          }
        jpeg_finish_decompress (&info);   //finish decompressing
        // Write to _Bitmap
        int xx, yy;
        int Index;
        int Pixel;
        //
        Img = NULL;
        if (x && y)
          Img = BitmapCreate (Window, x, y);
        if (Img)
          {
            Index = 0;
            for (yy = 0; yy < y; yy++)
              for (xx = 0; xx < x; xx++)
                {
                  Pixel = jdata [Index] | jdata [Index + 1] << 8 | jdata [Index + 2] << 16;
                  Index += 3;
                  BitmapSetPixel (Img, xx, yy, Pixel);
                }
          }
        jpeg_destroy_decompress (&info);
        fclose (file);
        free (jdata);
      }
    return Img;
    #endif // _Windows
  }

_Bitmap* BitmapLoad (_Window *Window, char *Filename)
  {
    if (StrSame (StrFindFileExtension (Filename), ".bmp"))
      return LoadBMP (Window, Filename);
    if (StrSame (StrFindFileExtension (Filename), ".png"))
      return LoadPNG (Window, Filename);
    if (StrSame (StrFindFileExtension (Filename), ".jpg") || StrSame (StrFindFileExtension (Filename), ".jpeg"))
      return LoadJPEG (Window, Filename);
    return NULL;
  }

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

bool SaveBMP (_Window *Window, _Bitmap *Bitmap, char *Filename)
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
    SizeScan = (Width * 3 + 3) & ~3;
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
    info_header.biBitCount = 24; // 24-bit RGB
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
          }
        p += SizeScan;
      }
    FileWrite (file, Pixels, SizePixelData); //fwrite (Pixels, Size, 1, fp);
    free (Pixels);
    // Close file
    FileClose (file); //fclose (fp);
    return true;
  }

bool SavePNG (_Window *Window, _Bitmap *Bitmap, char *Filename)
  {
    #ifndef JPGPNG
    return false;
    #else
    int Width, Height;
    int pixel_size = 3;
    int depth = 8;
    FILE * fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte ** row_pointers = NULL;
    //
    fp = fopen (Filename, "wb");
    if (fp)
      {
        BitmapGetSize (Bitmap, &Width, &Height);
        png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr)
          {
            info_ptr = png_create_info_struct (png_ptr);
            if (info_ptr)
              {
                // Set image attributes
                png_set_IHDR (png_ptr,
                              info_ptr,
                              Width,
                              Height,
                              depth,
                              PNG_COLOR_TYPE_RGB,
                              PNG_INTERLACE_NONE,
                              PNG_COMPRESSION_TYPE_DEFAULT,
                              PNG_FILTER_TYPE_DEFAULT);
                // Initialize rows of PNG
                row_pointers = (png_byte **) png_malloc (png_ptr, Height * sizeof (png_byte *));
                for (y = 0; y < Height; y++)
                  {
                    png_byte *row = (png_byte *) png_malloc (png_ptr, Width * pixel_size);
                    row_pointers [y] = row;
                    for (x = 0; x < Width; x++)
                      {
                        int pixel = BitmapGetPixel (Bitmap, x, y);
                        *row++ = pixel;
                        *row++ = pixel >> 8;
                        *row++ = pixel >> 16;
                      }
                  }
                // Write the image data to "fp"
                png_init_io (png_ptr, fp);
                png_set_rows (png_ptr, info_ptr, row_pointers);
                png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
                for (y = 0; y < Height; y++)
                  png_free (png_ptr, row_pointers [y]);
                png_free (png_ptr, row_pointers);
              }
            png_destroy_write_struct (&png_ptr, &info_ptr);
          }
        fclose (fp);
        return true;
      }
    return false;
    #endif
  }

bool SaveJPEG (_Window *Window, _Bitmap *Bitmap, char *Filename)
  {
    return false;
  }

bool  BitmapSave (_Window *Window, _Bitmap* Bitmap, char *Filename)
  {
    if (StrSame (StrFindFileExtension (Filename), ".bmp"))
      return SaveBMP (Window, Bitmap, Filename);
    if (StrSame (StrFindFileExtension (Filename), ".png"))
      return SavePNG (Window, Bitmap, Filename);
    if (StrSame (StrFindFileExtension (Filename), ".jpg") || StrSame (StrFindFileExtension (Filename), ".jpeg"))
      return SaveJPEG (Window, Bitmap, Filename);
    return false;
  }


///////////////////////////////////////////////////////////////////////////////////
//
// SUPPORT

void BitmapCopy (_Bitmap *Source, _Bitmap *Dest, _Rect rSource, _Point pDest)
  {
    int x, y;
    //
    for (y = 0; y < rSource.Height; y++)
      for (x = 0; x < rSource.Width; x++)
        BitmapSetPixel (Dest, pDest.x + x, pDest.y + y, BitmapGetPixel (Source, rSource.x + x, rSource.y + y));
  }

void BitmapCopyScale (_Bitmap *Source, _Bitmap *Dest, _Rect rSource, _Rect rDest)
  {
    int xs, ys, xd, yd;
    int a;
    bool get, put;
    int pixel;
    //
    xs = ys = 0;
    xd = yd = 0;
    get = put = true;
    while (true)
      {
        if (get & put)
          {
            get = false;
            pixel = BitmapGetPixel (Source, xs, ys);
          }
        if (put)
          {
            put = false;
            BitmapSetPixel (Dest, xd, yd, pixel);
          }
        a = (xs+1) * rDest.Width - (xd+1) * rSource.Width;
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
        if (xs >= rSource.Width && get)
          {
            get = put = true;
            xs = 0;
            xd = 0;
            a = (ys+1) * rDest.Height - (yd+1) * rSource.Height;
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
            if (ys >= rSource.Height && get)
              break;
          }
      }
  }

void BitmapFill (_Bitmap *Dest, _Rect Rect, int Pixel)
  {
    int x, y;
    //
    for (y = 0; y < Rect.Height; y++)
      for (x = 0; x < Rect.Width; x++)
        BitmapSetPixel (Dest, x + Rect.x, y + Rect.y, Pixel);
  }

// Stretch keeping aspect ratio
_Point SizeStretch (_Point Size, _Point Space)
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

