########################################################################
#  Copyright(c) 2019 Arm Corporation All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Arm Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#########################################################################

.text
.arch armv8-a

/*int mem_zero_detect_neon(void *buf, size_t n)*/

// input: buf -> x0
// input: n -> x1
// output: -> x0 (true or false)

.global mem_zero_detect_neon
.type mem_zero_detect_neon, %function

mem_zero_detect_neon:
	cmp	x1, #(16*24-1)
	b.ls	.loop_16x24_end

.loop_16x24: // 16x24 block loop

	ldr	q0, [x0]
	ldr	q1, [x0, #16]
	ldr	q2, [x0, #(16*2)]
	ldr	q3, [x0, #(16*3)]
	ldr	q4, [x0, #(16*4)]
	ldr	q5, [x0, #(16*5)]
	ldr	q6, [x0, #(16*6)]
	ldr	q7, [x0, #(16*7)]
	ldr	q16, [x0, #(16*8)]
	ldr	q17, [x0, #(16*9)]
	ldr	q18, [x0, #(16*10)]
	ldr	q19, [x0, #(16*11)]
	ldr	q20, [x0, #(16*12)]
	ldr	q21, [x0, #(16*13)]
	ldr	q22, [x0, #(16*14)]
	ldr	q23, [x0, #(16*15)]
	ldr	q24, [x0, #(16*16)]
	ldr	q25, [x0, #(16*17)]
	ldr	q26, [x0, #(16*18)]
	ldr	q27, [x0, #(16*19)]
	ldr	q28, [x0, #(16*20)]
	ldr	q29, [x0, #(16*21)]
	ldr	q30, [x0, #(16*22)]
	ldr	q31, [x0, #(16*23)]

	add	x0, x0, #(16*24)

	orr	v0.16b, v0.16b, v1.16b
	orr	v2.16b, v2.16b, v3.16b
	orr	v4.16b, v4.16b, v5.16b
	orr	v6.16b, v6.16b, v7.16b
	orr	v16.16b, v16.16b, v17.16b
	orr	v18.16b, v18.16b, v19.16b
	orr	v20.16b, v20.16b, v21.16b
	orr	v22.16b, v22.16b, v23.16b
	orr	v24.16b, v24.16b, v25.16b
	orr	v26.16b, v26.16b, v27.16b
	orr	v28.16b, v28.16b, v29.16b
	orr	v30.16b, v30.16b, v31.16b

	orr	v0.16b, v0.16b, v2.16b
	orr	v4.16b, v4.16b, v6.16b
	orr	v16.16b, v16.16b, v18.16b
	orr	v20.16b, v20.16b, v22.16b
	orr	v24.16b, v24.16b, v26.16b
	orr	v28.16b, v28.16b, v30.16b

	orr	v0.16b, v0.16b, v4.16b
	orr	v16.16b, v16.16b, v20.16b
	orr	v24.16b, v24.16b, v28.16b

	orr	v0.16b, v0.16b, v16.16b
	orr	v0.16b, v0.16b, v24.16b

	mov	x3, v0.d[0]
	mov	x2, v0.d[1]
	orr	x2, x3, x2
	cbnz	x2, .fail_exit

	// loop condition check
	sub	x1, x1, #(16*24)
	cmp	x1, #(16*24-1)
	b.hi	.loop_16x24

.loop_16x24_end:
	cmp	x1, #(16*8-1)
	b.ls	.loop_16x8_end

.loop_16x8: // 16x8 block loop
	ldr	q0, [x0]
	ldr	q1, [x0, #16]
	ldr	q2, [x0, #(16*2)]
	ldr	q3, [x0, #(16*3)]
	ldr	q4, [x0, #(16*4)]
	ldr	q5, [x0, #(16*5)]
	ldr	q6, [x0, #(16*6)]
	ldr	q7, [x0, #(16*7)]

	add	x0, x0, #(16*8)

	orr	v0.16b, v0.16b, v1.16b
	orr	v2.16b, v2.16b, v3.16b
	orr	v4.16b, v4.16b, v5.16b
	orr	v6.16b, v6.16b, v7.16b

	orr	v0.16b, v0.16b, v2.16b
	orr	v4.16b, v4.16b, v6.16b
	orr	v0.16b, v0.16b, v4.16b

	mov	x3, v0.d[0]
	mov	x2, v0.d[1]
	orr	x2, x3, x2
	cbnz	x2, .fail_exit

	sub	x1, x1, #(16*8)
	cmp	x1, #(16*8-1)
	b.hi	.loop_16x8

.loop_16x8_end:
	cmp	x1, #(8*8-1)
	b.ls	.loop_8x8_end

.loop_8x8: // 8x8 block loop
	ldp	x2, x3, [x0]
	ldp	x4, x5, [x0, #16]
	ldp	x6, x7, [x0, #32]
	ldp	x8, x9, [x0, #48]

	add	x0, x0, #(8*8)

	orr	x2, x2, x3
	orr	x4, x4, x5
	orr	x6, x6, x7
	orr	x8, x8, x9
	orr	x2, x2, x4
	orr	x6, x6, x8
	orr	x2, x2, x6

	cbnz	x2, .fail_exit

	sub	x1, x1, #(8*8)
	cmp	x1, #(8*8-1)
	b.hi	.loop_8x8

.loop_8x8_end:
	cmp	x1, #(8-1)
	b.ls	.handle_remainder

.loop_8: // loop per 8bytes
	ldr	x2, [x0]
	add	x0, x0, #8
	cbnz	x2, .fail_exit

	sub	x1, x1, #8
	cmp	x1, #7
	b.hi	.loop_8

.loop_8_end:

	// check remaining bytes
.handle_remainder:
	mov	w2, #0

	cmp	x1, #0
	beq	.handle_reminder_end
	cmp	x1, #1
	beq	.case1
	cmp	x1, #2
	beq	.case2
	cmp	x1, #3
	beq	.case3
	cmp	x1, #4
	beq	.case4
	cmp	x1, #5
	beq	.case5
	cmp	x1, #6
	beq	.case6

.case7: // case7 drop here directly
	ldrb	w3, [x0]
	add	x0, x0, #1
	orr	w2, w2, w3
.case6:
	ldrb	w3, [x0]
	add	x0, x0, #1
	orr	w2, w2, w3
.case5:
	ldrb	w3, [x0]
	add	x0, x0, #1
	orr	w2, w2, w3
.case4:
	ldr	w3, [x0]
	orr	w2, w2, w3
	b	.handle_reminder_end
.case3:
	ldrb	w3, [x0]
	add	x0, x0, #1
	orr	w2, w2, w3
.case2:
	ldrh	w3, [x0]
	orr	w2, w2, w3
	b	.handle_reminder_end
.case1:
	ldrb	w3, [x0]
	orr	w2, w2, w3

.handle_reminder_end:
	cbz	w2, .pass_exit

.fail_exit:
	mov	w0, #0xffffffff
	ret

.pass_exit:
	mov	w0, #0x0
	ret
