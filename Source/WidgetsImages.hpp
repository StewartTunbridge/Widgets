///////////////////////////////////////////////////////////////////////////////
//
// WIDGETS: IMAGE SUPPORT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
///////////////////////////////////////////////////////////////////////////////


// Load / Save BMP, PNG ...

extern _Bitmap* BitmapLoad (_Window *Window, char *Filename);
extern bool BitmapSave (_Window *Window, _Bitmap* Bitmap, char *Filename);

extern void BitmapCopy (_Bitmap *Source, _Bitmap *Dest, _Rect rSource, _Point pDest);
extern void BitmapCopyScale (_Bitmap *Source, _Bitmap *Dest, _Rect rSource, _Rect rDest);
extern void BitmapFill (_Bitmap *Dest, _Rect Rec, int Pixel);

extern _Point SizeStretch (_Point Size, _Point Space);
