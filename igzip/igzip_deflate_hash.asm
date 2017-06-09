%include "options.asm"
%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "huffman.asm"
%include "reg_sizes.asm"

%define DICT_SLOP 8
%define DICT_END_SLOP 4

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%define swap1 rdi
%define swap2 rsi
%else
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%define swap1 r8
%define swap2 rcx
%endif

%define stream arg1

%define dict_offset arg2

%define dict_len arg3
%define f_i arg3

%define f_i_tmp swap1

%define hash swap2

%define hash2 r9

%define hash3 r10

%define hash4 r11

%define f_i_end rax

%macro FUNC_SAVE 0
%ifidn __OUTPUT_FORMAT__, win64
	push rsi
	push rdi
%endif
%endm

%macro FUNC_RESTORE 0
%ifidn __OUTPUT_FORMAT__, win64
	pop rdi
	pop rsi
%endif
%endm

global isal_deflate_hash_lvl0_01
isal_deflate_hash_lvl0_01:
	FUNC_SAVE

%ifnidn (arg1, stream)
	mov stream, arg1
%endif
%ifnidn (arg2, dict_next)
	mov dict_offset, arg2
%endif

	mov	f_i_end %+ d, dword [stream + _total_in]
	neg	f_i
	add	f_i, f_i_end

	sub	dict_offset, f_i

	sub	f_i_end, DICT_SLOP
	cmp	f_i, f_i_end
	jg	end_main

main_loop:
	lea	f_i_tmp, [f_i + 2]

	xor	hash, hash
	crc32	hash %+ d, dword [f_i + dict_offset]

	xor	hash2, hash2
	crc32	hash2 %+ d, dword [f_i + dict_offset + 1]

	xor	hash3, hash3
	crc32	hash3 %+ d, dword [f_i_tmp + dict_offset]

	xor	hash4, hash4
	crc32	hash4 %+ d, dword [f_i_tmp + dict_offset + 1]

	and	hash, HASH_MASK
	and	hash2, HASH_MASK
	and	hash3, HASH_MASK
	and	hash4, HASH_MASK

	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w
	add	f_i, 1

	mov	[stream + _internal_state_head + 2 * hash2], f_i %+ w
	add	f_i, 3

	mov	[stream + _internal_state_head + 2 * hash3], f_i_tmp %+ w
	add	f_i_tmp, 1

	mov	[stream + _internal_state_head + 2 * hash4], f_i_tmp %+ w

	cmp	f_i, f_i_end
	jle	main_loop

end_main:
	add	f_i_end, DICT_SLOP - DICT_END_SLOP
	cmp	f_i, f_i_end
	jg	end

end_loop:
	xor	hash, hash
	crc32	hash %+ d, dword [f_i + dict_offset]

	and	hash, HASH_MASK
	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	add	f_i, 1
	cmp	f_i, f_i_end
	jle	end_loop
end:
	FUNC_RESTORE
	ret
