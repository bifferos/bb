#!/usr/bin/env python
"""

    Copyright (c) Bifferos 2009 (sales@bifferos.com)
    
    This is free software, licensed under the GNU General Public License v2.
    See COPYING for more information.
        
    Requirements:
    
    - Python 2.4 or later (not 3.0)
    - ctypes (if using Python 2.4 needed as an add-on).
    - Nasm (version 2.0 or later required).
    

    JTAG introduction
    
    JTAG consists of two registers - an instruction register and a data register.
    The intruction register for RDC is 8 bits.
    The data register size depends on the instruction and can be 8,16 or 32 bits.
    
    There are three basic types of operation:
    - Instructions without parameters or return values
    - Instructions with parameters
    - Instructions with return values

    Whilst the JTAG protocol seems to allow shifting in data to the data register at 
    the same time as shifting out this does not appear to be used for RDC.

    Each operation consists of the instruction, followed by run-test-idle state,
    A series of values may then be written or read.  Return to the run-test-idle 
    state indicates completion of the operation.

    01 Reset
    
    Issues a reset to the CPU.  Completion can be polled for with the 0x0f 
    instruction.  This instruction may take a long time to execute.  There are 
    no parameters or return values.
    
    02 Execute machine code
    
    Starts execution of previously loaded code (with the 06 instuction).  There 
    are no parameters or return values.
    
    03 'Halt'
    
    Stop execution.
    
    04 'Go'
    
    Start execution of the currently loaded firmware.

    06 Load machine code.
    
    Used to load a sequence of instructions prior to execution.  There
    follows one or more 32-bit values, which consist of little-endian 16-bit 
    machine code instructions padded up to the nearest 32-bit boundary with NOPs 
    (0x90).
    
    07 (Unknown)
    
    No parameters.
    
    08, 09 (Unknown)
    
    2 32-bit parameters are required.
    
    0A, 0B (Unknown)
    
    Unknown.  4 32-bit parameters are required.
    
    0C Get flags
    
    Returns 3 x 32-bit values.  Gets the 32-bit state of the flag, EIP and CS 
    registers.  The upper word of the CS value is padded with NOPs (0x9090).
    
    e.g.:     00000002    EFlag, flags
              0000FFF0    EIP, instruction pointer
              9090F000    CS, code segment
         
    0D Set flags
    
    Requires 3 x 32-bit values.  See above for format.
    
    0E Examine EAX
    
    Returns the 32-bit contents of the EAX register.  EAX is the only offset
    register available for direct reading.  This necessitates executing machine
    code to move the contents of other registers into it ready for inspection.
    
    0F Status query
    
    Queries the state of the CPU.  Returns zero if unknown.  If the CPU is 
    running, a reset may be required (01) to re-gain control.  The status is 
    always returned as 16-bit.
    
    8381 Indicates cpu has been reset.
    8323 Indicates halted code
    8324 Indicates code running in the debugger (halt is possible)
    
    11 32-bit memory access
    
    Followed by flags (8-bit)
    0x18 - 8-bit read
    0x19 - 16-bit read
    0x1d - 16-bit write
    
    Extracting by segment-offset is achieved by code execution and read-back 
    of results (see 06).
    
    20,21,22,23,24,25,26,27,28,29,2a,2b
    
    Instructions used during debugging.  All return single 32-bit values.
    
    30 Examine CR0 (32 bits)    
    31 Examine CR2 (32 bits)
    32 Examine CR3 (32 bits)
    33 Examine CR4 (32 bits)

"""

import struct, os
from ctypes import *


def GetMachineCode(txt):
  file("assembly.S","wb").write(txt)
  #if os.system("nasm assembly.S &> /dev/null"): return None
  if os.system("nasm assembly.S"): return None
  return file("assembly","rb").read()


def Nasm(line):
  "Assemble instructions to op-codes, return one or more 32-bit values"
  machine = GetMachineCode(line)
  while len(machine)%4: machine += "\x90"
  return struct.unpack("L"*(len(machine)/4), machine)


class IOPort:
  "Access to an 8-bit IO port under Linux"
  def __init__(self, port):
    print "Registering port 0x%x" % port
    res = cdll.LoadLibrary("libc.so.6").ioperm(port, 1, 1)
    print "ioperm() ==",res
    writeit = """  BITS 32  
                   mov eax, [esp+4]
		   mov dx, 0x%x
		   out dx, al
                   ret           """ % port
    ok = GetMachineCode(writeit)
    self.c_write = create_string_buffer(ok)
    self.Write = CFUNCTYPE(None, c_int32)(addressof(self.c_write))

    readit = """   BITS 32
		   mov dx, 0x%x
		   in al, dx
                   ret           """ % port
    ok = GetMachineCode(readit)
    self.c_read = create_string_buffer(ok)
    self.Read = CFUNCTYPE(c_int32)(addressof(self.c_read))
    

class Xilinx:

  # Parallel bits for Xilinx cable
  BIT_TDI = 1
  BIT_TCK = 2
  BIT_TMS = 4
  BIT_TRI = 8
  BIT_ENABLE = 0x10
  BIT_CABLE = 0x40

  def __init__(self):
    self.p_out = IOPort(0x378)
    self.p_in = IOPort(0x379)

  def io_outb(self,b):
    self.p_out.Write((b & 0x7) | self.BIT_CABLE | self.BIT_ENABLE)

  def io_inb(self):
    if self.p_in.Read() & 16 :
      return 1
    return 0

  def set_state(self, tms, tdi):
    "Clock to reach a given state (also used for shifting data in)"
    val = 0
    if tms: val |= self.BIT_TMS
    if tdi: val |= self.BIT_TDI
    self.io_outb(val)    
    val |= self.BIT_TCK
    self.io_outb(val)

  def falling_clock(self, tms, tdi):
    """Output data latched on falling clock.  Only reason to do this is to sample
        output... i think"""
    val = self.BIT_TCK
    if tms: val |= self.BIT_TMS
    if tdi: val |= self.BIT_TDI
    self.io_outb(val)
    val &= ~self.BIT_TCK   # lower the clock
    self.io_outb(val)
    return self.io_inb()   # sample and return output.

  def any_to_reset(self):
    "Return to tlr, from any state.  Do this at the start!"
    self.set_state(1,0)
    self.set_state(1,0)
    self.set_state(1,0)
    self.set_state(1,0)
    self.set_state(1,0)

  def idle_to_shift_ir(self):
    self.set_state(1,0)  # SELECT_DR_SCAN
    self.set_state(1,0)  # SELECT_IR_SCAN
    self.set_state(0,0)  # CAPTURE_IR
    self.set_state(0,0)  # SHIFT_IR

  def idle_to_reset(self):
    self.set_state(1,0)  # SELECT_DR_SCAN
    self.set_state(1,0)  # SELECT_IR_SCAN
    self.set_state(1,0)  # RESET

  def idle_to_shift_dr(self):
    self.set_state(1,0)  # SELECT_DR_SCAN
    self.set_state(0,0)  # CAPTURE_DR
    self.set_state(0,0)  # SHIFT_DR

  def reset_to_idle(self):
    """keep in reset for a few cycles - sometimes RDC loader software does only one 
       of these, sometimes two.  Two shouldn't hurt"""
    self.set_state(1,0)
    self.set_state(1,0)  
    self.set_state(0,0)  # RUN_TEST_IDLE

  def shift_to_update(self):
    "The same for both IR and DR - the most common transition"
    self.set_state(1,0)  # EXIT_[DR|IR]
    self.set_state(1,0)  # UPDATE_[DR|IR]

  def update_to_shift_dr(self):
    "Round the loop to read another value"
    self.set_state(1,0)  # select DR
    self.set_state(0,0)  # capture DR
    self.set_state(0,0)  # shift

  def update_to_idle(self):
    self.set_state(0,0)  # IDLE

  def shift_to_idle(self):
    "The same for both IR and DR - the most common transition"
    self.shift_to_update()
    self.set_state(0,0)  # IDLE

  def shift_in(self, val, bits):
    "put bits into register"
    index = 1L
    for i in xrange(0,bits):
      if (val&index):
        self.set_state(0, 1)
      else:
        self.set_state(0, 0)
      index <<= 1

  def shift_out(self, bits):
    "pull bits out of register"
    index = 1L
    out = 0
    for i in xrange(0,bits):
      if self.falling_clock(0, 0):
        out |= index
      index <<= 1
    return out

  def write_ir(self, command):
    "IR writes always come from and go to idle state"
    self.idle_to_shift_ir()
    self.shift_in(command, 8)
    self.shift_to_idle()

  def read_dr(self, bits):
    "dr reads always go to update state"
    tmp = self.shift_out(bits)
    self.shift_to_update()
    return tmp

  def write_dr(self, val, bits):
    self.shift_in(val, bits)
    self.shift_to_update()


class RDC:
  "RDC Jtag stuff"
  def __init__(self):
    "Two commands go to IR, then a 16-bit read of DR"
    x = Xilinx()
    ready=0
    count=0
    x.any_to_reset()         # Put into test-logic reset from wherever it is now.
  
    while not ready:
      x.reset_to_idle()
      x.idle_to_shift_ir()
      x.shift_in(0x01, 8)
      x.shift_to_idle()
      x.idle_to_shift_ir()
      x.shift_in(0x0f, 8)
      x.shift_to_idle()
      x.idle_to_shift_dr()
      ready = x.shift_out(16)
      x.shift_to_idle()
      if (ready): break
      x.idle_to_reset()
      count += 1
      if count>1000000: break

    #print "Value read back: %x in %d cycles" % (ready, count)
    # ready should be 0x8381
    print "Result of init: %x" % ready
    self.x = x
    
    self.Exec("""
       mov dx, 0xcf8         # PCI cfg adr
       mov eax, 0x80003840   # bus control
       out dx, eax
       mov dx, 0xcfc         # PCI cfg data
       mov eax, 0x87ff0600   # bit 16 == FRR 1 (E0000->FFFFF)
       out dx, eax
       """)

  def ReadData(self, command, data_bits, toread):
    self.x.write_ir(command)
    out = []
    if toread:
      self.x.idle_to_shift_dr()
      for i in xrange(0, toread):
        out.append( self.x.read_dr(data_bits) )
        if i==(toread-1):
          self.x.update_to_idle()   # last one
        else:
          self.x.update_to_shift_dr()  # more to go
    return out

  def WriteData(self, command, data_bits, *data):
    self.x.write_ir(command)
    if data:
      self.x.idle_to_shift_dr()
      for i in xrange(0,len(data)):
        self.x.write_dr(data[i], data_bits)
        if i==(len(data)-1):
          self.x.update_to_idle()   # last one
        else:
          self.x.update_to_shift_dr()  # more to go

  def Status(self):
    return self.ReadData(0x0f, 16, 1)[0]

  def ResetFlags(self):
    self.WriteData(0x0d, 32, 0x2, 0xff00, 0x9090f000 )

  def RunCode(self, code):
    self.WriteData(0x06, 32, *code)      # load
    self.WriteData(0x02, 32)             # execute
    s1 = self.Status()         # check status
    s2 = self.Status()         # check status
    if s2!=0x8381:
      raise IOError("Error executing code")

  def eax(self):
    return self.ReadData(0x0e, 32, 1)[0]
    
  def Go(self):
    self.x.write_ir(0x04)
    s1 = self.Status()         # check status
    s2 = self.Status()         # check status

  def Reset(self):
    self.x.write_ir(0x05)
    s1 = self.Status()         # check status
    s2 = self.Status()         # check status  
  
  def MemAccess(self, addr, code):
    self.x.write_ir(0x11)
    self.x.idle_to_shift_dr()
    self.x.write_dr(addr, 32)
    self.x.update_to_shift_dr()  # more to go
    self.x.write_dr(code, 8)
  
  def DumpMem(self, addr, count):
    self.MemAccess(addr, 0x18)
    data = ""
    for i in xrange(0,count):
      self.x.update_to_shift_dr()  # more to go
      data += chr(self.x.read_dr(8))
    self.x.update_to_idle()
    return data

  def ReadMem16(self, addr):
    self.MemAccess(addr, 0x19)
    self.x.update_to_shift_dr()  # more to go
    data = self.x.read_dr(16)
    self.x.update_to_idle()
    return data

  def WriteMem16(self, addr, val):
    self.MemAccess(addr, 0x1d)
    self.x.update_to_shift_dr()  # more to go
    self.x.write_dr(val, 16)
    self.x.update_to_idle()

  def EonSectorErase(self,addr):
    self.WriteMem16(0xffffaaaa, 0xaaaa)
    self.WriteMem16(0xffff5554, 0x5555)
    self.WriteMem16(0xffffaaaa, 0x8080)
    self.WriteMem16(0xffffaaaa, 0xaaaa)
    self.WriteMem16(0xffff5554, 0x5555)
    self.WriteMem16(addr, 0x3030)
    r = 1
    while 0xffff != r:
      r = self.ReadMem16(addr)
      print "%08x" % r
    print "Erase done"

  def EonProgram(self,addr, byte):
    self.WriteMem16(0xffffaaaa, 0xaaaa)
    self.WriteMem16(0xffff5554, 0x5555)
    self.WriteMem16(0xffffaaaa, 0xA0A0)
    self.WriteMem16(addr, byte)
    while byte != self.ReadMem16(addr):
      pass
    
  def Exec(self, script):
    # interpreter for script
    for i in script.split("\n"):
      if i.split("#")[0].strip():
        params = Nasm(i.split("#")[0])
        if params:
          print "Instruction:",i
          self.ResetFlags()
          self.RunCode(params)
        else:
          exec(i.strip())
          
  def DumpSector(self, address):
    data = ""
    for i in xrange(0,0x10000,0x80):
      self.ResetFlags()
      mem = address+i
      data += self.DumpMem(mem,0x80)
      if not (mem % 0x1000):
        print "%08x" % mem
    return data


