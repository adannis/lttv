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
#include <babeltrace/trace-handle.h>
#include <babeltrace/ctf/events.h>
#include <babeltrace/ctf/iterator.h>

void lttv_process_traceset_begin(LttvTraceset *traceset,
		LttvHooks *before_traceset,
		LttvHooks *before_trace,
		LttvHooks *event)
{
	struct bt_iter_pos begin_pos;
	/* simply add hooks in context. _before hooks are called by add_hooks. */
	/* It calls all before_traceset, before_trace, and before_tracefile hooks. */
	lttv_traceset_add_hooks(traceset,
				before_traceset,
				before_trace,
				event);
	


	begin_pos.type = BT_SEEK_BEGIN;

	if(!traceset->iter) {
        traceset->iter = bt_ctf_iter_create(lttv_traceset_get_context(traceset),
                                        &begin_pos,
                                        NULL);
	}
}

guint lttv_process_traceset_middle(LttvTraceset *traceset,
					LttTime end,
					gulong nb_events,
					const LttvTracesetPosition *end_position)
{
	unsigned count = 0;
	gint last_ret = 0;
	LttvTracesetPosition *currentPos;
        
	struct bt_ctf_event *bt_event;
	
	LttvEvent event;
  
	while(TRUE) {

		if(last_ret == TRUE || ((count >= nb_events) && (nb_events != G_MAXULONG))) {
			break;
		}

		if((bt_event = bt_ctf_iter_read_event(traceset->iter)) != NULL) {

			LttTime time = ltt_time_from_uint64(bt_ctf_get_timestamp(bt_event));
			if(ltt_time_compare(end, time) <= 0) {
				break;
			}
			
			currentPos = lttv_traceset_create_current_position(traceset);
			if(lttv_traceset_position_compare(currentPos,end_position ) == 0){
				lttv_traceset_destroy_position(currentPos);
				break;
			}
			lttv_traceset_destroy_position(currentPos);
			count++;

			event.bt_event = bt_event;

			/* Retreive the associated state */
			event.state = g_ptr_array_index(traceset->state_trace_handle_index,
							bt_ctf_event_get_handle_id(bt_event));
			
			last_ret = lttv_hooks_call(traceset->event_hooks, &event);

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

	lttv_hooks_remove_list(traceset->event_hooks, event);
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
        struct bt_iter_pos seekpos;
        int ret;
	if (traceset->iter == NULL) {
		g_warning("Iterator not valid in seek_time");
		return;
	}
        seekpos.type = BT_SEEK_TIME;
        seekpos.u.seek_time = ltt_time_to_uint64(start);

        ret = bt_iter_set_pos(bt_ctf_get_iter(traceset->iter), &seekpos);
        if(ret < 0) {
                printf("Seek by time error: %s,\n",strerror(-ret));
        }
}

guint lttv_process_traceset_seek_n_forward(LttvTraceset *traceset,
                guint n,
                check_handler *check,
                gboolean *stop_flag,
                LttvFilter *filter1,
                LttvFilter *filter2,
                LttvFilter *filter3,
                gpointer data)
{
        unsigned count = 0;
        while(count < n) {
		if(bt_iter_next(bt_ctf_get_iter(traceset->iter)) < 0) {
			printf("ERROR NEXT\n");
			break;
		}
		count++;
        }
      return count;  
}

guint lttv_process_traceset_seek_n_backward(LttvTraceset *ts,
                guint n,
                gdouble ratio,
                check_handler *check,
                gboolean *stop_flag,
                LttvFilter *filter1,
                LttvFilter *filter2,
                LttvFilter *filter3,
                gpointer data)
{
        guint i, count, ret;
        gint extraEvent = 0;
        guint64 initialTimeStamp, previousTimeStamp;
        LttvTracesetPosition *initialPos, *previousPos, *currentPos, beginPos;
        struct bt_iter_pos pos;
        beginPos.bt_pos = &pos;
        beginPos.iter = ts->iter;
        beginPos.bt_pos->type = BT_SEEK_BEGIN;
        beginPos.timestamp = G_MAXUINT64;
        beginPos.cpu_id = INT_MAX;
        /*Save initial position of the traceset*/
        initialPos = lttv_traceset_create_current_position (ts);
        
        /*Get the timespan of the initial position*/
        initialTimeStamp = lttv_traceset_position_get_timestamp(initialPos);
        /* 
         * Create a position before the initial timestamp according
         * to the ratio of nanosecond/event hopefully before the
         * the desired seek position
         */
        while(1){
		previousTimeStamp = initialTimeStamp - n*(guint)ceil(ratio);

		previousPos = lttv_traceset_create_time_position(ts,ltt_time_from_uint64(previousTimeStamp));
		if(initialTimeStamp == previousTimeStamp)
			break;
                
                currentPos = lttv_traceset_create_time_position(ts,ltt_time_from_uint64(previousTimeStamp));
                /*Corner case: When we are near the beginning of the trace and the previousTimeStamp is before
                        * the beginning of the trace. We have to seek to the first event.
                        */
                if((lttv_traceset_position_compare(currentPos,&beginPos ) == 0)){
                                lttv_traceset_seek_to_position(&beginPos);
                                break;
                        }
                /*move traceset position */
                lttv_state_traceset_seek_position(ts, previousPos);
                /* iterate to the initial position counting the number of event*/
                count = 0;
                do {
                        if((ret = lttv_traceset_position_compare(currentPos,initialPos)) == 1){       
                                bt_iter_next(bt_ctf_get_iter(ts->iter));
                                lttv_traceset_destroy_position(currentPos);
                                currentPos = lttv_traceset_create_current_position(ts);
                                count++;
                        }
                }while(ret != 0);
                
                /*substract the desired number of event to the count*/
                extraEvent = count - n;
                if (extraEvent >= 0) {
                        //if the extraEvent is over 0 go back to previousPos and 
                        //move forward the value of extraEvent times
                        lttv_state_traceset_seek_position(ts, previousPos);
                        
                        for(i = 0 ; i < extraEvent ; i++){
                                if(bt_iter_next(bt_ctf_get_iter(ts->iter)) < 0){
                                        printf("ERROR NEXT\n");
                                        break;
                                }
                                
                        }
                        break; /* we successfully seeked backward */
                }
                else{ 
                        /* if the extraEvent is below 0 create a position before and start over*/  
			ratio = ratio * 16;
                }
                lttv_traceset_destroy_position(currentPos);
        }
        return 0;
}
