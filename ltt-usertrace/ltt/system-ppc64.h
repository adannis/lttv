#ifndef __PPC64_SYSTEM_H
#define __PPC64_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//#include <linux/config.h>
//#include <linux/compiler.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/hw_irq.h>
#include <asm/memory.h>

/*
 * Memory barrier.
 * The sync instruction guarantees that all memory accesses initiated
 * by this processor have been performed (with respect to all other
 * mechanisms that access memory).  The eieio instruction is a barrier
 * providing an ordering (separately) for (a) cacheable stores and (b)
 * loads and stores to non-cacheable memory (e.g. I/O devices).
 *
 * mb() prevents loads and stores being reordered across this point.
 * rmb() prevents loads being reordered across this point.
 * wmb() prevents stores being reordered across this point.
 * read_barrier_depends() prevents data-dependent loads being reordered
 *	across this point (nop on PPC).
 *
 * We have to use the sync instructions for mb(), since lwsync doesn't
 * order loads with respect to previous stores.  Lwsync is fine for
 * rmb(), though.
 * For wmb(), we use sync since wmb is used in drivers to order
 * stores to system memory with respect to writes to the device.
 * However, smp_wmb() can be a lighter-weight eieio barrier on
 * SMP since it is only used to order updates to system memory.
 */
#define mb()   __asm__ __volatile__ ("sync" : : : "memory")
#define rmb()  __asm__ __volatile__ ("lwsync" : : : "memory")
#define wmb()  __asm__ __volatile__ ("sync" : : : "memory")
#define read_barrier_depends()  do { } while(0)

#define set_mb(var, value)	do { var = value; smp_mb(); } while (0)
#define set_wmb(var, value)	do { var = value; smp_wmb(); } while (0)

#ifdef CONFIG_SMP
#define smp_mb()	mb()
#define smp_rmb()	rmb()
#define smp_wmb()	__asm__ __volatile__ ("eieio" : : : "memory")
#define smp_read_barrier_depends()  read_barrier_depends()
#else
#define smp_mb()	__asm__ __volatile__("": : :"memory")
#define smp_rmb()	__asm__ __volatile__("": : :"memory")
#define smp_wmb()	__asm__ __volatile__("": : :"memory")
#define smp_read_barrier_depends()  do { } while(0)
#endif /* CONFIG_SMP */

/*
 * Atomic exchange
 *
 * Changes the memory location '*ptr' to be val and returns
 * the previous value stored there.
 *
 * Inline asm pulled from arch/ppc/kernel/misc.S so ppc64
 * is more like most of the other architectures.
 */
static inline unsigned long
__xchg_u32(volatile int *m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
	EIEIO_ON_SMP
"1:	lwarx %0,0,%3		# __xchg_u32\n\
	stwcx. %2,0,%3\n\
2:	bne- 1b"
	ISYNC_ON_SMP
 	: "=&r" (dummy), "=m" (*m)
	: "r" (val), "r" (m)
	: "cc", "memory");

	return (dummy);
}

static inline unsigned long
__xchg_u64(volatile long *m, unsigned long val)
{
	unsigned long dummy;

	__asm__ __volatile__(
	EIEIO_ON_SMP
"1:	ldarx %0,0,%3		# __xchg_u64\n\
	stdcx. %2,0,%3\n\
2:	bne- 1b"
	ISYNC_ON_SMP
	: "=&r" (dummy), "=m" (*m)
	: "r" (val), "r" (m)
	: "cc", "memory");

	return (dummy);
}

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 */
extern void __xchg_called_with_bad_pointer(void);

static inline unsigned long
__xchg(volatile void *ptr, unsigned long x, int size)
{
	switch (size) {
	case 4:
		return __xchg_u32(ptr, x);
	case 8:
		return __xchg_u64(ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;
}

#define xchg(ptr,x)							     \
  ({									     \
     __typeof__(*(ptr)) _x_ = (x);					     \
     (__typeof__(*(ptr))) __xchg((ptr), (unsigned long)_x_, sizeof(*(ptr))); \
  })

#define tas(ptr) (xchg((ptr),1))

#define __HAVE_ARCH_CMPXCHG	1

static inline unsigned long
__cmpxchg_u32(volatile int *p, int old, int new)
{
	unsigned int prev;

	__asm__ __volatile__ (
	EIEIO_ON_SMP
"1:	lwarx	%0,0,%2		# __cmpxchg_u32\n\
	cmpw	0,%0,%3\n\
	bne-	2f\n\
	stwcx.	%4,0,%2\n\
	bne-	1b"
	ISYNC_ON_SMP
	"\n\
2:"
	: "=&r" (prev), "=m" (*p)
	: "r" (p), "r" (old), "r" (new), "m" (*p)
	: "cc", "memory");

	return prev;
}

static inline unsigned long
__cmpxchg_u64(volatile long *p, unsigned long old, unsigned long new)
{
	unsigned long prev;

	__asm__ __volatile__ (
	EIEIO_ON_SMP
"1:	ldarx	%0,0,%2		# __cmpxchg_u64\n\
	cmpd	0,%0,%3\n\
	bne-	2f\n\
	stdcx.	%4,0,%2\n\
	bne-	1b"
	ISYNC_ON_SMP
	"\n\
2:"
	: "=&r" (prev), "=m" (*p)
	: "r" (p), "r" (old), "r" (new), "m" (*p)
	: "cc", "memory");

	return prev;
}

/* This function doesn't exist, so you'll get a linker error
   if something tries to do an invalid cmpxchg().  */
extern void __cmpxchg_called_with_bad_pointer(void);

static inline unsigned long
__cmpxchg(volatile void *ptr, unsigned long old, unsigned long new, int size)
{
	switch (size) {
	case 4:
		return __cmpxchg_u32(ptr, old, new);
	case 8:
		return __cmpxchg_u64(ptr, old, new);
	}
	__cmpxchg_called_with_bad_pointer();
	return old;
}

#define cmpxchg(ptr,o,n)						 \
  ({									 \
     __typeof__(*(ptr)) _o_ = (o);					 \
     __typeof__(*(ptr)) _n_ = (n);					 \
     (__typeof__(*(ptr))) __cmpxchg((ptr), (unsigned long)_o_,		 \
				    (unsigned long)_n_, sizeof(*(ptr))); \
  })

/*
 * We handle most unaligned accesses in hardware. On the other hand 
 * unaligned DMA can be very expensive on some ppc64 IO chips (it does
 * powers of 2 writes until it reaches sufficient alignment).
 *
 * Based on this we disable the IP header alignment in network drivers.
 */
#define NET_IP_ALIGN   0

#define arch_align_stack(x) (x)

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif
