main:main.o http_parser.o tool.o log.o HttpConnection.o RequestParser.o
	g++ -std=c++14 main.o http_parser.o tool.o log.o HttpConnection.o RequestParser.o -o http_server
http_parser.o:http_parser.h http_parser.c
	g++ -std=c++14 -c http_parser.c
tool.o:tool.h tool.cpp
	g++ -std=c++14 -c tool.cpp
log.o:log.h log.cpp
	g++ -std=c++14 -c log.cpp
HttpConnection.o:HttpConnection.h HttpConnection.cpp
	g++ -std=c++14 -c HttpConnection.cpp
RequestParser.o:RequestParser.h RequestParser.cpp
	g++ -std=c++14 -c RequestParser.cpp
main.o:main.cpp
	g++ -std=c++14 -c main.cpp

clean:
	rm *.o