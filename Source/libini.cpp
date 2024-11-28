///////////////////////////////////////////////////////////////////////////////
//
// COLLECTION CLASS


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libini.hpp"
#include "lib.hpp"


_Collection::_Collection (void)
  {
    Data = NULL;
    DataAlloc = 0;
    DataSize = 0;
  }

_Collection::~_Collection (void)
  {
    int i;
    //
    for (i = 0; i < DataSize; i++)
      free (Data [i]);
    delete[] Data;
  }

bool _Collection::SizeCheck (int Index)
  {
    DataItem *DataNew;
    int DataAllocNew;
    //
    DataAllocNew = DataAlloc;
    while (Index + 1 > DataAllocNew)
      if (DataAllocNew == 0)
        DataAllocNew = 1;
      else
        DataAllocNew = DataAllocNew * 2;
    if (DataAllocNew != DataAlloc)
      {
        DataNew = new DataItem [DataAllocNew];
        if (DataNew == NULL)
          return false;
        memcpy (DataNew, Data, DataAlloc * sizeof (DataItem));
        delete[] Data;
        Data = DataNew;
        DataAlloc = DataAllocNew;
      }
    while (DataSize <= Index)
      Data [DataSize++] = NULL;
    return true;
  }

void *_Collection::Read (int Index)
  {
    if (SizeCheck (Index))
      return Data [Index];
    return NULL;
  }

bool _Collection::Write (int Index, DataItem NewData)
  {
    if (SizeCheck (Index))
      {
        free (Data [Index]);
        Data [Index] = NewData;
        return true;
      }
    return false;
  }

bool _Collection::Add (DataItem NewData)
  {
    return Write (DataSize, NewData);
  }

void _Collection::Clear (void)
  {
    int i;
    //
    for (i = 0; i < DataSize; i++)
      free (Data [i]);
    delete[] Data;
    Data = NULL;
    DataSize = 0;
    DataAlloc = 0;
  }


///////////////////////////////////////////////////////////////////////////////
//
// STRINGS CLASS

_Strings::_Strings (void) : _Collection ()
  {
    //
  }

_Strings::~_Strings (void)
  {
    //
  }

bool _Strings::ReadFromFile (char *Filename)
  {
    int File;
    char * RawData = NULL;
    int RawDataSize = 0;
    // was SDL_RWops *File = SDL_RWFromFile (Filename, "rb");
    File = FileOpen (Filename, foRead);
    if (File > 0)
      {
        // was RawDataSize = SDL_RWseek (File, 0, RW_SEEK_END);
        //     SDL_RWseek (File, 0, RW_SEEK_SET);
        RawDataSize = FileSize (Filename);
        if (RawDataSize > 0)
          {
            RawData = (char *) malloc (RawDataSize);
            if (RawData)
              {
                // was SDL_RWread (File, RawData, RawDataSize, 1);
                //     SDL_RWclose (File);
                FileRead (File, (byte *) RawData, RawDataSize);
                FileClose (File);
                int a = 0;
                int b = 0;
                while (true)
                  {
                    // Read a line
                    while (true)
                      {
                        if (b >= RawDataSize)
                          break;
                        if (RawData [b] == cr || RawData [b] == lf)   // End of line
                          break;
                        b++;
                      }
                    // Build and store string
                    char *s = (char *) malloc (b - a + 1);
                    memcpy (s, &RawData [a], b - a);
                    s [b - a] = '\0';
                    Add ((DataItem) s);
                    // Are we finished
                    if  (b >= RawDataSize)
                      break;
                    // Step past lf/crlf
                    if (RawData [b] == cr)
                      b = b + 2;
                    else
                      b++;
                    // Now read next line
                    a = b;
                  }
                free (RawData);
                return true;
              }
          }
      }
    return false;
  }

char crlf [2] = {cr, lf};

bool _Strings::WriteToFile (char *Filename)
  {
    int File;
    int i;
    char *Line;
    int Len;
    //
    // was File = SDL_RWFromFile (Filename, "w");
    File = FileOpen (Filename, foWrite);
    if (File > 0)
      {
        for (i = 0; i < DataSize; i++)
          {
            Line = (char *) Read (i);
            Len = StrLen (Line);
            // was SDL_RWwrite (File, Line, Len, 1);
            //     SDL_RWwrite (File, crlf, sizeof (crlf), 1);
            FileWrite (File, (byte *) Line, Len);
            FileWrite (File, (byte *) crlf, sizeof (crlf));
          }
        // was SDL_RWclose (File);
        FileClose (File);
        return true;
      }
    return false;
  }


///////////////////////////////////////////////////////////////////////////////
//
// INI FILE CLASS

_INI::_INI (char *Filename) : _Strings ()
  {
    ReadFromFile (Filename);
  }

_INI::~_INI (void)
  {
    //####
    //write if changed
  }

char *_INI::KeyFind (char *Key, int *Index)
  {
    int i;
    char *s;
    //
    i = 0;
    while (true)
      {
        if (i >= DataSize)
          {
            if (Index)
              *Index = -1;
            return NULL;
          }
        s = (char *) _Collection::Read (i);
        if (s)
          {
            int x = 0;
            while (true)
              {
                if (Key [x] == 0)   // End Key
                  if ((*s < ' ') || (*s == '='))   // And End Line Key therefore found
                    {
                      if (*s)   // Step past separator character (tab or equals)
                        s++;
                      // Debug
                      char *s1 = (char *) _Collection::Read (i);
                      char *s2 = (char *) malloc (StrLen (s1) + 16);
                      char *x = s2;
                      StrCat (&x, "INI: ");
                      StrCat (&x, s1);
                      *x = 0;
                      DebugAdd (s2); //, 0x404040);
                      free (s2);
                      //
                      if (Index)
                        *Index = i;
                      return s;
                    }
                  else
                    break;
                if ((*s < ' ') || (*s == '='))   // end line key reached therefore not found
                  break;
                if (UpCase (Key [x]) != UpCase (*s))   // mismatch so not found
                  break;
                x++;
                s++;
              }
          }
        i++;
      }
  }

char *_INI::Read (char *Key)   // Returns a pointer to data inside _INI (do not free)
  {
    return KeyFind (Key, NULL);
  }

int _INI::ReadNum (char *Key, int ValueDefault)
  {
    char *s = KeyFind (Key, NULL);
    if (s)
      return StrGetNum (&s);
    else
      return ValueDefault;
  }

bool _INI::Write (char *Key, char *Value)
  {
    int i;
    char *s, *x;
    //
    s = (char *) malloc (StrLen (Key) + 1 + StrLen (Value) + 1);
    x = s;
    StrCat (&x, Key);
    *x++ = tab;
    StrCat (&x, Value);
    *x = 0;
    KeyFind (Key, &i);
    if (i >= 0)
      return _Collection::Write (i, (DataItem) s);
    return _Collection::Add ((DataItem) s);
  }

bool _INI::WriteNum (char *Key, int Value)
  {
    char St [16];
    char *x;
    //
    x = St;
    NumToStr (&x, Value);
    *x = 0;
    x = NULL;
    StrAssignCopy (&x, St);
    return Write (Key, St);
  }

