#ifndef _IPXSERVER_H_
#define _IPXSERVER_H_

#include "config.h"
#include "SDL_net.h"

struct packetBuffer {
	Bit8u buffer[1024];
	Bit16s packetSize;  // Packet size remaining in read
	Bit16s packetRead;  // Bytes read of total packet
	bool inPacket;      // In packet reception flag
	bool connected;		// Connected flag
	bool waitsize;
    unsigned int timeout;
};

/* Historically 213 (the DOSBox IPX-tunnel convention), but that's a
   privileged port on Unix-like systems (macOS/Tiger/IRIX all require
   root to bind < 1024) -- moved to an unprivileged default so the
   server/launcher never need su/sudo. Still overridable via argv[1]. */
#define DEFAULT_IPX_PORT 21300

/* Fixed, unprivileged UDP port the LAN-discovery responder listens on --
   independent of the (user-configurable) game port above, so a launcher's
   Join dialog can find a server without knowing its game port in advance.
   See DiscoveryServerLoop() in AAServer.cpp. */
#define DEFAULT_DISCOVERY_PORT 21399
#define DISCOVERY_REQUEST_MAGIC  "AASERVER_DISCOVER"
#define DISCOVERY_REPLY_PREFIX   "AASERVER_HERE:"

#define SOCKETTABLESIZE 256
#define CONVIP(hostvar) hostvar & 0xff, (hostvar >> 8) & 0xff, (hostvar >> 16) & 0xff, (hostvar >> 24) & 0xff
#define CONVIPX(hostvar) hostvar[0], hostvar[1], hostvar[2], hostvar[3], hostvar[4], hostvar[5]


void IPX_StopServer();
bool IPX_StartServer(Bit16u portnum);
bool IPX_isConnectedToServer(Bits tableNum, IPaddress ** ptrAddr);

Bit8u packetCRC(Bit8u *buffer, Bit16u bufSize);

#endif
