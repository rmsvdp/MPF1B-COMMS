ORG     $1800

; puertos pio en microprofessor 1b        
        DA      EQU $80
        BB      EQU $81
        CA      EQU $82
        CB      EQU $83
        MODE0   EQU    $0F            ; OUTPUT
        MODE1   EQU    $4F            ; INPUT
        MODE2   EQU    $BF            ; BIDIRECCIONAL
        MODE3   EQU    $CF            ; BIT CONTROL

INIT_O  LD      A,MODE0
        OUT     (CA),A
        LD      A,$01
WR_BYTE OUT     (DA),A
        NOP
        HALT
;--- binary output
;       CD 09 18 3E 01 CD 0F 18 76 3E 0F 32 82 00 C9 D3 80 00 C9   

