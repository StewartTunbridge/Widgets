///////////////////////////////////////////////////////////////////////////////
//
// TCP/IP SERVER / CLIENT CLASS
//
///////////////////////////////////////////////////////////////////////////////
//
// History
// -------
// 19 Apr 2017 System clock: Remove SDL references
// 23 Jul 2021 Tidy. Independent of SDL


#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <utime.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <string.h>

#include "TCP.hpp"
//#include "stringlib.hpp"

extern void DebugAdd (const char *Message);

extern void DebugAddError (const char *Tag);

void _TCP::Open (void)
  {
    bool OK;
    const static int True = 1;
    //
    OK = false;
    if (HostIP == NULL || HostIP [0] == 0)   // Server
      {
        DeviceSocket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (DeviceSocket >= 0)
          {
            setsockopt (DeviceSocket, SOL_SOCKET, SO_REUSEADDR, &True, sizeof (True));   // Make Port available on closing
            memset (&SocketAddr, '0', sizeof (SocketAddr));
            SocketAddr.sin_family = AF_INET;
            SocketAddr.sin_addr.s_addr = htonl (INADDR_ANY);
            SocketAddr.sin_port = htons (Port);
            if (bind (DeviceSocket, (struct sockaddr*) &SocketAddr, sizeof (SocketAddr)) == 0)
              if (listen (DeviceSocket, 10) == 0)
                {
                  Status |= StatusServing;
                  OK = true;
                }
          }
      }
    else   // Client
      {
        DeviceSocket = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (DeviceSocket >= 0)
          {
            memset (&SocketAddr, '0', sizeof (SocketAddr));
            SocketAddr.sin_family = AF_INET;
            SocketAddr.sin_port = htons (Port);
            if (inet_pton (AF_INET, HostIP, &SocketAddr.sin_addr) >= 0)
              {
                Status |= StatusConnecting;
                OK = true;
              }
          }
      }
    if (!OK)
      {
        close (DeviceSocket);
        DeviceSocket = -1;
        DebugAddError ("TCP::Open");
      }
    RetryTick = ClockMS ();
    BufferIn->AddTime = RetryTick;
    BufferOut->AddTime = RetryTick;
  }

void _TCP::Close (void)
  {
    if (DeviceFile >= 0)
      close (DeviceFile);
    DeviceFile = -1;
    if (DeviceSocket >= 0)
      close (DeviceSocket);
    DeviceSocket = -1;
    Status = 0x00;
  }

void _TCP::Reconnect (void)
  {
    if (HostIP && HostIP [0])   // Client
      {
        Close ();
        Open ();
      }
    else   // Server
      {
        if (DeviceFile >= 0)   // Old connection must go
          close (DeviceFile);
        DeviceFile = -1;
      }
  }

_TCP::_TCP (int BufferSize) : _TransportLayer (BufferSize)
  {
    HostIP = NULL;
    Status = 0x00;
    Port = -1;
    DeviceSocket = -1;
    DeviceFile = -1;
  }

void _TCP::Configure (const char *Device, int Param)
  {
    if (Port >= 0)
      Close ();
    StrAssignCopy (&HostIP, Device);
    Port = Param;
    //
    Open ();
  }

_TCP::~_TCP (void)
  {
    if (HostIP)
      free (HostIP);
    Close ();
  }

void _TCP::Poll (void)
  {
    int df;
    int flags;
    byte Data [128];
    int DataLen;
    byte Status_;
    //
    Status_ = Status;
    if (HostIP && HostIP [0])   // Client
      {
        if (Status & StatusConnecting)
          if (connect (DeviceSocket, (struct sockaddr *) &SocketAddr, sizeof (SocketAddr)) >= 0)
            {
              DeviceFile = DeviceSocket;
              if (fcntl (DeviceSocket, F_SETFL, O_NONBLOCK) != -1)
                Status = (Status & ~StatusConnecting) | StatusConnected | StatusNewConnection;
            }
          else if (ClockMS () - RetryTick > 3000)   // give up in 3 seconds
            Reconnect ();
      }
    else   // Server
      {
        // Accept new connections
        df = accept (DeviceSocket, (struct sockaddr*) NULL, NULL);
        if (df >= 0)   // New TCP Connection
          {
            if (DeviceFile >= 0)   // Old connection must go
              close (DeviceFile);
            DeviceFile = df;
            if ((flags = fcntl (DeviceFile, F_GETFL, 0)) >= 0)
              if (fcntl (DeviceFile, F_SETFL, flags | O_NONBLOCK) >= 0)
                Status |= StatusConnected | StatusNewConnection;
          }
      }
    if ((Status ^ Status_) & StatusNewConnection)
      DebugAdd ("New Connection");
    if (DeviceFile >= 0)
      {
        // Data available?
        DataLen = read (DeviceFile, Data, sizeof (Data));
        if (DataLen > 0)
          BufferIn->Add (Data, DataLen);
        else if (DataLen < 0 && errno != EAGAIN)
          {
            DebugAddError ("TCP::Poll");//: read: errno = " + CardToStr (errno, 0) + " - " + strerror (errno));
            Reconnect ();
          }
        // Data to go?
        signal (SIGPIPE, SIG_IGN);
        DataLen = BufferOut->ReadBlock (Data, sizeof (Data), false);
        if (DataLen)
          if (write (DeviceFile, Data, DataLen) == DataLen)
            BufferOut->Discard (DataLen);
          else
            if (errno != EAGAIN)
              Reconnect ();
      }
  }
