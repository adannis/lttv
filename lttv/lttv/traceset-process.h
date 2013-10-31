/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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
 * MA 02110-1301, USA.
 */

#ifndef PROCESSTRACE_H
#define PROCESSTRACE_H

#include <string.h>
#include <lttv/traceset.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>

//typedef void LttvFilter;	/* TODO (MD) */
typedef struct _LttvFilter LttvFilter;

/* Run through the events in a traceset in sorted order calling all the
	 hooks appropriately. It starts at the current time and runs until end or
	 nb_events are processed. */

void lttv_process_traceset(LttvTraceset *traceset, LttTime end, 
		unsigned nb_events);

/* Process traceset can also be done in smaller pieces calling begin,
 * then seek and middle repeatedly, and end. The middle function return the
 * number of events processed. It will be smaller than nb_events if the end time
 * or end position is reached. */


void lttv_process_traceset_begin(LttvTraceset *traceset,
		LttvHooks *before_traceset,
		LttvHooks *before_trace,
		LttvHooks *event);

guint lttv_process_traceset_middle(LttvTraceset *traceset,
		LttTime end,
		gulong nb_events,
		const LttvTracesetPosition *end_position);

void lttv_process_traceset_end(LttvTraceset *traceset,
		LttvHooks *after_traceset,
		LttvHooks *after_trace,
		LttvHooks *event);

guint lttv_process_traceset_update(LttvTraceset *traceset);


void lttv_process_traceset_seek_time(LttvTraceset *traceset, LttTime start);

void lttv_traceset_compute_time_span(LttvTraceset *traceset,
		TimeInterval *time_span);
#ifdef BABEL_CLEANUP
gboolean lttv_process_traceset_seek_position(LttvTraceset *traceset, 
		const LttvTracesetPosition *pos);
#endif /*babel_cleanup*/
void lttv_process_trace_seek_time(LttvTrace *trace, LttTime start);

void lttv_traceset_add_hooks(LttvTraceset *traceset,
			     LttvHooks *before_traceset,
			     LttvHooks *before_trace,
			     LttvHooks *event);

void lttv_traceset_remove_hooks(LttvTraceset *traceset,
				LttvHooks *after_traceset,
				LttvHooks *after_trace,
				LttvHooks *event);

void lttv_trace_add_hooks(LttvTrace *trace,
			  LttvHooks *before_trace,
			  LttvHooks *event);

void lttv_trace_remove_hooks(LttvTrace *trace,
			     LttvHooks *after_trace,
			     LttvHooks *event);

LttvTracesetPosition *
lttv_traceset_position_new(const LttvTraceset *traceset);

void lttv_traceset_position_save(const LttvTraceset *traceset,
		LttvTracesetPosition *pos);

void lttv_traceset_position_destroy(LttvTracesetPosition *pos);

void lttv_traceset_position_copy(LttvTracesetPosition *dest,
		const LttvTracesetPosition *src);

gint
lttv_traceset_pos_pos_compare(const LttvTracesetPosition *pos1,
		const LttvTracesetPosition *pos2);

gint lttv_traceset_ts_pos_compare(const LttvTraceset *traceset,
		const LttvTracesetPosition *pos2);
#ifdef BABEL_CLEANUP/*Already in traceset.h*/
LttTime
lttv_traceset_position_get_time(const LttvTracesetPosition *pos);
#endif //babel_cleanup
/* Seek n events forward and backward (without filtering) : only use these where
 * necessary : the seek backward is costy. */

#define BACKWARD_SEEK_MUL 2 /* Multiplication factor of time_offset between
                               backward seek iterations */
#ifdef BABEL_CLEANUP
static const gdouble seek_back_default_offset = 10;
#endif //babel_cleanup
static const gdouble SEEK_BACK_DEFAULT_RATIO = 20;

typedef gboolean check_handler(guint count, gboolean *stop_flag, gpointer data);

guint lttv_process_traceset_seek_n_forward(LttvTraceset *traceset,
		guint n,
		check_handler *check,
		gboolean *stop_flag,
		LttvFilter *filter1,
		LttvFilter *filter2,
		LttvFilter *filter3,
		gpointer data);

typedef void (*seek_time_fct)(LttvTraceset *traceset, LttTime start);

/* If first_offset is ltt_time_zero, it will choose a default value */


guint lttv_process_traceset_seek_n_backward(LttvTraceset *self,
                guint n,
                gdouble ratio,/*nanosecond/event*/
                check_handler *check,
                gboolean *stop_flag,
                LttvFilter *filter1,
                LttvFilter *filter2,
                LttvFilter *filter3,
                gpointer data);

#endif // PROCESSTRACE_H
