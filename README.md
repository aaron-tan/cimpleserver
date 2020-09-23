# CimpleServer
CimpleServer (pronounced "simple") is a server program written in C that receives requests from clients and sends back responses. The program is able to support multiple connecting clients and is also able to compress and decompress responses and requests.

## Introduction
The server creates TCP sockets to listen for requests from clients. It reads the requests and checks for a specific message header type. If the message header type is a recognised type the appropriate response is sent back to the client. A list of message header types are detailed below.

CimpleServer uses system calls to handle connections. These calls can be found using the manpages (manpages were utilised heavily during the development of this program).

The address and port on which the server will be listening on is provided with a config file. A sample config file (sample_config) is provided with IP address set to 127.0.0.1 (localhost) and port 9000. The config file also contains the name of a target directory. This target directory will be used to retrieve files and send to the client or place file contents sent from a client. A new config file can be generated using the genconf.c file in the conf directory. To create a config file run `./genconf -i <dotted quad ipv4 address xxx.xxx.xxx.xxx> -p <port number> -t <target directory> CONFIG_FILENAME`

**select (2)** is used to handle multiple connecting clients. Although this may not be the most efficient way of handling multiple connecting clients however this may be a something to improve on in future. Possibly using threads.

## Message structure
The server will accept connections from clients which are requests. Once the server receives these requests the server will form an appropriate response accordingly.

All client requests and responses consists of a series of bytes. These messages are sent and received in the following format:

+ 1 Byte message header
  * First 4 bits - Type digit: Hexadecimal digit that defines the type of request or response
  * 5th bit - Compression bit: If this bit is 1, the payload is compressed otherwise we can read the payload as is
  * 6th bit - Compression required: If this bit is 1 in a request, then the server must compress the response (except for error responses). Otherwise, compression is optional. It has no meaning in a response message.
  * 7th and 8th bit - padding bits, except for retrieve requests. If the client sends a retrieve request with the 7th bit set to 1 then the server must send the entire requested file contents back to the client. Otherwise, the server seeks the file to the offset and reads from the offset to data length provided in the payload and packages this as part of the response.

+ 8 bytes - Unsigned integer payload length in network byte order (big endian)

+ Variable byte payload - sequence of bytes with length specified previously.

For example: {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xab, 0xcd, 0xef, 0x23, 0x76, 0xeb, 0xcb}

+ 08 - 1 byte message header. This is an echo request (see below)
  - 0000 - 4 bits type digit
  - 1 - 1 bit flag means the payload is compressed
  - 0 - 1 bit flag meaning the response does not have to be compressed
  - 00 - 2 bits padding
+ 00 00 00 00 00 00 07 - 8 bytes for a payload length of 7 bytes
+ ab cd ef 23 76 eb cb - the 7 byte payload

## Request/response
The following is a list of all possible requests clients can send and responses from the server. Each has their own type digit and structure.

### Error functionality
Any client request of an unknown type digit will be sent back an error response by the server. The response is sent with a type digit of 0xf with no payload (payload length of 0) and the connection is closed. This response is also sent if there are any other errors. Error messages are not compressed.

### Echo functionality
The echo request type digit is 0x0 with an arbitrary byte payload. If the payload is compressed it will be decompressed first.

In response, the server will send back a response of:
+ 1 byte header with type digit 0x1
+ 8 byte payload length representing an arbitrary unsigned integer
+ Payload of arbitrary bytes which is a copy of the payload sent in the request.

If necessary, the payload will be compressed.

### Directory listing functionality
The listing request type digit is 0x2 with no payload and payload length 0.

The server will send back a response of:
+ 1 byte header with type digit 0x3
+ 8 byte payload length representing an arbitrary unsigned integer
+ Payload of arbitrary bytes of all the filenames in the target directory specified in the config file provided.

Subdirectories and symlinks are not included. The filenames are sent end to end and are separated by NULL bytes. If the directory is empty, a single NULL byte is sent as payload.

### File size query
Request type 0x4 is a file size query. The payload containing a NULL-terminated filename string.

Server response will contain:
+ 1 byte header with type digit 0x5
+ 8 byte payload length
+ 8 byte payload of the size of the target file (in network byte order)

If the file does not exist, an error response is sent.

## Retrieve file
Request type 0x6 is a retrieve file request. This requests for part or whole of a file in the target directory specified in the config file. Besides the usual structure of a typical message it also has:

+ 4 bytes - an arbitrary sequence of bytes representing a session ID.
+ 8 bytes - the starting offset of the file that is to be retrieved
+ 8 bytes - the length of the data to be retrieved
+ Variable bytes - the null terminated string representing the filename.

In response, the server will send a message with type digit 0x7 with payload consisting of a portion of the file contents that was requested from the target directory. The start of the file is seeked forwards using the offset that was part of the payload from the request message, and a length specified from the same request message previously is read into the response. This is sent back to the client.

For now, handling of multiple connections requesting file retrieval is not yet implemented. If multiple connections are made with file retrieval request a response with type 0x7 and empty payload is sent instead. However, this may be a feature that could be implemented in future.

## Receive file
Receive request type digit 0x9 is a receive file request. Any client connected to the server can send a receive request which contains as its payload the contents of a file that it wants the server to receive.

The structure of the message will be as follows:
+ 1 byte message header - type digit 0x9
+ 8 bytes - payload length
Payload of the message consists of:
+ First 8 bytes - length of the filename including the null byte
+ Variable length obtained from the previous 8 bytes is the filename itself
+ Rest of the payload contains the data of the file.

Once the server successfully receives the request message and writes the contents to the file in the target directory, the server will send back a response containing type digit 0xa with zero payload and payload length. This is a confirmation response sent to the client to let the client know that the server has successfully received the message.

## Shutdown
Request type digit 0x8 with no payload is a shutdown command. The server will close all connections, clean up all allocated memory, processes and threads and exit.

## Compression
Compression is applied by replacing the uncompressed bytes in the payload with variable length bit sequences known as bit codes. The bit codes are given by a compression dictionary which defines a mapping of bytes to a variable length bit sequence to replace it by and consists of 256 segments. Each of the segments corresponds to bytes ranging from 0x00 to 0xff. Note that there will be padding of 0 bits to align the sequence to the byte boundary.

The compression dictionary provided in the binary file **compression.dict** contains the following structure consisting of 256 segments representing bytes 0x00 to 0xff, no padding in between:
+ 1 byte - unsigned integer representing the length of the code
+ Variable number of bits - the bit code used to encode the byte value

To compress a payload simply read the segments until you find the byte you want. Then read the length of the code and replace the byte with that bit code. Repeat for each byte in the payload and the result will be the compressed payload.

To simplify this, I create a dictionary which maps each bit code to the byte value it represents. This is done with the `create_dict` function in **compress.c** which will read the bit code following the procedure above. This dictionary allows us to 'look-up' the bit code we want. I create an array of `struct bit_code` and for each byte we can simply call, for example, `code_arr[0x00]` and this will return the `struct bit_code` which contains the bit code we need as a member variable.
