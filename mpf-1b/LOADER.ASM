ORG     $1800
; puertos pio en microprofessor 1b        
PA      EQU     $80
PB      EQU     $81
CA      EQU     $82
CB      EQU     $83
OUTPUT  EQU     $0F            ; OUTPUT
INPUT   EQU     $4F            ; INPUT

;------------------------------------------------------------
INICIO  CALL    RDBLK;
        HALT
;  Cargar programa en codigo máquina desde arduino
;
; RDBLK : Lee un bloque de código máquina con cabecera
;         La cabecera son 6 bytes correspondiéndose con :
;         00,01 : punto de carga
;         02,03 : tamaño
;         04,05 : dirección de ejecución
;         'n' bytes , correspondiendose con el tamaño del programa
RDBLK   CALL    INIT       ; inicialiar PIO
;       Lee la cabecera de 6 bytes
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
; en estos momentos DE contiene el punto de carga,
; BC el tamaño y HL la dirección de ejecución        
;-------------------------
; Leemos el bloque de código maquina
RD_MC   CALL RD_BYTE
        CALL SHOW_S     ; cambia status en mpf-1b
        LD   (DE),A
        INC  DE
        DEC  BC
        LD   A,B
        OR   C
        JR   NZ, RD_MC
        RET

;-------------------------
; Inicializa el GIPO configurando el puerto A y el puerto B de entrada
; del puerto B sólo se utiliza el bit 0
INIT    LD  A,INPUT
        OUT  (CA),A
        LD  A,$CF       ; 1100 1111b
        OUT  (CB),A
        LD  A,$FF
        OUT  (CB),A
        RET        
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

; Muestra avance en mpf-1b
SHOW_S  NOP
FIN     RET
;-------------- binario
