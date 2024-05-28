OBJS = tcpSocket.o udpSocket.o main.o
CC = g++ -std=c++17
CFLAGS = -Wall -O2 -c
LFLAGS = -Wall
binary = something

main : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $(binary)

main.o : main.cpp
	$(CC) $(CFLAGS) main.cpp -o $@

tcpSocket.o:
	$(CC) $(CFLAGS) ../sharedLib/TcpSocket.cpp -o $@

udpSocket.o:
	$(CC) $(CFLAGS) ../sharedLib/UdpSocket.cpp -o $@

clean:
	rm -f $(binary) $(OBJS)