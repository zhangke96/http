#include "HttpConnection.h"

const char *HttpConnection::init()
{
	len = 80 * 1024;
	buf = new char[len];
	workP = buf;
	return buf;
}

HttpConnection::HttpConnection(HttpConnection &&rhs) noexcept
	: aParser(std::move(rhs.aParser)), state(std::move(rhs.state)), clfd(std::move(rhs.clfd)),
		dfdwrap(std::move(rhs.dfdwrap)), sendFdWrap(std::move(rhs.sendFdWrap))
{
	buf = rhs.buf; rhs.buf = nullptr;
	workP = rhs.workP;
	recved = std::move(rhs.recved);
	len = std::move(rhs.len);
	nparsed = std::move(rhs.nparsed);
}
HttpConnection& HttpConnection::operator=(HttpConnection &&rhs) noexcept
{
	if (this != &rhs)
	{
		aParser = std::move(rhs.aParser);
		state = std::move(rhs.state);
		clfd = std::move(rhs.clfd);
		buf = rhs.buf; rhs.buf = nullptr;
		workP = rhs.workP;
		recved = std::move(rhs.recved);
		len = std::move(rhs.len);
		nparsed = std::move(rhs.nparsed);
		dfdwrap = std::move(rhs.dfdwrap);
		sendFdWrap = std::move(rhs.sendFdWrap);
	}
	return *this;
}
int HttpConnection::read()
{
	if (aParser.isFinished()) // finished the read the request
		return 0;
	recved = recv(clfd.getfd(), buf, len - 1, 0);
	if (recved < 0)
		return recved;
	nparsed = aParser.parser_execute(workP, recved);
	if (aParser.isFinished() && state == Http_Connection_Receiving)
	{
		state = Http_Connection_ReceiveAll;
		std::cout << "request file :" << aParser.getUrl() << std::endl;
	}
	if (nparsed != recved)
	{
		workP = workP + nparsed;
		len = len - 1 - nparsed;
	}
	return recved;
}

int HttpConnection::write()
{
	if (aParser.getMethod() != HTTP_GET) // now only support Get method
	{
		Log(LOG_WARN, "client ask for unsupport http method");
		return -1;
	}
	if (aParser.isFinished() && state == Http_Connection_ReceiveAll) // send head first
	{
		state = Http_Connection_SendingHead;
		int dfd = open("./", O_RDONLY);
		if (dfd < 0)
		{
			Log(LOG_ERROR, "error when open dir ./" + std::string(strerror(errno)));
			return -1;
		}
		dfdwrap.reset(dfd);
		Log(LOG_INFO, "request file " + aParser.getUrl());
		int sendFd;
		if (aParser.getUrl() == "/")
			sendFd = open("index.html", O_RDONLY);
		else
			sendFd = open(aParser.getUrl().c_str() + 1, O_RDONLY);
		if (sendFd < 0) // error open file
		{
			Log(LOG_WARN, "failed to open file: " + aParser.getUrl());
			std::string notFoundHead = createNotFound();
			send(clfd.getfd(), notFoundHead.c_str(), notFoundHead.size(), 0);
			sendFd = open("404.html", O_RDONLY);
			if (sendFd < 0)
			{
				Log(LOG_ERROR, "failed to open 404 file");
				return -1;
			}
			sendFdWrap.reset(sendFd);
		}
		else
		{
			sendFdWrap.reset(sendFd);
			std::string ok = createOk();
			send(clfd.getfd(), ok.c_str(), ok.size(), 0);
		}
		// have sended head
		state = Http_Connection_SendingBody;
		sendABlock();
		return 0;
	}
	else if (state == Http_Connection_SendingBody)
	{
		sendABlock();
		return 0;
	}
	else
		return -1;
}

void HttpConnection::sendABlock()
{
	ssize_t readSize;
	ssize_t sendSize;
	size_t bufsize = 80 * 1024;
	int remainSize = bufsize - 2;
	char sizebuf[15];        // used to store chunked size
	int blockSize = 0;
	int index = 0;
	int size = 0;
	while ((size = sendFdWrap.read(buf + index, remainSize)) > 0)
	{
		blockSize += size;
		index += size;
		remainSize -= size;
	}
	if (size < 0)
	{
		Log(LOG_ERROR, "error when read file");
		return;
	}
	snprintf(sizebuf, 14, "%x\r\n", blockSize);
	buf[blockSize] = '\r';
	buf[blockSize + 1] = '\n';
	if ((sendSize = send(clfd.getfd(), sizebuf, strlen(sizebuf), 0)) != strlen(sizebuf))
	{
		Log(LOG_ERROR, "error when send chunked size");
		return;
	}
	if ((sendSize = send(clfd.getfd(), buf, blockSize + 2, 0)) != blockSize + 2)
	{
		Log(LOG_ERROR, "error when send chunked body");
		return;
	}
	if (sendFdWrap.endOfFile())
		state = Http_Connection_SendAll;
}