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

;;      name				size    align
FIELD	_small_code_lookup_large,	2 * (1 << (DECODE_LOOKUP_SIZE_LARGE)),	2
FIELD	_long_code_lookup_large,	2 * MAX_LONG_CODE_LARGE,		2

%assign _inflate_huff_code_large_size	_FIELD_OFFSET
%assign _inflate_huff_code_large_align	_STRUCT_ALIGN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START_FIELDS	;; inflate huff code

;;      name				size    align
FIELD	_small_code_lookup_small,	2 * (1 << (DECODE_LOOKUP_SIZE_SMALL)),	2
FIELD	_long_code_lookup_small,	2 * MAX_LONG_CODE_SMALL,		2

%assign _inflate_huff_code_small_size	_FIELD_OFFSET
%assign _inflate_huff_code_small_align	_STRUCT_ALIGN
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
FIELD	_lit_huff_code,	_inflate_huff_code_large_size,	_inflate_huff_code_large_align
FIELD	_dist_huff_code,_inflate_huff_code_small_size,	_inflate_huff_code_small_align
FIELD	_block_state,	4,	4
FIELD	_bfinal,	4,	4
FIELD	_type0_block_len,	4, 	4
FIELD	_copy_overflow_len,  	4,	4
FIELD	_copy_overflow_dist,	4,	4

%assign _inflate_state_size		_FIELD_OFFSET
%assign _inflate_state_align	_STRUCT_ALIGN

_lit_huff_code_small_code_lookup	equ	_lit_huff_code+_small_code_lookup_large
_lit_huff_code_long_code_lookup		equ	_lit_huff_code+_long_code_lookup_large

_dist_huff_code_small_code_lookup	equ	_dist_huff_code+_small_code_lookup_small
_dist_huff_code_long_code_lookup	equ	_dist_huff_code+_long_code_lookup_small

ISAL_BLOCK_NEW_HDR	equ	0
ISAL_BLOCK_HDR		equ	1
ISAL_BLOCK_TYPE0	equ	2
ISAL_BLOCK_CODED	equ	3
ISAL_BLOCK_END		equ	4
ISAL_BLOCK_FINISH	equ	5
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

