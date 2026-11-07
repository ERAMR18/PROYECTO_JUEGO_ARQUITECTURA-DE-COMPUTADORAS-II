.syntax unified
.cpu cortex-m3
.thumb

.global system_init_assembly

@ ---- Direcciones correctas del AT91SAM3X8E (Arduino Due) ----
.equ PMC_BASE,    0x400E0600
.equ PMC_PCER0,   0x010        @ Habilita reloj de perifericos ID 0-31
.equ PMC_PCER1,   0x100        @ Habilita reloj de perifericos ID 32-63

.equ PIOB_BASE,   0x400E1000   @ Puerto B: el pin digital 2 del Due es PB25
.equ PIO_PER,     0x000
.equ PIO_ODR,     0x014        @ Output Disable (entrada)
.equ PIO_PUER,    0x064        @ Pull-up Enable

.equ ADC_BASE,    0x400C0000
.equ ADC_CHER,    0x010        @ Channel Enable Register

.text
.thumb_func
system_init_assembly:
    push {r4-r7, lr}

    @ 1. Reloj de PIOB (ID 12) y del ADC (ID 37 = bit 5 de PCER1)
    ldr r0, =PMC_BASE
    ldr r1, =(1 << 12)
    str r1, [r0, #PMC_PCER0]
    ldr r1, =(1 << 5)
    str r1, [r0, #PMC_PCER1]

    @ 2. Boton en PB25 (pin 2): entrada con pull-up interno
    ldr r0, =PIOB_BASE
    ldr r1, =(1 << 25)
    str r1, [r0, #PIO_PER]
    str r1, [r0, #PIO_ODR]
    str r1, [r0, #PIO_PUER]

    @ 3. ADC: canales 6 y 7 (A1 = AD6, A0 = AD7) para el joystick
    ldr r0, =ADC_BASE
    movs r1, #0xFF             @ canales 0-7 (A7..A0), para diagnostico
    str r1, [r0, #ADC_CHER]

    pop {r4-r7, pc}