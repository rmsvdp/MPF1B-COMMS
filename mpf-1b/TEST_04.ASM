ORG     $1800
; puertos pio en microprofessor 1b        
PA      EQU     $80
PB      EQU     $81
CA      EQU     $82
CB      EQU     $83
OUTPUT  EQU     $0F            ; OUTPUT
INPUT   EQU     $4F            ; INPUT

;------------------------------------------------------------
; Este programa lee 4 bytes consecutivos, de acuerdo al 
; protocolo implementado en TEST_02.ASM
; cuando se detiene la ejecución la información está
; a partir de $1900
;------------------------------------------------------------

INIT    LD  A,INPUT
        OUT  (CA),A
        LD  A,$CF       ; 1100 1111b
        OUT  (CB),A
        LD  A,$FF
        OUT  (CB),A
        LD HL,$1900
RDHEAD  CALL    RD_BYTE
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE    ;Leer tamaño
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE
        LD      (HL),A       
        INC     HL       
        HALT
;-----------------------------------------------
;RD_BYTE : Lee de forma asíncrona un byte
;CE :       PIO inicializado
;CS :       A = Byte recibido
; el z80 se queda esperando a que la linea strobe ( igual al bit 0 del
; puerto B se estabilice a cero. Despues no leerá el puerto A hasta
; el flanco de subida del bit 0 del puerto B)
;-----------------------------------------------
RD_BYTE IN A,(PB)        ; lee estado pin 0
        BIT 0,A          ; Test bit 0
        JR NZ,RD_BYTE     
RISE    IN A,(PB)
        BIT 0,A          ; Test bit 0
        JR Z, RISE
GETDATA IN A,(PA)
        RET
;-----------------------------------------------