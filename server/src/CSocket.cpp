#include "IDebug.h"
#include <thread>

#if defined(_WIN32) || defined(_WIN64)

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
	#define SHUT_RD				SD_RECEIVE
	#define SHUT_RDWR			SD_BOTH

	// The following #define would cause some snprintf linking errors in cstdio with MinGW
	#if !defined(__GNUC__) && !(defined(_WIN32) || defined(_WIN64))
	  #define snprintf _snprintf
	#endif
#else
	#include <netdb.h>
	#include <errno.h>
	#include <unistd.h>
	#include <fcntl.h>

	#include <netinet/tcp.h>
	#define SOCKET_ERROR	-1
#endif

// Don't send a signal.  Should only affect Linux.
#ifndef MSG_NOSIGNAL
	#define MSG_NOSIGNAL 0
#endif

#include <memory.h>
#include <stdio.h>
#include "CSocket.h"

// Change this to any printf()-like function you use for logging purposes.
#define SLOG(x, ...)		 if (0) printf(x, ## __VA_ARGS__)
//////

// Function declarations.
static const char* errorMessage(int error);
static int identifyError(int source = 0);

// From main.cpp
#include "CLog.h"
extern CLog serverlog;
#include "IUtil.h"

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
	SOCKET max = 0;
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
			if (sock > max) max = sock;
			if (stub->canRecv()) FD_SET(sock, &set_read);
			if (stub->canSend()) FD_SET(sock, &set_write);
		}
		++i;
	}
	fd_max = max;

	// Do the select.
	select(fd_max + 1, &set_read, &set_write, 0, &tm);

	// Loop through all the socket handles and call relevant functions.
	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end();)
	{
		CSocketStub* stub = *i;
		if (stub == 0) continue;

		SOCKET sock = stub->getSocketHandle();
		if (sock != INVALID_SOCKET)
		{
			bool success = true;
			if (success && FD_ISSET(sock, &set_read)) success = stub->onRecv();
			if (success && FD_ISSET(sock, &set_write)) success = stub->onSend();
			if (!success)
			{
				stub->onUnregister();
				*i = 0;
			}
		}
		++i;
	}

	// Add new stubs.
	stubList.insert(stubList.end(), newStubs.begin(), newStubs.end());
	newStubs.clear();

	return true;
}

bool CSocketManager::updateSingle(CSocketStub* stub, bool pRead, bool pWrite, long sec, long usec)
{
	fd_set set_read;
	fd_set set_write;
	struct timeval tm;

	if (stub == 0) return false;

	tm.tv_sec = sec;
	tm.tv_usec = usec;
	FD_ZERO(&set_read);
	FD_ZERO(&set_write);

	// Put the socket handle into the set.
	SOCKET sock = stub->getSocketHandle();
	if (sock == INVALID_SOCKET) return false;
	if (pRead && stub->canRecv()) FD_SET(sock, &set_read);
	if (pWrite && stub->canSend()) FD_SET(sock, &set_write);

	// Do the select.
	select(fd_max + 1, &set_read, &set_write, 0, &tm);

	// Call relevant functions.
	if (FD_ISSET(sock, &set_read))
	{
		if (stub->onRecv() == false)
		{
			stub->onUnregister();
			vecReplace<CSocketStub*>(stubList, stub, 0);	// Will remove during next call to update().
			return false;
		}
	}
	if (FD_ISSET(sock, &set_write))
	{
		if (stub->onSend() == false)
		{
			stub->onUnregister();
			vecReplace<CSocketStub*>(stubList, stub, 0);
			return false;
		}
	}

	return true;
}

bool CSocketManager::registerSocket(CSocketStub* stub)
{
	SOCKET sock = stub->getSocketHandle();
	if (stub->onRegister())
	{
		if (sock > fd_max) fd_max = sock;
		newStubs.push_back(stub);
		return true;
	}
	return false;
}

bool CSocketManager::unregisterSocket(CSocketStub* stub)
{
	SOCKET sock = stub->getSocketHandle();

	bool found = false;
	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end(); ++i)
	{
		CSocketStub* stub = *i;
		if (stub == 0) continue;

		SOCKET sock2 = stub->getSocketHandle();
		if (sock2 == sock)
		{
			stub->onUnregister();
			*i = 0;
			found = true;
			break;
		}
	}

	return found;
}

void CSocketManager::cleanup(bool callOnUnregister)
{
	for (std::vector<CSocketStub*>::iterator i = stubList.begin(); i != stubList.end(); ++i)
	{
		CSocketStub* stub = *i;
		if (stub == 0) continue;

		if (callOnUnregister)
			stub->onUnregister();
	}
	stubList.clear();
	fd_max = 0;
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
		error = getaddrinfo(host, port, &hints, &res);
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
	{
		memcpy((void*)&properties.address, res->ai_addr, res->ai_addrlen);
		properties.addresslen = res->ai_addrlen;
	}

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
		if (::bind(properties.handle, (struct sockaddr *)&properties.address, properties.addresslen) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] bind() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			return SOCKET_BIND_ERROR;
		}
	}

	// Connect the socket.
	if (properties.type != SOCKET_TYPE_SERVER)
	{
		if (::connect(properties.handle, (struct sockaddr *)&properties.address, properties.addresslen) == SOCKET_ERROR)
		{
			SLOG("[CSocket::connect] connect() returned error: %s\n", errorMessage(identifyError()));
			disconnect();
			return SOCKET_CONNECT_ERROR;
		}
	}

	// Disable the nagle algorithm.
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
	{
		int nagle = 1;
		setsockopt(properties.handle, IPPROTO_TCP, TCP_NODELAY, (char*)&nagle, sizeof(nagle));
	}

	// Set as non-blocking.
#if defined(_WIN32) || defined(_WIN64)
	u_long flags = 1;
	ioctlsocket(properties.handle, FIONBIO, &flags);
#else
	int flags = fcntl(properties.handle, F_GETFL, 0);
	fcntl(properties.handle, F_SETFL, flags | O_NONBLOCK);
#endif

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
	if (shutdown(properties.handle, SHUT_RDWR) == SOCKET_ERROR)
	{
		int error = identifyError();
		if (error == ENOTSOCK)
		{
			properties.handle = INVALID_SOCKET;
			properties.state = SOCKET_STATE_DISCONNECTED;
			return;
		}
		SLOG("[CSocket::destroy] shutdown returned error: %s\n", errorMessage(error));
	}

	// Mark socket as terminating.
	properties.state = SOCKET_STATE_TERMINATING;

	// Gracefully shut it down.
	/*
	if (properties.protocol == SOCKET_PROTOCOL_TCP)
	{
		int count = 0;
		char buff[ 0x2000 ];
		int size;
		while (++count < 3)
		{
			size = recv(properties.handle, buff, 0x2000, 0);
			if (size == 0) break;
			if (size == SOCKET_ERROR) break;
		}
	}
	*/

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
	properties.handle = INVALID_SOCKET;
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
		if (delay != 0)
		{
			std::chrono::milliseconds dur{ delay };
			std::this_thread::sleep_for(dur);
		}
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

	// Disable the nagle algorithm.
	if (props.protocol == SOCKET_PROTOCOL_TCP)
	{
		int nagle = 1;
		setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (char*)&nagle, sizeof(nagle));
	}

	// Set as non-blocking.
#if defined(_WIN32) || defined(_WIN64)
	u_long flags = 1;
	ioctlsocket(handle, FIONBIO, &flags);
#else
	int flags = fcntl(properties.handle, F_GETFL, 0);
	fcntl(handle, F_SETFL, flags | O_NONBLOCK);
#endif

	// Accept the connection by calling getsockopt.
	int type, typeSize = sizeof(int);
	getsockopt(handle, SOL_SOCKET, SO_TYPE, (char*)&type, (socklen_t*)&typeSize);

	return sock;
}

int CSocket::sendData(char* data, unsigned int* dsize)
{
	int intError = 0;

	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
	{
		*dsize = 0;
		return 0;
	}

	// Send our data, yay!
	int sent = 0;
	if ((sent = ::send(properties.handle, data, *dsize, MSG_NOSIGNAL)) == SOCKET_ERROR)
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
			case EAGAIN:
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
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
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
	if (properties.state == SOCKET_STATE_DISCONNECTED || properties.handle == INVALID_SOCKET)
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
	strncpy(properties.description, strDescription, MIN(strlen(strDescription), SOCKET_MAX_DESCRIPTION - 1));
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

const char* CSocket::getRemotePort()
{
	char* portret;
	static char port[32];
	memset((void*)port, 0, 32);

	// Grab the IP address.
	int error = getnameinfo((struct sockaddr*)&properties.address, sizeof(struct sockaddr_storage), 0, 0, port, 32, NI_NUMERICSERV);
	if (error) return 0;
	portret = port;
	return portret;
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
	using namespace std::chrono_literals;
	// errorOut("debuglog.txt", ":: Destroying socket system...");
#if defined(_WIN32) || defined(_WIN64)
	int intTimeCheck = 0;

	while (intTimeCheck++ < 3)
	{
		if (WSACleanup() == SOCKET_ERROR)
			SLOG("[CSocket::socketSystemDestroy] WSACleanup() returned error: %s\n", errorMessage(identifyError()));
		std::this_thread::sleep_for(1s);
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
			//#ifdef __GNUC__
			//	__gnu_cxx::snprintf(buf, 32, "%d", error);
			//#else
				snprintf(buf, 32, "%d", error);
			//#endif 
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
