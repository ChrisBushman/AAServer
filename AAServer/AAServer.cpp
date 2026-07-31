// AAServer.cpp : Defines the entry point for the console application.
//

#ifdef TARGET_UNIX
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#else
#include "stdafx.h"
#include <windows.h>
#endif
#include "config.h"
#include "ipxserver.h"
#include <stdlib.h>
#include <string.h>
#include "ipx.h"
#include <time.h>
#if defined(TARGET_UNIX) && defined(AA_REAL_SDL12)
/* Real SDL 1.2 (e.g. Tigerbrew on PPC/Tiger, Nekoware on IRIX) installs
   headers at the classic bare path, not namespaced under SDL2/ like
   Homebrew's sdl12-compat + SDL2 combo on modern macOS. */
#include <SDL.h>
#elif defined(TARGET_UNIX)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif
extern "C" {
void PacketPrint(void *aData, unsigned int aSize);
}

#define CONNECT_TIMEOUT     900 // seconds
IPaddress ipxServerIp;  // IPAddress for server's listening port
UDPsocket ipxServerSocket;  // Listening server socket
UDPsocket discoverySocket;  // LAN-discovery responder socket (see DiscoveryServerLoop)
static Bit16u g_serverPort;  // portnum as passed to IPX_StartServer, host byte order

#define DEBUG_PACKETS 0

packetBuffer connBuffer[SOCKETTABLESIZE];

Bit8u inBuffer[IPXBUFFERSIZE];
IPaddress ipconn[SOCKETTABLESIZE];  // Active TCP/IP connection 
UDPsocket tcpconn[SOCKETTABLESIZE];  // Active TCP/IP connections
SDLNet_SocketSet serverSocketSet;
//TIMER_TickHandler* serverTimer;

void UnpackIP(PackedIP ipPack, IPaddress * ipAddr)
{
    ipAddr->host = ipPack.host;
    ipAddr->port = ipPack.port;
}

void PackIP(IPaddress ipAddr, PackedIP *ipPack)
{
    ipPack->host = ipAddr.host;
    ipPack->port = ipAddr.port;
}

Bit8u packetCRC(Bit8u *buffer, Bit16u bufSize)
{
    Bit8u tmpCRC = 0;
    Bit16u i;
    for (i = 0; i < bufSize; i++) {
        tmpCRC ^= *buffer;
        buffer++;
    }
    return tmpCRC;
}

/*
 static void closeSocket(Bit16u sockidx) {
 Bit32u host;

 host = ipconn[sockidx].host;
 LOG_MSG("IPXSERVER: %d.%d.%d.%d disconnected\n", CONVIP(host));

 SDLNet_TCP_DelSocket(serverSocketSet,tcpconn[sockidx]);
 SDLNet_TCP_Close(tcpconn[sockidx]);
 connBuffer[sockidx].connected = false;
 connBuffer[sockidx].waitsize = false;
 }
 */

static void sendIPXPacket(Bit8u *buffer, Bit16s bufSize)
{
    Bit16u srcport, destport;
    Bit32u srchost, desthost;
    Bit16u i;
    Bits result;
    UDPpacket outPacket;
    outPacket.channel = -1;
    outPacket.data = buffer;
    outPacket.len = bufSize;
    outPacket.maxlen = bufSize;
    IPXHeader *tmpHeader;
    tmpHeader = (IPXHeader *)buffer;

    srchost = tmpHeader->src.addr.byIP.host;
    desthost = tmpHeader->dest.addr.byIP.host;

    srcport = tmpHeader->src.addr.byIP.port;
    destport = tmpHeader->dest.addr.byIP.port;

    if (bufSize > 120)
        bufSize = 120; // clip test
#if 0
    printf("IPX packet IN : (%d.%d.%d.%d:%d) (len:%d) [", CONVIP(srchost),
            srcport, bufSize);
    for (i = 0; i < bufSize; i++) {
        if (i == sizeof(IPXHeader)+4)
            printf("| ");
        printf("%02X ", buffer[i]);
    }
    printf("]\n");
    fflush(stdout);
#endif
#if DEBUG_PACKETS
    PacketPrint(buffer+sizeof(IPXHeader)+4, bufSize-(sizeof(IPXHeader)+4));
#endif

    if (desthost == 0xffffffff) {
        // Broadcast
        for (i = 0; i < SOCKETTABLESIZE; i++) {
            if (connBuffer[i].connected) {
#if DEBUG_PACKETS
                printf("    %d) ", i);
#endif
                if ((ipconn[i].host == srchost)
                        && (ipconn[i].port == srcport)) {
#if DEBUG_PACKETS
                    printf("SELF\n");
#endif
                    connBuffer[i].timeout = CONNECT_TIMEOUT;
                } else {
                    outPacket.address = ipconn[i];
#if DEBUG_PACKETS
                    printf("IPX bpacket OUT: (%d.%d.%d.%d:%d) [", 
                            CONVIP(outPacket.address.host),
                            outPacket.address.port);
                    for (j = 0; j < outPacket.len; j++) {
                        if (j == sizeof(IPXHeader))
                            printf("| ");
                        printf("%02X ", outPacket.data[j]);
                    }
                    printf("]\n");
                    fflush(stdout);
#endif
                    result = SDLNet_UDP_Send(ipxServerSocket, -1, &outPacket);
                    if (result == 0) {
                        LOG_MSG("IPXSERVER: %s\n", SDLNet_GetError());
                    }
                    //LOG_MSG("IPXSERVER: Packet of %d bytes sent from %d.%d.%d.%d to %d.%d.%d.%d (BROADCAST) (%x CRC)\n", bufSize, CONVIP(srchost), CONVIP(ipconn[i].host), packetCRC(&buffer[30], bufSize-30));
                }
            }
        }
    fflush(stdout);
    } else {
        // Specific address
        for (i = 0; i < SOCKETTABLESIZE; i++) {
            if (connBuffer[i].connected) {
                //printf("    %d) ", i);
                // Is this us in the list or someone else?
                if ((ipconn[i].host == srchost)
                        && (ipconn[i].port == srcport)) {
                    // We're sending a packet.  Good.  Don't send to ourself,
                    // but update the timeout.
                    //printf("SELF\n");
                    connBuffer[i].timeout = CONNECT_TIMEOUT;
                } else {
                    // This is someone else.  Is this where we need to send a packet?
                    if ((ipconn[i].host == desthost) && (ipconn[i].port == destport)) {
                        outPacket.address = ipconn[i];
#if DEBUG_PACKETS
                        printf("    %d) IPX rpacket OUT: (%d.%d.%d.%d:%d) [", i,
                                CONVIP(outPacket.address.host),
                                outPacket.address.port);
                        for (j = 0; j < outPacket.len; j++) {
                            if (j == sizeof(IPXHeader))
                                printf("| ");
                            printf("%02X ", outPacket.data[j]);
                        }
                        printf("]\n");
                        fflush(stdout);
#endif
                        result = SDLNet_UDP_Send(ipxServerSocket, -1, &outPacket);
                        if (result == 0) {
                            LOG_MSG("IPXSERVER: %s\n", SDLNet_GetError());
                        }
                        //LOG_MSG("IPXSERVER: Packet sent from %d.%d.%d.%d to %d.%d.%d.%d\n", CONVIP(srchost), CONVIP(desthost));
                    } else {
                        //printf("NOT DEST\n");
                    }
                }
            }
        }
//        printf("End i %d\n\n", i);
    fflush(stdout);
    }
}

bool IPX_isConnectedToServer(Bits tableNum, IPaddress ** ptrAddr)
{
    if (tableNum >= SOCKETTABLESIZE)
        return false;
    *ptrAddr = &ipconn[tableNum];
    return connBuffer[tableNum].connected;
}

static void ackClient(IPaddress clientAddr)
{
    IPXHeader regHeader;
    UDPpacket regPacket;
    Bits result;

    SDLNet_Write16(0xffff, regHeader.checkSum);
    SDLNet_Write16(sizeof(regHeader), regHeader.length);

    SDLNet_Write32(0, regHeader.dest.network);
    PackIP(clientAddr, &regHeader.dest.addr.byIP);
    SDLNet_Write16(0x2, regHeader.dest.socket);

    SDLNet_Write32(1, regHeader.src.network);
    PackIP(ipxServerIp, &regHeader.src.addr.byIP);
    SDLNet_Write16(0x2, regHeader.src.socket);
    regHeader.transControl = 0;
    regHeader.counter = 0; // unused/unvalidated by the client; zeroed rather than left as stack garbage

    regPacket.data = (Uint8 *)&regHeader;
    regPacket.len = sizeof(regHeader);
    regPacket.maxlen = sizeof(regHeader);
    regPacket.address = clientAddr;
    // Send registration string to client.  If client doesn't get this, client will not be registered
    result = SDLNet_UDP_Send(ipxServerSocket, -1, &regPacket);

}

static void IPX_ServerLoop()
{
    UDPpacket inPacket;
    IPaddress tmpAddr;

    //char regString[] = "IPX Register\0";

    Bit16u i;
    Bit32u host;
    Bits result;

    inPacket.channel = -1;
    inPacket.data = &inBuffer[0];
    inPacket.maxlen = IPXBUFFERSIZE;

    result = SDLNet_UDP_Recv(ipxServerSocket, &inPacket);
    if (result != 0) {
        // Check to see if incoming packet is a registration packet
        // For this, I just spoofed the echo protocol packet designation 0x02
        IPXHeader *tmpHeader;
        tmpHeader = (IPXHeader *)&inBuffer[0];

        // Check to see if echo packet
        if (SDLNet_Read16(tmpHeader->dest.socket) == 0x2) {
            // Null destination node means its a server registration packet
            if (tmpHeader->dest.addr.byIP.host == 0x0) {
                /* A registration packet's self-reported src (tmpAddr) is
                   always zeroed by the client (see ipx_client.cpp's
                   IBuildRegistrationPacket, shared by both the initial
                   connect and the periodic keepalive) -- comparing
                   against it can never match a real ipconn[] entry, so
                   the old "is this an existing client re-registering"
                   check below used to be dead code, and every keepalive
                   round-trip fell through to claiming a brand-new table
                   slot instead of refreshing its real one. Scan for an
                   existing match on the actual observed UDP source
                   (inPacket.address) first, before ever claiming a free
                   slot as a new connection. */
                UnpackIP(tmpHeader->src.addr.byIP, &tmpAddr);

                for (i = 0; i < SOCKETTABLESIZE; i++) {
                    if (connBuffer[i].connected
                            && (ipconn[i].host == inPacket.address.host)
                            && (ipconn[i].port == inPacket.address.port)) {
                        LOG_MSG("IPXSERVER: Reconnect from %d.%d.%d.%d:%d [%d]\n",
                                CONVIP(inPacket.address.host), inPacket.address.port, i);
                        fflush(stdout);
                        connBuffer[i].timeout = CONNECT_TIMEOUT;
                        ackClient(inPacket.address);
                        return;
                    }
                }

                for (i = 0; i < SOCKETTABLESIZE; i++) {
                    if (!connBuffer[i].connected) {
                        ipconn[i] = inPacket.address;

                        connBuffer[i].connected = true;
                        host = ipconn[i].host;
                        LOG_MSG("IPXSERVER: Connect from %d.%d.%d.%d:%d [%d]\n",
                                CONVIP(host), ipconn[i].port, i);
                        fflush(stdout);
                        connBuffer[i].timeout = CONNECT_TIMEOUT;
                        ackClient(inPacket.address);
                        return;
                    }
                }
            }
        } else {
            // IPX packet is complete.  Now interpret IPX header and send to respective IP address
            sendIPXPacket((Bit8u *)inPacket.data, inPacket.len);
        }
    }
}

void IPX_StopServer()
{
    //TIMER_DelTickHandler(&IPX_ServerLoop);
    SDLNet_UDP_Close(ipxServerSocket);
    if (discoverySocket) {
        SDLNet_UDP_Close(discoverySocket);
    }
}

/* Answers LAN "is anybody there" probes from a launcher's Join dialog, on
   a fixed well-known port separate from the (user-configurable) game port
   -- see DEFAULT_DISCOVERY_PORT. Polled once per main-loop iteration,
   same non-blocking-recv pattern as IPX_ServerLoop. Replies directly to
   the sender's address (from inPacket.address) rather than broadcasting,
   so it works the same whether the probe arrived via broadcast or unicast. */
void DiscoveryServerLoop(void)
{
    static Bit8u discoveryBuffer[64];
    UDPpacket inPacket;
    UDPpacket outPacket;
    char reply[64];
    int replyLen;
    size_t magicLen = strlen(DISCOVERY_REQUEST_MAGIC);

    if (!discoverySocket)
        return;

    inPacket.channel = -1;
    inPacket.data = discoveryBuffer;
    inPacket.maxlen = sizeof(discoveryBuffer) - 1;

    if (SDLNet_UDP_Recv(discoverySocket, &inPacket) <= 0)
        return;

    if ((size_t)inPacket.len < magicLen)
        return;
    discoveryBuffer[inPacket.len] = '\0';
    if (strncmp((char *)discoveryBuffer, DISCOVERY_REQUEST_MAGIC, magicLen) != 0)
        return;

    /* "AAServer" as a fixed display name, not a real gethostname() lookup --
       keeps this responder free of any extra platform-specific networking
       headers/init (Winsock's gethostname() needs care around WSAStartup
       ordering that isn't worth adding just for a cosmetic label). Users
       can give a server a friendlier name on the client side when saving
       it to their own server list. */
    /* sprintf, not snprintf -- this ancient IRIX/TGCware GCC toolchain
       doesn't declare snprintf under this build's flags (-U__c99), and
       every other string-formatting call in this codebase already uses
       sprintf. Safe here: reply is 64 bytes, and the formatted content
       (a 14-char prefix, a 16-bit port's up to 5 digits, and the fixed
       string ":AAServer") tops out well under 30. */
    replyLen = sprintf(reply, "%s%d:AAServer", DISCOVERY_REPLY_PREFIX, (int)g_serverPort);

    outPacket.channel = -1;
    outPacket.data = (Bit8u *)reply;
    outPacket.len = replyLen;
    outPacket.address = inPacket.address;
    SDLNet_UDP_Send(discoverySocket, -1, &outPacket);
}

void UpdateConnections(void)
{
    unsigned int i;

    for (i = 0; i < SOCKETTABLESIZE; i++) {
        if (connBuffer[i].connected) {
            connBuffer[i].timeout--;
            if (connBuffer[i].timeout == 0) {
                Bit16u srcport = ipconn[i].port;
                Bit32u srchost = ipconn[i].host;
                printf("Timeout connection %d.%d.%d.%d:%d -- Closed!\n",
                        CONVIP(srchost), srcport);
                connBuffer[i].connected = 0;
                fflush(stdout);
            }
        }
    }
}

bool IPX_StartServer(Bit16u portnum)
{
    Bit16u i;
    clock_t t;
    clock_t lastCheck;

    if (!SDLNet_ResolveHost(&ipxServerIp, NULL, portnum)) {
        //serverSocketSet = SDLNet_AllocSocketSet(SOCKETTABLESIZE);
        ipxServerSocket = SDLNet_UDP_Open(portnum);
        if (!ipxServerSocket) {
            printf("Failed to create server socket: %s\n", SDLNet_GetError());
            return false;
        }
        g_serverPort = portnum;

        /* Non-fatal if this can't be opened (e.g. another AAServer already
           running on this machine has it) -- LAN discovery is a convenience
           for the Join dialog, not required to host/join a game directly by
           IP, so the whole server shouldn't refuse to start over it. */
        discoverySocket = SDLNet_UDP_Open(DEFAULT_DISCOVERY_PORT);
        if (!discoverySocket) {
            printf("Warning: LAN discovery responder unavailable (%s) -- server will only be joinable by direct IP.\n", SDLNet_GetError());
        }

        for (i = 0; i < SOCKETTABLESIZE; i++) {
            connBuffer[i].connected = false;
        }

        printf("Server started on port %d\n", portnum);
        //TIMER_AddTickHandler(&IPX_ServerLoop);
        lastCheck = clock();
        while (1) {
            IPX_ServerLoop();
            DiscoveryServerLoop();
            t = clock();
            if ((t - lastCheck) >= CLOCKS_PER_SEC) {
                lastCheck += CLOCKS_PER_SEC;
                // 1 second has gone by
                UpdateConnections();
            }
            SDL_Delay(1);
        }

        return true;
    } else {
        printf("Server could not resolve host!\n");
    }
    return false;
}

#ifdef TARGET_UNIX
/* Guard env var: set on the relaunched copy before it execs into a
   terminal, so that copy (which still sees --console, if the caller left
   it in) never tries to relaunch itself again. */
#define CONSOLE_SPAWNED_ENV "AASERVER_CONSOLE_SPAWNED"

static std::string ShellQuoteArg(const char *s)
{
    std::string out = "'";
    for (const char *p = s; *p; p++) {
        if (*p == '\'')
            out += "'\\''";
        else
            out += *p;
    }
    out += "'";
    return out;
}

static bool ResolveExecutablePath(char *outPath, size_t outSize)
{
#ifdef __APPLE__
    char rawPath[4096];
    uint32_t rawSize = sizeof(rawPath);
    if (_NSGetExecutablePath(rawPath, &rawSize) != 0)
        return false;
    return realpath(rawPath, outPath) != NULL;
#else
    ssize_t len = readlink("/proc/self/exe", outPath, outSize - 1);
    if (len <= 0)
        return false;
    outPath[len] = '\0';
    return true;
#endif
}

/* Re-launches this same binary inside a new, visible terminal window, for
   use when AAServer has no controlling tty of its own -- e.g. double-
   clicked in Finder, or spawned as a fully detached child the way
   AALauncher's "Start Server" button does (QProcess::startDetached() on
   Qt builds, NSTask on the Cocoa/PPC build), neither of which leaves it
   with any visible output or window. Opt-in via --console rather than
   automatic: a real dedicated-server deployment (systemd, Docker, plain
   SSH+nohup) legitimately wants to run with no tty, and unconditionally
   forcing a terminal open would break that. Only reached under
   TARGET_UNIX -- the native Windows build already gets a console for
   free via its Console subsystem linker setting. */
static bool SpawnInTerminal(int argc, char **argv)
{
    char exePath[4096];
    if (!ResolveExecutablePath(exePath, sizeof(exePath)))
        return false;

    std::string cmd = ShellQuoteArg(exePath);
    for (int i = 1; i < argc; i++) {
        cmd += " ";
        cmd += ShellQuoteArg(argv[i]);
    }

    setenv(CONSOLE_SPAWNED_ENV, "1", 1);

#ifdef __APPLE__
    /* Terminal.app's "do script" runs the given string through the user's
       shell in a new window, so cmd (already shell-quoted per argument)
       is exactly what belongs here. Escape it for embedding inside the
       double-quoted AppleScript string literal. */
    std::string script = "tell application \"Terminal\" to do script \"";
    for (char c : cmd) {
        if (c == '"' || c == '\\')
            script += '\\';
        script += c;
    }
    script += "\"";
    execlp("osascript", "osascript",
           "-e", script.c_str(),
           "-e", "tell application \"Terminal\" to activate",
           (char *)NULL);
    return false; // only reached if osascript itself couldn't be exec'd
#else
    /* Try common Linux terminal emulators in turn. Each is exec'd as
       "<emulator> <separator> /bin/sh -c '<cmd>'" so the shell (not the
       terminal emulator) is what parses cmd's quoting -- avoids relying
       on each emulator's own argv-vs-single-string -e/-- conventions. */
    struct { const char *bin; const char *sep; } emulators[] = {
        {"x-terminal-emulator", "-e"},
        {"gnome-terminal", "--"},
        {"konsole", "-e"},
        {"xfce4-terminal", "-e"},
        {"xterm", "-e"},
    };
    for (size_t i = 0; i < sizeof(emulators) / sizeof(emulators[0]); i++) {
        execlp(emulators[i].bin, emulators[i].bin, emulators[i].sep,
               "/bin/sh", "-c", cmd.c_str(), (char *)NULL);
        // execlp only returns on failure (e.g. emulator not installed);
        // fall through and try the next one.
    }
    return false;
#endif
}
#endif

#ifdef TARGET_UNIX
int main(int argc, char *argv[])
#else
int _tmain(int argc, _TCHAR* argv[])
#endif
{
#ifdef TARGET_UNIX
    /* Pull --console out of argv before anything else (including the
       existing positional port-argument parsing below) looks at it. */
    bool consoleRequested = false;
    {
        int w = 1;
        for (int r = 1; r < argc; r++) {
            if (strcmp(argv[r], "--console") == 0)
                consoleRequested = true;
            else
                argv[w++] = argv[r];
        }
        argc = w;
    }
    if (consoleRequested && !getenv(CONSOLE_SPAWNED_ENV)) {
        if (SpawnInTerminal(argc, argv))
            return 0; // unreachable: SpawnInTerminal execs on success
        printf("Warning: --console requested but no terminal emulator could be launched; continuing without one.\n");
        fflush(stdout);
    }
    /* Identify the window/tab for whoever's looking at the process list
       or a pile of terminal windows, regardless of how it got a tty. */
    printf("\033]0;AAServer - Amulets & Armor Dedicated Server\007");
#else
    /* Accept and ignore --console here too, so callers (e.g. AALauncher)
       can pass it unconditionally across platforms -- the native Windows
       build already opens a console window on its own (Console subsystem
       linker setting), so there's nothing to spawn. */
    {
        int w = 1;
        for (int r = 1; r < argc; r++) {
            if (_tcscmp(argv[r], _T("--console")) != 0)
                argv[w++] = argv[r];
        }
        argc = w;
    }
#endif
    printf("Amulets & Armor IPX Server v1.00\n");
    printf("--------------------------------\n");
    fflush(stdout);

    if (SDL_Init(0) == -1) {
        printf("SDL_Init: %s\n", SDL_GetError());
        exit(1);
    }
    if (SDLNet_Init() == -1) {
        printf("SDLNet_Init: %s\n", SDLNet_GetError());
        exit(2);
    }

    Bit16u port = DEFAULT_IPX_PORT;
    if (argc >= 2) {
#ifdef TARGET_UNIX
        int parsed = atoi(argv[1]);
#else
        /* argv[1] is _TCHAR* here, which is wchar_t* under this project's
           Windows Unicode builds -- atoi() only accepts char*. _ttoi is
           tchar.h's width-agnostic equivalent (expands to _wtoi/atoi to
           match). */
        int parsed = _ttoi(argv[1]);
#endif
        if (parsed > 0 && parsed <= 65535) {
            port = (Bit16u)parsed;
        } else {
#ifdef TARGET_UNIX
            printf("Invalid port '%s', using default %d\n", argv[1], DEFAULT_IPX_PORT);
#else
            _tprintf(_T("Invalid port '%s', using default %d\n"), argv[1], DEFAULT_IPX_PORT);
#endif
        }
    }

    IPX_StartServer(port);
    return 0;
}

