;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2016 Intel Corporation All rights reserved.
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;    * Redistributions of source code must retain the above copyright
;      notice, this list of conditions and the following disclaimer.
;    * Redistributions in binary form must reproduce the above copyright
;      notice, this list of conditions and the following disclaimer in
;      the documentation and/or other materials provided with the
;      distribution.
;    * Neither the name of Intel Corporation nor the names of its
;      contributors may be used to endorse or promote products derived
;      from this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
;  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
;  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
;  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
;  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
;  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

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

START_FIELDS	;; BitBuf2

;;      name		size    align
FIELD	_m_bits,	8,	8
FIELD	_m_bit_count,	4,	4
FIELD	_m_out_buf,	8,	8
FIELD	_m_out_end,	8,	8
FIELD	_m_out_start,	8,	8

%assign _BitBuf2_size	_FIELD_OFFSET
%assign _BitBuf2_align	_STRUCT_ALIGN

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START_FIELDS	;; isal_zstate

;;      name		size    align
FIELD	_b_bytes_valid,	4,	4
FIELD	_b_bytes_processed,	4,	4
FIELD	_file_start,	8,	8
FIELD	_crc,		64,	16
FIELD	_bitbuf,	_BitBuf2_size,	_BitBuf2_align
FIELD	_state,		4,	4
FIELD	_count,		4,	4
FIELD   _tmp_out_buff,	16,	1
FIELD   _tmp_out_start,	4,	4
FIELD	_tmp_out_end,	4,	4
FIELD	_last_flush,	4,	4
FIELD	_has_gzip_hdr,	4,	4
FIELD	_has_eob,		4,	4
FIELD	_has_eob_hdr,	4,	4
FIELD	_left_over, 4,	4
FIELD	_buffer,	BSIZE+16,	32
FIELD	_head,		HASH_SIZE*2,	16

%assign _isal_zstate_size	_FIELD_OFFSET
%assign _isal_zstate_align	_STRUCT_ALIGN

_bitbuf_m_bits		equ	_bitbuf+_m_bits
_bitbuf_m_bit_count	equ	_bitbuf+_m_bit_count
_bitbuf_m_out_buf	equ	_bitbuf+_m_out_buf
_bitbuf_m_out_end	equ	_bitbuf+_m_out_end
_bitbuf_m_out_start	equ	_bitbuf+_m_out_start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START_FIELDS	;; isal_zstream

;;      name		size    align
FIELD	_next_in,	8,	8
FIELD	_avail_in,	4,	4
FIELD	_total_in,	4,	4
FIELD	_next_out,	8,	8
FIELD	_avail_out,	4,	4
FIELD	_total_out,	4,	4
FIELD	_hufftables,	8,	8
FIELD	_end_of_stream,	4,	4
FIELD   _flush,		4,	4
FIELD	_internal_state,	_isal_zstate_size,	_isal_zstate_align

%assign _isal_zstream_size	_FIELD_OFFSET
%assign _isal_zstream_align	_STRUCT_ALIGN

_internal_state_b_bytes_valid		  equ   _internal_state+_b_bytes_valid
_internal_state_b_bytes_processed	 equ   _internal_state+_b_bytes_processed
_internal_state_file_start		  equ   _internal_state+_file_start
_internal_state_crc			  equ   _internal_state+_crc
_internal_state_bitbuf			  equ   _internal_state+_bitbuf
_internal_state_state			  equ   _internal_state+_state
_internal_state_count			  equ   _internal_state+_count
_internal_state_tmp_out_buff		  equ   _internal_state+_tmp_out_buff
_internal_state_tmp_out_start		  equ   _internal_state+_tmp_out_start
_internal_state_tmp_out_end		  equ   _internal_state+_tmp_out_end
_internal_state_last_flush		  equ   _internal_state+_last_flush
_internal_state_has_gzip_hdr		  equ	_internal_state+_has_gzip_hdr
_internal_state_has_eob		  equ   _internal_state+_has_eob
_internal_state_has_eob_hdr		  equ   _internal_state+_has_eob_hdr
_internal_state_left_over		  equ   _internal_state+_left_over
_internal_state_buffer			  equ   _internal_state+_buffer
_internal_state_head			  equ   _internal_state+_head
_internal_state_bitbuf_m_bits		  equ   _internal_state+_bitbuf_m_bits
_internal_state_bitbuf_m_bit_count	equ   _internal_state+_bitbuf_m_bit_count
_internal_state_bitbuf_m_out_buf	  equ   _internal_state+_bitbuf_m_out_buf
_internal_state_bitbuf_m_out_end	  equ   _internal_state+_bitbuf_m_out_end
_internal_state_bitbuf_m_out_start	equ   _internal_state+_bitbuf_m_out_start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ZSTATE_HDR			equ	1
ZSTATE_BODY			equ	2
ZSTATE_FLUSH_READ_BUFFER	equ	3
ZSTATE_SYNC_FLUSH		equ	4
ZSTATE_TRL			equ	6

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_NO_FLUSH		equ 0
_SYNC_FLUSH		equ 1
_FULL_FLUSH		equ 2
_STORED_BLK		equ 0
%assign _STORED_BLK_END 65535
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

