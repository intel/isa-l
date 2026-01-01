##################################################################
#  Copyright (c) 2025 sanechips Technologies Co., Ltd.
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
#    * Neither the name of sanechips Corporation nor the names of its
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
########################################################################

# RISC-V RVV implementation of gf_vect_dot_prod_rvv

# Function: gf_vect_dot_prod_rvv
# Arguments:
#   a0: len (vector length)
#   a1: vlen (number of source vectors)
#   a2: gftbls (pointer to GF(2^8) multiplication tables)
#   a3: src (pointer to array of source vector pointers)
#   a4: dest (pointer to destination vector)

# Local variables:
#   t0: vec_i (source vector index)
#   t1: ptr (pointer to current source vector)
#   t2: pos (current position in vector)
#   t3: tbl1 (pointer to current GF table)

# Vector registers:
#   v0: z_mask0f (mask for low 4 bits)
#   v1: z_src (source vector data)
#   v2: z_src_lo (low 4 bits of source vector)
#   v3: z_src_hi (high 4 bits of source vector)
#   v4: z_dest (destination vector)
#   v5: z_gft1_lo (low 8 bits of GF table)
#   v6: z_gft1_hi (high 8 bits of GF table)

#if HAVE_RVV
.option arch, +v
.global gf_vect_dot_prod_rvv
.type gf_vect_dot_prod_rvv, @function

gf_vect_dot_prod_rvv:
    # Check if len < 16
    li t4, 16
    blt a0, t4, .return_fail

    vsetvli t5, zero, e8, m1, ta, ma  # Set vector length to maximum
    # Initialize pos = 0
    li t2, 0

    # Multiply vlen by 8 (each pointer is 8 bytes)
    slli a1, a1, 3

.Llooprvv_vl:
    # Check if pos >= len
    bge t2, a0, .return_pass

    # Clear z_dest
    vmv.v.i v4, 0

    # Initialize vec_i = 0
    li t0, 0

    # Reset tbl1 to gftbls
    mv t3, a2

.Llooprvv_vl_vects:
    # Load src[vec_i] into ptr
    add t6, a3, t0            # src + vec_i * 8
    ld t1, 0(t6)              # Load pointer to current source vector

    # Load src data into z_src
    add t1, t1, t2          # add offset
    vle8.v v1, (t1)           # Load source vector into v1

    # Increment vec_i
    addi t0, t0, 8

    # Load GF table (low and high)
    vle8.v v5, (t3)           # Load low 8 bits of GF table
    addi t3, t3, 16           # Move to next GF table entry
    vle8.v v6, (t3)           # Load high 8 bits of GF table
    addi t3, t3, 16           # Move to next GF table entry

    # Split src into low and high 4 bits
    vand.vi v2, v1, 0x0F      # z_src_lo = z_src & z_mask0f
    vsrl.vi v3, v1, 4         # z_src_hi = z_src >> 4

    # GF multiplication (table lookup)
    vrgather.vv v8, v5, v2    # z_gft1_lo = GF table lookup for low 4 bits
    vrgather.vv v9, v6, v3    # z_gft1_hi = GF table lookup for high 4 bits

    # GF addition (XOR)
    vxor.vv v4, v4, v8        # z_dest ^= z_gft1_lo
    vxor.vv v4, v4, v9        # z_dest ^= z_gft1_hi

    # Check if vec_i < vlen
    blt t0, a1, .Llooprvv_vl_vects

    # Store z_dest to dest[pos]
    vse8.v v4, (a4)           # Store destination vector
    add a4, a4, t5           # Move dest pointer to next position

    # Increment pos
    add t2, t2, t5           # pos += 16 (vector length)

    j .Llooprvv_vl

.return_pass:
    li a0, 0                  # Return 0 (success)
    ret

.return_fail:
    li a0, 1                  # Return 1 (failure)
    ret

#endif
