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
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    btg LATA,#0
    ulnk
    retfie
.end

