dnl slinie(aux_ptr,aux_ptr_ub,sieve_interval)
dnl We save %ebx,%esi,%edi,%ebp and also have one auto variable and the
dnl return address on the stack. Therefore, the stack offset of the
dnl first arg is 24.
define(aux_ptr_arg,24(%esp))dnl
define(aux_ptr_ub,28(%esp))dnl
define(sieve_interval,32(%esp))dnl
dnl Now, the registers which we are going to use
define(sieve_ptr,%edi)dnl
define(sieve_ptr_ub,%ebp)dnl
define(root,%edx)dnl
define(prime,%ecx)dnl
define(sieve_log,%ah)dnl
define(sv0,%al)dnl
define(sv1,%bh)dnl
dnl The bx-register may also be used for auxilliary 32-bit values if sv1
dnl is not used
define(auxreg,%ebx)dnl
define(aux_ptr,%esi)dnl
dnl Offset of the various things from this pointer
define(prime_src,(%esi))dnl
define(proot_src,2(%esi))dnl
define(log_src,4(%esi))dnl
define(root_src,6(%esi))dnl
dnl We store the int difference projective_root-prime here:
define(proot,(%esp))dnl
dnl This macro is taken from the GNU info documentation of m4.
define(`forloop',
       `pushdef(`$1', `$2')_forloop(`$1', `$2', `$3', `$4')popdef(`$1')')dnl
define(`_forloop',
       `$4`'ifelse($1, `$3', ,
     		 `define(`$1', incr($1))_forloop(`$1', `$2', `$3', `$4')')')dnl
function_head(slinie)
	pushl %ebx
	pushl %esi
	pushl %edi
	pushl %ebp
	subl $4,%esp
	movl aux_ptr_arg,aux_ptr
	movl $-8,auxreg
	cmpl aux_ptr,aux_ptr_ub
	jbe slinie_ende
	addl auxreg,aux_ptr_ub
slinie_fbi_loop:
	movzwl proot_src,auxreg
	movzwl prime_src,prime
	movzwl root_src,root
	subl prime,auxreg
	movb log_src,sieve_log
	movl auxreg,proot	
	movl sieve_interval,sieve_ptr_ub
forloop(`i',1,j_per_strip,`
	movl root,sieve_ptr
	xorl auxreg,auxreg
	addl proot,root
	leal (sieve_ptr_ub,sieve_ptr),sieve_ptr
	cmovncl prime,auxreg
	add $n_i,sieve_ptr_ub
	add auxreg,root
	leal (prime,prime,4),auxreg
	subl auxreg,sieve_ptr_ub
slinie_loop`'i:
	addb sieve_log,(sieve_ptr,prime)
	addb sieve_log,(sieve_ptr)
	leal (sieve_ptr,prime,2),sieve_ptr
	addb sieve_log,(sieve_ptr,prime)
	addb sieve_log,(sieve_ptr)
	cmpl sieve_ptr,sieve_ptr_ub
	leal (sieve_ptr,prime,2),sieve_ptr
	ja slinie_loop`'i
	leal (sieve_ptr_ub,prime,4),sieve_ptr_ub
	cmpl sieve_ptr,sieve_ptr_ub
	jbe slinie_last_se`'i
	addb sieve_log,(sieve_ptr)
	addb sieve_log,(sieve_ptr,prime)
	leal (sieve_ptr,prime,2),sieve_ptr
slinie_last_se`'i:
	leal (sieve_ptr_ub,prime),sieve_ptr_ub
	cmpl sieve_ptr,sieve_ptr_ub
	jbe slinie_next_j`'i
	addb sieve_log,(sieve_ptr)
slinie_next_j`'i:
')
	cmpl aux_ptr,aux_ptr_ub
#	movl root,root_src
	leal 8(aux_ptr),aux_ptr
	ja slinie_fbi_loop
slinie_ende:
	addl $4,%esp
	popl %ebp
	popl %edi
	popl %esi
	popl %ebx
	ret
