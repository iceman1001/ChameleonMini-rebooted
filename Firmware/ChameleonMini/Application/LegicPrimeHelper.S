/*
 * LegicPrimeHelper.S
 * This is part of the Chameleon project
 * https://github.com/iceman1001/ChameleonMini-rebooted
*/

    ; offsts of the Lfsr struct members
    .equ lfsr_a, 0
    .equ lfsr_b, 1

    .extern Lfsr
    .section .text

    .global legic_prng_forward
    .global legic_prng_get_bit
    .global legic_xor_bits

; void legic_prng_forward(uint8_t count)
; void legic_prng_forward_nocheck entry expects the count in r24 too, but skips the zero test.
legic_prng_forward:

    tst     r24
    breq    legic_prng_forward_done

legic_prng_forward_nocheck:

    lds     r18, Lfsr + lfsr_a
    lds     r19, Lfsr + lfsr_b

legic_prng_forward_loop:
    //  Lfsr.a = (Lfsr.a >> 1 | (Lfsr.a ^ Lfsr.a >> 6) << 6) & 0x7F;
    mov     r20, r18
    bst     r18, 6          ; store bit 6 of lfsr.a in T
    lsr     r18
    rol     r20             ; get bit 0
;   clr     r21             ; can be obmitted, as we only need bit 0 of r20
    bld     r21, 0          ; load bit 0 of r20 from T
    eor     r20, r21
    bst     r20, 0          ; store eor result in T 
    bld     r18, 6          ; load bit 6 of lfsr.a from T
;    andi    r18, 0x7F       ; can be obmitted

    //  Lfsr.b = Lfsr.b >> 1 | (Lfsr.b ^ Lfsr.b >> 2 ^ Lfsr.b >> 3 ^ Lfsr.b >> 7) << 7;
    mov     r20, r19
    clr     r21
    bst     r19, 2          ; store bit 2 of lfsr.b in T
    bld     r21, 0          ; load bit 2 of lfsr.b from T
    eor     r20, r21        ; Lfsr.b ^ Lfsr.b >> 2
    bst     r19, 3          ; store bit 3 of lfsr.b in T
    bld     r21, 0          ; load bit 3 of lfsr.b from T
    eor     r20, r21        ; Lfsr.b ^ Lfsr.b >> 2 ^ Lfsr.b >> 3
    bst     r19, 7          ; store bit 7 of lfsr.b in T
    bld     r21, 0          ; load bit 2 of lfsr.b from T
    eor     r20, r21        ; Lfsr.b ^ Lfsr.b >> 2 ^ Lfsr.b >> 3 ^ Lfsr.b >> 7
    bst     r20, 0          ; store eor result in T
    lsr     r19             ; Lfsr.b >> 1
    bld     r19, 7          ; load bit 7 of lfsrb.b from T

    dec     r24
    brne    legic_prng_forward_loop

    sts     Lfsr + lfsr_a, r18         ; lfsr.a
    sts     Lfsr + lfsr_b, r19         ; lfsr.b
legic_prng_forward_done:
    ret

; in: r25:r24:r23:r22, r20
; out: r25:r24:r23:r22
legic_xor_bits:

    push    r2              ; n
    push    r4              ; r4-r7 out
    push    r5
    push    r6
    push    r7
    push    r8              ; r8-10 bit
    push    r9
    push    r10
    push    r11

    mov     r2, r20          ; n in r2
    movw    r30, r24         ; high 16 bits of value
    clr     r4
    clr     r5
    movw    r6, r4          ; out = 0
    movw    r8, r4          ; bit = 0 
    movw    r10, r4         ; bit = 0
    inc     r8              ; bit = 1

legic_xor_bits_loop:
; legic_prng_get_bit_inlined
    lds     r18, Lfsr + lfsr_a

    ldi     r20, 0x07
    clr     r19
    bst     r18, 2          ; load bit 2 of lfsr.a in T
    bld     r19, 2          ; store T in bit 2 of idx 
    bst     r18, 3          ; load bit 3 of lfsr.a in T
    bld     r19, 1          ; store T in bit 1 of idx
    bst     r18, 4          ; load bit 4 of lfsr.a in T
    bld     r19, 0          ; store T in bit 0 of idx
    sub     r20, r19        ; idx = 7 - ((Lfsr.a & 4) | (Lfsr.a >> 2 & 2) | (Lfsr.a >> 4 & 1))
    lds     r19, Lfsr + lfsr_b
    breq    legic_prng_get_bit_inline_done
legic_prng_get_bit_inline_loop:
    lsr     r19             ; Lfsr.b >> 1
    dec     r20
    brne    legic_prng_get_bit_inline_loop
legic_prng_get_bit_inline_done:
    andi    r19, 0x01       ; Lfsr.b >> idx & 1
; legic_prng_get_bit_inlined

    eor     r22, r19         ; ((uint8_t)value ^ legic_prng_get_bit()) & 1

    lsr     r31              ; shift out the bit to carry
    ror     r30
    ror     r23
    ror     r22

    ; out |= bit, if the XORed bit is set
    brcc    legic_xor_bits_nobit
    or      r4, r8
    or      r5, r9
    or      r6, r10
    or      r7, r11
legic_xor_bits_nobit:
    lsl     r8               ; bit <<= 1
    rol     r9
    rol     r10
    rol     r11

    ldi     r24, 1
    rcall   legic_prng_forward_nocheck  ; clobbers r18-r21,r24

    dec     r2
    brne    legic_xor_bits_loop

    ; copy out into return value
    movw    r22, r4
    movw    r24, r6

    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     r7
    pop     r6
    pop     r5
    pop     r4
    pop     r2
    ret

    