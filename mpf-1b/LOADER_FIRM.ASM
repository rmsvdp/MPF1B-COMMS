;------------------------------------------------------------
; LOADER_FIRM.ASM
; 03/07/2025
; RMS
;------------------------------------------------------------

ORG     $1800
; puertos pio en microprofessor 1b        
PA      EQU     $80
PB      EQU     $81
CA      EQU     $82
CB      EQU     $83
OUTPUT  EQU     $0F            ; OUTPUT
INPUT   EQU     $4F            ; INPUT
CONTROL EQU     $CF            ; CONTROL

;------------------------------------------------------------
; Este programa carga el programa cargador que interactúa
; con arduino. Lo ubica en $2000
;------------------------------------------------------------

; -- Inicializa GPIO ----------------------------------------
INIT    LD      A,INPUT    ; código de entrada
        OUT     (CB),A     ; puerto B
        LD      A,CONTROL  ; 1100 1111b
        OUT     (CA),A     ; Puerto A de control
        LD      A,$FF      ; Todo entradas
        OUT     (CA),A
;       Lee loader -----------------------------------------
        LD      DE,$2000
        LD      BC,72      ; 72 bytes
RD_MC   CALL RD_BYTE
        LD   (DE),A
        INC  DE
        DEC  BC
        LD   A,B
        OR   C
        JR   NZ, RD_MC
        HALT 
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
