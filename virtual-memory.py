from bitstring import BitArray
import threading
import time

#64mb ram
#page 4mb

MB = 1024 * 1024

memory = 64 * MB

buffer = bytearray(memory)

isinmemory = BitArray(256)
struct = [] # 256 * 3 byte
#TODO: add own: just arry no list

#  64MB 4MB 4096 4194304
#  read 2000s byte => page 0
#  read 5000000s byte => page 1

# Used RAM
# is in memory: 32 byte (256bit)
# 256 *
# lastused => 2byte => 18h
# where start address: 1 byte 0 - 15 255=>unmapped
# 800 byte

currenttimer = 0

def addressToPageNr(address):
    for i in range(256):
        min = i * 4 * MB
        max = (i+1) * 4 * MB
        if address >= min and address < max:
            return i

def getAddressPage(pagenr):
    startaddr = getStartAddressInMemory(pagenr)
    min = startaddr * 4 * MB
    max = min + 4 * MB
    return buffer[min:max]

def isPageInMemory(pagenr):
    return isinmemory[pagenr]

def setIsPageInMemory(pagenr, isin):
    isinmemory[pagenr] = int(isin)

def isAddressInMemory(address):
    return isPageInMemory(addressToPageNr(address))

def getLastUsed(pagenr):
    byte1 = struct[pagenr][0].to_bytes(1, byteorder="big")
    byte2 = struct[pagenr][1].to_bytes(1, byteorder="big")
    return int.from_bytes(byte1 + byte2, "big")

def setLastUsed(pagenr, time):
    byte12 = time.to_bytes(2, byteorder="big")
    byte3 = struct[pagenr][2].to_bytes(1, byteorder="big")
    byte123 = byte12[0].to_bytes(1, byteorder="big") + byte12[1].to_bytes(1, byteorder="big") + byte3[0].to_bytes(1, byteorder="big")
    struct[pagenr] = byte123

def getStartAddressInMemory(pagenr):
    byte3 = struct[pagenr][2].to_bytes(1, byteorder="big")
    return int.from_bytes(byte3, "big")

def setStartAddressInMemory(pagenr, address):
    byte1 = struct[pagenr][0].to_bytes(1, byteorder="big")
    byte2 = struct[pagenr][1].to_bytes(1, byteorder="big")
    byte3 = address.to_bytes(1, byteorder="big")
    byte123 = byte1[0].to_bytes(1, byteorder="big") + byte2[0].to_bytes(1, byteorder="big") + byte3[0].to_bytes(1, byteorder="big")
    struct[pagenr] = byte123

def isAddressPlusLengthInSamePage(address, length):
    page = addressToPageNr(address)
    pagemaxaddress = (page * 4 * MB) + (4 * MB - 1)
    return address + length <= pagemaxaddress

def isPageEmpty(pagenr):
    for byte in getAddressPage(pagenr):
        if byte != 0:
            return False
    return True
    

def handlePageFault(pagenr):
    unusedpage = getUnusedPage()
    realunused = getStartAddressInMemory(unusedpage)
    if not isPageEmpty(unusedpage):
        writePageToDisk(unusedpage)
    else:
        setIsPageInMemory(unusedpage, False)
        setStartAddressInMemory(unusedpage, 255)
    loadPageFromDisk(pagenr, realunused)

def readFromMemory(address):
    page = addressToPageNr(address)
    if isPageInMemory(page):
        newaddr = address - (4 * MB * page)
        setLastUsed(page, currenttimer)
        return getAddressPage(page)[newaddr]
    else:
        handlePageFault(page)
        return readFromMemory(address)

def writeToMemory(address, byte):
    page = addressToPageNr(address)
    if isPageInMemory(page):
        diffaddr = address - (4 * MB * page)
        newaddr = getStartAddressInMemory(page) * 4 * MB + diffaddr
        setLastUsed(page, currenttimer)
        buffer[newaddr] = byte
    else:
        handlePageFault(page)
        writeToMemory(address, byte)


def writeFileToDisk(name, array):
    nfile = open(name, 'wb')
    nfile.write(array)
    nfile.close()

def readFileFromDisk(name):
    nfile = open(name, 'rb')   
    buff = nfile.read()
    nfile.close()
    return buff

def writeBytesToBuffer(array, address):
    for i in range(len(array)):
        buffer[address + i] = array[i]

def writePageToDisk(pagenr):
    setIsPageInMemory(pagenr, False)
    writeFileToDisk(str(pagenr), getAddressPage(pagenr))
    setStartAddressInMemory(pagenr, 255)

def loadPageFromDisk(pagenr, startaddr):
    try:
        buf = readFileFromDisk(str(pagenr))
    except FileNotFoundError:
        buf = bytearray(4 * MB)
    writeBytesToBuffer(buf, startaddr * 4 * MB)
    setIsPageInMemory(pagenr, True)
    setStartAddressInMemory(pagenr, startaddr)

def getUnusedPage():
    longesttime = 0
    longest = -1
    for i in range(256):
        if isPageInMemory(i):
            timediff = currenttimer - getLastUsed(i)
            if timediff > longesttime:
                longesttime = timediff
                longest = i

    return longest

def incTimer():
    global currenttimer
    global thread
    currenttimer += 1
    thread = threading.Timer(0.1, incTimer) # old 1.0
    thread.start()

def setup():
    for i in range(16): #0-15
        isinmemory[i] = 1

    for i in range(1, 256): # 256 * 3 byte
        struct.append(b'\x00\x00\xff')

    for i in range(16):
        setStartAddressInMemory(i, i)
        setLastUsed(i, currenttimer)

    incTimer()
    

setup()

def showmappings():
    buf = ""
    for i in range(25):
        buf += str(i) + ":" + str(getStartAddressInMemory(i)) + " " 
    print(buf)

import random


def getActiveMappings():
    count = 0
    for i in range(256):
        if isPageInMemory(i):
            count += 1
    print(count)


def eversecond():
    global thread1s
    global time1s

    times = 0.0
    time = 0.0
    stby = 0.0

    for i in range(0, random.randint(4,12)):
        stby = currenttimer
        writeToMemory(i * 4 * MB + 1024, random.randint(0,255))
        time = currenttimer - stby
        if time > 0:
            print("time1s: " + str(time))
    
    getActiveMappings()

    thread1s = threading.Timer(1.0, eversecond)
    thread1s.start()


def every30second():
    global thread30s   
    global time30s

    times = 0.0
    time = 0.0
    stby = 0.0

    for i in range(random.randint(7,14),26):
        stby = currenttimer
        writeToMemory(i * 4 * MB + 1024, random.randint(0,255))
        time = currenttimer - stby
        print("time30s: " + str(time))
    

    thread30s = threading.Timer(30.0, every30second)
    thread30s.start()

def everyminute():

    global thread60s    
    global time60s

    times = 0.0
    time = 0.0
    stby = 0.0

    for i in range(16, random.randint(18,78)):
        stby = currenttimer
        writeToMemory(i * 4 * MB + 1024, random.randint(0,255))
        time = currenttimer - stby
        print("time60s: " + str(time))

    buf = ""
    for i in range(25):
        buf += str(i) + ":" + str(getStartAddressInMemory(i)) + " " 
    print(buf)

    thread60s = threading.Timer(60.0, everyminute)
    thread60s.start()


#eversecond()
#every30second()
#everyminute()

input("done?")


getActiveMappings()
writeToMemory(1024, 255)
getActiveMappings()
writeToMemory(len(buffer) + 1024, 255)
getActiveMappings()
for i in range(16, random.randint(18,78)):
    writeToMemory(i * 4 * MB + 1024, random.randint(0,255))
getActiveMappings()



thread.cancel()
