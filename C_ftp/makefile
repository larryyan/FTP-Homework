all: ftp_server ftp_client

ftp_server: ftp_server.c ftp_common.h
	gcc -o ftp_server ftp_server.c -lpthread -lcrypt

ftp_client: ftp_client.c ftp_common.h
	gcc -o ftp_client ftp_client.c
