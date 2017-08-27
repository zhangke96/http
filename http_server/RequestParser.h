#ifndef REQUESTPARSER
#define REQUESTPARSER

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <functional>
#include "http_parser.h"

class RequestParser
{

public:
	typedef int callback_t(http_parser *);
	typedef int callbackl_t(http_parser *, const char *, size_t);
	RequestParser();
	bool isFinished() const { return finished; };
	size_t parser_execute(const char *buf, size_t recved);     /* recved is the length of the content in buf */
	int getMethod() const /* return http method */
	{
		return method;
	}
	std::string getUrl() const    /* return url */
	{
		return url;
	}
	//std::string getHostName() const; /* return host name */
	std::string getValue(const std::string &field) const /* return the value correspond to the field */
	{
		auto index = fvpairs.find(field);
		if (index == fvpairs.end())
			return "NULL";
		else
			return index->second;
	}
	std::string getValue(const char *field) const /* return the value correspond to the field */
	{
		auto index = fvpairs.find(std::string(field));
		if (index == fvpairs.end())
			return "NULL";
		else
			return index->second;
	}
	//    std::string getCookie() const /*return cookie */
	std::string getGetMethodParameter(const std::string &id) const /* return the value correspond to the id */
	{
		return url;
	}
	RequestParser(const RequestParser &) = delete;
	RequestParser &operator=(const RequestParser &) = delete;
	RequestParser(RequestParser &&rhs) noexcept
		: fvpairs(std::move(rhs.fvpairs)), tempField(std::move(rhs.tempField)),
		  url(std::move(rhs.url))
	{
		num = std::move(rhs.num);
		parser = rhs.parser; rhs.parser = nullptr;
		parser->data = this;
		settings = std::move(rhs.settings);
		finished = std::move(rhs.finished);
		method = std::move(rhs.method);
	}
	RequestParser& operator=(RequestParser &&rhs) noexcept
	{
		if (this != &rhs)
		{
			free(parser);
			parser = rhs.parser; rhs.parser = nullptr;
			num = std::move(rhs.num);
			fvpairs = std::move(rhs.fvpairs);
			tempField = std::move(rhs.tempField);
			settings = std::move(rhs.settings);
			url = std::move(rhs.url);
			finished = std::move(rhs.finished);
			method = std::move(rhs.method);
		}
		return *this;
	}
    ~RequestParser()
    {
        free(parser);
    }
private:
    int num;
    http_parser *parser;
    std::map<std::string, std::string> fvpairs;   // Header fields and values
    std::string tempField = "";
    http_parser_settings settings;
    std::string url = "";
    bool finished;
    int method = -1;
   /* std::function<int(http_parser *)> call_message_begin_cb_function
    = [this](http_parser *p)
            {
                std::cout << "begin" << std::endl;
                return 0;
            };*/
    static int call_message_begin_cb(http_parser *p)
    {
        // printf("call_message_begin\n");
        return 0;
    }
   /* std::function<int(http_parser *, const char *, size_t)> call_header_field_cb_function
    = [this](http_parser *p, const char *buf, size_t len)
        {
            tempField.assign(buf, len);
            return 0;
        };*/
    static int call_header_field_cb(http_parser *p, const char *buf, size_t len)
    {
        // printf("call_header_field %s %zu\n", buf, len);
        RequestParser *now = static_cast<RequestParser *>(p->data);
        (now->tempField).assign(buf, len);
//        return call_message_begin_cb;
        return 0;
    }
   /* std::function<int(http_parser *, const char *, size_t)> call__header_value_cb_function
    = [this](http_parser *p, const char *buf, size_t len)
        {
            fvpairs.insert({tempField, std::string(buf, len)});
            tempField.clear();
            return 0;
        };*/
    static int call_header_value_cb(http_parser *p, const char *buf, size_t len)
    {
        // printf("call_header_value %s %zu\n", buf, len);
        RequestParser *now = static_cast<RequestParser *>(p->data);
        (now->fvpairs).insert({now->tempField, std::string(buf, len)});
        (now->tempField).clear();
        return 0;
    }
   /* std::function<int(http_parser *, const char *, size_t)> call_request_url_cb_function
    = [this](http_parser *p, const char *buf, size_t len)
        {
            url.assign(buf, len);
            return 0;
        };*/
    static int call_request_url_cb(http_parser *p, const char *buf, size_t len)
    {
        // printf("call_request_url %s %zu\n", buf, len);
        RequestParser *now = static_cast<RequestParser *>(p->data);
        (now->url).assign(buf, len);
        return 0;
    }
    /*std::function<int(http_parser *, const char *, size_t)> call_body_cb_function
    = [](http_parser *p, const char *buf, size_t len)
        {
            return 0;
        };*/
    static int call_body_cb(http_parser *p, const char *buf, size_t len)
    {
        // printf("call_body %s %zu\n", buf, len);
        return 0;
    }
   /* std::function<int(http_parser *)> call_headers_complete_cb_function
    = [](http_parser *p)
        {
            return 0;
        };*/
    static int call_headers_complete_cb(http_parser *p)
    {
        // printf("call_headers_complete\n");
        return 0;
    }
    /*std::function<int(http_parser *)> call_message_complete_cb_function
    = [this](http_parser *p)
        {
            finished = true;
            method = p->method;
            return 0;
        };*/
    static int call_message_complete_cb(http_parser *p)
    {
       // printf("call_message_complete\n");
       //  printf("method: %d\n", p->method);
        RequestParser *now = static_cast<RequestParser *>(p->data);
        now->finished = true;
        now->method = p->method;
        return 0;
    }
    /*std::function<int(http_parser *, const char *, size_t)> call_response_status_cb_function
    = [](http_parser *p, const char *buf, size_t len)
        {
            return 0;
        };*/
    static int call_response_status_cb(http_parser *p, const char *buf, size_t len)
    {
        // printf("call_reponse_status %s %zu\n", buf, len);
        return 0;
    }
   /* std::function<int(http_parser *)> call_chunk_header_cb_function
    = [](http_parser *p)
        {
            return 0;
        };*/
    static int call_chunk_header_cb(http_parser *p)
    {
        // printf("call_chunk_header\n");
        return 0;
    }
   /* std::function<int(http_parser *)> call_chunk_complete_cb_function
    = [](http_parser *p)
        {
            return 0;
        };*/
    static int call_chunk_complete_cb(http_parser *p)
    {
        // printf("call_chunk_complete\n");
        return 0;
    }
};

#endif // !REQUESTPARSER

