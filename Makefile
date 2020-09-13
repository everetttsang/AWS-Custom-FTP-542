all: server.c client.c
	gcc -o server server.c
	gcc -o client client.c
	rm output.txt

clean: server client
		rm -f server
		rm -f client

test: output.txt test.txt
		md5 output.txt
		md5 test.txt
