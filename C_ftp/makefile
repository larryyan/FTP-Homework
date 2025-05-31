all: ftp_server ftp_client

ftp_server: ftp_server.c ftp_common.h
	gcc -o ftp_server ftp_server.c ftp_common.c -lpthread -lcrypt -lyaml

ftp_client: ftp_client.c ftp_common.h
	gcc -o ftp_client ftp_client.c ftp_common.c -lyaml
