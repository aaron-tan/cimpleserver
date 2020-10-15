#!/usr/bin/python
# Usage: ./python_client.py <IPv4 address> <Port number>
import socket
import sys

HOST = str(sys.argv[1])
PORT = int(sys.argv[2])

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.sendall('\x80\x00\x00\x00\x00\x00\x00\x00\x00')
