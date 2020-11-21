/**********************************************************************
  Copyright(c) 2019 Arm Corporation All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of Arm Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************/
#ifndef __AARCH64_DATA_STRUCT_H__
#define __AARCH64_DATA_STRUCT_H__
#ifdef __ASSEMBLY__

.macro start_struct name:req
	.set _FIELD_OFFSET,0
	.set _STRUCT_ALIGN,0
.endm
.macro end_struct name:req
	.set _\name\()_size,_FIELD_OFFSET
	.set _\name\()_align,_STRUCT_ALIGN
.endm
.macro field name:req, size:req, align:req
	.set _FIELD_OFFSET,(_FIELD_OFFSET + (\align) - 1) & (~ ((\align)-1))
	.set \name,_FIELD_OFFSET
	.set _FIELD_OFFSET,_FIELD_OFFSET + \size
	.if \align > _STRUCT_ALIGN
		.set _STRUCT_ALIGN, \align
	.endif
.endm

/// BitBuf2
start_struct BitBuf2
	///	 name	size	align
	field _m_bits,	8,	8
	field _m_bit_count,	4,	4
	field _m_out_buf,	8,	8
	field _m_out_end,	8,	8
	field _m_out_start,	8,	8
end_struct BitBuf2

/// isal_mod_hist
#define HIST_ELEM_SIZE 4
start_struct isal_mod_hist
	///	 name	size	align
	field _d_hist,	30*HIST_ELEM_SIZE,	HIST_ELEM_SIZE
	field _ll_hist,	513*HIST_ELEM_SIZE,	HIST_ELEM_SIZE
end_struct isal_mod_hist

/// hufftables_icf
#define HUFF_CODE_SIZE 4
start_struct hufftables_icf
	///	 name	size	align
	field _dist_table,	31	*	HUFF_CODE_SIZE,	HUFF_CODE_SIZE
	field _lit_len_table,	513	*	HUFF_CODE_SIZE,	HUFF_CODE_SIZE
end_struct hufftables_icf

/// hash8k_buf
start_struct hash8k_buf
	///	 name	size	align
	field _hash8k_table,	2	*	IGZIP_HASH8K_HASH_SIZE,	2
end_struct hash8k_buf

/// hash_map_buf
start_struct hash_map_buf
	///	 name	size	align
	field _hash_table,	2	*	IGZIP_HASH_MAP_HASH_SIZE,	2
	field _matches_next,	8,	8
	field _matches_end,	8,	8
	field _matches,	4*4*1024,	4
	field _overflow,	4*LA,	4
end_struct hash_map_buf

/// level_buf
#define DEF_MAX_HDR_SIZE 328
start_struct level_buf
	///	 name	size	align
	field _encode_tables,	_hufftables_icf_size,	_hufftables_icf_align
	field _hist,	_isal_mod_hist_size,	_isal_mod_hist_align
	field _deflate_hdr_count,	4,	4
	field _deflate_hdr_extra_bits,4,	4
	field _deflate_hdr,	DEF_MAX_HDR_SIZE,	1
	field _icf_buf_next,	8,	8
	field _icf_buf_avail_out,	8,	8
	field _icf_buf_start,	8,	8
	field _lvl_extra,	_hash_map_buf_size,	_hash_map_buf_align
end_struct level_buf

.set _hash8k_hash_table , _lvl_extra + _hash8k_table
.set _hash_map_hash_table , _lvl_extra + _hash_table
.set _hash_map_matches_next , _lvl_extra + _matches_next
.set _hash_map_matches_end , _lvl_extra + _matches_end
.set _hash_map_matches , _lvl_extra + _matches
.set _hist_lit_len , _hist+_ll_hist
.set _hist_dist , _hist+_d_hist

/// isal_zstate
start_struct isal_zstate
	///	 name	size	align
	field _total_in_start,4,	4
	field _block_next,	4,	4
	field _block_end,	4,	4
	field _dist_mask,	4,	4
	field _hash_mask,	4,	4
	field _state,	4,	4
	field _bitbuf,	_BitBuf2_size,	_BitBuf2_align
	field _crc,	4,	4
	field _has_wrap_hdr,	1,	1
	field _has_eob_hdr,	1,	1
	field _has_eob,	1,	1
	field _has_hist,	1,	1
	field _has_level_buf_init,	2,	2
	field _count,	4,	4
	field _tmp_out_buff,	16,	1
	field _tmp_out_start,	4,	4
	field _tmp_out_end,	4,	4
	field _b_bytes_valid,	4,	4
	field _b_bytes_processed,	4,	4
	field _buffer,	BSIZE,	1
	field _head,	IGZIP_LVL0_HASH_SIZE*2,	2
end_struct isal_zstate

.set _bitbuf_m_bits , _bitbuf+_m_bits
.set _bitbuf_m_bit_count , _bitbuf+_m_bit_count
.set _bitbuf_m_out_buf , _bitbuf+_m_out_buf
.set _bitbuf_m_out_end , _bitbuf+_m_out_end
.set _bitbuf_m_out_start , _bitbuf+_m_out_start

/// isal_zstream
start_struct isal_zstream
	///	 name	size	align
	field _next_in,	8,	8
	field _avail_in,	4,	4
	field _total_in,	4,	4
	field _next_out,	8,	8
	field _avail_out,	4,	4
	field _total_out,	4,	4
	field _hufftables,	8,	8
	field _level,	4,	4
	field _level_buf_size,	4,	4
	field _level_buf,	8,	8
	field _end_of_stream,	2,	2
	field _flush,	2,	2
	field _gzip_flag,	2,	2
	field _hist_bits,	2,	2
	field _internal_state,	_isal_zstate_size,	_isal_zstate_align
end_struct isal_zstream

.set _internal_state_total_in_start , _internal_state+_total_in_start
.set _internal_state_block_next , _internal_state+_block_next
.set _internal_state_block_end , _internal_state+_block_end
.set _internal_state_b_bytes_valid , _internal_state+_b_bytes_valid
.set _internal_state_b_bytes_processed , _internal_state+_b_bytes_processed
.set _internal_state_crc , _internal_state+_crc
.set _internal_state_dist_mask , _internal_state+_dist_mask
.set _internal_state_hash_mask , _internal_state+_hash_mask
.set _internal_state_bitbuf , _internal_state+_bitbuf
.set _internal_state_state , _internal_state+_state
.set _internal_state_count , _internal_state+_count
.set _internal_state_tmp_out_buff , _internal_state+_tmp_out_buff
.set _internal_state_tmp_out_start , _internal_state+_tmp_out_start
.set _internal_state_tmp_out_end , _internal_state+_tmp_out_end
.set _internal_state_has_wrap_hdr , _internal_state+_has_wrap_hdr
.set _internal_state_has_eob , _internal_state+_has_eob
.set _internal_state_has_eob_hdr , _internal_state+_has_eob_hdr
.set _internal_state_has_hist , _internal_state+_has_hist
.set _internal_state_has_level_buf_init , _internal_state+_has_level_buf_init
.set _internal_state_buffer , _internal_state+_buffer
.set _internal_state_head , _internal_state+_head
.set _internal_state_bitbuf_m_bits , _internal_state+_bitbuf_m_bits
.set _internal_state_bitbuf_m_bit_count , _internal_state+_bitbuf_m_bit_count
.set _internal_state_bitbuf_m_out_buf , _internal_state+_bitbuf_m_out_buf
.set _internal_state_bitbuf_m_out_end , _internal_state+_bitbuf_m_out_end
.set _internal_state_bitbuf_m_out_start , _internal_state+_bitbuf_m_out_start

///	 Internal	States
.set ZSTATE_NEW_HDR , 0
.set ZSTATE_HDR , (ZSTATE_NEW_HDR + 1)
.set ZSTATE_CREATE_HDR , (ZSTATE_HDR + 1)
.set ZSTATE_BODY , (ZSTATE_CREATE_HDR + 1)
.set ZSTATE_FLUSH_READ_BUFFER , (ZSTATE_BODY + 1)
.set ZSTATE_FLUSH_ICF_BUFFER , (ZSTATE_FLUSH_READ_BUFFER + 1)
.set ZSTATE_TYPE0_HDR , (ZSTATE_FLUSH_ICF_BUFFER + 1)
.set ZSTATE_TYPE0_BODY , (ZSTATE_TYPE0_HDR + 1)
.set ZSTATE_SYNC_FLUSH , (ZSTATE_TYPE0_BODY + 1)
.set ZSTATE_FLUSH_WRITE_BUFFER , (ZSTATE_SYNC_FLUSH + 1)
.set ZSTATE_TRL , (ZSTATE_FLUSH_WRITE_BUFFER + 1)

.set _NO_FLUSH , 0
.set _SYNC_FLUSH , 1
.set _FULL_FLUSH , 2
.set _STORED_BLK , 0
.set IGZIP_NO_HIST , 0
.set IGZIP_HIST , 1
.set IGZIP_DICT_HIST , 2
#endif
#endif
