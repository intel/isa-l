;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;  Copyright(c) 2011-2017 Intel Corporation All rights reserved.
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

%include "reg_sizes.asm"
%include "multibinary.asm"

[bits 32]

extern xor_gen_base
extern xor_gen_sse
extern pq_gen_base
extern pq_gen_sse
extern xor_check_base
extern xor_check_sse
extern pq_check_base
extern pq_check_sse

mbin_interface xor_gen
mbin_interface pq_gen
mbin_interface xor_check
mbin_interface pq_check

mbin_dispatch_init5 xor_gen, xor_gen_base, xor_gen_sse, xor_gen_sse, xor_gen_sse
mbin_dispatch_init5 pq_gen, pq_gen_base, pq_gen_sse, pq_gen_sse, pq_gen_sse
mbin_dispatch_init5 xor_check, xor_check_base, xor_check_sse, xor_check_sse, xor_check_sse
mbin_dispatch_init5 pq_check, pq_check_base, pq_check_sse, pq_check_sse, pq_check_sse
