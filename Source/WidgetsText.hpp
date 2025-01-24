/////////////////////////////////////////////////////////////////////////
//
// WIDGETS: TEXT
//
// Author: Stewart Tunbridge, Pi Micros
// Email:  stewarttunbridge@gmail.com
// Copyright (c) 2000-20025 Stewart Tunbridge, Pi Micros
//
/////////////////////////////////////////////////////////////////////////

extern int ItalicSlope;
extern bool ForceMonochrome;

// FONT STYLES
#define fsNone 0x00
#define fsBold 0x01
#define fsItalic 0x02
#define fsUnderline 0x04
#define fsOutline 0x08
#define fsShadow 0x10
#define fsFillBG 0x20   // Fill character rectangel with background colour
#define fsRotate 0x40   // Rotate 90 deg CCW
#define fsNoGrayscale 0x80

typedef struct
  {
    byte Style;
    int Colour;
    int ColourBG;   // used for "Blending" & Filling (when Style == fsFillBG)
    // private
    void *Typeface;
    int Size;
    int YAdvance;
    int YOffset;
  } _Font;

typedef bool _TextCallBack (_Window *Window, char **Text, _Font *Font, _Point *Posn, _Point *Size);
extern _TextCallBack *TextCallBack;

extern char *TextCursor;

extern _Font *FontCreate (char *FontName, int Size, byte Style, int Colour);
extern void FontDestroy (_Font *Font);

extern _Point TextSize (_Font *Font, char *Text);
extern int TextIndex (_Font *Font, char *Text, int PosX);

extern void TextRender (_Window *Window, _Font *Font, char *Text, _Point xy);
