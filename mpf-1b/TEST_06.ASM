;-----------------------------
; NOTA DEL 03/07/2025
; No se estaba usando la señal de /ASTB para
; validar los datos en el PA
; Aún así el bit 1 del PA siempre queda a 0
ORG     $0000
        DI
        JP      CLS
ORG     $3A00
CLS     LD      HL,$4000
        LD      BC,$17FF
        LD      DE,$4001
        LD      (HL),$00
        LDIR
        LD      HL,$5800
        LD      BC,$02FF
        LD      DE,$5801
        LD      (HL),$07
        LDIR     
        CALL    PRT
LOOP:   NOP
        NOP
JR      LOOP
PRT     LD      HL,_TXT
        LD      B, _END -_TXT
PRT_C   LD      A,(HL)
        RST     #10
        INC     HL
        DJNZ    PRT_C
        RET
_TXT    DB      $10,$01
        DB      $16,$0A,$0A
        DB      'MPF 1B EMULATOR'
_END:   DB      $         
