/* This file is part of the Linux Trace Toolkit trace reading library
 * Copyright (C) 2003-2004 Mathieu Desnoyers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA
 */

#ifndef COMPILER_H
#define COMPILER_H

/* Fast prediction if likely branches */
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
 

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({  type __dummy; \
  typeof(x) __dummy2; \
  (void)(&__dummy == &__dummy2); \
  1; \
})

/* Deal with 32 wrap correctly */
#define guint32_after(a,b) \
  (typecheck(guint32, a) && \
   typecheck(guint32, b) && \
   ((gint32)(b) - (gint32)(a) < 0))
#define guint32_before(a,b)  guint32_after(b,a)

#define guint32_after_eq(a,b) \
  (typecheck(guint32, a) && \
   typecheck(guint32, b) && \
   ((gint32)(b) - (gint32)(a) <= 0))
#define guint32_before_eq(a,b)  guint32_after_eq(b,a)

#define __EXPORT __attribute__ ((visibility ("default")))

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#endif //COMPILER_H
