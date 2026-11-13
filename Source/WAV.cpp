bool SaveBuffWAV (AnsiString FileName, short *Buffer, int Size /*bytes*/)
  {
    int f;
    /////////////////////////////////////////////////////////
    // Open File
    f = FileOpen (FileName, fmOpenWrite);
    if (f >= 0)   // File Exists
      SetEndOfFile ((void *) f);   // truncate to 0 length
    else
      f = FileCreate (FileName);   // Make new file
    if (f < 0)   // Neither was successful
      return false;
    // Write Headers
    // Chunk RIFF
    FileWriteString (f, "RIFF");   // Chunk ID
    FileWriteBytes (f, 12 + 8 + 16 + 8 + Size, 4);   // Chunk Size
    FileWriteString (f, "WAVE");   // Format
    // Sub Chunk Format
    FileWriteString (f, "fmt ");   // SubChunk ID - Format
    FileWriteBytes (f, 16, 4);   // SubChunk Size
    FileWriteBytes (f, 1, 2);   // Format
    FileWriteBytes (f, 2, 2);   // Channels
    FileWriteBytes (f, 44100, 4);   // Sample Rate
    FileWriteBytes (f, 44100 * 4, 4);   // Byte Rate
    FileWriteBytes (f, 4, 2);   // Block Align
    FileWriteBytes (f, 16, 2);   // Bits Per Sample
    // Sub Chunk Data
    FileWriteString (f, "data");   // SubChunk Data
    FileWriteBytes (f, Size, 4);   // SubChunk Size
    FileWrite (f, Buffer, Size);
    FileClose (f);
    return true;
  }
