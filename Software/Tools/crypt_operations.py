#!/usr/bin/python

from __future__ import with_statement
import sys

"""
There are 4 operations:
 - createbin does the same as the original Createbin.exe file (AES+scrambling)
 - decryptbin removes the AES encryption _but not_ the scrambling
 - scramblebin removes/adds (it's symmetric) the scrambling.
 - scramblehex removes/adds (it's symmetric) the scrambling but for ihex files.

The crypto operations need the PyCrypto package.
"""

def createbin(file_inp, file_out):
    from Crypto.Cipher import AES
    data_inp = file_inp.read()
    if len(data_inp) % 16 != 0:
        data_inp += b'\0'*(16-len(data_inp)%16)
    data_inp += b'\xcd'*(16-len(data_inp)%16) #strange fill in value on heap
    for i in range(0,len(data_inp),16):
        aes = AES.new(b'designed by dxls', AES.MODE_CBC, '\0'*16)
        block = data_inp[i:i+16]
        scrambled = [(block[j] ^ ((0x2d + i + j)&0xff)) for j in range(16)]
        out = aes.decrypt(bytes(scrambled))
        file_out.write(out)

def decryptbin(file_inp, file_out):
    from Crypto.Cipher import AES
    data_inp = file_inp.read()
    for i in range(0,len(data_inp),16):
        aes = AES.new(b'designed by dxls', AES.MODE_CBC, '\0'*16)
        block = data_inp[i:i+16]
        out = aes.encrypt(bytes(block))
        file_out.write(out)

def scramblebin(file_inp, file_out):
    initial = 0x2d
    data_inp = file_inp.read()
    for i in range(len(data_inp)):
        file_out.write(bytes([((initial + i) & 0xff) ^ data_inp[i]]))

def scramblehex(file_inp, file_out):
    data_inp = [str(x,"ascii").strip() for x in file_inp.readlines()]
    data_out = []
    TYPE_DATA = 0
    TYPE_EOF = 1
    def tohex(x):
        return hex(x)[2:].rjust(2,'0').upper()
    def fromhex(x):
        return int(x,16)
    for line in data_inp:
        if line[0] != ":":
            print("Invalid ihex file.")
            sys.exit(-1)
        count, address, type_, data, checksum = line[1:3], line[3:7], line[7:9], line[9:-2], line[-2:]
        count = fromhex(count)
        address = fromhex(address)
        type_ = fromhex(type_)
        checksum = fromhex(checksum)
        outline = line[:9]
        if type_ == TYPE_DATA:
            bytes_ = [fromhex(data[x*2:x*2+2]) for x in range(count)]
            for i, x in enumerate(bytes_):
                bytes_[i] = ((address + 0x2d + i) & 0xff) ^ x
            outline += "".join([tohex(x) for x in bytes_])
            checksum = 0
            for x in range(1, len(outline) , 2):
                checksum += fromhex(outline[x:x+2])
            checksum = (0x100 - (checksum & 0xff)) & 0xff
            outline += tohex(checksum)
        else:
            outline += line[9:].strip()
        data_out.append(outline)
    data_out.append("") #blank line needed for dfu-programmer
    file_out.write("\r\n".join(data_out).encode("ascii"))

operations = {
    "createbin": createbin,
    "decryptbin": decryptbin,
    "scramblebin": scramblebin,
    "scramblehex": scramblehex
}

def main(argv):
    if len(argv) != 4:
        print("Usage: {} <action> <inputfile> <outputfile>".format(argv[0]))
        sys.exit(1)
    if argv[1] not in operations.keys():
        print("Action should be one of: {}".format(",".join(operations.keys())))
        sys.exit(2)
    
    with open(argv[2], "rb") as file_inp, open(argv[3], "wb") as file_out:
        operations[argv[1]](file_inp, file_out)
        
    sys.exit(0)

if __name__ == '__main__':
    main(sys.argv)
