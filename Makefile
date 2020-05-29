GCC=gcc
CFLAGS=-O0 -Wall -Werror -Werror=vla -g -std=gnu11 -lm -lpthread -lrt
CLIENT_FLAGS=-O0 -Wall -Werror -g -std=gnu11 -fsanitize=address
CFLAG_SAN=$(CFLAGS) -fsanitize=address
DEPS=
OBJ=readconfig.o request.o response.o compression.o

server: server.c $(OBJ)
	$(GCC) -o $@ $^ $(CFLAG_SAN)
	rm *.o

readconfig.o: conf/readconfig.c conf/readconfig.h
	$(GCC) -c -o $@ $< $(CFLAGS)

request.o: reqres/request.c reqres/request.h
	$(GCC) -c -o $@ $< $(CFLAGS)

response.o: reqres/response.c reqres/response.c
	$(GCC) -c -o $@ $< $(CFLAGS)

compression.o: reqres/compression.c reqres/compression.h
	$(GCC) -c -o $@ $< $(CFLAGS)

client: client-scaffold.c
	$(GCC) -o client-scaffold $< $(CLIENT_FLAGS)

clean:
	rm *.o
