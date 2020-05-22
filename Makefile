GCC=gcc
CFLAGS=-O0 -Wall -Werror -Werror=vla -g -std=gnu11 -lm -lpthread -lrt
CFLAG_SAN=$(CFLAGS) -fsanitize=address
DEPS=
OBJ=readconfig.o request.o response.o

server: server.c $(OBJ)
	$(GCC) -o $@ $^ $(CFLAG_SAN)
	rm *.o

readconfig.o: conf/readconfig.c conf/readconfig.h
	$(GCC) -c -o $@ $< $(CFLAGS)

request.o: reqres/request.c reqres/request.h
	$(GCC) -c -o $@ $< $(CFLAGS)

response.o: reqres/response.c reqres/response.c
	$(GCC) -c -o $@ $< $(CFLAGS)

clean:
	rm *.o
