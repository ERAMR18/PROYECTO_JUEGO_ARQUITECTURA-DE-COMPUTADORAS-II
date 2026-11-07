.syntax unified
.cpu cortex-m3
.thumb

.global read_inputs_assembly
.global estado_boton
.global eje_x
.global eje_y
.global lectura_adc

.equ PIOB_BASE,   0x400E1000
.equ PIO_PDSR,    0x03C        @ Estado actual de los pines

.equ ADC_BASE,    0x400C0000
.equ ADC_CR,      0x00
.equ ADC_ISR,     0x30
.equ ADC_CDR0,    0x50         @ CDR0 + 4*n = canal n

.data
.align 4
estado_boton: .word 0          @ 1 = presionado, 0 = suelto
eje_x:        .word 2048       @ 0..4095 (centro aprox 2048)
eje_y:        .word 2048
lectura_adc:  .space 32        @ 8 canales: lectura_adc[7]=A0, [6]=A1, ... [0]=A7

.text
.thumb_func
read_inputs_assembly:
    push {r4-r7, lr}

    @ ---------- Boton (PB25 / pin 2) ----------
    @ Con pull-up: suelto = 1, presionado = 0
    ldr r0, =PIOB_BASE
    ldr r1, [r0, #PIO_PDSR]
    ldr r2, =(1 << 25)
    tst r1, r2
    ite eq
    moveq r3, #1               @ bit en 0 -> presionado
    movne r3, #0
    ldr r2, =estado_boton
    str r3, [r2]

    @ ---------- ADC: lee los 8 canales (A0..A7) ----------
    ldr r0, =ADC_BASE
    movs r1, #2
    str r1, [r0, #ADC_CR]      @ START: inicia conversion
    ldr r4, =100000            @ limite de espera para no colgarse

espera_adc:
    ldr r1, [r0, #ADC_ISR]
    ands r1, r1, #0xFF         @ EOC0..EOC7
    cmp r1, #0xFF
    beq adc_listo
    subs r4, r4, #1
    bne espera_adc
    b fin_inputs               @ timeout: se conservan las lecturas anteriores

adc_listo:
    ldr r2, =lectura_adc
    movs r3, #0
copia_adc:
    lsls r5, r3, #2            @ r5 = n * 4
    adds r6, r0, r5            @ ADC_BASE + n*4
    ldr r1, [r6, #ADC_CDR0]    @ lee canal n (esto limpia su EOC)
    str r1, [r2, r5]
    adds r3, r3, #1
    cmp r3, #8
    blt copia_adc

    @ joystick por defecto: A0 (canal 7) = X, A1 (canal 6) = Y
    ldr r1, [r2, #28]
    ldr r3, =eje_x
    str r1, [r3]
    ldr r1, [r2, #24]
    ldr r3, =eje_y
    str r1, [r3]

fin_inputs:
    pop {r4-r7, pc}