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

// From Widget.cpp
extern void RenderDrawLine (int Colour, int x1, int y1, int x2, int y2);
extern void RenderFillRect (_Rect Rect, int Colour);
extern void RenderDrawPixelAlpha (int x, int y, int Colour, byte Alpha);
extern _Point TargetBitmapScroll;
extern _Point TargetBitmapSize;

int ItalicSlope = 10;
bool ForceMonochrome = false;

const int FontScale = 64;

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAddInt (const char *St, int i);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect *r);
extern void DebugAddHex (const char *St, int p);

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

bool TextProcessSpecial (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    char *t;
    int TabSize;
    //
    if (TextCallBack)
      if (TextCallBack (Window, Text, Font, Posn, Size))
        return true;
    t = *Text;
    if (*t == tab)
      {
        (*Text)++;
        TabSize = Font->YAdvance * 4;
        if (Posn)
          Posn->x = ((Posn->x / TabSize) + 1) * TabSize;
        if (Size)
          Size->x = ((Size->x / TabSize) + 1) * TabSize;
        return true;
      }
    return false;
  }

_Point FreeTypeMeasureString (char *St, _Font *Font, int *Index = NULL, int Search = -1)   // Size of string
  {
    _Point Size;
    char *St_, *St__;
    int Ch;
    FT_UInt GlyphIndex;
    int dx;
    FT_Face Face;
    int x;
    //
    Face = (FT_Face) Font->Typeface;
    FT_Select_Charmap (Face , ft_encoding_unicode);
    St_ = St;
    Size = {0, 0};
    if (Font && St)
      {
        if (Font->Style & (fsOutline | fsShadow))
          Font->Style |= fsNoGrayscale;
        Size.y = Font->YAdvance;
        while (true)
          {
            x = Size.x;
            St__ = St_;
            if (!TextProcessSpecial (0, &St_, Font, NULL, &Size))
              {
                if ((byte) *St_ < ' ')
                  break;
                dx = CharWidthAdjustment (Font);
                Ch = UTF8Read (&St_);   // Read UTF8 character
                //if (Ch > 0x7F)   // Unicodde
                //  DebugAddHex ("UTFRead", Ch);//####
                if (Ch < 0)   // Invalid UTF8 character
                  break;
                GlyphIndex = FT_Get_Char_Index (Face, Ch);
                if (FT_Load_Glyph (Face, GlyphIndex, LoadMode (Font->Style)))
                  break;
                Size.x += dx + Face->glyph->metrics.horiAdvance / FontScale; //GlyphWidth (Font, Face->glyph)
              }
            if (Search >= 0 && (Size.x + x) / 2 >= Search)
              break;
          }
      }
    else
      DebugAdd ("No Font Loaded");
    if (Index)
      *Index = St__ - St;
    return Size;
  }

int CalcColour (int c1, int c2, int Split)   // Split is 0..255
  {
    if (Split == 0)
      return c1;
    if (Split == 0xFF)
      return c2;
    if (c1 == c2)
      return c1;
    //
    return AlphaBlend (c2, c1, Split);
  }

const int BoxX [] = {-1,  0,  1, -1, 1, -1, 0, 1};
//const int BoxX [] = {-2,  0,  2, -2, 2, -2, 0, 2};
const int BoxY [] = {-1, -1, -1,  0, 0,  1, 1, 1};
//const int BoxY [] = {-2, -2, -2,  0, 0,  2, 2, 2};

void CopyGlyph (_Window *Window, FT_GlyphSlot Glyph, byte Style, _Point xy, int dx, int ColFG, int YOffset)
  {
    byte *BitmapData, alpha;
    int x0, y0, x, y, x_, y_;
    int bx, by;
    int i;
    //
    if (Glyph->bitmap.rows == 0 || Glyph->bitmap.width == 0)
      return;
    bx = Glyph->bitmap.width + dx;
    by = Glyph->bitmap.rows;
    x0 = xy.x + Glyph->metrics.horiBearingX / FontScale;
    y0 = xy.y + YOffset - Glyph->metrics.horiBearingY / FontScale;
    if (Style & fsRotate)
      {
        Swap (bx, by);
        x0 = xy.x + YOffset - Glyph->metrics.horiBearingY / FontScale;
        y0 = xy.y - Glyph->bitmap.width;
      }
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
              {
                x_ = x;
                y_ = y;
                if (Style & fsRotate)
                  {
                    x_ = y;
                    y_ = by - 1 - x;
                  }
                if (Style & fsOutline)
                  for (i = 0; i < SIZEARRAY (BoxX); i++)
                    RenderDrawPixelAlpha (x0 + x_ + BoxX [i], y0 + y_ + BoxY [i], ColFG, alpha);
                else
                  RenderDrawPixelAlpha (x0 + x_, y0 + y_, ColFG, alpha);
              }
          }
        BitmapData += Glyph->bitmap.pitch;
      }
  }

bool FreeTypeRenderString (_Window *Window, char *St, _Font *Font, _Point xy)
  {
    _Point xy0;
    int Ch_;
    FT_UInt GlyphIndex;
    FT_GlyphSlot Glyph;
    int ColourHL;
    int dx, acc, i;
    int x, y;
    //
    xy0 = xy;
    ColourHL = cWhite;
    if (ColourR (Font->Colour) + ColourG (Font->Colour) + ColourB (Font->Colour) > 3 * 255 / 2)
      ColourHL = cBlack;
    acc = Max (1, (Font->Size + 5) / 12); //CharAccentSize (Font);
    while (true)
      {
        if (!TextProcessSpecial (Window, &St, Font, &xy, NULL))
          {
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
            if (FT_Render_Glyph (Glyph, RenderMode (Font->Style))) // FT_Outline_Render
              return false;
            x = xy.x - TargetBitmapScroll.x;
            y = xy.y - TargetBitmapScroll.y;
            i = dx + Glyph->metrics.horiAdvance / FontScale; //GlyphWidth (Font, Glyph) + dx;
            if (x + Font->YAdvance >= 0 && y + Font->YAdvance >= 0 &&
                x < TargetBitmapSize.x && y < TargetBitmapSize.y)
              {
                if (Font->ColourBG >= 0)
                  if (Font->Style & fsRotate)
                    RenderFillRect ({xy.x, xy.y - Glyph->metrics.horiAdvance / FontScale - dx, Font->YAdvance, Glyph->metrics.horiAdvance / FontScale + dx}, Font->ColourBG);
                  else
                    RenderFillRect ({xy.x, xy.y, Glyph->metrics.horiAdvance / FontScale + dx, Font->YAdvance}, Font->ColourBG);
                if (Font->Style & (fsOutline | fsShadow))
                  {
                    if (Font->Style & fsShadow)
                      CopyGlyph (Window, Glyph, Font->Style & ~(fsOutline | fsShadow), {xy.x + acc, xy.y + acc}, dx, ColourHL, Font->YOffset);
                    if (Font->Style & fsOutline)
                      CopyGlyph (Window, Glyph, Font->Style, {xy.x, xy.y}, dx, ColourHL, Font->YOffset);
                  }
                CopyGlyph (Window, Glyph, Font->Style & ~(fsOutline | fsShadow), {xy.x, xy.y}, dx, Font->Colour, Font->YOffset);
                if (Font->Style & fsUnderline)
                  if (Font->Style & fsRotate)
                    RenderDrawLine (Font->Colour, xy.x + Font->YOffset + 1, xy.y, xy.x + Font->YOffset + 1, xy.y - i);
                  else
                    RenderDrawLine (Font->Colour, xy.x, xy0.y + Font->YOffset + 1, xy.x + i, xy.y + Font->YOffset + 1);
              }
            if (Font->Style & fsRotate)
              xy.y -= i;
            else
              xy.x += i;
          }
      }
    return true;
  }


/////////////////////////////////////////////////////////////////////////
//
// Interface to the Outside

// Font Create / Destroy
//

int FontCount = 0;

_Font *FontCreate (char *Filename, int Size, byte Style)//, int Colour)
  {
    bool OK;
    char *Filename_;
    _Font *Font;
    //
    OK = false;
    Filename_ = NULL;
    if (!FileExists (Filename))
      {
        Filename_ = ResourcePathAdd (Filename);
        Filename = Filename_;
      }
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
    if (FT_New_Face (FreeTypeLibrary, Filename, 0, (FT_Face *) &Font->Typeface) == 0)
      if (FT_Set_Pixel_Sizes ((FT_Face) Font->Typeface, 0, Size) == 0)
        {
          Font->Size = Size;
          Font->Style = fsNone;
          Font->Style = Style;
          Font->Colour = -1;//Colour;
          Font->ColourBG = -1;   // undefined => use Component colour
          Font->YAdvance = ((FT_Face) Font->Typeface)->size->metrics.height / FontScale;
          Font->YOffset = ((FT_Face) Font->Typeface)->size->metrics.ascender / FontScale - 1;  // -1 gives better centering
          OK = true;
        }
    free (Filename_);
    if (OK)
      return Font;
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
    _Point Res;
    int Col;
    byte Style;
    //
    Col = font->Colour;
    Style = font->Style;
    Res = FreeTypeMeasureString (Text, font);
    font->Colour = Col;
    font->Style = Style;
    return Res;
  }

int TextIndex (_Font *Font, char *Text, int PosX)
  {
    int Index;
    int Col;
    byte Style;
    //
    if (PosX <= 0)
      return 0;
    Col = Font->Colour;
    Style = Font->Style;
    FreeTypeMeasureString (Text, Font, &Index, PosX);
    Font->Colour = Col;
    Font->Style = Style;
    return Index;
  }

void TextRender (_Window *Window, _Font *Font, char *Text, _Point xy)
  {
    //RenderFillRect (Window, {xy.x-1, xy.y-1, 3, 3}, cRed);  //####
    if (Font && Text)
      FreeTypeRenderString (Window, Text, Font, xy);
  }
