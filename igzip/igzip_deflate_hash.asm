%include "options.asm"
%include "lz0a_const.asm"
%include "data_struct2.asm"
%include "huffman.asm"
%include "reg_sizes.asm"

%define DICT_SLOP 4

%ifidn __OUTPUT_FORMAT__, win64
%define arg1 rcx
%define arg2 rdx
%define arg3 r8
%else
%define arg1 rdi
%define arg2 rsi
%define arg3 rdx
%endif

%define stream arg1

%define dict_offset arg2

%define dict_len arg3
%define f_i arg3

%define data r9

%define hash r10

%define f_i_end r11

global isal_deflate_hash_lvl0_01
isal_deflate_hash_lvl0_01:
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
	jg	end

main_loop:
	mov	data %+ d, [f_i + dict_offset]
	compute_hash	hash, data
	and	hash, HASH_MASK
	mov	[stream + _internal_state_head + 2 * hash], f_i %+ w

	add	f_i, 1
	cmp	f_i, f_i_end
	jle	main_loop
end:
	ret
