/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lttv/traceset-process.h>
#include <lttv/traceset.h>
#include <lttv/event.h>
#include <babeltrace/context.h>
#include <babeltrace/iterator.h>

#include <babeltrace/ctf/events.h>
#include <babeltrace/ctf/iterator.h>

void lttv_process_traceset_begin(LttvTraceset *traceset,
		LttvHooks *before_traceset,
		LttvHooks *before_trace,
		LttvHooks *event)
{

	/* simply add hooks in context. _before hooks are called by add_hooks. */
	/* It calls all before_traceset, before_trace, and before_tracefile hooks. */
	lttv_traceset_add_hooks(traceset,
				before_traceset,
				before_trace,
				event);
	

}

guint lttv_process_traceset_middle(LttvTraceset *traceset,
		LttTime end,
		gulong nb_events,
		const LttvTracesetPosition *end_position)
{
	
	unsigned count = 0;
		
	struct bt_ctf_event *bt_event;
	
	LttvEvent event;
        struct bt_iter_pos begin_pos;

	begin_pos.type = BT_SEEK_BEGIN;

	if(!traceset->iter) {
        traceset->iter = bt_ctf_iter_create(lttv_traceset_get_context(traceset),
                                        &begin_pos,
                                        NULL);
	}
	while(TRUE) {

		if((count >= nb_events) && (nb_events != G_MAXULONG)) {
			break;
		}

		if((bt_event = bt_ctf_iter_read_event(traceset->iter)) != NULL) {
			
			count++;

			event.bt_event = bt_event;
			/* TODO ybrosseau 2012-04-01: use bt_ctf_get_trace_handle 
			   to retrieve the right state container */
			event.state = traceset->tmpState;
			
			lttv_hooks_call(traceset->event_hooks, &event);

			if(bt_iter_next(bt_ctf_get_iter(traceset->iter)) < 0) {
				printf("ERROR NEXT\n");
				break;
			}
		} else {
			/* READ FAILED */
			
			break;
		
		}
	}
	


	return count;
}

void lttv_process_traceset_end(LttvTraceset *traceset,
			       LttvHooks *after_traceset,
			       LttvHooks *after_trace,	
			       LttvHooks *event)
{
	/* Remove hooks from context. _after hooks are called by remove_hooks. */
	/* It calls all after_traceset, after_trace, and after_tracefile hooks. */
	lttv_traceset_remove_hooks(traceset,
				   after_traceset,
				   after_trace,
				   event);

}


void lttv_traceset_add_hooks(LttvTraceset *traceset,
			     LttvHooks *before_traceset,
			     LttvHooks *before_trace,
			     LttvHooks *event)
{
	
	guint i, nb_trace;

	LttvTrace *trace;

	lttv_hooks_call(before_traceset, traceset);
	
	lttv_hooks_add_list(traceset->event_hooks, event);

	nb_trace = lttv_traceset_number(traceset);

	for(i = 0 ; i < nb_trace ; i++) {
		trace = (LttvTrace *)g_ptr_array_index(traceset->traces,i);
		lttv_trace_add_hooks(trace,
					     before_trace,
					     event
					     );
	}
}
void lttv_traceset_remove_hooks(LttvTraceset *traceset,
				LttvHooks *after_traceset,
				LttvHooks *after_trace,
				LttvHooks *event)
{

	guint i, nb_trace;

	LttvTrace *trace;

	nb_trace = lttv_traceset_number(traceset);

	for(i = 0 ; i < nb_trace ; i++) {
		trace = (LttvTrace *)g_ptr_array_index(traceset->traces,i);
		lttv_trace_remove_hooks(trace,
						after_trace,
						event);

	}

	lttv_hooks_call(after_traceset, traceset);


}


void lttv_trace_add_hooks(LttvTrace *trace,
			  LttvHooks *before_trace,
			  LttvHooks *event)
{
	lttv_hooks_call(before_trace, trace);
}

void lttv_trace_remove_hooks(LttvTrace *trace,
			     LttvHooks *after_trace,
			     LttvHooks *event)

{
	lttv_hooks_call(after_trace, trace);

}

void lttv_process_traceset_seek_time(LttvTraceset *traceset, LttTime start)
{
#ifdef WAIT_FOR_BABELTRACE_FIX_SEEK_ZERO
        struct bt_iter_pos seekpos;
        int ret;
        seekpos.type = BT_SEEK_TIME;
        seekpos.u.seek_time = ltt_time_to_uint64(start);
        ret = bt_iter_set_pos(bt_ctf_get_iter(self->iter), &seekpos);
        if(ret < 0) {
                printf("Seek by time error: %s,\n",strerror(-ret));
        }
#else
#warning Seek time disabled because of babeltrace bugs
#endif
 
}
