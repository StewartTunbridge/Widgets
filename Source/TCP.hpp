///////////////////////////////////////////////////////////////////////////////
//
// TCP/IP SERVER / CLIENT CLASS
// ============================
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef __tcp
#define __tcp

#include "lib.hpp"
#include "TransportLayer.hpp"

#include <netinet/in.h>

class _TCP: public _TransportLayer
  {
    private:
      char *HostIP;
      int Port;
      struct sockaddr_in SocketAddr;
      int DeviceSocket;
      int DeviceFile;
      int RetryTick;
      void Open (void);
      void Close (void);
    public:
      _TCP (int BufferSize);
      ~_TCP (void);
      void Configure (const char *Device, int Param);   // IP (""=server), Port
      void Reconnect (void);
      void Poll (void);
  };

#endif // __tcp
