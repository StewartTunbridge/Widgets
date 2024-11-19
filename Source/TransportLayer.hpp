///////////////////////////////////////////////////////////////////////////////
//
// TRANSPORT LAYER CLASS
// =====================
// Base class for communication classes
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __TransportLayer
#define __TransportLayer

#include "lib.hpp"

#define StatusConnected 0x01
#define StatusServing 0x02
#define StatusNewConnection 0x04
#define StatusConnecting 0x08

class _TransportLayer
  {
    public:
      int BufferSize;
      TRingBuffer *BufferIn;
      TRingBuffer *BufferOut;
      byte Status;
      //
      virtual void Configure (const char *Device, int Param) = 0;
      virtual void Reconnect (void) = 0;
      virtual void Poll (void) = 0;
      //
      _TransportLayer (int BufferSize_);
      ~_TransportLayer (void);
  };

#endif
