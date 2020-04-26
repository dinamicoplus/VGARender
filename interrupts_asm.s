.section	.isr.text,code,keep
.align	2
.global __CCT1Interrupt
__CCT1Interrupt:
    lnk #0
    bclr IFS6, #5
    ulnk
    retfie
.global __CCP1Interrupt
__CCP1Interrupt:	
    lnk #0
    bclr IFS3, #15
    push w0
    mov _pix, w0
    mov w0, LATA
    mov _pix+2, w0
    mov w0, LATA
    mov _pix+4, w0
    mov w0, LATA
    mov _pix+6, w0
    mov w0, LATA
    mov _pix+8, w0
    mov w0, LATA
    mov _pix+10, w0
    mov w0, LATA
    mov _pix+12, w0
    mov w0, LATA
    mov _pix+14, w0
    mov w0, LATA
    mov _pix+16, w0
    mov w0, LATA
    mov _pix+18, w0
    mov w0, LATA
    mov _pix+20, w0
    mov w0, LATA
    mov _pix+22, w0
    mov w0, LATA
    
    bclr LATA,#0
    pop w0
    ulnk
    retfie
.end

