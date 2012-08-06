#!/usr/bin/env python
#
# Bifferboard bootstrapping server
#

import string, struct, socket, select, thread, StringIO, os, time

# IP address of the server (this machine) 
if os.name == "nt" :
    # Need to configure hostname manually for windows.
    TFTP_HOST = "10.0.0.7"
else :
    TFTP_HOST = os.popen( "/sbin/ifconfig eth0 |grep inet").read()[20:].split(" ")[0]
#    TFTP_HOST = "192.168.1.80"
    
# location of kernel/initrd
KERNEL_IMAGE_BIN = "bzImage"   # Max len 127!

#
# TFTP Errors
#
class TFTPError(Exception):
    pass

#
# A class for a TFTP Connection
#
class TFTPConnection:

    RRQ  = 1
    WRQ  = 2
    DATA = 3
    ACK  = 4
    ERR  = 5
    HDRSIZE = 4  # number of bytes for OPCODE and BLOCK in header

    def __init__(self, host="", port=0, blocksize=512, timeout=2.0, retry=5):
        self.host = host
        self.port = port
        self.blocksize = blocksize
        self.timeout   = timeout
        self.retry     = retry

        self.client_addr = None
        self.sock        = None
        self.active      = 0
        self.blockNumber = 0
        self.lastpkt     = ""

        self.mode        = ""
        self.filename    = ""
        self.file        = None

        self.bind(host, port)

    def bind(self, host="", port=0):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock = sock
        if host or port:
            sock.bind(host, port)

    def send(self, pkt=""):
        self.sock.sendto(pkt, self.client_addr)
        self.lastpkt = pkt

    def recv(self):
        sock        = self.sock
        F           = sock.fileno()
        client_addr = self.client_addr
        timeout     = self.timeout            
        retry       = self.retry

        while retry:
            r,w,e = select.select( [F], [], [F], timeout)
            if not r:
                # We timed out -- retransmit
                retry = retry - 1
                self.retransmit()
            else:
                # Read data packet
                pktsize = self.blocksize + self.HDRSIZE
                data, addr = sock.recvfrom(pktsize)
                if addr == client_addr:
                    break
        else:
            raise TFTPError(4, "Transfer timed out")
        
        return self.parse(data)

    def parse(self, data, unpack=struct.unpack):
        buf = buffer(data)
        pkt = {}
        opcode = pkt["opcode"] = unpack("!h", buf[:2])[0]
        if ( opcode == self.RRQ ) or ( opcode == self.WRQ ):
            filename, mode, junk  = string.split(data[2:], "\000")
            pkt["filename"] = filename
            pkt["mode"]     = mode
        elif opcode == self.DATA:
            block  = pkt["block"] = unpack("!h", buf[2:4])[0]
            data   = pkt["data"]  = buf[4:]
        elif opcode == self.ACK:
            block  = pkt["block"] = unpack("!h", buf[2:4])[0]
        elif opcode == self.ERR:
            errnum = pkt["errnum"] = unpack("!h", buf[2:4])[0]
            errtxt = pkt["errtxt"] = buf[4:-1]
        else:
            raise TFTPError(4, "Unknown packet type")

        return pkt

    def retransmit(self):
        self.sock.sendto(self.lastpkt, self.client_addr)
        return

    def connect(self, addr, data):
        self.client_addr = addr
        RRQ  = self.RRQ
        WRQ  = self.WRQ
        DATA = self.DATA
        ACK  = self.ACK
        ERR  = self.ERR

        try:
            pkt    = self.parse(data)
            opcode = pkt["opcode"]
            if opcode not in (RRQ, WRQ):
                raise TFTPError(4, "Bad request")
            
            # Start lock-step transfer
            self.active = 1
            if opcode == RRQ:
                self.handleRRQ(pkt)
            else:
                self.handleWRQ(pkt)

            # Loop until done
            while self.active:
                pkt = self.recv()
                opcode = pkt["opcode"]
                if opcode == DATA:
                    self.recvData(pkt)
                elif opcode == ACK:
                    self.recvAck(pkt)
                elif opcode == ERR:
                    self.recvErr(pkt)
                else:
                    raise TFTPError(5, "Invalid opcode")
        except TFTPError, detail:
            self.sendError( detail[0], detail[1] )

        return

    def recvAck(self, pkt):
        if pkt["block"] == self.blockNumber:
            # We received the correct ACK
            self.handleACK(pkt)
        return

    def recvData(self, pkt):
        if pkt["block"] == self.blockNumber:
            # We received the correct DATA packet
            self.active = ( self.blocksize == len(pkt["data"]) )
            self.handleDATA(pkt)
        return

    def recvErr(self, pkt):
        self.handleERR(pkt)
        self.retransmit()
    
    def sendData(self, data, pack=struct.pack):
        blocksize = self.blocksize
        block     = self.blockNumber = self.blockNumber + 1
        lendata   = len(data)
        format = "!hh%ds" % lendata
        pkt = pack(format, self.DATA, block, data)
        self.send(pkt)
        #time.sleep(0.001)
        self.active  = (len(data) == blocksize)

    def sendAck(self, pack=struct.pack):
        block            = self.blockNumber
        self.blockNumber = self.blockNumber + 1
        pkt = pack("!hh", self.ACK, block)
        self.send(pkt)
        
    def sendError(self, errnum, errtext, pack=struct.pack):
        errtext = errtext + "\000"
        format = "!hh%ds" % len(errtext)
        outdata = pack(format, self.ERR, errnum, errtext)
        self.sock.sendto(outdata, self.client_addr)
        return
    
    #
    # Override these handle* methods as needed
    #

    def handleRRQ(self, pkt):
        filename  = pkt["filename"]
        mode      = pkt["mode"]
        print filename, mode
        self.file = self.readRequest(filename, mode)
        self.sendData( self.file.read(self.blocksize) )
        return

    def handleWRQ(self, pkt):
        filename  = pkt["filename"]
        mode      = pkt["mode"]
        self.file = self.writeRequest(filename, mode)
        self.sendAck()
        return

    def handleACK(self, pkt):
        if self.active:
            self.sendData( self.file.read(self.blocksize) )
        return
    
    def handleDATA(self, pkt):
        self.sendAck()
        data = pkt["data"]
        self.file.write( data )
        
    def handleERR(self, pkt):
        print pkt["errtxt"]
        return

    #
    # You should definitely override these
    #
    def readRequest(self, filename, mode):
        return StringIO.StringIO("")

    def writeRequest(self, filename, mode):
        return StringIO.StringIO()


class TFTPServer:

    """TFTP Server
    Implements a threaded TFTP Server.  Each request is handled
    in its own thread
    """

    def __init__(self, host="", port=16869,
                 conn=TFTPConnection, srcports=[] ):
        self.host = host
        self.port = port
        self.conn = conn
        self.srcports = srcports

        self.sock = None
        self.bind(host, port)

    def bind(self, host, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock = sock
        sock.bind( (host, port) )

    def forever(self):
        while 1:
            data, addr = self.sock.recvfrom(516)
            self.handle(addr, data)     

    def handle(self, addr, data):
        if self.srcports:
            nextport = self.srcports.pop(0)
            self.srcports.append( nextport )
            T = self.conn( self.host, nextport )
        else:
            T = self.conn( self.host )
        thread.start_new_thread( T.connect, (addr, data) )
        return

	
def bootp_server(port) :
	bootp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM,0)
	bootp.bind( ("", port) )
	bootp.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
	bootp.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	
	while 1:
		(data, addr) = bootp.recvfrom( 300, socket.MSG_TRUNC )
		print "Received bootp request from: " + `addr`
		if len(data) != 300: continue
		d = {}
		(d["op"],d["htype"],d["hlen"],d["hops"],d["xid"],d["secs"],
			d["ciaddr"],d["yiaddr"],d["siaddr"],d["giaddr"],
			d["chaddr"],d["sname"],d["file"],d["vend"]
		) = struct.unpack("!4B I Hxx 4I 16s 64s 128s 64s",data)
	
		d["op"] = 2
		# d["yiaddr"] = d["ciaddr"]
		d["yiaddr"]=struct.unpack("!I",socket.inet_aton("192.168.0.159"))[0]
		d["siaddr"]=struct.unpack("!I",socket.inet_aton(TFTP_HOST))[0]
		d["file"] = KERNEL_IMAGE_BIN
	
		res_pkt = struct.pack("!4B I Hxx 4I 16s 64s 128s 64s",
			d["op"],d["htype"],d["hlen"],d["hops"],d["xid"],d["secs"],
			d["ciaddr"],d["yiaddr"],d["siaddr"],d["giaddr"],
			d["chaddr"],d["sname"],d["file"],d["vend"] )        
                    	
		bootp_reply = socket.socket( socket.AF_INET, socket.SOCK_DGRAM, 0 )
                bootp_reply.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
		bootp_reply.sendto( res_pkt, 0, ("255.255.255.255",68))

	

if __name__ == "__main__":
    import sys
    from StringIO import StringIO

    #
    # Subclass to create our own TFTP Connection object
    #
    class MyTFTP( TFTPConnection ):
        
        def readRequest(self, filename, mode):
            print "tftp request:",filename
            return file(filename)
    
        def writeRequest(self, filename, mode):
            return StringIO.StringIO()

    try:
        print "Starting bootp server"
        thread.start_new_thread( bootp_server, (67,) )
	print "Starting tftp server"
        serv = TFTPServer( "", 69, conn=MyTFTP )
        serv.forever()
    except KeyboardInterrupt, SystemExit:
        pass
	
	

 	  	 
