	.file	"nr-compose-transform.c"
	.text
	.align 2
.globl nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_0
	.type	nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_0,@function
nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_0:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$48, %esp
	movl	8(%ebp), %eax
	movl	%eax, -8(%ebp)
	movl	40(%ebp), %eax
	addl	$16, %eax
	movl	(%eax), %eax
	movl	%eax, -12(%ebp)
	movl	40(%ebp), %eax
	addl	$20, %eax
	movl	(%eax), %eax
	movl	%eax, -16(%ebp)
	movl	$0, -24(%ebp)
.L29:
	movl	-24(%ebp), %eax
	cmpl	16(%ebp), %eax
	jl	.L32
	jmp	.L28
.L32:
	movl	-8(%ebp), %eax
	movl	%eax, -28(%ebp)
	movl	-12(%ebp), %eax
	movl	%eax, -32(%ebp)
	movl	-16(%ebp), %eax
	movl	%eax, -36(%ebp)
	movl	$0, -20(%ebp)
.L33:
	movl	-20(%ebp), %eax
	cmpl	12(%ebp), %eax
	jl	.L36
	jmp	.L34
.L36:
	movl	-32(%ebp), %eax
	sarl	$12, %eax
	movl	%eax, -40(%ebp)
	cmpl	$0, -40(%ebp)
	js	.L37
	movl	-40(%ebp), %eax
	cmpl	28(%ebp), %eax
	jge	.L37
	movl	-36(%ebp), %eax
	sarl	$12, %eax
	movl	%eax, -44(%ebp)
	cmpl	$0, -44(%ebp)
	js	.L37
	movl	-44(%ebp), %eax
	cmpl	32(%ebp), %eax
	jge	.L37
	movl	-44(%ebp), %eax
	movl	%eax, %edx
	imull	36(%ebp), %edx
	movl	-40(%ebp), %eax
	sall	$2, %eax
	leal	(%eax,%edx), %eax
	addl	24(%ebp), %eax
	movl	%eax, -48(%ebp)
	movl	-48(%ebp), %eax
	addl	$3, %eax
	movzbl	(%eax), %eax
	imull	44(%ebp), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movl	%eax, -52(%ebp)
	cmpl	$0, -52(%ebp)
	je	.L37
	cmpl	$255, -52(%ebp)
	je	.L41
	movl	-28(%ebp), %eax
	addl	$3, %eax
	cmpb	$0, (%eax)
	jne	.L40
.L41:
	movl	-28(%ebp), %ecx
	movl	-48(%ebp), %eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %ecx
	incl	%ecx
	movl	-48(%ebp), %eax
	incl	%eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %ecx
	addl	$2, %ecx
	movl	-48(%ebp), %eax
	addl	$2, %eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %edx
	addl	$3, %edx
	movzbl	-52(%ebp), %eax
	movb	%al, (%edx)
	jmp	.L37
.L40:
	movl	-28(%ebp), %ecx
	movl	$255, %eax
	movl	%eax, %edx
	subl	-52(%ebp), %edx
	movl	-28(%ebp), %eax
	movzbl	(%eax), %eax
	imull	%eax, %edx
	movl	-48(%ebp), %eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	(%eax,%edx), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %ecx
	incl	%ecx
	movl	$255, %eax
	movl	%eax, %edx
	subl	-52(%ebp), %edx
	movl	-28(%ebp), %eax
	incl	%eax
	movzbl	(%eax), %eax
	imull	%eax, %edx
	movl	-48(%ebp), %eax
	incl	%eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	(%eax,%edx), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %ecx
	addl	$2, %ecx
	movl	$255, %eax
	movl	%eax, %edx
	subl	-52(%ebp), %edx
	movl	-28(%ebp), %eax
	addl	$2, %eax
	movzbl	(%eax), %eax
	imull	%eax, %edx
	movl	-48(%ebp), %eax
	addl	$2, %eax
	movzbl	(%eax), %eax
	imull	-52(%ebp), %eax
	leal	(%eax,%edx), %eax
	leal	127(%eax), %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ecx)
	movl	-28(%ebp), %ebx
	addl	$3, %ebx
	movl	$255, %eax
	movl	%eax, %ecx
	subl	-52(%ebp), %ecx
	movl	-28(%ebp), %eax
	addl	$3, %eax
	movzbl	(%eax), %edx
	movl	$255, %eax
	subl	%edx, %eax
	movl	%ecx, %edx
	imull	%eax, %edx
	movl	$65152, %eax
	subl	%edx, %eax
	movl	%eax, %edx
	movl	$-2139062143, %eax
	mull	%edx
	movl	%edx, %eax
	shrl	$7, %eax
	movb	%al, (%ebx)
.L37:
	movl	40(%ebp), %eax
	movl	(%eax), %edx
	leal	-32(%ebp), %eax
	addl	%edx, (%eax)
	movl	40(%ebp), %eax
	addl	$4, %eax
	movl	(%eax), %edx
	leal	-36(%ebp), %eax
	addl	%edx, (%eax)
	leal	-28(%ebp), %eax
	addl	$4, (%eax)
	leal	-20(%ebp), %eax
	incl	(%eax)
	jmp	.L33
.L34:
	movl	40(%ebp), %eax
	addl	$8, %eax
	movl	(%eax), %edx
	leal	-12(%ebp), %eax
	addl	%edx, (%eax)
	movl	40(%ebp), %eax
	addl	$12, %eax
	movl	(%eax), %edx
	leal	-16(%ebp), %eax
	addl	%edx, (%eax)
	movl	20(%ebp), %edx
	leal	-8(%ebp), %eax
	addl	%edx, (%eax)
	leal	-24(%ebp), %eax
	incl	(%eax)
	jmp	.L29
.L28:
	addl	$48, %esp
	popl	%ebx
	popl	%ebp
	ret
.Lfe2:
	.size	nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_0,.Lfe2-nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_0

/*
 *
 *	dbits	52(%ebp)
 *	alpha	48(%ebp)
 *	FF_S	44(%ebp)
 *
 *	d	-32(%ebp) -> %edi
 *	i	-60(%ebp) -> %esi
 *	sx	-64(%ebp) -> %ebx
 *	sy	-68(%ebp) -> %ecx
 *	s	-72(%ebp)
 *
 *	%mm0	a a a a
 *	%mm1	FgA
 *	%mm2	SumFgA
 *	%mm3	a a a 255
 *	%mm4
*/

	.align 2
.globl nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_n
	.type	nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_n,@function
nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_n:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$72, %esp
	pushl	%edi
	pushl	%esi

/* Load %mm7 with [0 0 0 0] */
	movl	$0, %eax
	movd	%eax, %mm7

/* Load %mm6 with [128 128 128 128] */
	movl	$0x80808080, %eax
	movd	%eax, %mm6
	punpcklbw %mm7, %mm6

/* Load %mm5 with [255 255 255 255] */
	movl	$0xffffffff, %eax
	movd	%eax, %mm5
	punpcklbw %mm7, %mm5

/* Load %mm0 with [a a a a] */
/* Check full opacity */
	movzbl	48(%ebp), %eax
	movd	%eax, %mm0
	punpcklwd %mm0, %mm0
	punpckldq %mm0, %mm0

	movl	$1, %eax
	movzbl	52(%ebp), %ecx
	sall	%cl, %eax
	movl	%eax, -8(%ebp)
	movl	8(%ebp), %eax
	movl	%eax, -12(%ebp)
	movl	40(%ebp), %eax
	addl	$16, %eax
	movl	(%eax), %eax
	movl	%eax, -16(%ebp)
	movl	40(%ebp), %eax
	addl	$20, %eax
	movl	(%eax), %eax
	movl	%eax, -20(%ebp)
	movl	$0, -28(%ebp)
.L44:
	movl	-28(%ebp), %eax
	cmpl	16(%ebp), %eax
	jl	.L47
	jmp	.exit_n
.L47:
	movl	-12(%ebp), %eax
	movl	%eax, -32(%ebp)
	movl	-16(%ebp), %eax
	movl	%eax, -36(%ebp)
	movl	-20(%ebp), %eax
	movl	%eax, -40(%ebp)
	movl	$0, -24(%ebp)
.L48:
	movl	-24(%ebp), %eax
	cmpl	12(%ebp), %eax
	jl	.L51
	jmp	.L49
.L51:

/* Zero accumulator */
	movq	%mm7, %mm2

	movl	-8(%ebp), %esi
	sub	$1, %esi
	shll	$3, %esi
.for_i_n:
	movl	%esi, %edx
	movl	44(%ebp), %eax

	movl	(%eax,%edx), %ebx
	addl	-36(%ebp), %ebx
	sarl	$12, %ebx
	cmpl	$0, %ebx
	js	.next_i_n
	cmpl	28(%ebp), %ebx
	jge	.next_i_n
/* We multiply sx by 4 here */
	shll	$2, %ebx

	movl	%esi, %edx
	addl	$4, %edx
	movl	44(%ebp), %eax

	movl	(%eax,%edx), %ecx
	addl	-40(%ebp), %ecx
	sarl	$12, %ecx
	cmpl	$0, %ecx
	js	.next_i_n
	cmpl	32(%ebp), %ecx
	jge	.next_i_n
/* We multiply sy by srs here */
	imull	36(%ebp), %ecx

	movl	%ecx, %edx
	leal	(%ebx,%edx), %eax
	addl	24(%ebp), %eax

/* Fg -> %mm1 */
	movl	(%eax), %eax
	testl	$0xff000000, %eax
	jz	.next_i_n
	movd	%eax, %mm1
	punpcklbw %mm7, %mm1

/* [a a a 255] -> %mm3 */
	shrl	$24, %eax
	movl	$0x1010101, %edx
	mull	%edx
	orl	$0xff000000, %eax
	movd	%eax, %mm3
	punpcklbw %mm7, %mm3

/* [Fg * a] -> mm1 */
	pmullw	%mm3, %mm1
	paddw	%mm6, %mm1
	movq	%mm1, %mm4
	psrlw	$8, %mm4
	paddw	%mm4, %mm1
	psrlw	$8, %mm1

/* Add to accumulator */
	paddw	%mm1, %mm2

.next_i_n:
	subl	$8, %esi
	jnb	.for_i_n

/* Divide components by sample size */
	movd	52(%ebp), %mm3
	psrlw	%mm3, %mm2

/* Multiply by alpha */
	pmullw	%mm0, %mm2
	paddw	%mm6, %mm2
	movq	%mm2, %mm4
	psrlw	$8, %mm4
	paddw	%mm4, %mm2
	psrlw	$8, %mm2

	movl	-32(%ebp), %edi

/* [255 - FgA] -> mm1 */
	movq	%mm2, %mm1
	punpckhwd %mm1, %mm1
	punpckhdq %mm1, %mm1
	pxor	%mm5, %mm1

/* Bg -> mm3 */
	movd	(%edi), %mm3
	punpcklbw %mm7, %mm3

/* Fg + ((255 - FgA) * Bg) / 255 */

	pmullw	%mm1, %mm3
	paddw	%mm6, %mm3
	movq	%mm3, %mm4
	psrlw	$8, %mm4
	paddw	%mm4, %mm3
	psrlw	$8, %mm3
	paddw	%mm2, %mm3

/* Store pixel */
	packuswb %mm3, %mm3
	movd	%mm3, (%edi)

.L58:
	movl	40(%ebp), %eax
	movl	(%eax), %edx
	leal	-36(%ebp), %eax
	addl	%edx, (%eax)
	movl	40(%ebp), %eax
	addl	$4, %eax
	movl	(%eax), %edx
	leal	-40(%ebp), %eax
	addl	%edx, (%eax)
	leal	-32(%ebp), %eax
	addl	$4, (%eax)
	leal	-24(%ebp), %eax
	incl	(%eax)
	jmp	.L48
.L49:
	movl	40(%ebp), %eax
	addl	$8, %eax
	movl	(%eax), %edx
	leal	-16(%ebp), %eax
	addl	%edx, (%eax)
	movl	40(%ebp), %eax
	addl	$12, %eax
	movl	(%eax), %edx
	leal	-20(%ebp), %eax
	addl	%edx, (%eax)
	movl	20(%ebp), %edx
	leal	-12(%ebp), %eax
	addl	%edx, (%eax)
	leal	-28(%ebp), %eax
	incl	(%eax)
	jmp	.L44

.exit_n:
	emms
	popl	%esi
	popl	%edi
	addl	$72, %esp
	popl	%ebx
	popl	%ebp
	ret
.Lfe3:
	.size	nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_n,.Lfe3-nr_mmx_R8G8B8A8_P_R8G8B8A8_P_R8G8B8A8_N_TRANSFORM_n
	.ident	"GCC: (GNU) 3.2"
