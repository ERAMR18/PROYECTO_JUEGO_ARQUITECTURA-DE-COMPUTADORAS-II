.syntax unified
.cpu cortex-m3
.thumb

.global update_physics_assembly
.global nave_x
.global nave_y
.global disparo_activo
.global disparo_x
.global disparo_y

@ ---- Ajustes ----
.equ SHIP_W,        15
.equ MIN_X,         0
.equ MAX_X,         113        @ 128 - 15
.equ MIN_Y,         10         @ debajo del marcador
.equ MAX_Y,         55         @ 64 - 9
.equ SPEED,         3          @ pixeles por cuadro
.equ BULLET_SPEED,  5
.equ TH_LOW,        1200       @ umbral joystick hacia un lado
.equ TH_HIGH,       2900       @ umbral joystick hacia el otro
.equ INVERT_X,      0          @ poner 1 si la nave va al reves en X
.equ INVERT_Y,      0          @ poner 1 si la nave va al reves en Y

.data
.align 4
nave_x:        .word 10
nave_y:        .word 28
disparo_activo: .word 0
disparo_x:     .word 0
disparo_y:     .word 0

.text
.thumb_func
update_physics_assembly:
    push {r4-r7, lr}

    ldr r4, =nave_x
    ldr r5, [r4]               @ r5 = x
    ldr r6, =nave_y
    ldr r7, [r6]               @ r7 = y

    @ ---------- Eje X ----------
    ldr r0, =eje_x
    ldr r0, [r0]
    ldr r1, =TH_LOW
    cmp r0, r1
    blt x_bajo
    ldr r1, =TH_HIGH
    cmp r0, r1
    bgt x_alto
    b x_fin
x_bajo:
.if INVERT_X
    adds r5, r5, #SPEED
.else
    subs r5, r5, #SPEED
.endif
    b x_fin
x_alto:
.if INVERT_X
    subs r5, r5, #SPEED
.else
    adds r5, r5, #SPEED
.endif
x_fin:
    cmp r5, #MIN_X
    bge x_ok1
    movs r5, #MIN_X
x_ok1:
    cmp r5, #MAX_X
    ble x_ok2
    movs r5, #MAX_X
x_ok2:
    str r5, [r4]

    @ ---------- Eje Y ----------
    ldr r0, =eje_y
    ldr r0, [r0]
    ldr r1, =TH_LOW
    cmp r0, r1
    blt y_bajo
    ldr r1, =TH_HIGH
    cmp r0, r1
    bgt y_alto
    b y_fin
y_bajo:
.if INVERT_Y
    adds r7, r7, #SPEED
.else
    subs r7, r7, #SPEED
.endif
    b y_fin
y_alto:
.if INVERT_Y
    subs r7, r7, #SPEED
.else
    adds r7, r7, #SPEED
.endif
y_fin:
    cmp r7, #MIN_Y
    bge y_ok1
    movs r7, #MIN_Y
y_ok1:
    cmp r7, #MAX_Y
    ble y_ok2
    movs r7, #MAX_Y
y_ok2:
    str r7, [r6]

    @ ---------- Disparo ----------
    ldr r0, =disparo_activo
    ldr r1, [r0]
    cmp r1, #0
    bne mover_bala             @ ya hay bala: solo se mueve

    ldr r2, =estado_boton
    ldr r2, [r2]
    cmp r2, #1
    bne fin_physics            @ sin bala y sin boton: nada

    adds r2, r5, #SHIP_W       @ bala sale de la punta de la nave
    ldr r3, =disparo_x
    str r2, [r3]
    adds r2, r7, #4            @ centro vertical
    ldr r3, =disparo_y
    str r2, [r3]
    movs r1, #1
    str r1, [r0]               @ disparo_activo = 1
    b fin_physics

mover_bala:
    ldr r3, =disparo_x
    ldr r2, [r3]
    adds r2, r2, #BULLET_SPEED
    str r2, [r3]
    cmp r2, #127
    ble fin_physics
    movs r1, #0
    str r1, [r0]               @ salio de pantalla: desactivar

fin_physics:
    pop {r4-r7, pc}