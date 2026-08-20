/* ====================================================================
   Proyecto: Sumador Multiplexado (Plantilla Base para Clase)
   Target:   STM32F103 (Proteus / Blue Pill)
   Reloj:    HSI Interno (8 MHz)
   Descripción de Entradas/Salidas:
   - Entradas: PA0 - PA7  -> DIP Switch (Datos A y B)
   - Pulsador: PC13       -> Control de avance (Pull-Up)
   - Salidas:  PB0 - PB7  -> Resultado Suma (8 bits)
               PB8        -> Acarreo / Carry Out (1 bit)
               PB13       -> LED D1 (Indicador Dato A capturado)
               PB14       -> LED D2 (Indicador Dato B capturado)
               PB15       -> LED D3 (Indicador Suma realizada)
   ==================================================================== */

    .syntax unified
    .cpu cortex-m3
    .thumb

    .text
    .align 2
    .global main
    .global Reset_Handler
    .type main, %function
    .type Reset_Handler, %function
    .thumb_func

/* --- Direcciones de Registros RCC, AFIO y GPIO --- */
.equ RCC_APB2ENR,     0x40021018

.equ AFIO_BASE,       0x40010000
.equ AFIO_MAPR,       (AFIO_BASE + 0x04)

.equ GPIOA_CRL,       0x40010800
.equ GPIOA_IDR,       0x40010808

.equ GPIOB_CRL,       0x40010C00
.equ GPIOB_CRH,       0x40010C04
.equ GPIOB_ODR,       0x40010C0C

.equ GPIOC_CRH,       0x40011004
.equ GPIOC_IDR,       0x40011008

Reset_Handler:
main:
    cpsid i                       @ Deshabilitar interrupciones durante la inicialización

    /* 1. Habilitar reloj para AFIO, GPIOA, GPIOB y GPIOC */
    LDR R0, =RCC_APB2ENR
    LDR R1, [R0]
    ORR R1, R1, #(1 << 0) | (1 << 2) | (1 << 3) | (1 << 4)
    STR R1, [R0]

    /* 2. Deshabilitar JTAG en AFIO_MAPR para liberar PB3 y PB4 como GPIOs */
    LDR R0, =AFIO_MAPR
    LDR R1, [R0]
    BIC R1, R1, #(0x7 << 24)
    ORR R1, R1, #(0x2 << 24)
    STR R1, [R0]

    /* 3. Configurar PA0-PA7 como Entradas Digitales Floating (0x44444444) */
    LDR R0, =GPIOA_CRL
    LDR R1, =0x44444444
    STR R1, [R0]

    /* 4. Configurar PB0-PB7 como Salidas Push-Pull 2MHz (0x22222222) */
    LDR R0, =GPIOB_CRL
    LDR R1, =0x22222222
    STR R1, [R0]

    /* 5. Configurar PB8, PB13, PB14 y PB15 como Salidas Push-Pull 2MHz */
    LDR R0, =GPIOB_CRH
    LDR R1, =0x22200002           @ PB15, PB14, PB13 y PB8 en modo salida
    STR R1, [R0]

    /* 6. Configurar PC13 como Entrada Digital Normal (Pull-Up) */
    LDR R0, =GPIOC_CRH
    LDR R1, [R0]
    BIC R1, R1, #(0x0F << 20)
    ORR R1, R1, #(0x04 << 20)
    STR R1, [R0]

/* --- INICIO DE LA MÁQUINA DE ESTADOS --- */
inicio_proceso:
    /* REPOSO INICIAL: Limpiar todas las salidas del Puerto B */
    LDR R0, =GPIOB_ODR
    MOV R1, #0
    STR R1, [R0]

    /* ====================================================================
       EJEMPLO DEMOSTRATIVO: CAPTURA DEL DATO A
       ==================================================================== */
    /* 1. Espera a que el usuario presione el pulsador en PC13 */
    BL esperar_pulsacion

    /* 2. Lee el puerto de entrada PA0-PA7 y almacena el valor en R4 */
    LDR R0, =GPIOA_IDR
    LDR R4, [R0]
    AND R4, R4, #0xFF              @ R4 = Dato A (Aplica máscara para 8 bits)

    /* 3. Indica visualmente que el Dato A fue ingresado encendiendo el LED D1 (PB13) */
    LDR R0, =GPIOB_ODR
    LDR R1, =(1 << 13)            @ PB13 = 1
    STR R1, [R0]


/* ====================================================================
   DESDE AQUÍ DESARROLLEMOS EL CÓDIGO EN CLASE
   ====================================================================
   OBJETIVOS PARA LOS ESTUDIANTES:
   1. CAPTURA DEL DATO B:
      - Esperar la segunda pulsación en PC13 (usar 'BL esperar_pulsacion').
      - Leer el nuevo dato de PA0-PA7 y guardarlo en el registro R5.
      - Apagar el LED D1 (PB13) y encender el LED D2 (PB14).

   /* 1. Espera a que el usuario presione el pulsador en PC13 */
    BL esperar_pulsacion

    /* 2. Lee el puerto de entrada PA0-PA7 y almacena el valor en R4 */
    LDR R0, =GPIOA_IDR
    LDR R5, [R0]
    AND R5, R5, #0xFF              @ R5 = Dato B (Aplica máscara para 8 bits)

    /* 3. Indica visualmente que el Dato A fue ingresado encendiendo el LED D1 (PB13) */
    LDR R0, =GPIOB_ODR
    LDR R1, =(1 << 13)            @ PB13 = 1
    STR R1, [R0]

   /*2. CÁLCULO Y MUESTRA DE LA SUMA:
      - Esperar la tercera pulsación en PC13.
      - Sumar R4 + R5 y guardar el resultado en R6.
      - Apagar el LED D2 (PB14), encender el LED D3 (PB15) y mostrar
        el resultado de la suma en las salidas PB0-PB8.*/
    /* 1. Espera a que el usuario presione el pulsador en PC13 */
    BL esperar_pulsacion
    ADDS R6, R4, R5

    LDR R0, = GPIOB_ODR
    MOV R1, #(1<<15)
    STR R1, [R0]

    BL esperar_pulsacion

    MOV R7, R6
    AND R7, R7, #0xFF

    MOV R2, R6
    LSRS R2, R2, #8
    AND R2, R2, #1

    ORR R7, R7, R2, LSL #8

    LDR R0, =GPIOB_ODR
    STR R7, [R0]
   /*3. REINICIO DE LA SECUENCIA:
      - Esperar la cuarta pulsación en PC13.
      - Salta evamente a 'inicio_proceso' para reiniciar todo a cero.*/

    @ --> Escriban su código a partir de esta línea:
	BL esperar_pulsacion

	B inicio_proceso


    /* Bucle infinito de seguridad en caso de no reiniciar */
bucle_espera:
    B bucle_espera


/* ====================================================================
   SUBRUTINA: esperar_pulsacion
   Sincroniza la lectura del pulsador PC13 (detecta presionar y soltar)
   ==================================================================== */
esperar_pulsacion:
    LDR R0, =GPIOC_IDR

esperar_cero:
    LDR R1, [R0]
    TST R1, #(1 << 13)
    BNE esperar_cero              @ Espera mientras PC13 sea 1 (reposo)

    /* Antirrebote al presionar */
    LDR R2, =8000
delay1:
    SUBS R2, R2, #1
    BNE delay1

esperar_uno:
    LDR R1, [R0]
    TST R1, #(1 << 13)
    BEQ esperar_uno               @ Espera mientras PC13 sea 0 (presionado)

    /* Antirrebote al soltar */
    LDR R2, =8000
delay2:
    SUBS R2, R2, #1
    BNE delay2

    BX LR
