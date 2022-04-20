.SILENT:

all: server

%.o: %.cpp
	gcc -c -o $@ $<

server: server.o rooms.o client.o
	g++ -o server server.o rooms.o client.o

clean:
	@rm -rf server *.o
