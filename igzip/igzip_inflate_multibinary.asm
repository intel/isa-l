; <COPYRIGHT_TAG>

default rel
[bits 64]

%ifidn __OUTPUT_FORMAT__, elf64
%define WRT_OPT         wrt ..plt
%else
%define WRT_OPT
%endif

%include "reg_sizes.asm"

extern decode_huffman_code_block_stateless_base
extern decode_huffman_code_block_stateless_01
extern decode_huffman_code_block_stateless_04

section .text

%include "multibinary.asm"


mbin_interface		decode_huffman_code_block_stateless
mbin_dispatch_init5	decode_huffman_code_block_stateless, decode_huffman_code_block_stateless_base, decode_huffman_code_block_stateless_01, decode_huffman_code_block_stateless_01, decode_huffman_code_block_stateless_04
