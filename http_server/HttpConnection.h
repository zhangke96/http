#pragma once
#include "aux_class.h"
#include "RequestParser.h"
#include <sys/socket.h>
#include "log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

enum HttpConnectionState
{
	Http_Connection_Receiving,
	Http_Connection_ReceiveAll,
	Http_Connection_SendingHead,
	Http_Connection_SendingBody,
	Http_Connection_SendAll
};

static const char terminate[] = "0\r\n\r\n";
// This class is designed for HTTP connection:close
// copy constructor!!!
struct HttpConnection
{
public:
	explicit HttpConnection(fdwrap clientfd) : clfd(clientfd), state(Http_Connection_Receiving)
	{
		if (init() == nullptr)
		{
			// do not have enough memory
		}
	}
	~HttpConnection() { delete[] buf; }
	HttpConnection(const HttpConnection &rhs) = delete;
	HttpConnection &operator=(const HttpConnection &rhs) = delete;
	/*{
		clfd = rhs.clfd;
		recved = rhs.recved;
		len = rhs.len;
		nparsed = rhs.nparsed;
		dfdwrap = rhs.dfdwrap;
		sendFdWrap = rhs.sendFdWrap;
		state = rhs.state;
		aParser = rhs.aParser;
		buf = new char[len];
		workP = buf;
		return *this;
	}*/
	HttpConnection(HttpConnection &&rhs) noexcept;
	HttpConnection& operator=(HttpConnection &&rhs) noexcept;
	int getfd() { return clfd.getfd(); }
	bool getAllRequest() const { return aParser.isFinished(); }
	bool sendAllBody() const { return state == Http_Connection_SendAll; }
	int read();  // read the request from the client and parse
				 // return size, error return 0
	int write(); // write response to the client
				 // return size, error return 0

	RequestParser aParser;
	HttpConnectionState state;
	
private:
	fdwrap clfd;
	const char *init();  // initialize the buf and some counters
	char *buf;           // client request buffer
	char *workP;         // work poi nter
	ssize_t recved;      // record the size read from the client
	size_t len;          // record the size buffer remain
	size_t nparsed;      // record the size parsed
	fdwrap dfdwrap;      // RAII directory
	fdwrap sendFdWrap;   // RAII send file
	void sendABlock();   // everytime send a max block size to split network equally
};

