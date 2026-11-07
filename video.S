.syntax unified
.cpu cortex-m3
.thumb

.global render_buffer_assembly
.global buffer_pantalla

.data
.align 4
buffer_pantalla:
    .space 1024, 0x00          @ 128 x 64 pixeles / 8 = 1024 bytes

.text
@ Limpia el buffer de video (pantalla negra) escribiendo 256 palabras de 32 bits
.thumb_func
render_buffer_assembly:
    ldr r0, =buffer_pantalla
    movs r1, #0
    mov r2, #256
limpiar_loop:
    str r1, [r0], #4
    subs r2, r2, #1
    bne limpiar_loop
    bx lr