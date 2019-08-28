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

#define FIELD(name,size,align) \
	.set _FIELD_OFFSET,(_FIELD_OFFSET + (align) - 1) & (~ ((align)-1)); \
	.equ name,_FIELD_OFFSET ;					\
	.set _FIELD_OFFSET,_FIELD_OFFSET + size;                        \
	.if align > _STRUCT_ALIGN;                                      \
		.set _STRUCT_ALIGN, align;                              \
	.endif;

#define START_STRUCT(name) .set _FIELD_OFFSET,0;.set _STRUCT_ALIGN,0;

#define END_STRUCT(name)   .set _##name##_size,_FIELD_OFFSET;\
			   .set _##name##_align,_STRUCT_ALIGN

#define CONST(name,value)  .equ name,value


/// BitBuf2
START_STRUCT(BitBuf2)
	///	 name	size	align
	FIELD (	 _m_bits,	8,	8 )
	FIELD (	 _m_bit_count,	4,	4 )
	FIELD (	 _m_out_buf,	8,	8 )
	FIELD (	 _m_out_end,	8,	8 )
	FIELD (	 _m_out_start,	8,	8 )
END_STRUCT(BitBuf2)


/// isal_mod_hist
#define HIST_ELEM_SIZE 4
START_STRUCT(isal_mod_hist)
	///	 name	size	align
	FIELD (	 _d_hist,	30*HIST_ELEM_SIZE,	HIST_ELEM_SIZE )
	FIELD (	 _ll_hist,	513*HIST_ELEM_SIZE,	HIST_ELEM_SIZE )
END_STRUCT(isal_mod_hist)


/// hufftables_icf
#define HUFF_CODE_SIZE 4
START_STRUCT(hufftables_icf)
	///	 name	size	align
	FIELD (	 _dist_table,	31	*	HUFF_CODE_SIZE,	HUFF_CODE_SIZE )
	FIELD (	 _lit_len_table,	513	*	HUFF_CODE_SIZE,	HUFF_CODE_SIZE )
END_STRUCT(hufftables_icf)


/// hash8k_buf
START_STRUCT(hash8k_buf)
	///	 name	size	align
	FIELD (	 _hash8k_table,	2	*	IGZIP_HASH8K_HASH_SIZE,	2 )
END_STRUCT(hash8k_buf)


/// hash_map_buf
START_STRUCT(hash_map_buf)
	///	 name	size	align
	FIELD (	 _hash_table,	2	*	IGZIP_HASH_MAP_HASH_SIZE,	2 )
	FIELD (	 _matches_next,	8,	8 )
	FIELD (	 _matches_end,	8,	8 )
	FIELD (	 _matches,	4*4*1024,	4 )
	FIELD (	 _overflow,	4*LA,	4 )
END_STRUCT(hash_map_buf)


/// level_buf
#define DEF_MAX_HDR_SIZE 328
START_STRUCT(level_buf)
	///	 name	size	align
	FIELD (	 _encode_tables,	_hufftables_icf_size,	_hufftables_icf_align )
	FIELD (	 _hist,	_isal_mod_hist_size,	_isal_mod_hist_align )
	FIELD (	 _deflate_hdr_count,	4,	4 )
	FIELD (	 _deflate_hdr_extra_bits,4,	4 )
	FIELD (	 _deflate_hdr,	DEF_MAX_HDR_SIZE,	1 )
	FIELD (	 _icf_buf_next,	8,	8 )
	FIELD (	 _icf_buf_avail_out,	8,	8 )
	FIELD (	 _icf_buf_start,	8,	8 )
	FIELD (	 _lvl_extra,	_hash_map_buf_size,	_hash_map_buf_align )
END_STRUCT(level_buf)


CONST( _hash8k_hash_table , _lvl_extra + _hash8k_table )
CONST( _hash_map_hash_table , _lvl_extra + _hash_table )
CONST( _hash_map_matches_next , _lvl_extra + _matches_next )
CONST( _hash_map_matches_end , _lvl_extra + _matches_end )
CONST( _hash_map_matches , _lvl_extra + _matches )
CONST( _hist_lit_len , _hist+_ll_hist )
CONST( _hist_dist , _hist+_d_hist )


/// isal_zstate
START_STRUCT(isal_zstate)
	///	 name	size	align
	FIELD (	 _total_in_start,4,	4 )
	FIELD (	 _block_next,	4,	4 )
	FIELD (	 _block_end,	4,	4 )
	FIELD (	 _dist_mask,	4,	4 )
	FIELD (	 _hash_mask,	4,	4 )
	FIELD (	 _state,	4,	4 )
	FIELD (	 _bitbuf,	_BitBuf2_size,	_BitBuf2_align )
	FIELD (	 _crc,	4,	4 )
	FIELD (	 _has_wrap_hdr,	1,	1 )
	FIELD (	 _has_eob_hdr,	1,	1 )
	FIELD (	 _has_eob,	1,	1 )
	FIELD (	 _has_hist,	1,	1 )
	FIELD (	 _has_level_buf_init,	2,	2 )
	FIELD (	 _count,	4,	4 )
	FIELD (	 _tmp_out_buff,	16,	1 )
	FIELD (	 _tmp_out_start,	4,	4 )
	FIELD (	 _tmp_out_end,	4,	4 )
	FIELD (	 _b_bytes_valid,	4,	4 )
	FIELD (	 _b_bytes_processed,	4,	4 )
	FIELD (	 _buffer,	BSIZE,	1 )
	FIELD (	 _head,	IGZIP_LVL0_HASH_SIZE*2,	2 )
END_STRUCT(isal_zstate)



CONST( _bitbuf_m_bits , _bitbuf+_m_bits )
CONST( _bitbuf_m_bit_count , _bitbuf+_m_bit_count )
CONST( _bitbuf_m_out_buf , _bitbuf+_m_out_buf )
CONST( _bitbuf_m_out_end , _bitbuf+_m_out_end )
CONST( _bitbuf_m_out_start , _bitbuf+_m_out_start )


/// isal_zstream
START_STRUCT(isal_zstream)
	///	 name	size	align
	FIELD (	 _next_in,	8,	8 )
	FIELD (	 _avail_in,	4,	4 )
	FIELD (	 _total_in,	4,	4 )
	FIELD (	 _next_out,	8,	8 )
	FIELD (	 _avail_out,	4,	4 )
	FIELD (	 _total_out,	4,	4 )
	FIELD (	 _hufftables,	8,	8 )
	FIELD (	 _level,	4,	4 )
	FIELD (	 _level_buf_size,	4,	4 )
	FIELD (	 _level_buf,	8,	8 )
	FIELD (	 _end_of_stream,	2,	2 )
	FIELD (	 _flush,	2,	2 )
	FIELD (	 _gzip_flag,	2,	2 )
	FIELD (	 _hist_bits,	2,	2 )
	FIELD (	 _internal_state,	_isal_zstate_size,	_isal_zstate_align )
END_STRUCT(isal_zstream)



CONST( _internal_state_total_in_start , _internal_state+_total_in_start )
CONST( _internal_state_block_next , _internal_state+_block_next )
CONST( _internal_state_block_end , _internal_state+_block_end )
CONST( _internal_state_b_bytes_valid , _internal_state+_b_bytes_valid )
CONST( _internal_state_b_bytes_processed , _internal_state+_b_bytes_processed )
CONST( _internal_state_crc , _internal_state+_crc )
CONST( _internal_state_dist_mask , _internal_state+_dist_mask )
CONST( _internal_state_hash_mask , _internal_state+_hash_mask )
CONST( _internal_state_bitbuf , _internal_state+_bitbuf )
CONST( _internal_state_state , _internal_state+_state )
CONST( _internal_state_count , _internal_state+_count )
CONST( _internal_state_tmp_out_buff , _internal_state+_tmp_out_buff )
CONST( _internal_state_tmp_out_start , _internal_state+_tmp_out_start )
CONST( _internal_state_tmp_out_end , _internal_state+_tmp_out_end )
CONST( _internal_state_has_wrap_hdr , _internal_state+_has_wrap_hdr )
CONST( _internal_state_has_eob , _internal_state+_has_eob )
CONST( _internal_state_has_eob_hdr , _internal_state+_has_eob_hdr )
CONST( _internal_state_has_hist , _internal_state+_has_hist )
CONST( _internal_state_has_level_buf_init , _internal_state+_has_level_buf_init )
CONST( _internal_state_buffer , _internal_state+_buffer )
CONST( _internal_state_head , _internal_state+_head )
CONST( _internal_state_bitbuf_m_bits , _internal_state+_bitbuf_m_bits )
CONST( _internal_state_bitbuf_m_bit_count , _internal_state+_bitbuf_m_bit_count )
CONST( _internal_state_bitbuf_m_out_buf , _internal_state+_bitbuf_m_out_buf )
CONST( _internal_state_bitbuf_m_out_end , _internal_state+_bitbuf_m_out_end )
CONST( _internal_state_bitbuf_m_out_start , _internal_state+_bitbuf_m_out_start )

///	 Internal	States
CONST( ZSTATE_NEW_HDR , 0 )
CONST( ZSTATE_HDR , (ZSTATE_NEW_HDR + 1) )
CONST( ZSTATE_CREATE_HDR , (ZSTATE_HDR + 1) )
CONST( ZSTATE_BODY , (ZSTATE_CREATE_HDR + 1) )
CONST( ZSTATE_FLUSH_READ_BUFFER , (ZSTATE_BODY + 1) )
CONST( ZSTATE_FLUSH_ICF_BUFFER , (ZSTATE_FLUSH_READ_BUFFER + 1) )
CONST( ZSTATE_TYPE0_HDR , (ZSTATE_FLUSH_ICF_BUFFER + 1) )
CONST( ZSTATE_TYPE0_BODY , (ZSTATE_TYPE0_HDR + 1) )
CONST( ZSTATE_SYNC_FLUSH , (ZSTATE_TYPE0_BODY + 1) )
CONST( ZSTATE_FLUSH_WRITE_BUFFER , (ZSTATE_SYNC_FLUSH + 1) )
CONST( ZSTATE_TRL , (ZSTATE_FLUSH_WRITE_BUFFER + 1) )

CONST( _NO_FLUSH , 0 )
CONST( _SYNC_FLUSH , 1 )
CONST( _FULL_FLUSH , 2 )
CONST( _STORED_BLK , 0 )
CONST( IGZIP_NO_HIST , 0 )
CONST( IGZIP_HIST , 1 )
CONST( IGZIP_DICT_HIST , 2 )
#endif
#endif
