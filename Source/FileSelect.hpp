////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SELECT FILE
//
////////////////////////////////////////////////////////////////////////////////////////////////////

// FileSelect ()
// -------------
// Create a form for user to select a file from available files and explore directories
// If a file is selected, ActionFileSelected () will be called with the filepath in Result.
// Filter:
//   If not NULL, this must be included in any filename for it to be listed
//   If it starts with a dot (.) then this will be added to a typed filename if no extension exists
// SaveFile:
//   false => Lists existing files only, presumably for reading
//   true => Adds an edit field for creating new files, resumably for writing


typedef void (*_ActionResultChars) (char *Result);   // callback on file selection
extern void FileSelect (char *Title, char *Filter, bool SaveFile, _ActionResultChars ActionFileSelected);

extern bool FileSelectActive;   // true if form currently exists

extern int DirItemsCompare (_DirItem *Data1, _DirItem *Data2);   // Used by ListSort (). Can be used elsewhere. Sorts by IsDir then Name
