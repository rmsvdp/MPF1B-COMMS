;-----------------------------
; NOTA DEL 03/07/2025
; No se estaba usando la señal de /ASTB para
; validar los datos en el PA
; Aún así el bit 1 del PA siempre queda a 0

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
; Este programa lee 4 bytes consecutivos, de acuerdo al 
; protocolo implementado en TEST_04.ASM
; Se han invertido los puertos, ahora PB es de entrada y PA
; es de control en su bit 0 ( que si funcionaba)
; es preciso cambiar /ASTB por /BSTB
;------------------------------------------------------------

INIT    LD      A,INPUT    ; código de entrada
        OUT     (CB),A     ; puerto B
        LD      A,CONTROL  ; 1100 1111b
        OUT     (CA),A     ; Puerto A de control
        LD      A,$FF      ; Todo entradas
        OUT     (CA),A
        LD      HL,$1900   ; buffer de recepcción     
RDHEAD  CALL    RD_BYTE    ; byte 1
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE    ; byte 2
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE    ; byte 3
        LD      (HL),A       
        INC     HL
        CALL    RD_BYTE    ; byte 4     
        LD      (HL),A       
        INC     HL       
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