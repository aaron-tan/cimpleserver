# CimpleServer
CimpleServer (pronounced "simple") is a server program written in C that receives requests from clients and sends back responses. The program is able to support multiple connecting clients and is also able to compress and decompress responses and requests.

## Introduction
The server creates TCP sockets to listen for requests from clients. It reads the requests and checks for a specific message header type. If the message header type is a recognised type the appropriate response is sent back to the client. A list of message header types are detailed below.

CimpleServer uses system calls to handle connections. These calls can be found using the manpages (manpages were utilised heavily during the development of this program).

The address and port on which the server will be listening on is provided with a config file. A sample config file (sample_config) is provided with IP address set to 127.0.0.1 (localhost) and port 9000. The config file also contains the name of a target directory. This target directory will be used to retrieve files and send to the client or place file contents sent from a client. A new config file can be generated using the genconf.c file in the conf directory. To create a config file run `./genconf -i <dotted quad ipv4 address xxx.xxx.xxx.xxx> -p <port number> -t <target directory> CONFIG_FILENAME`

The main program uses the system calls **socket (2)** to create TCP sockets with IPv4 protocol (AF_INET) and **bind (2)** to bind the address and port provided in the config file to the socket. Then we use **listen (2)**, **accept (2)**, **send (2)** and **recv(2)** to listen for and accept connections and checking them before sending or receiving responses and requests.

To handle multiple connecting clients I have used **select (2)** to do so. I understand select may not be the most efficient way of handling multiple connecting clients however this may be a something to improve on in future. Possibly using threads.
