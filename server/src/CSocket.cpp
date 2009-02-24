#if defined(_WIN32) || defined(_WIN64)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#define WINVER 0x0501
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>

#ifndef __GNUC__ // rain
	#pragma comment(lib, "ws2_32.lib")
#endif
	// Some of these might not be valid in Linux, but I don't really care right now.
	//#define WSANOTINITIALISED	WSANOTINITIALISED
	#define ENETDOWN			WSAENETDOWN
	#define EADDRINUSE			WSAEADDRINUSE
	#define EINTR				WSAEINTR
	#define EINPROGRESS			WSAEINPROGRESS
	#define EALREADY			WSAEALREADY
	#define EADDRNOTAVAIL		WSAEADDRNOTAVAIL
	#define EAFNOSUPPORT		WSAEAFNOSUPPORT
	#define ECONNREFUSED		WSAECONNREFUSED
	#define EFAULT				WSAEFAULT
	#define EINVAL				WSAEINVAL
	#define EISCONN				WSAEISCONN
	#define ENETUNREACH			WSAENETUNREACH
	#define ENOBUFS				WSAENOBUFS
	#define ENOTSOCK			WSAENOTSOCK
	#define ETIMEDOUT			WSAETIMEDOUT
	#define EWOULDBLOCK			WSAEWOULDBLOCK
	#define EAGAIN				WSAEWOULDBLOCK
	#define EACCES				WSAEACCES
	#define ENOTCONN			WSAENOTCONN
	#define ENETRESET			WSAENETRESET
	#define EOPNOTSUPP			WSAEOPNOTSUPP
	#define ESHUTDOWN			WSAESHUTDOWN
	#define EMSGSIZE			WSAEMSGSIZE
	#define ECONNABORTED		WSAECONNABORTED
	#define ECONNRESET			WSAECONNRESET
	#define EHOSTUNREACH		WSAEHOSTUNREACH
	#define SHUT_WR				SD_SEND

	#define sleep Sleep
	#define snprintf _snprintf
#else
	#include <netdb.h>
	#include <errno.h>
	#include <unistd.h>
	#include <fcntl.h>

	#include <sys/socket.h>
	#include <netinet/in.h>
	#define SOCKET_ERROR	-1
	#define INVALID_SOCKET	(unsigned int)-1

	typedef unsigned int SOCKET;
#endif

#include <memory.h>
#include <stdio.h>
#include "CSocket.h"

// Change this to any printf()-like function you use for logging purposes.
#define SLOG	serverlog.out
//////

// Function declarations.
static const char* errorMessage(int error);
static int identifyError(int source = 0);

// From main.cpp
#include "CLog.h"
extern CLog serverlog;

bool CSocketManager::update(long sec, long usec)
{
	fd_set set_read;
	fd_set set_write;
	struct timeval tm;

	tm.tv_sec = sec;
	tm.tv_usec = usec;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);

	// Put all the socket handles into the set.
	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end();)
	{
		CSocketStub* stub = *i;
		if (stub == 0)
		{
			i = stubList.erase(i);
			continue;
		}
		SOCKET sock = stub->getSocketHandle();
		if (sock != INVALID_SOCKET)
		{
			FD_SET(sock, &set_read);
			if (stub->canSend()) FD_SET(sock, &set_write);
		}
		++i;
	}

	// Do the select.
	select(fd_max + 1, &set_read, &set_write, 0, &tm);

	// Loop through all the socket handles and call relevant functions.
	blockStubs = true;
	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end();)
	{
		CSocketStub* stub = *i;
		if (stub == 0)
		{
			i = stubList.erase(i);
			continue;
		}
		bool erased = false;
		SOCKET sock = stub->getSocketHandle();
		if (sock != INVALID_SOCKET)
		{
			if (FD_ISSET(sock, &set_read))
			{
				if (stub->onRecv() == false)
				{
					i = stubList.erase(i);
					erased = true;
				}
			}
			if (!erased && FD_ISSET(sock, &set_write))
			{
				if (stub->onSend() == false)
				{
					i = stubList.erase(i);
					erased = true;
				}
			}
		}
		if (!erased) ++i;
	}
	blockStubs = false;

	// If any stubs were added while parsing data, add them to the list now.
	if (newStubs.size() != 0)
	{
		for (std::vector<CSocketStub*>::iterator i = newStubs.begin(); i != newStubs.end(); ++i)
		{
			CSocketStub* stub = *i;
			stubList.push_back(stub);
		}
		newStubs.clear();
	}

	// If any stubs were removed while parsing data, remove them now.
	if (removeStubs.size() != 0)
	{
		for (std::vector<CSocketStub*>::iterator i = removeStubs.begin(); i != removeStubs.end();)
		{
			CSocketStub* stub = *i;
			for (std::vector<CSocketStub*>::iterator j = stubList.begin(); j != stubList.end();)
			{
				CSocketStub* search = *j;
				if (stub == search)
					j = stubList.erase(j);
				else ++j;
			}
			i = removeStubs.erase(i);
		}
		removeStubs.clear();
	}

	return true;
}

bool CSocketManager::registerSocket(CSocketStub* stub)
{
	SOCKET sock = stub->getSocketHandle();
	if (sock > fd_max) fd_max = sock;
	if (blockStubs) newStubs.push_back(stub);
	else stubList.push_back(stub);
	return true;
}

bool CSocketManager::unregisterSocket(CSocketStub* stub)
{
	SOCKET sock = stub->getSocketHandle();

	bool found = false;
	bool findNewMax = false;
	if (sock == fd_max) findNewMax = true;
	SOCKET max = 0;

	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end();)
	{
		SOCKET sock2 = (*i)->getSocketHandle();
		if (findNewMax && sock2 != sock && sock2 > max) max = sock2;
		if (sock2 == sock)
		{
			if (blockStubs)
			{
				(*i) = 0;
				removeStubs.push_back(stub);
				++i;
			}
			else i = stubList.erase(i);
			found = true;
			if (!findNewMax) break;
		}
		else ++i;
	}

	if (findNewMax) fd_max = max;
	return found;
}



int CSocket::was_initiated = 0;

// Class functions
CSocket::CSocket()
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	memset((char *)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
}

CSocket::CSocket(const char* host, const char* port, sock_properties* properties)
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	if (properties != 0)
		memcpy((void*)&this->properties, properties, sizeof(sock_properties));
	else
	{
		memset((char *)&this->properties.description, 0, SOCKET_MAX_DESCRIPTION);
	}
	this->init(host, port);
}

CSocket::~CSocket()
{
	// Destroy if still connected.
	if (properties.state != SOCKET_STATE_DISCONNECTED)
		disconnect();
}

int CSocket::init(const char* host, const char* port, int protocol)
{
	struct addrinfo hints;
	struct addrinfo* res;

	// Make sure a TCP socket is disconnected.
	if (properties.protocol == SOCKET_PROTOCOL_TCP && properties.state != SOCKET_STATE_DISCONNECTED)
	{
		SLOG("[ERROR] Socket %s is already connected.\n", properties.description);
		return SOCKET_ALREADY_CONNECTED;
	}

	// Start creating the hints.
	memset((struct sockaddr_storage*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memset((void*)&hints, 0, sizeof(hints));
	if (properties.protocol == SOCKET_PROTOCOL_TCP) hints.ai_socktype = SOCK_STREAM;
	if (properties.protocol == SOCKET_PROTOCOL_UDP) hints.ai_socktype = SOCK_DGRAM;

	// Choose the protocol we want.
	switch (protocol)
	{
		case SOCKET_PROTOCOL_ANY:
			hints.ai_family = AF_UNSPEC;
			break;
		case SOCKET_PROTOCOL_IPV4:
			hints.ai_family = AF_INET;
			break;
		case SOCKET_PROTOCOL_IPV6:
			hints.ai_family = AF_INET6;
			break;
	}

	// Create the host.
	int error;
	if (properties.type == SOCKET_TYPE_CLIENT && host != 0)
		error = getaddrinfo(host, port, &hints, &res);
	else if (properties.type == SOCKET_TYPE_SERVER)
	{
		hints.ai_flags = AI_PASSIVE;		// Local socket.
		error = getaddrinfo(0, port, &hints, &res);
	}
	else
	{
		SLOG("[ERROR] Socket %s's properties.type is invalid.\n", properties.description);
		return SOCKET_ERROR;
	}

	// Check for errors.
	if (error)
	{
		SLOG("[CSocket::init] getaddrinfo() returned error: %d\n", error);
		return SOCKET_HOST_UNKNOWN;
	}
	else
		memcpy((void*)&properties.address, res->ai_addr, res->ai_addrlen);

	return SOCKET_OK;
}

int CSocket::connect()
{
	// Make sure the socket is disconnected.
	if (properties.state != SOCKET_STATE_DISCONNECTED)
		return SOCKET_ALREADY_CONNECTED;

	// Flag the socket as connecting.
	properties.state = SOCKET_STATE_CONNECTING;

	// Create socket.
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
		properties.handle = socket(AF_INET, SOCK_STREAM, 0);
	else
		properties.handle = socket(AF_INET, SOCK_DGRAM, 0);

	// Make sure the socket was created correctly.
	if (properties.handle == INVALID_SOCKET)
	{
		SLOG("[CSocket::connect] socket() returned INVALID_SOCKET.\n");
		properties.state = SOCKET_STATE_DISCONNECTED;
		return SOCKET_INVALID;
	}

	// Bind the socket if it is a server-type socket.
	if (properties.type == SOCKET_TYPE_SERVER)
	{
		// Let us reuse the address.  Freaking bind.
		int value = 1;
		setsockopt(properties.handle, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value));

		// Bind the socket.
		if (::bind(properties.handle, (struct sockaddr *)&properties.address, sizeof(properties.address)) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] bind() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			/*
			#if defined(_WIN32) || defined(_WIN64)
				closesocket(properties.handle);
			#else
				close(properties.handle);
			#endif
			properties.state = SOCKET_STATE_DISCONNECTED;
			*/
			return SOCKET_BIND_ERROR;
		}
	}

	// Connect the socket.
	if (properties.type != SOCKET_TYPE_SERVER)
	{
		if (::connect(properties.handle, (struct sockaddr *)&properties.address, sizeof(properties.address)) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] connect() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			/*
			#if defined(WIN32) || defined(WIN64)
				closesocket(properties.handle);
			#else
				close(properties.handle);
			#endif
			properties.state = SOCKET_STATE_DISCONNECTED;
			*/
			return SOCKET_CONNECT_ERROR;
		}
	}

	// Socket connected!
	properties.state = SOCKET_STATE_CONNECTED;

	// Listening sockets.
	if (properties.type == SOCKET_TYPE_SERVER)
	{
		if (properties.protocol == SOCKET_PROTOCOL_UDP)
			properties.state = SOCKET_STATE_LISTENING;
		else if (properties.protocol == SOCKET_PROTOCOL_TCP)
		{
			if (::listen(properties.handle, SOMAXCONN) == SOCKET_ERROR)
			{
				SLOG("[CSocket::connect] listen() returned error: %s\n", errorMessage(identifyError()));
				disconnect();
				/*
				#if defined(WIN32) || defined(WIN64)
					closesocket(properties.handle);
				#else
					close(properties.handle);
				#endif
				properties.state = SOCKET_STATE_DISCONNECTED;
				*/
				return SOCKET_CONNECT_ERROR;
			}

			properties.state = SOCKET_STATE_LISTENING;
		}
	}
	return SOCKET_OK;
}

void CSocket::disconnect()
{
	// Shut down the socket.
	if (shutdown(properties.handle, SHUT_WR) == SOCKET_ERROR)
		SLOG("[CSocket::destroy] shutdown returned error: %s\n", errorMessage(identifyError()));

	// Mark socket as terminating.
	properties.state = SOCKET_STATE_TERMINATING;

	// Gracefully shut it down.
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
	{
		char buff[ 0x2000 ];
		int size;
		while ( true )
		{
			size = recv( properties.handle, buff, 0x2000, 0 );
			if (size == 0) break;
			if (size == SOCKET_ERROR)
			{
				int e = identifyError();
				if (!(e == EWOULDBLOCK || e == EINPROGRESS)) break;
			}
		}
	}

	// Destroy the socket of d00m.
#if defined(_WIN32) || defined(_WIN64)
	if (closesocket(properties.handle) == SOCKET_ERROR)
	{
		SLOG("[CSocket::destroy] closesocket ");
#else
	if (close(properties.handle) == SOCKET_ERROR)
	{
		SLOG("[CSocket::destroy] close ");
#endif
		SLOG("returned error: %s\n", errorMessage(identifyError()));
	}

	// Reset the socket state.
	properties.state = SOCKET_STATE_DISCONNECTED;
}

int CSocket::reconnect(long delay, int tries)
{
	int socket_reconnect_delay = delay;
	int socket_reconnect_attempts = tries;

	if (delay == 0)
		delay = socket_reconnect_delay;
	if (tries == 0)
		tries = socket_reconnect_attempts;

	for (int i = 0; i < tries; i++)
	{
		switch (this->connect())
		{
			case SOCKET_OK:
			case SOCKET_ALREADY_CONNECTED:
				return SOCKET_OK;
				break;
			case SOCKET_INVALID:
			case SOCKET_BIND_ERROR:
			case SOCKET_CONNECT_ERROR:
			default:
				// Do nothing.
				break;
		}
		if (delay != 0) sleep(delay);
	}
	return SOCKET_CONNECT_ERROR;
}

CSocket* CSocket::accept()
{
	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return 0;

	// Only server type TCP sockets can accept new connections.
	if (properties.type != SOCKET_TYPE_SERVER || properties.protocol != SOCKET_PROTOCOL_TCP)
		return 0;

	sockaddr_storage addr;
	int addrlen = sizeof(addr);
	SOCKET handle = 0;

	// Try to accept a new connection.
	handle = ::accept(properties.handle, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
	if (handle == INVALID_SOCKET)
	{
		int error = identifyError();
		if (error == EWOULDBLOCK || error == EINPROGRESS) return 0;
		SLOG("[CSocket::accept] accept() returned error: %s\n", errorMessage(error));
		return 0;
	}

	// Create the new socket to store the new connection.
	CSocket* sock = new CSocket();
	sock_properties props;
	memset((void*)&props, 0, sizeof(sock_properties));
	memset((void*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memcpy((void*)&props.address, &addr, sizeof(addr));
	props.protocol = properties.protocol;
	props.type = SOCKET_TYPE_CLIENT;
	props.state = SOCKET_STATE_CONNECTED;
	props.handle = handle;
	sock->setProperties(props);
	sock->setDescription(sock->getRemoteIp());

	// Accept the connection by calling getsockopt.
	int type, typeSize = sizeof(int);
	getsockopt(handle, SOL_SOCKET, SO_TYPE, (char*)&type, (socklen_t*)&typeSize);

	return sock;
}

int CSocket::sendData(char* data, unsigned int* dsize)
{
	int intError = 0;

	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
	{
		*dsize = 0;
		return 0;
	}

	// Send our data, yay!
	int sent = 0;
	if ((sent = ::send(properties.handle, data, *dsize, 0)) == SOCKET_ERROR)
	{
		sent = 0;
		intError = identifyError();
		switch (intError)
		{
			case ENETDOWN:
			case ENETRESET:
			case ENOTCONN:
			case EHOSTUNREACH:
			case ECONNABORTED:
			case ECONNRESET:
			case ETIMEDOUT:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				return 0;
				break;
		}
		disconnect();
		return 0;
	}

	// Remove what we sent from the total size.
	*dsize -= sent;

	// Return how much data was ultimately sent.
	return sent;
}

char* CSocket::getData(unsigned int* dsize)
{
	int size = 0;
	int intError = 0;

	// Create the buffer.
	const int BUFFLEN = 0x8000;	// 32KB.
	static char buff[0x8000];	// 32KB.

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
	{
		*dsize = 0;
		return 0;
	}

	// Allocate buff.
	memset((void*)buff, 0, BUFFLEN);

	// Get our data
	if (properties.protocol == SOCKET_PROTOCOL_UDP)
		size = recvfrom(properties.handle, buff, BUFFLEN, 0, 0, 0);
	else
		size = recv(properties.handle, buff, BUFFLEN, 0);

	// Check for error!
	if (size == SOCKET_ERROR)
	{
		size = 0;
		intError = identifyError();
		switch (intError)
		{
			case ENETDOWN:
			case ENETRESET:
			case ENOTCONN:
			case EHOSTUNREACH:
			case ECONNABORTED:
			case ECONNRESET:
			case ETIMEDOUT:
			case ESHUTDOWN:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				break;
			default:
				break;
		}
	}

	// If size is 0, the socket was disconnected.
	if (size == 0)
		disconnect();

	// Set dsize to how much data was returned.
	*dsize = size;

	// Return the data.
	return buff;
}

char* CSocket::peekData(unsigned int* dsize)
{
	// Create the buffer.
	const int BUFFLEN = 0x8000;	// 32KB.
	static char buff[0x8000];	// 32KB.
	int intError;

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
	{
		*dsize = 0;
		return 0;
	}

	// Allocate buff.
	memset((void*)buff, 0, BUFFLEN);

	// Get our data
	int size;
	if (properties.protocol == SOCKET_PROTOCOL_UDP)
		size = recvfrom(properties.handle, buff, BUFFLEN, MSG_PEEK, 0, 0);
	else
		size = recv(properties.handle, buff, BUFFLEN, MSG_PEEK);

	// Check for error!
	if (size == SOCKET_ERROR)
	{
		size = 0;
		intError = identifyError();
		switch (intError)
		{
			case ENETDOWN:
			case ENETRESET:
			case ENOTCONN:
			case EHOSTUNREACH:
			case ECONNABORTED:
			case ECONNRESET:
			case ETIMEDOUT:
			case ESHUTDOWN:
				// Destroy the bad socket and create a new one.
				SLOG("%s - Connection lost!  Reason: %s\n", properties.description, errorMessage(intError));
				disconnect();
				break;
			default:
				break;
		}
		return 0;
	}

	// Return the data.
	*dsize = size;
	return buff;
}

int CSocket::setProtocol(int sock_proto)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.protocol = sock_proto;
	else
		return SOCKET_INVALID;
	return SOCKET_OK;
}

int CSocket::setType(int sock_type)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.type = sock_type;
	else
		return SOCKET_INVALID;
	return SOCKET_OK;
}

int CSocket::setDescription(const char *strDescription)
{
	memset((void*)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
	memcpy((void*)&properties.description, strDescription, SOCKET_MAX_DESCRIPTION - 1);
	return SOCKET_OK;
}

int CSocket::setProperties(sock_properties newprop)
{
	// Set the socket properties.
	memcpy((void*)&this->properties, (void *)&newprop, sizeof(sock_properties));

	return SOCKET_OK;
}

int CSocket::setState(int iState)
{
	this->properties.state = iState;
	return SOCKET_OK;
}

const char* CSocket::getRemoteIp()
{
	char* hostret;
	static char host[1025];
	memset((void*)host, 0, 1025);

	// Grab the IP address.
	int error = getnameinfo((struct sockaddr*)&properties.address, sizeof(struct sockaddr_storage), host, 1025, 0, 0, NI_NUMERICHOST);
	if (error) return 0;
	hostret = host;
	return hostret;
}

const char* CSocket::getLocalIp()
{
	char* hostret;
	static char host[1025];
	char host2[1025];

	struct sockaddr *sa;
	int salen;
	struct addrinfo hints;
	struct addrinfo *res;
	int error;

	// Get the local host name.
	error = gethostname(host2, sizeof(host2));
	if (error) return 0;

	// Get a sockaddr for the local host.
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	error = getaddrinfo(host2, 0, &hints, &res);
	if (error) return 0;

	// Translate into an IP address.
	sa = res->ai_addr;
	salen = res->ai_addrlen;
	error = getnameinfo(sa, salen, host, 1025, 0, 0, NI_NUMERICHOST);
	if (error) return 0;

	hostret = host;
	return hostret;
}

int CSocket::socketSystemInit()
{
//	errorOut("debuglog.txt", ":: Initializing socket system...");
#if defined(_WIN32) || defined(_WIN64)
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		SLOG("Failed to initialize winsocks!\n");
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		SLOG("Failed to initialize winsocks!  Wasn't version 2.2!\n");
		WSACleanup();
		return 1;
	}
#elif defined(PSPSDK)
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON) < 0)
		return -1;
	if (sceUtilityLoadNetModule(PSP_NET_MODULE_INET) < 0)
		return -1;
	if (pspSdkInetInit() != 0)
		return -2; // false
	if (sceNetApctlConnect(1) != 0)
		return -3;

	while (true)
	{
		int state = 0;
		if (sceNetApctlGetState(&state) != 0)
			return -3;
		if (state == 4)
			break;

		sceKernelDelayThread(1000 * 50); // 50ms
	}
#endif

	CSocket::was_initiated = 1;
	return 0;
}

void CSocket::socketSystemDestroy()
{
//	errorOut("debuglog.txt", ":: Destroying socket system...");
#if defined(_WIN32) || defined(_WIN64)
	int intTimeCheck = 0;

	while (intTimeCheck++ < 3)
	{
		if (WSACleanup() == SOCKET_ERROR)
			SLOG("[CSocket::socketSystemDestroy] WSACleanup() returned error: %s\n", errorMessage(identifyError()));
		sleep(1000);
	}
#endif
}

const char* errorMessage(int error)
{
	// These can happen a lot.  Don't display any errors about them.
	if (error == EWOULDBLOCK || error == EINPROGRESS)
		return 0;

	switch (error)
	{
#if defined(_WIN32) || defined(_WIN64)
		case WSANOTINITIALISED:
			return "WSANOTINITIALISED"; break;
#endif
		case ENETDOWN:
			return "ENETDOWN"; break;
		case EADDRINUSE:
			return "EADDRINUSE"; break;
		case EINTR:
			return "EINTR"; break;
		case EINPROGRESS:
			return "EINPROGRESS"; break;
		case EALREADY:
			return "EALREADY"; break;
		case EADDRNOTAVAIL:
			return "EADDRNOTAVAIL"; break;
		case EAFNOSUPPORT:
			return "EAFNOSUPPORT"; break;
		case ECONNREFUSED:
			return "ECONNREFUSED"; break;
		case EFAULT:
			return "EFAULT"; break;
		case EINVAL:
			return "EINVAL"; break;
		case EISCONN:
			return "EISCONN"; break;
		case ENETUNREACH:
			return "ENETUNREACH"; break;
		case ENOBUFS:
			return "ENOBUFS"; break;
		case ENOTSOCK:
			return "ENOTSOCK"; break;
		case ETIMEDOUT:
			return "ETIMEDOUT"; break;
		case EWOULDBLOCK:
			return "EWOULDBLOCK"; break;
		case EACCES:
			return "EACCES"; break;
		case ENOTCONN:
			return "ENOTCONN"; break;
		case ENETRESET:
			return "ENETRESET"; break;
		case EOPNOTSUPP:
			return "EOPNOTSUPP"; break;
		case ESHUTDOWN:
			return "ESHUTDOWN"; break;
		case EMSGSIZE:
			return "EMSGSIZE"; break;
		case ECONNABORTED:
			return "ECONNABORTED"; break;
		case ECONNRESET:
			return "ECONNRESET"; break;
		case EHOSTUNREACH:
			return "EHOSTUNREACH"; break;
		default:
		{
			static char buf[32];
			snprintf(buf, 32, "%d", error);
			return buf;
		}
	}
}

int identifyError(int source)
{
#if defined(_WIN32) || defined(_WIN64)
	return WSAGetLastError();
#else
	if (source != 0)
		return h_errno;
	else
		return errno;
#endif
}

#undef SLOG
