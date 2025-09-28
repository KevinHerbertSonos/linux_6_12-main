#include <linux/linkage.h>
#inlucde <asm/assembler.h>

		.text
		.align 5
ENTRY(process_row_4x4)
		/* r0: input pointer, r1: output pointer */
		/* r2: width, r3: height */
		push		{r8-r12}

		lsl		r2,  r2, #2 /* r2:   width * 4 */
		lsl		r3,  r3, #2 /* r3:  height * 4 */

		mov		r8,  r0
		add		r9,  r8,  r2 /* width * 4 */
		add		r10, r9,  r2
		add		r11, r10, r2
		add		r0,  r11,  r2

		lsr		r12, r2, #4 /* r12: width * 4 / 16 */
1:		vld1.32		{q0}, [r8]!
		vld1.32		{q1}, [r9]!
		vzip.32		q0, q1
		vld1.32		{q2}, [r10]!
		vld1.32		{q3}, [r11]!
		vzip.32		q2, q3

		vswp		d1, d4
		vswp		d3, d6

		vrev64.32	q0, q0
		vrev64.32	q1, q1
		vrev64.32	q2, q2
		vrev64.32	q3, q3

		vswp		d0, d1
		vst1.32		{q0}, [r1], r3 /* height * 4 */
		vswp		d2, d3
		vswp		d4, d5
		vst1.32		{q2}, [r1], r3
		vst1.32		{q1}, [r1], r3
		vswp		d6, d7
		vst1.32		{q3}, [r1], r3

		subs		r12, r12, #1
		bne		1b

		pop		{r8-r12}
		bx lr
ENDPROC(process_row_4x4)

		.align 5
ENTRY(process_row_4x4_anti)
		/* r0: input pointer, r1: output pointer */
		/* r2: width, r3: height */
		push		{r8-r12}

		lsl		r2,  r2, #2 /* r2:   width * 4 */
		lsl		r3,  r3, #2 /* r3:  height * 4 */
		neg		r3,  r3

		mov		r8,  r0
		add		r9,  r8,  r2 /* width * 4 */
		add		r10, r9,  r2
		add		r11, r10, r2
		add		r0,  r11, r2

		lsr		r12, r2, #4 /* r12: width * 4 / 16 */
1:		vld1.32		{q0}, [r8]!
		vld1.32		{q1}, [r9]!
		vzip.32		q0, q1
		vld1.32		{q2}, [r10]!
		vld1.32		{q3}, [r11]!
		vzip.32		q2, q3

		vswp		d1, d4
		vswp		d3, d6

		vst1.32		{q0}, [r1], r3 /* height * 4 */
		vst1.32		{q2}, [r1], r3
		vst1.32		{q1}, [r1], r3
		vst1.32		{q3}, [r1], r3

		subs		r12, r12, #1
		bne		1b

		pop		{r8-r12}
		bx lr
ENDPROC(process_row_4x4_anti)
