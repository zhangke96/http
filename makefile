main:main.c http_parser.c http_parser.h tool.h
	gcc -Wall main.c http_parser.c -lpthread -o main
