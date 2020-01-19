    .section	.isr.text,code,keep
    .align	2
    .global __CCT1Interrupt
    __CCT1Interrupt:
	lnk #0
	bclr IFS6, #5
	btg LATA,#1
	btg LATA,#1
	ulnk
	retfie
    .end

