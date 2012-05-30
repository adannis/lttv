/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2012 Yannick Brosseau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */
#ifndef LTTV_TRACE_H
#define LTTV_TRACE_H

#include <lttv/traceset.h>
typedef struct _LttvTrace LttvTrace;
//typedef struct _LttvTraceset LttvTraceset;

LttvTraceset *lttv_trace_get_traceset(LttvTrace *trace);

void lttv_trace_destroy(LttvTrace *t);

guint lttv_trace_get_ref_number(LttvTrace * t);

guint lttv_trace_ref(LttvTrace * t);

guint lttv_trace_unref(LttvTrace * t);

guint lttv_trace_get_num_cpu(LttvTrace *t);

/* An attributes table is attached to the set and to each trace in the set. */

LttvAttribute *lttv_trace_attribute(LttvTrace *t);

#endif //LTTV_TRACE_H
