# BITS 32

 .text

# Offset by one byte off oxcfc - so OP uses fewer instructions
PORT = 0xcfd

BIT_TDO_POS = 1      # to make it '1' or '0'
BIT_TDI_POS = 5      # to make it '1' or '0'

BIT_TDI = (1<<BIT_TDI_POS)
BIT_TCK = (1<<(12-8))
BIT_TMS = (1<<(11-8))
BIT_TDO = (1<<BIT_TDO_POS)


.macro delay value
.rept \value
	inb %dx, %al
.endr
.endm

 

.macro  set_state value
    movw $PORT, %dx
    movb $(BIT_TDO|\value), %al    # keep TDO high at all times for input.
    outb %al, %dx
    movb $(BIT_TDO|\value|BIT_TCK), %al     # bring clock high
    outb %al, %dx
.endm



.macro set_state_0_1   # tms, tdi
	set_state BIT_TDI
.endm

.macro set_state_1_0   # tms, tdi
	set_state BIT_TMS
.endm

.macro set_state_0_0   # tms, tdi
	set_state 0
.endm

.macro any_to_reset   # tms, tdi
	# Return to tlr, from any state.  Do this at the start!
	set_state_1_0
	set_state_1_0
	set_state_1_0
	set_state_1_0
	set_state_1_0
.endm

.macro idle_to_shift_ir
    set_state_1_0#  # SELECT_DR_SCAN
    set_state_1_0#  # SELECT_IR_SCAN
    set_state_0_0#  # CAPTURE_IR
    set_state_0_0#  # SHIFT_IR
.endm

.macro idle_to_reset
    set_state_1_0  # SELECT_DR_SCAN
    set_state_1_0  # SELECT_IR_SCAN
    set_state_1_0  # RESET
.endm

.macro idle_to_shift_dr
    set_state_1_0  # SELECT_DR_SCAN
    set_state_0_0  # CAPTURE_DR
    set_state_0_0  # SHIFT_DR
.endm

.macro reset_to_idle
    # keep in reset for a few cycles - sometimes RDC loader software does only one
    # of these, sometimes two.  Two shouldn't hurt
    set_state_1_0
    set_state_1_0  
    set_state_0_0  # RUN_TEST_IDLE
.endm

.macro shift_to_update
    # The same for both IR and DR - the most common transition
    set_state_1_0  # EXIT_[DR|IR]
    set_state_1_0  # UPDATE_[DR|IR]
.endm

.macro update_to_shift_dr
    # Round the loop to read another value
    set_state_1_0  # select DR
    set_state_0_0  # capture DR
    set_state_0_0  # shift
.endm

.macro update_to_idle
    set_state_0_0  # IDLE
.endm

.macro shift_to_idle
    # The same for both IR and DR - the most common transition
    shift_to_update
    set_state_0_0  # IDLE
.endm


#==================================================
#
# Write values out
#
#==================================================


# %ebx = out value
# arg = number of bits
# eax, %ebx, ecx, edx trashed on return.
.macro shift_in value
	movw $PORT, %dx
.rept \value
	movb  %bl, %al
	andb  $1, %al
	shlb  $BIT_TDI_POS, %al
	orb   $BIT_TDO, %al
        outb %al, %dx
	orb   $BIT_TCK, %al
        outb  %al, %dx
	shrl  $1, %ebx		# shift right, trash lsb
.endr
.endm


#==================================================
#
# Read in values (sampling in), value returned in eax
#
#==================================================


# pass number of bits to this macro
.macro sample value
	shrl $1, %ebx
	movb $(BIT_TDO|BIT_TCK), %al
	outb %al, %dx
	movb $BIT_TDO, %al   # clock low
	outb  %al, %dx
	inb  %dx, %al
	andl  $BIT_TDO, %eax  # remove other bits
	shl  $(\value - BIT_TDO_POS -1), %eax     # shift it up to fill point
	orl   %eax, %ebx
.endm


# bit count in ecx
# returns value in eax
.macro shift_out value
	movw $PORT, %dx
	xorl  %ebx, %ebx	       # holds our result.
.rept \value
	sample \value              # sample in bit position 16.
.endr
	movl %ebx, %eax
.endm




# arg1 - command to send
.macro write_ir value
    #IR writes always come from and go to idle state
    idle_to_shift_ir
    movb \value, %bl 		# Always 8-bit, fits in bl
    shift_in 8
    shift_to_idle
.endm


# Arg 1 - bit count
# Return value in %eax
.macro read_dr value
    # dr reads always go to update state
    shift_out \value
    movl %eax, %ebx
    shift_to_update
    movl %ebx, %eax
.endm




# %ebx has the value to write
.macro write_dr value
    shift_in \value
    shift_to_update
.endm


# in all cases %ebx has value to write
.macro write_dr8
	write_dr 8
.endm
.macro write_dr16
	write_dr 16
.endm
.macro write_dr32
	write_dr 32
.endm



.global jtag_init
jtag_init:
    push %edx
    push %ebx
    movw $PORT, %dx
    any_to_reset         # Put into test-logic reset from wherever it is now.



#.rept 30
#    reset_to_idle
#    write_ir $0x01        # back to idle
#    idle_to_reset
#.endr



jtag_init.loop:
    reset_to_idle

    write_ir $0x01        # back to idle
#    idle_to_reset
#    reset_to_idle

    delay 60

    write_ir $0x0f        # back to idle

    delay 60

    idle_to_shift_dr
    shift_out 16
    # store %eax
    movl %eax, %ebx
    shift_to_idle

    test $0xffff, %ebx
    jz jtag_init.cont
    movl %ebx, %eax

    pop %ebx
    pop %edx
    ret
jtag_init.cont:
    idle_to_reset
    jmp jtag_init.loop




.macro MemAccess_impl value1, value2
	write_ir $0x11
	idle_to_shift_dr
	movl \value1, %ebx
	write_dr32
	update_to_shift_dr    #  more to go
	movl \value2, %ebx
	write_dr8
.endm 



# Arg1 addr
# Arg2 code
.global jtag_MemAccess
jtag_MemAccess:
        push %edx
        push %ebx
	movw $PORT, %dx
	MemAccess_impl 12(%esp),  16(%esp)
        pop %ebx
        pop %edx
	ret


# Arg 1 addr
.global jtag_ReadMem16
jtag_ReadMem16:
        push %edx
        push %ebx
	movw $PORT, %dx
	MemAccess_impl 12(%esp), $0x19

	update_to_shift_dr
	read_dr 16
	movl %eax, %ebx
	update_to_idle
	movl %ebx, %eax
        pop %ebx
        pop %edx
	ret


# Arg 1 addr
# Arg 2 val
.global jtag_WriteMem16
jtag_WriteMem16:
        push %edx
        push %ebx
	mov $PORT, %dx
	MemAccess_impl 12(%esp), $0x1d

	update_to_shift_dr
	movl 16(%esp), %ebx
	write_dr16
	update_to_idle
        pop %ebx
        pop %edx
	ret


# Arg 1 val
.global jtag_write_ir
jtag_write_ir:
        push %edx
        push %ebx
	write_ir 12(%esp)
#	delay 400
        pop %ebx
        pop %edx
	ret

.global jtag_idle_to_shift_dr
jtag_idle_to_shift_dr:
        push %edx
        push %ebx
	idle_to_shift_dr
        pop %ebx
        pop %edx
	ret

.global jtag_read_dr8
jtag_read_dr8:
        push %edx
        push %ebx
#	delay 400
	read_dr 8
        pop %ebx
        pop %edx
	ret

.global jtag_read_dr16
jtag_read_dr16:
        push %edx
        push %ebx
#        delay 200
	read_dr 16
        pop %ebx
        pop %edx
	ret

.global jtag_read_dr32
jtag_read_dr32:
        push %edx
        push %ebx
#	delay 400
	read_dr 32
#	delay 400
        pop %ebx
        pop %edx
	ret

.global jtag_update_to_idle
jtag_update_to_idle:
        push %edx
        push %ebx
	update_to_idle
        pop %ebx
        pop %edx
	ret

.global jtag_update_to_shift_dr
jtag_update_to_shift_dr:
        push %edx
        push %ebx
	update_to_shift_dr
        pop %ebx
        pop %edx
	ret

.global jtag_write_dr32
jtag_write_dr32:
        push %edx
        push %ebx
	movl 12(%esp), %ebx
	write_dr32
        pop %ebx
        pop %edx
	ret


