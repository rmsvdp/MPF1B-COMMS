;------------------------------------------------------------
; LOADER_FINAL.ASM
; 03/07/2025
; RMS
;------------------------------------------------------------

ORG     $2000
; puertos pio en microprofessor 1b        
PA      EQU     $80
PB      EQU     $81
CA      EQU     $82
CB      EQU     $83
OUTPUT  EQU     $0F            ; OUTPUT
INPUT   EQU     $4F            ; INPUT
CONTROL EQU     $CF            ; CONTROL

;------------------------------------------------------------
; Este programa carga un bloque de codigo máquina desde arduino
;
; 1.- Lee una cabecera de 6 bytes correspondiéndose con :
;         00,01 : punto de carga
;         02,03 : tamaño
;         04,05 : dirección de ejecución
; 2.- Lee 'n' bytes (registro BC), correspondiendose con el tamaño del programa
;         se ubica en el punto de carga (registro DE)
; 3.  Salta la ejecución a la dirección apuntada por el registro HL
;------------------------------------------------------------

; -- Inicializa GPIO ----------------------------------------
INIT    LD      A,INPUT    ; código de entrada
        OUT     (CB),A     ; puerto B
        LD      A,CONTROL  ; 1100 1111b
        OUT     (CA),A     ; Puerto A de control
        LD      A,$FF      ; Todo entradas
        OUT     (CA),A
;       Lee la cabecera de 6 bytes ---------------------------
RDHEAD  CALL    RD_BYTE
        LD      D,A        ; Leer punto de carga
        CALL    RD_BYTE
        LD      E,A
        CALL    RD_BYTE    ;Leer tamaño
        LD      B,A
        CALL    RD_BYTE
        LD      C,A        
        CALL    RD_BYTE    ;Leer punto de ejecución
        LD      H,A
        CALL    RD_BYTE
        LD      L,A
;       Lee bloque de BC bytes -----------------------------
RD_MC   CALL RD_BYTE
        CALL SHOW_S     ; cambia status en mpf-1b
        LD   (DE),A
        INC  DE
        DEC  BC
        LD   A,B
        OR   C
        JR   NZ, RD_MC
;       transfiere ejecución a contenido HL si este es distinto de cero, sino detiene ejecución 
        LD      A,H
        OR      L
        JR      Z, RUN
        HALT 
RUN     JP      (HL)        
;-----------------------------------------------
;RD_BYTE : Lee de forma asíncrona un byte
;CE :       PIO inicializado
;CS :       A = Byte recibido
; el z80 se queda esperando a que la linea strobe ( igual al bit 0 del
; puerto A se estabilice a cero. Despues no leerá el puerto B hasta
; el flanco de subida del bit 0 del puerto A)
; Como la señal /BSTB está unida al bit 0 al pasar por cero, se
; valida el contenido del puerto B
;-----------------------------------------------
RD_BYTE IN A,(PA)        ; lee estado pin 0
        BIT 0,A          ; Test bit 0
        JR NZ,RD_BYTE     
RISE    IN A,(PA)        ; En este momento Bit A0 = 0 -> /BSTB activa
        BIT 0,A          ; Test bit 0
        JR Z, RISE       ; Esperar a que el Dispositivo autorize lectura
GETDATA IN A,(PB)        ; Se puede leer con seguridad
        RET
;-----------------------------------------------
SHOW_S  NOP
        RET
;-----------------------------------------------