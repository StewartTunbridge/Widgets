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

#define TextSmooth   // Approximates blended Text

#define FontScale 64

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAdd (const char *St, int n);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect *r);

_TextCallBack *TextCallBack = nullptr;

FT_Library  FreeTypeLibrary;

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
    if (Font)
      {
        Size.y = Font->YAdvance;
        dx = CharWidthAdjustment (Font);
        while (true)
          {
            if (TextCallBack)
              TextCallBack (0, &St_, Font, NULL, &Size);
            if ((byte) *St_ < ' ')
              break;
            St__ = St_;
            x__ = Size.x;
            Ch_ = UTF8Read (&St_);   // Read UTF8 character
            if (Ch_ < 0)   // Invalid UTF8 character
              break;
            GlyphIndex = FT_Get_Char_Index (Face, Ch_);
            if (FT_Load_Glyph (Face, GlyphIndex, FT_LOAD_DEFAULT))
              break;
            x_ = Size.x + Face->glyph->metrics.horiAdvance / FontScale + dx;
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
    #ifdef TextSmooth
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
          //Res |= (c1_ + (c2_ - c1_) * Split / 255) & Mask;
        else   // c1_ > c2_
          d = (c2_ + (c1_ - c2_) * (255 - Split) / 255) & Mask;
          //Res |= (c2_ + (c1_ - c2_) * (255 - Split) / Split) & Mask;
        Res |= d;
        Mask <<= 8;
      }
    #else
    if (Split < 0x80)
      Res = c1;
    else
      Res = c2;
    #endif // TextSmooth
    return Res;
  }

#define opBox     0x01
#define opOblique 0x02
#define opVert    0x04
#define opNoGrey  0x08

void CopyGlyph (_Window *Window, FT_GlyphSlot Glyph, _Point xy, int dx, byte op, int ColBG, int ColFG, bool Rotate, int YOffset)
  {
    byte *BitmapData, alpha;
    int x, y, x_, y_;
    int Pixel;
    int i;
    //
    BitmapData = Glyph->bitmap.buffer;
    if (xy.x + (int) Glyph->bitmap.width >= 0)
      for (y = 0; y < Glyph->bitmap.rows; y++)
        {
          for (x = 0; x < Glyph->bitmap.width + dx; x++)
            {
              // Calculate alpha allowing for fsBold
              alpha = 0;
              for (i = 0; i <= dx; i++)
                if (x - i >= 0 && x - i < Glyph->bitmap.width)
                  if (BitmapData [x - i] > alpha)
                    alpha = BitmapData [x - i];
              // Calculate pixel based on alpha and plot it
              if (alpha > 0x00)
                {
                  x_ = xy.x + x + Glyph->metrics.horiBearingX / FontScale;
                  y_ = xy.y + y + YOffset - Glyph->metrics.horiBearingY / FontScale;
                  if (op & opOblique)
                    x_ += (YOffset - y) / 4;
                  if (Rotate)
                    {
                      x_ = xy.x + y + YOffset - Glyph->metrics.horiBearingY / FontScale;
                      y_ = xy.y - x - Glyph->metrics.horiBearingX / FontScale;
                      if (op & opOblique)
                        y_ -= (YOffset - y) / 4;
                    }
                  if (op &opNoGrey)
                    Pixel = (alpha > 0x40) ? ColFG : ColBG;
                  else
                    Pixel = CalcColour (ColBG, ColFG, alpha);
                  if (op & opBox)
                    RenderFillRect (Window, {x_-1, y_-1, 3, 3}, Pixel);
                  else
                    RenderDrawPoint (Window, Pixel, x_, y_);
                }
            }
          BitmapData += Glyph->bitmap.width;
        }
  }

bool FreeTypeRenderString (_Window *Window, char *St, _Font *Font, _Point xy)
  {
    _Point xy0;
    int Ch_;
    FT_UInt GlyphIndex;
    FT_GlyphSlot Glyph;
    int ColourHL;
    int ColourBG;
    int dx, acc;
    byte op;
    //
    xy0 = xy;
    ColourHL = cWhite;
    if (ColourR (Font->Colour) + ColourG (Font->Colour) + ColourB (Font->Colour) > 3 * 255 / 2)
      ColourHL = cBlack;
    ColourBG = Font->ColourBG;
    if (Font->Style & (fsOutline)) //fsShadow |
      ColourBG = ColourHL;
    dx = CharWidthAdjustment (Font);
    acc = Max (1, (Font->Size + 5) / 16); //CharAccentSize (Font);
    while (true)
      {
        if (TextCallBack)
          TextCallBack (Window, &St, Font, &xy, NULL);
        if ((byte) *St < ' ')
          break;
        Ch_ = UTF8Read (&St);   // Read UTF8 character
        if (Ch_ < 0)   // Invalid UTF8 character
          return false;
        GlyphIndex = FT_Get_Char_Index ((FT_Face) Font->Typeface, Ch_);
        if (FT_Load_Glyph ((FT_Face) Font->Typeface, GlyphIndex, FT_LOAD_DEFAULT /*| FT_LOAD_TARGET_MONO*/))
          return false;
        Glyph = ((FT_Face) Font->Typeface)->glyph;
        if (FT_Render_Glyph (((FT_Face) Font->Typeface)->glyph, FT_RENDER_MODE_NORMAL))
          return false;
        if (Font->Style & fsFillBG)
          if (Font->Style & fsRotate)
            RenderFillRect (Window, {xy.x, xy.y - Glyph->metrics.horiAdvance / FontScale + dx, Font->YAdvance, Glyph->metrics.horiAdvance / FontScale + dx}, Font->ColourBG);
          else
            RenderFillRect (Window, {xy.x, xy.y, Glyph->metrics.horiAdvance / FontScale + dx, Font->YAdvance}, Font->ColourBG);
        op = 0x00;
        if (Font->Style & fsItalic)
          op = opOblique;
        //if (Font->Style & (fsShadow | fsOutline))
        //  op |= opNoGrey;
        if (Font->Style & fsShadow)
          CopyGlyph (Window, Glyph, {xy.x + acc, xy.y + acc}, dx, op | opNoGrey, ColourBG, ColourHL, Font->Style & fsRotate, Font->YOffset);
        if (Font->Style & fsOutline)
          CopyGlyph (Window, Glyph, {xy.x, xy.y}, dx, op | opNoGrey | opBox, ColourBG, ColourHL, Font->Style & fsRotate, Font->YOffset);
        CopyGlyph (Window, Glyph, {xy.x, xy.y}, dx, op, ColourBG, Font->Colour, Font->Style & fsRotate, Font->YOffset);
        if (Font->Style & fsRotate)
          xy.y -= Glyph->metrics.horiAdvance / FontScale + dx;
        else
          xy.x += Glyph->metrics.horiAdvance / FontScale + dx;
      }
    if (Font->Style & fsUnderline)
      if (Font->Style & fsRotate)
        RenderDrawLine (Window, Font->Colour, xy0.x + Font->YOffset + 1, xy0.y, xy.x + Font->YOffset + 1, xy.y);
      else
        RenderDrawLine (Window, Font->Colour, xy0.x, xy0.y + Font->YOffset + 1, xy.x, xy.y + Font->YOffset + 1);
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
      FreeTypeInit ();
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
    if (Font && Text)
      FreeTypeRenderString (Window, Text, Font, xy);
  }
