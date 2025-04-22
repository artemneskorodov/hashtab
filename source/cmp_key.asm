;===============================================================================
global cmp_key_optimized
;===============================================================================

;-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
section .text
;-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

;===============================================================================
; Description: Function to compare keys which are smaller than 32 bytes.
;
; Expects:     YMM0  - 1sf string
;              [RDI] - 2nd string
;
; Returns:     ZF    - set if strings are equal, unset overwise
;              AL    - set to zero if strings are equal, not zero overwise
; Destroys:    YMM1 RAX
;
; Note:        AL setting to zeros if strings are equal is side effect of this
;              function. It can be used as a return value of boolean function
;              but with reversing of the result.
;-------------------------------------------------------------------------------
cmp_key_optimized:
    ;---------------------------------------------------------------------------
    ; Loading second key to YMM1.
    vmovdqa ymm1, [rdi]
    ;---------------------------------------------------------------------------
    ; Comparing, setting to 1s bytes which are equal. Mask is written to YMM2,
    ; so bytes which are set to 1s in YMM2 are equal.
    vpcmpeqd ymm1, ymm0, ymm1
    ;---------------------------------------------------------------------------
    ; Loading compare result mask to AL (8 bits are set as we use vmovmskps
    ; which treats ymm1 register as 8 values with 32 bits).
    vmovmskps rax, ymm1
    ;---------------------------------------------------------------------------
    ; Incrementing AL. If strings are equal we have AL == 0xFF, so ZF is set if
    ; after incrementing.
    inc al
    ;---------------------------------------------------------------------------
    ret
;===============================================================================

;-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
section .note.GNU-stack noalloc noexec nowrite progbits
;-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
