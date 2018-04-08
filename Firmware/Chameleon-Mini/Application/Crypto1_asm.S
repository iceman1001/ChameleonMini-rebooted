#include "Crypto1_asm.h"

.global Crypto1FilterOutput
.global Crypto1Bit
.global Crypto1Proceed

Crypto1FilterOutput: ; sped up filter function for crypto1
	push r18
	push r17 ; push registers we want to use - these need to be preserved, try and switch to better ones later
	push r16
	lds r22, StateOdd
	lds r23, StateOdd+1
	lds r24, StateOdd+2
	mov r21, r22 ; set up each shift value into register
	andi r22, 0xf
	mov r20, r23
	andi r23, 0xf
	andi r24, 0xf
	swap r21
	andi r21, 0x0f
	swap r20
	andi r20, 0x0f
	ldi r17, 0x0a ; set shift constant 0xEC57E80A - not using reserved registers to save push and pop cycles hence the weird choices
	ldi r16, 0xe8
	ldi r27, 0x57
	ldi r26, 0xec
	ldi ZH, hi8(TableA) ; set up first space for lookup
	ldi ZL, lo8(TableA)
	add ZL, r22
	adc ZH, r1
	lpm r22, Z
	cpi r22, 0
	breq Crypto1FilterOutputEndShiftSixteenBit
	mov r16, r26 ; this will shift 16 bits in two operations rather than the ton it usually takes
	mov r17, r27
Crypto1FilterOutputEndShiftSixteenBit:
	ldi ZH, hi8(TableB) ; set up first space for lookup
	ldi ZL, lo8(TableB)
	add ZL, r21
	adc ZH, r1
	lpm r21, Z
	cpi r21, 0
	breq Crypto1FilterOutputEndShiftEightBit
	mov r17, r16
Crypto1FilterOutputEndShiftEightBit:
	ldi ZH, hi8(TableA) ; set up first space for lookup
	ldi ZL, lo8(TableA)
	add ZL, r23
	adc ZH, r1
	lpm r23, Z
	cpi r23, 0
	breq Crypto1FilterOutputEndShiftFourBit
	swap r17
	andi r17, 0x0f ; may not be necessary
Crypto1FilterOutputEndShiftFourBit:
	ldi ZH, hi8(TableA)
	ldi ZL, lo8(TableA)
	add ZL, r20
	adc ZH, r1
	lpm r20, Z
	cpi r20, 0 ; if its zero, skip
	breq Crypto1FilterOutputEndShiftTwoBit
	lsr r17
	lsr r17
Crypto1FilterOutputEndShiftTwoBit:
	ldi ZH, hi8(TableB)
	ldi ZL, lo8(TableB)
	add ZL, r24
	adc ZH, r1
	lpm r24, Z
	cpi r24, 0
	breq Crypto1FilterOutputEndShiftOneBit
	lsr r17
Crypto1FilterOutputEndShiftOneBit:
	andi r17, 0x01
	mov r24, r17
	pop r16
	pop r17
	pop r18
	ret

Crypto1Proceed:
	# 16 bit shift
	eor r22, r24
	# 8 bit shift
	eor r22, r23
	# 4 bit shift
	mov r23, r22
	swap r23
	eor r22, r23
	andi r22, 0x0f
	# load shift value - endianness different to above to save an operation
	ldi r24, 0x96
	ldi r23, 0x69
	# shift value loop
	cpi r22, 0 ; if its zero, skip
	breq Crypto1ProceedSixteenBitShiftDone
Crypto1ProceedSixteenBitShiftCheck:
	lsr r23
	ror r24
	dec r22
	cpi r22, 0
	brne Crypto1ProceedSixteenBitShiftCheck
Crypto1ProceedSixteenBitShiftDone:
	andi r24, 0x01
	ret

Crypto1Bit:
	push r24 ; first param
	push r22 ; second param
	lds r22, (StateOdd)
	lds r23, (StateOdd+1)
	lds r24, (StateOdd+2)
	rcall Crypto1FilterOutput
	mov r25, r24 ; r25 contains filter output - r30 and r31 upper bits for calculations
	mov r0, r24
	ldi r30, 0x00
	ldi r31, 0x00
	lds r18, (StateEven)
	lds r19, (StateEven+1)
	lds r20, (StateEven+2)
	lds r22, (StateOdd)
	lds r23, (StateOdd+1)
	lds r24, (StateOdd+2)
	pop r26 ; second param
	pop r27 ; first param

	and r25, r26
	eor r25, r27

	push r16
	; and xor state 1 with 0x29CE5C
	ldi r16, 0x5c
	and r22, r16
	ldi r16, 0xce
	and r23, r16
	ldi r16, 0x29
	and r24, r16
	eor r25, r22
	eor r30, r23
	eor r31, r24

	; and xor state 0 with 0x0x870804
	ldi r16, 0x04
	and r18, r16
	ldi r16, 0x08
	and r19, r16
	ldi r16, 0x87
	and r20, r16

	eor r25, r18
	eor r30, r19
	eor r31, r20

	; reload again, seriously need to improve this
	lds r18, (StateEven)
	lds r19, (StateEven+1)
	lds r20, (StateEven+2)

	lsl r18
	rol r19
	rol r20

	; use parity function - r22,r23,r24
	push r24
	push r23
	push r22
	mov r22, r25
	mov r23, r30
	mov r24, r31
	rcall Crypto1Proceed
	mov r16, r24
	pop r22
	pop r23
	pop r24
	or r18,r16
	pop r16
	lds r22, (StateOdd)
	lds r23, (StateOdd+1)
	lds r24, (StateOdd+2)

	; store in opposite registers
	sts (StateEven), r22
	sts (StateEven+1), r23
	sts (StateEven+2), r24

	sts(StateOdd), r18
	sts(StateOdd+1), r19
	sts(StateOdd+2), r20

	mov r24, r0
	ret