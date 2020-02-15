import binascii
import socket
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
addr = ('127.0.0.1',12000)

packer = struct.Struct('I 2s f')
data = packer.pack(1, 'ab', 2.7)

try:
    print 'sending "%s"' % binascii.hexlify(data)
    sock.sendto(data,addr)

finally:
    print 'closing socket'
    sock.close()

