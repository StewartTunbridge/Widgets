/////////////////////////////////////////////////////////////////////////
//
// WIDGETS: TEXT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2024, 2026 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////


#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb/stb_truetype.h"

#include "WidgetsDriver.hpp"
#include "WidgetsText.hpp"

// From Widget.cpp
extern void RenderDrawLine (int Colour, int x1, int y1, int x2, int y2);
extern void RenderFillRect (_Rect Rect, int Colour);
extern void RenderDrawPixelAlpha (int x, int y, int Colour, byte Alpha);
extern _Point TargetBitmapScroll;
extern _Point TargetBitmapSize;

int ItalicSlope = 10;//%%%%
bool ForceMonochrome = false;

const int FontScale = 64;

extern void DebugAdd (const char *St);
extern void DebugAddS (const char *s1, const char *s2);
extern void DebugAddInt (const char *St, int i);
extern void DebugAddHex (const char *St, int p);
extern void DebugAddP (const char *St, _Point p);
extern void DebugAddR (const char *St, _Rect r);
//extern void DebugAddR (const char *St, _Rect *r);

_TextCallBack *TextCallBack = nullptr;

typedef struct
  {
    byte *FontData;
    stbtt_fontinfo FontInfo;
    float VScale;
  } __Font;

int CharWidthAdjustment (_Font *Font)
  {
    if (Font->Style & fsBold)
      return Max (1, Font->YAdvance / 14);
    return 0;
  }

void TabNext (int *x, _Font *Font)
  {
    int TabSize, TabGap;
    //
    TabSize = Font->YAdvance * 2;
    TabGap = Font->YAdvance / 2;
    *x = (((*x + TabGap) / TabSize) + 1) * TabSize;
  }

bool TextProcessSpecial (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size)
  {
    char *t;
    //
    Font->ColourBG = -1;
    if (TextCallBack)
      if (TextCallBack (Window, Text, Font, Posn, Size))
        return true;
    t = *Text;
    if (*t == '\t')
      {
        (*Text)++;
        if (Posn)
          TabNext (&Posn->x, Font);
        if (Size)
          TabNext (&Size->x, Font);
        return true;
      }
    return false;
  }

_Point MeasureString (char *St, _Font *Font, int *Index = NULL, int Search = -1)   // Size of string
  {
    _Point Size;
    char *St_, *St__;
    __Font *Font_;
    int Ch;
    int Width;
    int dx;
    int x;
    //
    St_ = St;
    Size = {0, 0};
    if (Font && St)
      {
        Font_ = (__Font *) Font->Typeface;
        Size.y = Font->YAdvance;
        while (true)
          {
            x = Size.x;
            St__ = St_;
            if (!TextProcessSpecial (0, &St_, Font, NULL, &Size))
              {
                if (StrLineEnd ((byte) *St_)) //< ' ')
                  break;
                dx = CharWidthAdjustment (Font);
                Ch = UTF8Read (&St_);   // Read UTF8 character
                if (Ch == 0)
                  break;
                if (Ch < 0)   // Invalid UTF8 character
                  Ch = '?';
                  //break;
                if (Ch < ' ')   // control char
                  Ch += '@';   // map to Alpha
                stbtt_GetCodepointHMetrics (&Font_->FontInfo, Ch, &Width, NULL);
                Width = (float) Width * Font_->VScale + 0.5;
                Size.x += dx + Width;
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

const int BoxX [] = {-1,  0,  1, -1, 1, -1, 0, 1};
//const int BoxX [] = {-2,  0,  2, -2, 2, -2, 0, 2};
const int BoxY [] = {-1, -1, -1,  0, 0,  1, 1, 1};
//const int BoxY [] = {-2, -2, -2,  0, 0,  2, 2, 2};

void CopyGlyph (_Window *Window, byte *Bits, _Point BitsSize, byte Style, _Point xy, int dx, int Col)
  {
    byte *BitmapData, alpha;
    int x0, y0, x, y, x_, y_;
    int i;
    //
    if (BitsSize.x == 0 || BitsSize.y == 0)
      return;
    x0 = xy.x;// + Glyph->metrics.horiBearingX / FontScale;
    y0 = xy.y;// - Glyph->metrics.horiBearingY / FontScale;
    /*if (Style & fsRotate)
      {
        Swap (bx, by);
        x0 = xy.x + YOffset - Glyph->metrics.horiBearingY / FontScale;
        y0 = xy.y - Glyph->bitmap.width;
      }*/
    BitmapData = Bits;
    // For every pixel in the Glyph
    for (y = 0; y < BitsSize.y; y++)
      {
        for (x = 0; x < BitsSize.x + dx; x++)
          {
            alpha = 0;
            if (Style & fsBold)
              {
                // Calculate alpha allowing for fsBold
                for (i = 0; i <= dx; i++)
                  if (x - i >= 0 && x - i < BitsSize.x)
                    alpha = Min (0xFF, (int) alpha + (int) BitmapData [x - i]);  // Max (alpha, BitmapData [x - i]);
              }
            else
              if (x < BitsSize.x)
                alpha = BitmapData [x];
            if ((Style & fsNoGrayscale) || ForceMonochrome)
              if (alpha > 128)
                alpha = 255;
              else
                alpha = 0;
            if (alpha > 0)   // coloured pixel
              {
                x_ = x;
                y_ = y;
                /*if (Style & fsRotate)
                  {
                    x_ = y;
                    y_ = by - 1 - x;
                  }*/
                if (Style & fsOutline)
                  for (i = 0; i < SIZEARRAY (BoxX); i++)
                    RenderDrawPixelAlpha (x0 + x_ + BoxX [i], y0 + y_ + BoxY [i], Col, alpha);
                else
                  RenderDrawPixelAlpha (x0 + x_, y0 + y_, Col, alpha);
              }
          }
        if ((Style & fsItalic) && (y % 5 == 0))
          x0--;
        BitmapData += BitsSize.x;
      }
  }

int RenderString (_Window *Window, char *St, _Font *Font, _Point xy)
  {
    __Font *Font_;
    _Point xy0;
    int Ch, Ch_;
    int XAdvance;
    byte *Bits;
    _Point Size, Offset;
    int ColourHL;
    int dx, acc;
    int x, y;
    //
    Font_ = (__Font *) Font->Typeface;
    xy0 = xy;
    ColourHL = cWhite;
    if (ColourR (Font->Colour) + ColourG (Font->Colour) + ColourB (Font->Colour) > 3 * 255 / 2)
      ColourHL = cBlack;
    acc = Max (1, Font->Size / 10); // Offset for Shadow and Underline
    while (true)
      {
        if (!TextProcessSpecial (Window, &St, Font, &xy, NULL))
          {
            if (StrLineEnd ((byte) *St))
              break;
            dx = CharWidthAdjustment (Font);
            Ch = UTF8Read (&St);   // Read UTF8 character
            if (Ch == 0)
              break;
            //if (Ch >= 0x80)   // Unicode  //####
            //  DebugAddHex ("Text Out Unicode", Ch);  //####
            if (Ch < 0)   // Invalid UTF8 character
              Ch = '?';
              //break; // return false;
            Ch_ = Ch;
            if (Ch < ' ')
              {
                Ch += '@';
                Font->Colour ^= 0x808080;
              }
            stbtt_GetCodepointHMetrics (&Font_->FontInfo, Ch, &XAdvance, NULL);
            XAdvance = (float) XAdvance * Font_->VScale + 0.5;
            Bits = stbtt_GetCodepointBitmap (&Font_->FontInfo, 0, Font_->VScale, Ch, &Size.x, &Size.y, &Offset.x, &Offset.y);
            x = xy.x - TargetBitmapScroll.x;
            y = xy.y - TargetBitmapScroll.y;
            if (x + Font->YAdvance >= 0 && y + Font->YAdvance >= 0 &&
                x < TargetBitmapSize.x && y < TargetBitmapSize.y)
              {
                if (Font->ColourBG >= 0)
                  /*if (Font->Style & fsRotate)
                    RenderFillRect ({xy.x, xy.y - Glyph->metrics.horiAdvance / FontScale - dx, Font->YAdvance, Glyph->metrics.horiAdvance / FontScale + dx}, Font->ColourBG);
                  else*/
                    RenderFillRect ({xy.x, xy.y, XAdvance + dx, Font->YAdvance}, Font->ColourBG);
                if (Bits)
                  {
                    if (Font->Style & fsShadow)
                      CopyGlyph (Window, Bits, Size, Font->Style & ~(fsOutline | fsShadow), {xy.x + acc + Offset.x, xy.y + acc + Font->YOffset + Offset.y}, dx, ColourHL);
                    if (Font->Style & fsOutline)
                      CopyGlyph (Window, Bits, Size, Font->Style, {xy.x + Offset.x, xy.y + Font->YOffset + Offset.y}, dx, ColourHL); //Font->Colour);
                      //CopyGlyph (Window, Bits, Size, Font->Style & ~fsOutline, {xy.x + Offset.x, xy.y + Font->YOffset + Offset.y}, dx, ColourHL);
                    CopyGlyph (Window, Bits, Size, Font->Style & ~(fsOutline | fsShadow), {xy.x + Offset.x, xy.y + Font->YOffset + Offset.y}, dx, Font->Colour);
                  }
              }
            free (Bits);
            if (Font->Style & fsUnderline)
              /*if (Font->Style & fsRotate)
                RenderDrawLine (Font->Colour, xy.x + Font->YOffset + 1, xy.y, xy.x + Font->YOffset + 1, xy.y - i);
              else*/
                RenderDrawLine (Font->Colour, xy.x, xy.y + Font->YOffset + acc, xy.x + XAdvance + dx, xy.y + Font->YOffset + acc);
            if (Ch_ < ' ')
              Font->Colour ^= 0x808080;
            /*if (Font->Style & fsRotate)
              xy.y -= i;
            else*/
              xy.x += XAdvance + dx;
          }
      }
    return xy.y - xy0.y;
  }


/////////////////////////////////////////////////////////////////////////
//
// Interface to the Outside

// Font Create / Destroy
//

void FontSizeSet (_Font *Font, int Size)
  {
    __Font *Font_;
    int Size_;
    int asc, dec, lg;
    //
    if (Font)
      {
        // #### DebugAddInt ("FontSizeSet", Size);
        Font_ = (__Font *) Font->Typeface;
        Size_ = Size * 6/5;//%%%% * 5 / 4;
        Font_->VScale = stbtt_ScaleForPixelHeight (&Font_->FontInfo, Size_);
        stbtt_GetFontVMetrics (&Font_->FontInfo, &asc, &dec, &lg);
        Font->Size = Size;
        Font->YAdvance = (asc - dec + lg) * Font_->VScale;  //Size; // %%%% ((FT_Face) Font->Typeface)->size->metrics.height / FontScale;
        Font->YOffset = asc * Font_->VScale; // %%%% ((FT_Face) Font->Typeface)->size->metrics.ascender / FontScale - 1;  // -1 gives better centering
      }
  }

_Font *FontCreate (char *Filename, int Size, byte Style)
  {
    _Font *Font;
    __Font *Font_;
    //
    int File;
    int FontDataSize;
    //
    Font = NULL;
    // Read File
    File = FileOpen_ (Filename, foRead);
    if (File > 0)
      {
        Font = (_Font *) malloc (sizeof (_Font));
        memset (Font, 0, sizeof (_Font));
        Font_ = (__Font *) malloc (sizeof (__Font));
        memset (Font_, 0, sizeof (__Font));
        FontDataSize = FileSize (File);
        Font_->FontData = (byte *) malloc (FontDataSize);
        FileRead (File, Font_->FontData, FontDataSize);
        FileClose (File);
        // Make _Font
        Font->Typeface = Font_;
        stbtt_InitFont (&Font_->FontInfo, Font_->FontData, stbtt_GetFontOffsetForIndex (Font_->FontData, 0));
        FontSizeSet (Font, Size);
        Font->Style = Style;
        Font->Colour = -1;
        Font->ColourBG = -1;   // undefined => use Component colour
      }
    return Font;
  }

void FontDestroy (_Font *Font)
  {
    __Font *Font_;
    //
    if (Font)
      {
        Font_ = (__Font *) Font->Typeface;
        if (Font_)
          {
            free (Font_->FontData);
            free (Font_);
          }
        free (Font);
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
    if (!font)
      return {0, 0};
    Col = font->Colour;
    Style = font->Style;
    Res = MeasureString (Text, font);
    font->Colour = Col;
    font->Style = Style;
    return Res;
  }

int TextIndex (_Font *Font, char *Text, int PosX)
  {
    int Index;
    int Col;
    byte Style;
    int Size;
    //
    if (PosX <= 0)
      return 0;
    Col = Font->Colour;
    Style = Font->Style;
    Size = Font->Size;
    MeasureString (Text, Font, &Index, PosX);
    Font->Colour = Col;
    Font->Style = Style;
    if (Font->Size != Size)   // we have changed the font size ...
      FontSizeSet (Font, Size);   // ... so restore
    return Index;
  }

void TextRender (_Window *Window, _Font *Font, char *Text, _Point xy)
  {
    //RenderFillRect (Window, {xy.x-1, xy.y-1, 3, 3}, cRed);  //####
    if (Font && Text)
      RenderString (Window, Text, Font, xy);
  }

bool TextCharDefined (_Font *Font, int Ch)
  {
    __Font *Font_;
    int i;
    //
    if (Font)
      {
        Font_ = (__Font *) Font->Typeface;
        i = stbtt_FindGlyphIndex (&Font_->FontInfo, Ch);
        if (i)
          return true;
      }
    return false;
  }


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// TEST FUNCTION

const char TestTable [] = " .:ioVM@";

void TTFTest (char *FontFile, int Ch, int Size)
  {
    int File;
    byte *FontData;
    int FontDataSize;
    stbtt_fontinfo Font;
    byte *Bits, *b;
    int Width, Height;
    _Point Offset;
    char St [100];
    int x, y;
    //
    File = FileOpen (FontFile, foRead);
    if (File > 0)
      {
        FontDataSize = FileSize (File);
        FontData = (byte *) malloc (FontDataSize);
        FileRead (File, FontData, FontDataSize);
        stbtt_InitFont (&Font, FontData, stbtt_GetFontOffsetForIndex (FontData, 0));
        Bits = stbtt_GetCodepointBitmap (&Font, 0, stbtt_ScaleForPixelHeight (&Font, Size), Ch, &Width, &Height, &Offset.x, &Offset.y);
        DebugAddR ("TTFTest", {Offset.x, Offset.y, Width, Height}); //####
        b = Bits;
        for (y = 0; y < Height; ++y)
          {
            for (x = 0; x < Width; ++x)
              St [x] = TestTable [*b++ >> 5];
            St [x] = 0;
            DebugAdd (St); //####
          }
        free (Bits);
        free (FontData);
        FileClose (File);
      }
  }
