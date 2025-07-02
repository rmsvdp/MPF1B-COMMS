ORG     $1800
; puertos pio en microprofessor 1b        
PA      EQU     $80
PB      EQU     $81
CA      EQU     $82
CB      EQU     $83
MODE0   EQU     $0F            ; OUTPUT
MODE1   EQU     $4F            ; INPUT
MODE2   EQU     $BF            ; BIDIRECCIONAL
MODE3   EQU     $CF            ; BIT CONTROL
;-------------------------------------------------------------------------
; Inicializa el GIPO configurando el puerto A y el puerto B de entrada
; del puerto B sólo se utiliza el bit 0
; Lee un byte y termina almacenando el valor en $1900
;-------------------------------------------------------------------------
INIT    LD  A,$4F
        OUT  (CA),A
        LD  A,$CF       ; 1100 1111b
        OUT  (CB),A
        LD  A,$FF
        OUT  (CB),A    
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
        LD ($1900),A
        HALT
