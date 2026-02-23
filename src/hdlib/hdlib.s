    .assume  adl=1


    .section        .text._hdl_HalfResSprite_NoClip
    .global _hdl_HalfResSprite_NoClip
    .type   _hdl_HalfResSprite_NoClip, @function

;-------------------------------------------------------------------------------
_hdl_HalfResSprite_NoClip:

    ; --- Setup ---
    pop     de              ; Get return address
    ex      (sp), iy        ; IY = Sprite Pointer
    push    de              ; Restore return address
    push    ix              ; Save IX

    ; Skip the 2-byte sprite header (width/height)
    inc     iy
    inc     iy

    ; Setup Destination Pointers
    ld      de, 0xD40000       ; DE = Write pointer for Left side
    ld      hl, 160
    add     hl, de              ; HL = Write pointer for Right side (DE + 160)

    ld      ixl, 120            ; Row Counter (120 rows)

.RowLoop:
    ld      b, 160              ; Column Counter (160 pixels)

.PixelLoop:
    ld      a, (iy)             ; Load pixel
    inc     iy
    
    or      a, a                ; Check if zero (Transparent)
    jr      z, .SkipPixel

    ld      (de), a             ; Draw Left
    ld      (hl), a             ; Draw Right

.SkipPixel:
    inc     de
    inc     hl
    djnz    .PixelLoop

.NextRow:
    ; --- Fixed Row Transition ---
    ; At this point, HL is already at the start of the next row (Left side).
    ; We need DE to be HL, and HL to be HL + 160.
    
    ex      de, hl              ; DE = HL (Start of next row). HL = Middle of previous row.
    ld      hl, 160
	add     hl, de          	; HL = DE + 160 (Right side of next row)

    dec     ixl
    jr      nz, .RowLoop

    pop     ix
    ret
	
;-------------------------------------------------------------------------------
