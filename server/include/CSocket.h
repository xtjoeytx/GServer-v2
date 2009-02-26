#ifndef CSOCKET_H
#define CSOCKET_H

// Defines
//#define SOCK_BUFFER						8192
//#define SEND_BUFFER						4096

#define SOCKET_MAX_DESCRIPTION			51

#define SOCKET_PROTOCOL_UDP				0
#define SOCKET_PROTOCOL_TCP				1
#define SOCKET_TYPE_CLIENT				0
#define SOCKET_TYPE_SERVER				1

#define SOCKET_STATE_DISCONNECTED		0
#define SOCKET_STATE_CONNECTING			1
#define SOCKET_STATE_CONNECTED			2
#define SOCKET_STATE_LISTENING			3
#define SOCKET_STATE_TERMINATING		4

#define SOCKET_OK						0
#define SOCKET_INVALID					1
#define SOCKET_HOST_UNKNOWN				2
#define SOCKET_BIND_ERROR				3
#define SOCKET_CONNECT_ERROR			4
#define SOCKET_ALREADY_CONNECTED		5
#define SOCKET_SEND_FAILED				6
#define SOCKET_UNKNOWN_DESC				7

#define SOCKET_PROTOCOL_ANY				0
#define SOCKET_PROTOCOL_IPV4			1
#define SOCKET_PROTOCOL_IPV6			2

#if defined(_WIN32) || defined(_WIN64)
	#include <winsock2.h>
#else
	#include <netinet/in.h>
 	#include <sys/socket.h>
	#include <sys/types.h>
	#include <arpa/inet.h>

	typedef unsigned int SOCKET;
#endif
#include <vector>


//! Base class that handles socket functions.
//! Derive from this class and define the functions.
//! Then, pass the class to CSocketManager::registerSocket().
class CSocketStub
{
	public:
		virtual bool onRecv() = 0;
		virtual bool onSend() = 0;
		virtual SOCKET getSocketHandle() = 0;
		virtual bool canSend() = 0;
};

//! Socket manager class.
class CSocketManager
{
	public:
		//! Constructor.
		CSocketManager() : fd_max(0), blockStubs(false) {}

		//! Updates the state of the sockets.
		//! Calls the functions of all the registered CSocketStub classes.
		//! \param sec Seconds to wait.
		//! \param usec Nanoseconds to wait.
		//! \return False if select() returned nothing, true otherwise.
		bool update(long sec = 0, long usec = 0);

		//! Updates a single socket.
		//! Calls the functions of the CSocketStub class.
		//! \param sec Seconds to wait.
		//! \param usec Nanoseconds to wait.
		//! \return False if select() returned nothing, true otherwise.
		bool updateSingle(CSocketStub* stub, long sec = 0, long usec = 0);

		//! Registers a class derived from CSocketStub into the management system.
		//! \param stub The class to add to the system.
		//! \return true.
		bool registerSocket(CSocketStub* stub);

		//! Unregisters a class.
		//! \param stub The class to remove from the system.
		//! \return False if stub is not found
		//! \return True if it is successfully removed.
		bool unregisterSocket(CSocketStub* stub);

	private:
		//! List of classes registered with the socket manager.
		std::vector<CSocketStub*> stubList;
		std::vector<CSocketStub*> newStubs;
		std::vector<CSocketStub*> removeStubs;

		//! Max socket descriptor.
		SOCKET fd_max;

		//! Are we accessing stubList?
		bool blockStubs;
};




//! Properties to pass to the socket.
struct sock_properties
{
	sock_properties() : handle(0), protocol(SOCKET_PROTOCOL_TCP),
		type(SOCKET_TYPE_CLIENT), state(SOCKET_STATE_DISCONNECTED) {}

	SOCKET handle;
	int protocol;
	int type;
	int state;
	char description[ SOCKET_MAX_DESCRIPTION ];
	sockaddr_storage address;
};

//! Socket class.
class CSocket
{
	private:
		//! Socket properties.
		sock_properties properties;

		//! For Winsocks related stuff.
		static int was_initiated;
		static int socketSystemInit();

	public:
		//! Constructors-Destructors.
		CSocket();
		CSocket(const char* host, const char* port, sock_properties* properties = 0);
		~CSocket();

		//! Initializes a socket (does not connect to it.)
		/*! If SOCKET_PROTOCOL_ANY is used on Windows Vista and up, and we are a server
			type socket, there is a good chance we will end up binding to the ipv6
			localhost.  Take caution as the socket won't be reachable via an ipv4 address. */
		//! \param host The host to connect to.
		//! \param port The port to connect to on the host.
		//! \param protocol The protocol of the socket.
		//! \return SOCKET_OK if everything went fine.
		//! \return SOCKET_ALREADY_CONNECTED if the socket is already connected.
		//! \return SOCKET_HOST_UNKNOWN if getaddrinfo() errored.
		//! \return SOCKET_ERROR if the socket's properties are malformed.
		int init(const char* host, const char* port, int protocol = SOCKET_PROTOCOL_IPV4);

		//! Connects the socket.
		//! \return SOCKET_OK if everything went fine.
		//! \return SOCKET_ALREADY_CONNECTED if the socket is already connected.
		//! \return SOCKET_INVALID if socket() returned an invalid socket.
		//! \return SOCKET_BIND_ERROR if bind() failed.
		//! \return SOCKET_CONNECT_ERROR if connect() or listen() failed.
		int connect();

		//! Disconnects the socket.
		void disconnect();

		//! Reconnects a socket.
		//! \param delay How many milliseconds to delay between each try.
		//! \param tries How many tries to reconnect before it fails.
		//! \return SOCKET_OK if it succeeds at reconnecting the socket.
		//! \return SOCKET_CONNECT_ERROR if it fails to reconnect the socket.
		int reconnect(long delay = 0, int tries = 1);

		//! Accepts a new socket of this socket was connect as a server type.
		//! \return A new socket, else a null pointer if it failed.
		CSocket* accept();

		//! Sends data across the socket.
		//! \param data The data to send.
		//! \param dsize The amount of data to send.  Will get changed to how much data is left to be sent (in case not everything was sent.)
		//! \return How much data was sent.
		int sendData(char* data, unsigned int* dsize);

		//! Gets data from the socket.
		//! \param dsize Is set to how much data was returned from the socket.
		//! \return The data from the socket.
		char* getData(unsigned int* dsize);

		//! Gets data from the socket without removing it.
		//! \param dsize Is set to how much data was returned from the socket.
		//! \return The data from the socket.
		char* peekData(unsigned int* dsize);

		//! Gets the socket handle.
		//! \return The socket handle.
		SOCKET getHandle();

		//! Gets the socket protocol.
		//! \return The socket protocol.
		int getProtocol();

		//! Gets the socket type.
		//! \return The socket type.
		int getType();

		//! Gets the socket description.
		//! \return The socket description.
		const char *getDescription();

		//! Gets the socket state.
		//! \return The socket state.
		int getState();

		//! Sets the socket protocol.
		/*! Sets the socket protocol.  Can only be used when the socket is disconnected. */
		//! \param sock_proto The protocol to set the socket to.
		//! \return SOCKET_OK if the protocol was successfully changed.
		//! \return SOCKET_INVALID if the socket is not disconnected.
		int setProtocol(int sock_proto);

		//! Sets the socket type.
		/*! Sets the socket type.  Can only be used when the socket is disconnected. */
		//! \param sock_type The type to set the socket to.
		//! \return SOCKET_OK if the type was successfully changed.
		//! \return SOCKET_INVALID if the socket is not disconnected.
		int setType(int sock_type);

		//! Sets the socket description.
		//! \param strDescription The new description to apply to the socket.
		//! \return SOCKET_OK
		int setDescription(const char *strDescription);

		//! Sets the socket's properties.
		//! \param newprop The properties to set.
		//! \return SOCKET_OK
		int setProperties(sock_properties newprop);

		//! Sets the socket's state.
		//! \param iState The new state of the socket.
		//! \return SOCKET_OK
		int setState(int iState);

		//! Gets the IP address of the device at the other end of the socket.
		//! \return The IP address.
		const char* getRemoteIp();

		//! Gets the IP address of the current device.
		/*! Gets the IP address of the current device.
			Linux beware.  It will return the IP address in /etc/hosts that corresponds
			to the hostname in /etc/hostname. */
		//! \return The IP address.
		const char* getLocalIp();

		//! Destroys the socket subsystems.
		/*! Windows specific. */
		static void socketSystemDestroy();
};

inline
SOCKET CSocket::getHandle()
{
	return properties.handle;
}

inline
int CSocket::getProtocol()
{
	return properties.protocol;
}

inline
int CSocket::getType()
{
	return properties.type;
}

inline
const char *CSocket::getDescription()
{
	return properties.description;
}

inline
int CSocket::getState()
{
	return properties.state;
}

#endif
