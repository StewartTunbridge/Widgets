///////////////////////////////////////////////////////////////////////////////
//
// COLLECTION CLASS


typedef int *DataItem;

class _Collection
  {
    private:
      DataItem *Data;
      int DataAlloc;
      bool SizeCheck (int Index);
    public:
      int DataSize;
      _Collection (void);
      ~_Collection (void);
      void *Read (int Index);
      bool Write (int Index, DataItem NewData);
      bool Add (DataItem NewData);
      void Clear (void);
  };


///////////////////////////////////////////////////////////////////////////////
//
// STRINGS CLASS

class _Strings : public _Collection
  {
    public:
      _Strings (void);
      ~_Strings (void);
      bool ReadFromFile (char *Filename);
      bool WriteToFile (char *Filename);
  };


///////////////////////////////////////////////////////////////////////////////
//
// INI FILE CLASS

class _INI : public _Strings
  {
    private:
      char *KeyFind (char *Key, int *Item);
    public:
      _INI (char *Filename);
      ~_INI (void);
      char *Read (char *Key);
      int ReadNum (char *Key, int ValueDefault);
      bool Write (char *Key, char *Value);
      bool WriteNum (char *Key, int Value);
  };

