
ORG     $1900
;       Cabecera de 6 bytes
SADDR   EQU     $
TAM     EQU     FIN-INICIO
START   DEFW    SADDR
LENGTH  DEFW    TAM
RUN     DEFW    INICIO
INICIO  LD      A,3
        LD      B,5
        ADD     A,B
        LD      (RESULT),A
        HALT
RESULT  DEFB    0        
FIN     NOP