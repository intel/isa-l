; <COPYRIGHT_TAG>

;; START_FIELDS
%macro START_FIELDS 0
%assign _FIELD_OFFSET 0
%assign _STRUCT_ALIGN 0
%endm

;; FIELD name size align
%macro FIELD 3
%define %%name  %1
%define %%size  %2
%define %%align %3

%assign _FIELD_OFFSET (_FIELD_OFFSET + (%%align) - 1) & (~ ((%%align)-1))
%%name	equ	_FIELD_OFFSET
%assign _FIELD_OFFSET _FIELD_OFFSET + (%%size)
%if (%%align > _STRUCT_ALIGN)
%assign _STRUCT_ALIGN %%align
%endif
%endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START_FIELDS	;; inflate huff code

;;      name			size    align
FIELD	_small_code_lookup,	2 * (1 << (DECODE_LOOKUP_SIZE)),	2
FIELD	_long_code_lookup,	2 * MAX_LONG_CODE,			2

%assign _inflate_huff_code_size		_FIELD_OFFSET
%assign _inflate_huff_code_align	_STRUCT_ALIGN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START_FIELDS	;; inflate state

;;      name		size    align
FIELD	_next_out,	8,	8
FIELD	_avail_out,	4,	4
FIELD	_total_out,	4,	4
FIELD	_next_in,	8,	8
FIELD	_read_in,	8,	8
FIELD	_avail_in,	4,	4
FIELD	_read_in_length,4,	4
FIELD	_lit_huff_code,	_inflate_huff_code_size,	_inflate_huff_code_align
FIELD	_dist_huff_code,_inflate_huff_code_size,	_inflate_huff_code_align
FIELD	_new_block,	1,	1
FIELD	_bfinal,	1,	1
FIELD	_btype,		1,	1

%assign _inflate_state_size		_FIELD_OFFSET
%assign _inflate_state_align	_STRUCT_ALIGN

_lit_huff_code_small_code_lookup	equ	_lit_huff_code+_small_code_lookup
_lit_huff_code_long_code_lookup		equ	_lit_huff_code+_long_code_lookup

_dist_huff_code_small_code_lookup	equ	_dist_huff_code+_small_code_lookup
_dist_huff_code_long_code_lookup	equ	_dist_huff_code+_long_code_lookup


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

