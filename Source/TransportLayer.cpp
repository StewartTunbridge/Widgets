///////////////////////////////////////////////////////////////////////////////
//
// TRANSPORT LAYER CLASS
// =====================
// Base class for communication classes
//
///////////////////////////////////////////////////////////////////////////////


#include "TransportLayer.hpp"


_TransportLayer::_TransportLayer (int BufferSize_)
  {
    BufferSize = BufferSize_;
    BufferIn = new TRingBuffer (BufferSize);
    BufferOut = new TRingBuffer (BufferSize);
    Status = 0;
  }

_TransportLayer::~_TransportLayer (void)
  {
    delete BufferIn;
    delete BufferOut;
  }
