
import binascii
import socket
import struct

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('',12000))

unpacker = struct.Struct('I 2s f')

while True:
    data, addr = sock.recvfrom(unpacker.size)
    print 'received "%s"' % binascii.hexlify(data)
    print unpacker.unpack(data)

