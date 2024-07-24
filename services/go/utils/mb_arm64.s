#include "textflag.h"

TEXT ·MemoryBarrier(SB),NOSPLIT|NOFRAME|TOPFRAME,$0-0
	DMB	$0xe	// DMB ST
	RET
