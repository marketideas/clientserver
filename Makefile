.SILENT:

all: server

%.o: %.cpp
	gcc -c -o $@ $<

server: server.o client.o
	g++ -o server server.o client.o -lpthread

clean:
	@rm -rf server *.o
