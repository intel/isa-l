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

#ifndef __BITBUF2_AARCH64_H__
#define __BITBUF2_AARCH64_H__
#include "options_aarch64.h"

#ifdef __ASSEMBLY__
.macro update_bits	stream:req,code:req,code_len:req,m_bits:req,m_bit_count:req \
			m_out_buf:req

	lsl	x_\code,x_\code,x_\m_bit_count
	orr	x_\m_bits,x_\code,x_\m_bits
	add	x_\m_bit_count,x_\code_len,x_\m_bit_count

	str	x_\m_bits,[x_\m_out_buf]

	and	w_\code,w_\m_bit_count,-8
	lsr	w_\code_len,w_\m_bit_count,3
	add	x_\m_out_buf,x_\m_out_buf,w_\code_len,uxtw
	sub	w_\m_bit_count,w_\m_bit_count,w_\code
	lsr	x_\m_bits,x_\m_bits,x_\code

	str	x_\m_bits,[stream,_internal_state_bitbuf_m_bits]
	str 	w_\m_bit_count,[stream,_internal_state_bitbuf_m_bit_count]
	str	x_\m_out_buf,[stream,_internal_state_bitbuf_m_out_buf]


.endm
#endif
#endif
