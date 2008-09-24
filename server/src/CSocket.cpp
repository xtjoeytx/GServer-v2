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

#include "CString.h"
#include "CSocket.h"
#include "CLog.h"
#include "main.h"

// From main.cpp
extern CLog serverlog;

int CSocket::was_initiated = 0;
static CString errorMessage(int error);
static int identifyError(int source = 0);

// Class functions
CSocket::CSocket()
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	properties.handle = 0;
	properties.protocol = 0;
	properties.type = 0;
	properties.options = 0;
	properties.state = SOCKET_STATE_DISCONNECTED;
	memset((char *)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
}

CSocket::CSocket(const CString& host, const CString& port, sock_properties* properties)
{
	if (CSocket::was_initiated == 0) CSocket::socketSystemInit();
	if (properties != 0)
		memcpy((void*)&this->properties, properties, sizeof(sock_properties));
	else
	{
		this->properties.handle = 0;
		this->properties.protocol = SOCKET_PROTOCOL_TCP;
		this->properties.type = SOCKET_TYPE_CLIENT;
		this->properties.options = 0;
		this->properties.state = SOCKET_STATE_DISCONNECTED;
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

int CSocket::init(const CString& host, const CString& port)
{
	struct addrinfo hints;
	struct addrinfo* res;

	// Make sure a TCP socket is disconnected.
	if (properties.protocol == SOCKET_PROTOCOL_TCP && properties.state != SOCKET_STATE_DISCONNECTED)
	{
		serverlog.out(CString() << "[ERROR] Socket " << properties.description << " is already connected.\n");
		return SOCKET_ALREADY_CONNECTED;
	}

	// Start creating the hints.
	memset((struct sockaddr_storage*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memset((void*)&hints, 0, sizeof(hints));
	if (properties.protocol == SOCKET_PROTOCOL_TCP) hints.ai_socktype = SOCK_STREAM;
	if (properties.protocol == SOCKET_PROTOCOL_UDP) hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_INET;

	// Create the host.
	int error;
	if (properties.type == SOCKET_TYPE_CLIENT && host.length() != 0)
		error = getaddrinfo(host.text(), port.text(), &hints, &res);
	else if (properties.type == SOCKET_TYPE_SERVER)
	{
		hints.ai_flags = AI_PASSIVE;		// Local socket.
		error = getaddrinfo(0, port.text(), &hints, &res);
	}
	else
	{
		serverlog.out(CString() << "[ERROR] Socket " << properties.description << "'s properties.type is invalid.\n");
		return SOCKET_ERROR;
	}

	// Check for errors.
	if (error)
	{
		serverlog.out(CString() << "[CSocket::init] getaddrinfo() returned error: " << CString(error) << "\n");
		return SOCKET_HOST_UNKNOWN;
	}
	else
		memcpy((void*)&properties.address, res->ai_addr, res->ai_addrlen);

	return 0;
}

void CSocket::destroy()
{
	// Shut down the socket.
	if (shutdown(properties.handle, SHUT_WR) == SOCKET_ERROR)
		serverlog.out(CString() << "[CSocket::destroy] shutdown returned error: " << errorMessage(identifyError()) << "\n");

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
			if (size == 0 || size != EWOULDBLOCK || size != EINPROGRESS)
				break;
		}
	}

	// Destroy the socket of d00m.
#if defined(_WIN32) || defined(_WIN64)
	if (closesocket(properties.handle) == SOCKET_ERROR)
	{
		serverlog.out("[CSocket::destroy] closesocket ");
#else
	if (close(properties.handle) == SOCKET_ERROR)
	{
		serverlog.out("[CSocket::destroy] close ");
#endif
		serverlog.out(CString() << "returned error: " << errorMessage(identifyError()) << "\n");
	}

	// Reset the socket state.
	properties.state = SOCKET_STATE_DISCONNECTED;
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
		serverlog.out("[CSocket::connect] socket() returned INVALID_SOCKET.\n");
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
			serverlog.out(CString() << "[CSocket::connect] bind() returned error: " << errorMessage(identifyError()) << "\n");
			destroy();
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
			serverlog.out(CString() << "[CSocket::connect] connect() returned error: " << errorMessage(identifyError()) << "\n");
			destroy();
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
				serverlog.out(CString() << "[CSocket::connect] listen() returned error: " << errorMessage(identifyError()) << "\n");
				destroy();
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

	// Turn on non-blocking mode.
	if (properties.options & SOCKET_OPTION_NONBLOCKING)
	{
#if defined(WIN32)
		unsigned long i = 1;
		ioctlsocket(properties.handle, FIONBIO, &i);
#elif defined(PSPSDK)
		unsigned long i = 1;
		sceNetInetSetsockopt(properties.handle, SOL_SOCKET, 0x1009, (const char*)&i, sizeof(u32));
#else
		fcntl(properties.handle, F_SETFL, O_NONBLOCK);
#endif
	}

	return 0;
}

int CSocket::disconnect()
{
	destroy();
	return 0;
}

int CSocket::reconnect(long delay, int tries)
{
	int socket_reconnect_delay = 0;
	int socket_reconnect_attempts = 1;

//	setting_geti("socket_reconnect_delay", &socket_reconnect_delay);
//	setting_geti("socket_reconnect_attempts", &socket_reconnect_attempts);

	if (delay == 0)
		delay = socket_reconnect_delay;
	if (tries == 0)
		tries = socket_reconnect_attempts;

	for (int i = 0; i < tries; i++)
	{
		switch (this->connect())
		{
			case 0:
			case SOCKET_ALREADY_CONNECTED:
				return 0;
				break;
			case SOCKET_INVALID:
			case SOCKET_BIND_ERROR:
			case SOCKET_CONNECT_ERROR:
			default:
				// Do nothing.
				break;
		}
		if (delay != 0) wait(delay);
	}
	return SOCKET_CONNECT_ERROR;
}

CSocket* CSocket::accept(long delay_sec, long delay_usec)
{
	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return 0;

	// Only server type TCP sockets can accept new connections.
	if (properties.type != SOCKET_TYPE_SERVER || properties.protocol != SOCKET_PROTOCOL_TCP)
		return 0;

	// If we have a delay, do a wait.
	if (delay_sec != 0 && delay_usec != 0)
	{
		fd_set set;
		struct timeval tm;
		tm.tv_sec = delay_sec;
		tm.tv_usec = delay_usec;
		FD_ZERO(&set);
		FD_SET(properties.handle, &set);
		select(properties.handle + 1, &set, 0, 0, &tm);
		if (!FD_ISSET(properties.handle, &set))
			return 0;
	}

	sockaddr_storage addr;
	int addrlen = sizeof(addr);
	SOCKET handle = 0;

	// Try to accept a new connection.
	handle = ::accept(properties.handle, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
	if (handle == INVALID_SOCKET)
	{
		int error = identifyError();
		if (error == EWOULDBLOCK || error == EINPROGRESS) return 0;
		serverlog.out(CString() << "[CSocket::accept] accept() returned error: " << errorMessage(error) << "\n");
		return 0;
	}

	// Create the new socket to store the new connection.
	CSocket* sock = new CSocket();
	sock_properties props;
	memset((void*)&props, 0, sizeof(sock_properties));
	memset((void*)&properties.address, 0, sizeof(struct sockaddr_storage));
	memcpy((void*)&props.address, &addr, sizeof(addr));
	props.options = properties.options;
	props.protocol = properties.protocol;
	props.type = SOCKET_TYPE_CLIENT;
	props.state = SOCKET_STATE_CONNECTED;
	props.handle = handle;
	sock->setProperties(props);
	sock->setDescription(sock->tcpIp());

	// Accept the connection by calling getsockopt.
	int type, typeSize = sizeof(int);
	getsockopt(handle, SOL_SOCKET, SO_TYPE, (char*)&type, (socklen_t*)&typeSize);

	return sock;
}

int CSocket::sendData(CString& data)
{
	int intError = 0;
	int size = 0;

	// Make sure the socket is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return SOCKET_INVALID;

	do
	{
		// See if we can send data.
		// If we can't, return how many bytes we did send.
		fd_set set;
		struct timeval tm;
		tm.tv_sec = tm.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(properties.handle, &set);
		select(properties.handle + 1, 0, &set, 0, &tm);
		if (!FD_ISSET(properties.handle, &set))
			return size;

		// Send our data, yay!
		int sent = 0;
		if ((sent = ::send(properties.handle, data.text(), data.length(), 0)) == SOCKET_ERROR)
		{
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
					serverlog.out(CString() << properties.description << " - Connection lost!  Reason: " << errorMessage(intError) << "\n");
					disconnect();
					return intError;
					break;
			}
			if (intError == EAGAIN || intError == EWOULDBLOCK || intError == EINPROGRESS) return size;
			disconnect();
			return intError;
		}

		// Remove what we sent.
		// Increase size by how much we sent.
		if (sent >= data.length())
			data.clear();
		else if (sent > 0)
			data.removeI(0, sent);
		size += sent;

	// Repeat while data is still left.
	} while (data.length() > 0 && intError == 0);

	// Return how much data was ultimately sent.
	return size;
}

int CSocket::getData()
{
	int size = 0;
	int intError = 0;
	//char buff[ 0x10000 ]; // 65536 bytes, 64KB
	char buff[ 0x2000 ]; // 8192 bytes, 8KB
	int bufflen = 0x2000;
	CString temp;

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return SOCKET_ERROR;

	do
	{
		// Make sure there is data to be read.
		// If size == bufflen, that means there may be more data.  Just in case,
		// call select so blocking sockets don't block.
		if (size == 0 || size == bufflen)
		{
			fd_set set;
			struct timeval tm;
			tm.tv_sec = tm.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(properties.handle, &set);
			select(properties.handle + 1, &set, 0, 0, &tm);
			if (!FD_ISSET(properties.handle, &set))
				return temp.length();
		}

		// Allocate buff.
		memset((void*)buff, 0, bufflen);

		// Get our data
		if (properties.protocol == SOCKET_PROTOCOL_UDP)
			size = recvfrom(properties.handle, buff, bufflen, 0, 0, 0);
		else
			size = recv(properties.handle, buff, bufflen, 0);

		// Add to the buffer.
		if (size > 0)
			temp.write(buff, size);

		// Check for error!
		if (size == SOCKET_ERROR)
		{
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
					serverlog.out(CString() << properties.description << " - Connection lost!  Reason: " << errorMessage(intError) << "\n");
					disconnect();
					break;
				default:
					break;
			}
		}
	} while (size > 0 && intError == 0);

	// If size is 0, the socket was disconnected.
	if (size == 0)
	{
		serverlog.out(CString() << properties.description << " - Connection lost!\n");
		disconnect();
	}

	// Add the data we just got to the buffer.
	if (temp.length() > 0)
		buffer.write(temp.text(), temp.length());

	// Return the amount of data obtained.
	return temp.length();
}

char* CSocket::peekData()
{
	//int recvsize = 0x10000;
	int recvsize = 0x2000;
	int intError;
	char *buff = 0;

	// Make sure it is connected!
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		return 0;

	// Make a buffer to store data!
	buff = new char[ recvsize ];
	memset((char *)buff, 0, recvsize);

	// Get our data
	if (properties.protocol == SOCKET_PROTOCOL_UDP)
		intError = recvfrom(properties.handle, buff, recvsize, MSG_PEEK, 0, 0);
	else
		intError = recv(properties.handle, buff, recvsize, MSG_PEEK);

	// Check for error!
	if (intError == SOCKET_ERROR)
	{
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
				serverlog.out(CString() << properties.description << " - Connection lost!  Reason: " << errorMessage(intError) << "\n");
				disconnect();
				break;
			default:
				break;
		}
		if (buff)
		{
			delete [] buff;
			buff = 0;
		}
		return 0;
	}

	// Return the data.
	return buff;
}

int CSocket::setProtocol(int sock_proto)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.protocol = sock_proto;
	else
		return SOCKET_INVALID;
	return 0;
}

int CSocket::setType(int sock_type)
{
	if (properties.state == SOCKET_STATE_DISCONNECTED)
		properties.type = sock_type;
	else
		return SOCKET_INVALID;
	return 0;
}

int CSocket::setOptions(int iOptions)
{
	bool changeBlocking = false;
	unsigned long i;

	// If we change the SOCKET_OPTION_NONBLOCKING option, adjust the socket mode.
	if (iOptions & SOCKET_OPTION_NONBLOCKING && !(properties.options & SOCKET_OPTION_NONBLOCKING))
	{
		changeBlocking = true;
		i = 1;
	}
	if (properties.options & SOCKET_OPTION_NONBLOCKING && !(iOptions & SOCKET_OPTION_NONBLOCKING))
	{
		changeBlocking = true;
		i = 0;
	}

	// Do the changes.
	if (changeBlocking)
	{
#if defined(WIN32)
		ioctlsocket(properties.handle, FIONBIO, &i);
#elif defined(PSPSDK)
		sceNetInetSetsockopt(properties.handle, SOL_SOCKET, 0x1009, (const char*)&i, sizeof(u32));
#else
		if (i == 1) fcntl(properties.handle, F_SETFL, O_NONBLOCK);
		else fcntl(properties.handle, F_SETFL, ~O_NONBLOCK);
#endif
	}

	// Set the options.
	properties.options = iOptions;

	return 0;
}

int CSocket::setDescription(const char *strDescription)
{
	memset((void*)&properties.description, 0, SOCKET_MAX_DESCRIPTION);
	memcpy((void*)&properties.description, strDescription, SOCKET_MAX_DESCRIPTION - 1);
	return 0;
}

int CSocket::setProperties(sock_properties newprop)
{
	// Store the old options.
	int oldoptions = properties.options;

	// Set the socket properties.
	memcpy((void*)&this->properties, (void *)&newprop, sizeof(sock_properties));

	// Restore the old options and try to set the new ones.
	properties.options = oldoptions;
	setOptions(newprop.options);

	return 0;
}

int CSocket::setState(int iState)
{
	this->properties.state = iState;
	return 0;
}

const char* CSocket::tcpIp()
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
		serverlog.out("Failed to initialize winsocks!\n");
		return 1;
	}

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		serverlog.out("Failed to initialize winsocks!  Wasn't version 2.2!\n");
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

	while (intTimeCheck++ < 10)
	{
		if (WSACleanup() == SOCKET_ERROR)
			serverlog.out(CString() << "[CSocket::socketSystemDestroy] WSACleanup() returned error: " << errorMessage(identifyError()) << "\n");
		wait(1000);
	}
#endif
}

CString errorMessage(int error)
{
	CString blank;

	// These can happen a lot.  Don't display any errors about them.
	if (error == EWOULDBLOCK || error == EINPROGRESS)
		return blank;

	switch (error)
	{
#if defined(_WIN32) || defined(_WIN64)
		case WSANOTINITIALISED:
			return CString("WSANOTINITIALISED"); break;
#endif
		case ENETDOWN:
			return CString("ENETDOWN"); break;
		case EADDRINUSE:
			return CString("EADDRINUSE"); break;
		case EINTR:
			return CString("EINTR"); break;
		case EINPROGRESS:
			return CString("EINPROGRESS"); break;
		case EALREADY:
			return CString("EALREADY"); break;
		case EADDRNOTAVAIL:
			return CString("EADDRNOTAVAIL"); break;
		case EAFNOSUPPORT:
			return CString("EAFNOSUPPORT"); break;
		case ECONNREFUSED:
			return CString("ECONNREFUSED"); break;
		case EFAULT:
			return CString("EFAULT"); break;
		case EINVAL:
			return CString("EINVAL"); break;
		case EISCONN:
			return CString("EISCONN"); break;
		case ENETUNREACH:
			return CString("ENETUNREACH"); break;
		case ENOBUFS:
			return CString("ENOBUFS"); break;
		case ENOTSOCK:
			return CString("ENOTSOCK"); break;
		case ETIMEDOUT:
			return CString("ETIMEDOUT"); break;
		case EWOULDBLOCK:
			return CString("EWOULDBLOCK"); break;
		case EACCES:
			return CString("EACCES"); break;
		case ENOTCONN:
			return CString("ENOTCONN"); break;
		case ENETRESET:
			return CString("ENETRESET"); break;
		case EOPNOTSUPP:
			return CString("EOPNOTSUPP"); break;
		case ESHUTDOWN:
			return CString("ESHUTDOWN"); break;
		case EMSGSIZE:
			return CString("EMSGSIZE"); break;
		case ECONNABORTED:
			return CString("ECONNABORTED"); break;
		case ECONNRESET:
			return CString("ECONNRESET"); break;
		case EHOSTUNREACH:
			return CString("EHOSTUNREACH"); break;
		default:
			return CString((int)error); break;
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
