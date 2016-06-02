%define ARCH 04
%define USE_HSWNI

%ifndef COMPARE_TYPE
%define COMPARE_TYPE 3
%endif

%include "igzip_buffer_utils_04.asm"
%include "igzip_body.asm"
