NAME=-2.6.19-lttng-$1
?_kernel_relay.c
?_block_blktrace.c"
FILE=../${PRENAME}${COUNT}${NAME}-atomic.diff
IN="
?_include_asm-alpha_atomic.h
?_include_asm-alpha_system.h
?_include_asm-arm_atomic.h
?_include_asm-generic_atomic.h
?_include_asm-i386_atomic.h
?_include_asm-ia64_atomic.h
?_include_asm-mips_atomic.h
?_include_asm-parisc_atomic.h
?_include_asm-powerpc_atomic.h
?_include_asm-sparc64_atomic.h
?_include_asm-x86_64_atomic.h
?_include_asm-x86_64_system.h"
FILE=../${PRENAME}${COUNT}${NAME}-local.diff
IN="
?_include_asm-alpha_local.h
?_include_asm-generic_local.h
?_include_asm-i386_local.h
?_include_asm-ia64_local.h
?_include_asm-mips_local.h
?_include_asm-parisc_local.h
?_include_asm-powerpc_local.h
?_include_asm-s390_local.h
?_include_asm-sparc64_local.h
?_include_asm-x86_64_local.h"
FILE=../${PRENAME}${COUNT}${NAME}-facilities.diff
VALUE=$(( ${VALUE} + 1 ))
printf -v COUNT "%02d" ${VALUE}
IN="?_include_linux_ltt-facilities.h
?_ltt_ltt-facilities.c"

for a in $IN; do wr $a $FILE; done

FILE=../${PRENAME}${COUNT}${NAME}-facility-core-headers.diff
IN="?_include_ltt_ltt-facility-core.h
?_include_ltt_ltt-facility-id-core.h
?_include_ltt_ltt-facility-select-core.h"

FILE=../${PRENAME}${COUNT}${NAME}-facility-loader-core.diff
IN="?_ltt_facilities_ltt-facility-loader-core.c
?_ltt_facilities_ltt-facility-loader-core.h
?_ltt_facilities_Makefile"
?_include_asm-x86_64_unistd.h"
IN="?_arch_ppc_kernel_misc.S