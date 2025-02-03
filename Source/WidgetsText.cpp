/////////////////////////////////////////////////////////////////////////
//
// WIDGETS: TEXT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////


#include "WidgetsDriver.hpp"
#include "WidgetsText.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

int ItalicSlope = 10;
bool ForceMonochrome = false;

const int FontScale = 64;

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAdd (const char *St, int n);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect *r);

_TextCallBack *TextCallBack = nullptr;

FT_Library  FreeTypeLibrary;
FT_Matrix Matrix;
FT_Vector Pen;

bool FreeTypeInit (void)
  {
    int err;
    int v1, v2, v3;
    char s [64], *ps;
    //
    err = FT_Init_FreeType (&FreeTypeLibrary);
    if (err)
      {
        DebugAdd ("FT_Init_FreeType ERROR");
        return false;
      }
    FT_Library_Version (FreeTypeLibrary, &v1, &v2, &v3);
    ps = s;
    StrCat (&ps, "FreeType Library: ");
    NumToStr (&ps, v1);
    StrCat (&ps, '.');
    NumToStr (&ps, v2);
    StrCat (&ps, '.');
    NumToStr (&ps, v3);
    *ps = 0;
    DebugAdd (s);
    return true;
  }

bool FreeTypeUninit (void)
  {
    FT_Done_FreeType (FreeTypeLibrary);
    return true;
  }

int CharWidthAdjustment (_Font *Font)
  {
    if (Font->Style & fsBold)
      return Max (1, Font->YAdvance / 10);
    return 0;
  }

int LoadMode (byte Style)
  {
    if (Style & fsNoGrayscale || ForceMonochrome)
      return FT_LOAD_TARGET_MONO;
    return FT_LOAD_DEFAULT;
  }

FT_Render_Mode RenderMode (byte Style)
  {
    if (Style & fsNoGrayscale || ForceMonochrome)
      return FT_RENDER_MODE_MONO;
    return FT_RENDER_MODE_NORMAL;
  }

/*
int GlyphWidth (_Font *Font, FT_GlyphSlot Glyph)
  {
    if (Font->Style & fsNoGrayscale || ForceMonochrome)
      return Glyph->bitmap.width + Max ((int) (Font->YAdvance / 10), 1);
    return Glyph->metrics.horiAdvance / FontScale;
  }
*/

_Point FreeTypeMeasureString (char *St, _Font *Font, int *Index = NULL, int Search = -1)   // Size of string
  {
    _Point Size;
    char *St_, *St__;;
    int Ch_;
    FT_UInt GlyphIndex;
    int dx;
    FT_Face Face;
    int x_, x__;
    //
    Face = (FT_Face) Font->Typeface;
    St__ = St_ = St;
    x__ = x_ = 0;
    Size = {0, 0};
    if (Font && St)
      {
        if (Font->Style & (fsOutline | fsShadow))
          Font->Style |= fsNoGrayscale;
        Size.y = Font->YAdvance;
        while (true)
          {
            if (TextCallBack)
              TextCallBack (0, &St_, Font, NULL, &Size);
            dx = CharWidthAdjustment (Font);
            if ((byte) *St_ < ' ')
              break;
            St__ = St_;
            x__ = Size.x;
            Ch_ = UTF8Read (&St_);   // Read UTF8 character
            if (Ch_ < 0)   // Invalid UTF8 character
              break;
            GlyphIndex = FT_Get_Char_Index (Face, Ch_);
            if (FT_Load_Glyph (Face, GlyphIndex, LoadMode (Font->Style)))
              break;
            //if (FT_Render_Glyph (((FT_Face) Font->Typeface)->glyph, RenderMode (Font->Style)))
            //  break;
              x_ = Size.x + dx + Face->glyph->metrics.horiAdvance / FontScale; //GlyphWidth (Font, Face->glyph)
            if ((Search >= 0) && x_ >= Search)
              break;
            Size.x = x_;
          }
      }
    else
      DebugAdd ("No Font Loaded");
    if (Index)
      if (x_ - Search < Search - x__)   // Pick closest side of the current charater
        *Index = St_ - St;   // Left side
      else
         *Index = St__ - St;   // Right side
    return Size;
  }

int CalcColour (int c1, int c2, int Split)   // Split is 0..255
  {
    int Res, Mask, c1_, c2_, d;
    int n;
    //
    if (Split == 0)
      return c1;
    if (Split == 0xFF)
      return c2;
    if (c1 == c2)
      return c1;
    //
    Res = 0;
    Mask = 0x0000FF;
    n = 3;
    while (n--)
      {
        c1_ = c1 & Mask;
        c2_ = c2 & Mask;
        if (c1_ == c2_)
          d = c1_;
          //Res |= c1_;
        else if (c1_ < c2_)
          d = (c1_ + (c2_ - c1_) * Split / 255) & Mask;
        else   // c1_ > c2_
          d = (c2_ + (c1_ - c2_) * (255 - Split) / 255) & Mask;
        Res |= d;
        Mask <<= 8;
      }
    return Res;
  }

//int DebugCount = 0;
//char DebugFile [] = "######.bmp";
//#include "WidgetsImages.hpp"

const int BoxX [] = {-1,  0,  1, -1, 1, -1, 0, 1};
//const int BoxX [] = {-2,  0,  2, -2, 2, -2, 0, 2};
const int BoxY [] = {-1, -1, -1,  0, 0,  1, 1, 1};
//const int BoxY [] = {-2, -2, -2,  0, 0,  2, 2, 2};

void CopyGlyph (_Window *Window, FT_GlyphSlot Glyph, byte Style, _Point xy, int dx, int ColBG, int ColFG, int YOffset)
  {
    byte *BitmapData, alpha;
    int x, y, x_, y_;
    int bx, by;
    int Pixel;
    int i;
    _Bitmap *bmp;
    //
    if (Glyph->bitmap.rows == 0 || Glyph->bitmap.width == 0)
      return;
    bx = Glyph->bitmap.width + dx;
    by = Glyph->bitmap.rows;
    if (Style & fsRotate)
      Swap (bx, by);
    bmp = BitmapCreate (Window, bx, by);
    /*for (y = 0; y < by; y++)
      for (x = 0; x < bx; x++)
        BitmapSetPixel (bmp, x, y, ColBG);*/
    BitmapData = Glyph->bitmap.buffer;
    // For every pixel in the Glyph
    for (y = 0; y < Glyph->bitmap.rows; y++)
      {
        for (x = 0; x < Glyph->bitmap.width + dx; x++)
          {
            // Calculate alpha allowing for fsBold
            alpha = 0;
            for (i = 0; i <= dx; i++)
              if (x - i >= 0 && x - i < Glyph->bitmap.width)
                if (Style & fsNoGrayscale || ForceMonochrome)   // black / white rendering
                  {
                    if (BitmapData [(x - i) >> 3] & Bit [7 - ((x - i) & 7)])
                      alpha = 255;
                  }
                else   // full alpha blend
                  if (BitmapData [x - i] > alpha)
                    alpha = BitmapData [x - i];
            if (alpha > 0)   // coloured pixel
              Pixel = CalcColour (ColBG, ColFG, alpha);   // Calculate pixel based on alpha and plot it
            else
              Pixel = ColBG;
            // Calculate pixel position on bmp
            if (Style & fsRotate)
              {
                x_ = y;
                y_ = by - 1 - x;
              }
            else
              {
                x_ = x;
                y_ = y;
              }
            BitmapSetPixel (bmp, x_, y_, Pixel);
          }
        BitmapData += Glyph->bitmap.pitch;
      }
    /***** char *c = DebugFile;
    NumToStr (&c, DebugCount++, 6 | DigitsZeros);
    BitmapSave (Window, bmp, DebugFile);
    */
    x = xy.x + Glyph->metrics.horiBearingX / FontScale;;
    y = xy.y + YOffset - Glyph->metrics.horiBearingY / FontScale;
    if (Style & fsRotate)
      {
        x = xy.x + YOffset - Glyph->metrics.horiBearingY / FontScale;
        y = xy.y - Glyph->bitmap.width;
      }
    if (Style & fsFillBG)
      ColBG = -1;
    if (x + bx >= 0 && y + by >= 0)
      {
        RenderBitmap (Window, bmp, {0, 0, bx, by}, {x, y, bx, by}, ColBG);
        if (Style & fsOutline)
          for (i = 0; i < SIZEARRAY (BoxX); i++)
            RenderBitmap (Window, bmp, {0, 0, bx, by}, {x + BoxX [i], y + BoxY [i], bx, by}, ColBG);
      }
    BitmapDestroy (bmp);
  }

bool FreeTypeRenderString (_Window *Window, char *St, _Font *Font, _Point xy)
  {
    _Point xy0;
    int Ch_;
    FT_UInt GlyphIndex;
    FT_GlyphSlot Glyph;
    int ColourHL;
    int dx, acc, i;
    //
    xy0 = xy;
    ColourHL = cWhite;
    if (ColourR (Font->Colour) + ColourG (Font->Colour) + ColourB (Font->Colour) > 3 * 255 / 2)
      ColourHL = cBlack;
    acc = Max (1, (Font->Size + 5) / 16); //CharAccentSize (Font);
    while (true)
      {
        if (TextCallBack)
          TextCallBack (Window, &St, Font, &xy, NULL);
        if (Font->Style & (fsOutline | fsShadow))
          Font->Style |= fsNoGrayscale;
        if (Font->Style & fsItalic)
          FT_Set_Transform ((FT_Face) Font->Typeface, &Matrix, &Pen);
        else
          FT_Set_Transform ((FT_Face) Font->Typeface, NULL, &Pen);
        dx = CharWidthAdjustment (Font);
        if ((byte) *St < ' ')
          break;
        Ch_ = UTF8Read (&St);   // Read UTF8 character
        if (Ch_ < 0)   // Invalid UTF8 character
          return false;
        GlyphIndex = FT_Get_Char_Index ((FT_Face) Font->Typeface, Ch_);
        if (FT_Load_Glyph ((FT_Face) Font->Typeface, GlyphIndex, LoadMode (Font->Style)))
          return false;
        Glyph = ((FT_Face) Font->Typeface)->glyph;
        if (FT_Render_Glyph (Glyph, RenderMode (Font->Style)))
          return false;
        if (Font->Style & fsFillBG)
          if (Font->Style & fsRotate)
            RenderFillRect (Window, {xy.x, xy.y - Glyph->metrics.horiAdvance / FontScale - dx, Font->YAdvance, Glyph->metrics.horiAdvance / FontScale + dx}, Font->ColourBG);
          else
            RenderFillRect (Window, {xy.x, xy.y, Glyph->metrics.horiAdvance / FontScale + dx, Font->YAdvance}, Font->ColourBG);
        if (Font->Style & fsOutline)
          {
            CopyGlyph (Window, Glyph, Font->Style, {xy.x, xy.y}, dx, Font->ColourBG, ColourHL, Font->YOffset);
            CopyGlyph (Window, Glyph, Font->Style & ~fsOutline, {xy.x, xy.y}, dx, ColourHL, Font->Colour, Font->YOffset);
          }
        else
          {
            if (Font->Style & fsShadow)
              CopyGlyph (Window, Glyph, Font->Style, {xy.x + acc, xy.y + acc}, dx, Font->ColourBG, ColourHL, Font->YOffset);
            CopyGlyph (Window, Glyph, Font->Style, {xy.x, xy.y}, dx, Font->ColourBG, Font->Colour, Font->YOffset);
          }
        i = dx + Glyph->metrics.horiAdvance / FontScale; //GlyphWidth (Font, Glyph) + dx;
        if (Font->Style & fsUnderline)
          if (Font->Style & fsRotate)
            RenderDrawLine (Window, Font->Colour, xy.x + Font->YOffset + 1, xy.y, xy.x + Font->YOffset + 1, xy.y - i);
          else
            RenderDrawLine (Window, Font->Colour, xy.x, xy0.y + Font->YOffset + 1, xy.x + i, xy.y + Font->YOffset + 1);
        if (Font->Style & fsRotate)
          xy.y -= i;
        else
          xy.x += i;
      }
    return true;
  }


/////////////////////////////////////////////////////////////////////////
//
// Interface to the Outside

// Font Create / Destroy
//

int FontCount = 0;

_Font *FontCreate (char *FontName, int Size, byte Style, int Colour)
  {
    _Font *Font;
    //
    if (FontCount++ == 0)
      {
        FreeTypeInit ();
        // Define a transformation matrix for italic characters
        Matrix.xx = (FT_Fixed) (CosDeg (ItalicSlope) * 0x10000L);
        Matrix.xy = (FT_Fixed) (SinDeg (ItalicSlope) * 0x10000L);
        Matrix.yx = 0;
        Matrix.yy = 0x10000L;
      }
    Font = (_Font *) malloc (sizeof (_Font));
    if (FT_New_Face (FreeTypeLibrary, FontName, 0, (FT_Face *) &Font->Typeface) == 0)
      if (FT_Set_Pixel_Sizes ((FT_Face) Font->Typeface, 0, Size) == 0)
        {
          Font->Size = Size;
          Font->Style = fsNone;
          Font->Style = Style;
          Font->Colour = Colour;
          Font->ColourBG = -1;   // undefined => use Component colour
          Font->YAdvance = ((FT_Face) Font->Typeface)->size->metrics.height / FontScale;
          Font->YOffset = ((FT_Face) Font->Typeface)->size->metrics.ascender / FontScale;
          return Font;
        }
    // Invalid FontName
    free (Font);
    if (--FontCount == 0)
      FreeTypeUninit ();
    return NULL;
  }

void FontDestroy (_Font *font)
  {
    if (font)
      {
        FT_Done_Face ((FT_Face) font->Typeface);
        //XFreeFont (((__Window *) Window)->XDisplay, font_->fs);
        free (font);
        if (--FontCount == 0)
          FreeTypeUninit ();
      }
  }

// Measure / Render
//

_Point TextSize (_Font *font, char *Text)
  {
    return FreeTypeMeasureString (Text, font);
  }

int TextIndex (_Font *Font, char *Text, int PosX)
  {
    int Index;
    //
    if (PosX <= 0)
      return 0;
    FreeTypeMeasureString (Text, Font, &Index, PosX);
    return Index;
  }

void TextRender (_Window *Window, _Font *Font, char *Text, _Point xy)
  {
    //RenderFillRect (Window, {xy.x-1, xy.y-1, 3, 3}, cRed);  //####
    if (Font && Text)
      FreeTypeRenderString (Window, Text, Font, xy);
  }
