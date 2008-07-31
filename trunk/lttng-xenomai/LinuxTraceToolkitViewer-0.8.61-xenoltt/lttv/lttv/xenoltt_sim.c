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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <lttv/module.h>
#include <lttv/xenoltt_sim.h>
#include <lttv/stats.h>
#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <ltt/facility.h>
#include <ltt/trace.h>
#include <ltt/event.h>
#include <ltt/type.h>


/****************************************************************************************************************************/
gboolean save_event(void *hook_data, void *call_data);
/****************************************************************************************************************************/

gboolean sim_every_event(void *hook_data, void *call_data)
{
  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;

  LttEvent *e = ltt_tracefile_get_event(tfcs->parent.parent.tf);

  LttvAttributeValue v;

  /* The current branch corresponds to the tracefile/process/interrupt state.
     Statistics are added within it, to count the number of events of this
     type occuring in this context. A quark has been pre-allocated for each
     event type and is used as name. */

  lttv_attribute_find(tfcs->current_event_types_tree, 
      ltt_eventtype_name(ltt_event_eventtype(e)), 
      LTTV_UINT, &v);
  (*(v.v_uint))++;
  return FALSE;
}

// Hook wrapper. call_data is a traceset context.
gboolean lttv_xenoltt_sim_hook_add_event_hooks(void *hook_data, void *call_data)
{
  LttvTracesetStats *tss = (LttvTracesetStats*)call_data;

  lttv_xenoltt_sim_add_event_hooks(tss);

  return 0;
}

void lttv_xenoltt_sim_add_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  GArray *hooks, *before_hooks;

  LttvTraceHook *hook;

  LttvTraceHookByFacility *thf;

  LttvAttributeValue val;

  gint ret;
	gint hn;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceStats *)self->parent.parent.traces[i];

    /* Find the eventtype id for the following events and register the
       associated by id hooks. */

    hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 16);
    g_array_set_size(hooks, 16);
    hn=0;
    /*
    LTT_EVENT_XENOLTT_THREAD_INIT,
    LTT_EVENT_XENOLTT_THREAD_SET_PERIOD,
    LTT_EVENT_XENOLTT_THREAD_WAIT_PERIOD,
    LTT_EVENT_XENOLTT_THREAD_MISSED_PERIOD,
    LTT_EVENT_XENOLTT_THREAD_SUSPEND,
    LTT_EVENT_XENOLTT_THREAD_START,
    LTT_EVENT_XENOLTT_THREAD_RESUME,
    LTT_EVENT_XENOLTT_THREAD_DELETE,
    LTT_EVENT_XENOLTT_THREAD_UNBLOCK,
    LTT_EVENT_XENOLTT_THREAD_RENICE,
    LTT_EVENT_XENOLTT_TIMER_TICK,
    LTT_EVENT_XENOLTT_SYNCH_SET_OWNER,
    LTT_EVENT_XENOLTT_SYNCH_UNLOCK,
    LTT_EVENT_XENOLTT_SYNCH_WAKEUP1,
    LTT_EVENT_XENOLTT_SYNCH_WAKEUPX,
    LTT_EVENT_XENOLTT_SYNCH_SLEEP_ON,
    LTT_EVENT_XENOLTT_SYNCH_FLUSH,
    LTT_EVENT_XENOLTT_SYNCH_FORGET,
    LTT_EVENT_XENOLTT_THREAD_SWITCH;
    */

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_INIT,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_SET_PERIOD,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_WAIT_PERIOD,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_MISSED_PERIOD,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_SUSPEND,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_START,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_RESUME,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_DELETE,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_SWITCH,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_THREAD_SWITCH,
        LTT_FIELD_XENOLTT_ADDRESS_OUT, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;

    ret = lttv_trace_find_hook(ts->parent.parent.t,
        LTT_FACILITY_XENOLTT, LTT_EVENT_XENOLTT_TIMER_TICK,
        LTT_FIELD_XENOLTT_ADDRESS, 0, 0,
        save_event, NULL,
        &g_array_index(hooks, LttvTraceHook, hn++));
    if(ret) hn--;
        
    g_array_set_size(hooks, hn);

    before_hooks = hooks;

    /* Add these hooks to each event_by_id hooks list */

    nb_tracefile = ts->parent.parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATS(g_array_index(ts->parent.parent.tracefiles,
                                  LttvTracefileContext*, j));
      lttv_hooks_add(tfs->parent.parent.event, sim_every_event, NULL, 
                     LTTV_PRIO_DEFAULT);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = &g_array_index(before_hooks, LttvTraceHook, k);
        for(l = 0; l<hook->fac_list->len;l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_add(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf,
              LTTV_PRIO_DEFAULT);
        }
      }

    }
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    *(val.v_pointer) = before_hooks;
  }
}

// Hook wrapper. call_data is a traceset context.
gboolean lttv_xenoltt_sim_hook_remove_event_hooks(void *hook_data, void *call_data)
{
   LttvTracesetStats *tss = (LttvTracesetStats*)call_data;

   lttv_xenoltt_sim_remove_event_hooks(tss);

   return 0;
}

void lttv_xenoltt_sim_remove_event_hooks(LttvTracesetStats *self)
{
  LttvTraceset *traceset = self->parent.parent.ts;

  guint i, j, k, l, nb_trace, nb_tracefile;

  LttvTraceStats *ts;

  LttvTracefileStats *tfs;

  GArray *before_hooks;

  LttvTraceHook *hook;
  
  LttvTraceHookByFacility *thf;

  LttvAttributeValue val;

  nb_trace = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb_trace ; i++) {
    ts = (LttvTraceStats*)self->parent.parent.traces[i];
    lttv_attribute_find(self->parent.parent.a, LTTV_STATS_BEFORE_HOOKS, 
        LTTV_POINTER, &val);
    before_hooks = *(val.v_pointer);

    /* Remove these hooks from each event_by_id hooks list */

    nb_tracefile = ts->parent.parent.tracefiles->len;

    for(j = 0 ; j < nb_tracefile ; j++) {
      tfs = LTTV_TRACEFILE_STATS(g_array_index(ts->parent.parent.tracefiles,
                                  LttvTracefileContext*, j));
      lttv_hooks_remove_data(tfs->parent.parent.event, sim_every_event, 
          NULL);

      for(k = 0 ; k < before_hooks->len ; k++) {
        hook = &g_array_index(before_hooks, LttvTraceHook, k);
        for(l = 0 ; l < hook->fac_list->len ; l++) {
          thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l);
          lttv_hooks_remove_data(
              lttv_hooks_by_id_find(tfs->parent.parent.event_by_id, thf->id),
              thf->h,
              thf);
        }
      }
    }
    g_debug("lttv_stats_remove_event_hooks()");
    g_array_free(before_hooks, TRUE);
  }
}



/****************************************************************************************************************************/



/* 
 This function will look into the thread list to find the corresponding thread and returns it
 This way we will be able to add a new event to this thread events list.
*/
ThreadEventData* lookup_or_create_thread(gulong address, guint prio, LttTime creation_time, GQuark name){
  int i, index = 0;
  ThreadEventData *temp_thread;
  ThreadEventData *temp_thread_2 = g_new(ThreadEventData, 1);
  temp_thread_2->address = address;
  temp_thread_2->prio = prio;
  temp_thread_2->creation_time = creation_time;
  temp_thread_2->name = name;
  temp_thread_2->event_list = g_array_new(FALSE, FALSE, sizeof(EventData*));
  

  for(i=0;i<thread_event_list->len;i++){
    temp_thread = g_array_index(thread_event_list, ThreadEventData*, i);
    if (temp_thread->address == temp_thread_2->address &&
        ltt_time_compare(temp_thread->creation_time,temp_thread_2->creation_time) == 0)
      return temp_thread;   // Thread is found we return it
    /* Otherwise we check for the priority, this will help us to defined the 
       index where to insert the thread. This way we don't to sort the thread list */
    else if(temp_thread_2->prio <= temp_thread->prio) index++;
  }
    
  g_array_insert_val(thread_event_list,index,temp_thread_2);

  return temp_thread_2;     // New inserted thread is returned
}

void calculate_event_time(guint index, ThreadEventData *temp_thread){
  LttTime next_tick = ltt_time_zero, delay, preempt_begin, preempt_time = ltt_time_zero, 
          last_write_event_time, last_read_event_time, original_event_time;
  EventData *new_event, *event;
  int i,j, overruns = 0;
  gboolean wait_period_called = FALSE;
  RunningThread *temp_running_thread = NULL;
  gboolean first_thread_switch, running = FALSE;
  LttTime new_period = ltt_time_from_double(temp_thread->period);
  
  temp_thread->new_event_list = g_array_new(FALSE, FALSE, sizeof(EventData*));
  
  // We will iterate on all event of this thread
  for(i=0;i<temp_thread->event_list->len;i++){
    // for the first event read
    if (i == 0){
      new_event =  g_array_index(temp_thread->event_list, EventData*, i);
      last_write_event_time = new_event->event_time;
      last_read_event_time = new_event->event_time;
      original_event_time = new_event->event_time;
    }
    else{
      last_write_event_time = new_event->event_time;
      last_read_event_time = original_event_time;
      new_event =  g_array_index(temp_thread->event_list, EventData*, i);
      original_event_time = new_event->event_time;
    }
    
    // Calculate the delay between to following events
    delay = ltt_time_sub(original_event_time,last_read_event_time);
    delay = ltt_time_sub(delay,preempt_time);

    // We need to save all events from the timer_tick until the wait_period event
    // At the same time we can calculate the new time of the event
    if (new_event->name == LTT_EVENT_XENOLTT_TIMER_TICK){
      //printf("NEW PERIOD\n");
      // The first tick will be unchanged
      if(ltt_time_compare(ltt_time_zero,next_tick) != 0){
        new_event->event_time = next_tick;
      }
      next_tick = ltt_time_add(new_event->event_time,new_period);
      wait_period_called = FALSE;      // We prepare for next period that should begin now

      g_array_append_val(temp_thread->new_event_list,new_event);  // insert the new timer_tick
      //printf("\tTIMER_TICK - TIME: %lu.%lu - %lu.%lu\n", original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
      
      first_thread_switch = TRUE;
      preempt_time = ltt_time_zero;
      preempt_begin = ltt_time_zero;
      
      /************************************************************************
       * Beginning of a new period
       * We must check for thread_switching (preemption)
       * new timer tick to create
       * overrun to create
       * event missed_period to create
       ************************************************************************/
      
      while(new_event->name != LTT_EVENT_XENOLTT_THREAD_WAIT_PERIOD){  // Until the end of the period
        i++;
        last_write_event_time = new_event->event_time;
        last_read_event_time = original_event_time;
        new_event =  g_array_index(temp_thread->event_list, EventData*, i);
        original_event_time = new_event->event_time;

        // Calculate the delay between to following events
        delay = ltt_time_sub(original_event_time,last_read_event_time);
        delay = ltt_time_sub(delay,preempt_time);

        // Need to test if we have exceeded the new period
        if(new_event->name != LTT_EVENT_XENOLTT_TIMER_TICK){
          if (ltt_time_compare(ltt_time_add(last_write_event_time,delay),next_tick) > 0){
            EventData *tick_event = g_new(EventData, 1);
            tick_event->event_time = next_tick;
            tick_event->name = LTT_EVENT_XENOLTT_TIMER_TICK;
            g_array_append_val(temp_thread->new_event_list,tick_event);
            next_tick = ltt_time_add(tick_event->event_time,new_period);
            //printf("\t%s - TIME: \t%lu.%lu\n", g_quark_to_string(tick_event->name),tick_event->event_time.tv_sec,tick_event->event_time.tv_nsec);
            overruns++;
          }  
        }
        
        
        // Check and treat every kind of event
        if(new_event->name == LTT_EVENT_XENOLTT_THREAD_INIT ||
          new_event->name == LTT_EVENT_XENOLTT_THREAD_SET_PERIOD ||
          new_event->name == LTT_EVENT_XENOLTT_THREAD_START ||
          new_event->name == LTT_EVENT_XENOLTT_THREAD_RESUME ||
          new_event->name == LTT_EVENT_XENOLTT_THREAD_RENICE ||
          new_event->name == LTT_EVENT_XENOLTT_THREAD_SUSPEND){
          new_event->event_time = ltt_time_add(last_write_event_time,delay); // New time of the event
          // Insert event in the period list
          g_array_append_val(temp_thread->new_event_list,new_event);
          //printf("\t%s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
        }
        // the first Thread_Switch indicate that the thread is now running
        else if(new_event->name == LTT_EVENT_XENOLTT_THREAD_SWITCH){
          if (first_thread_switch){
            running = TRUE;
            first_thread_switch = FALSE;
            new_event->event_time = ltt_time_add(last_write_event_time,delay); // New time of the event  
            // Insert event in the period list
            g_array_append_val(temp_thread->new_event_list,new_event);
            //printf("\t%s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
          }
          // Not the first thread switch, we will delete this event and the previous one that should be thread_suspend
          else if(running){ 
            running = FALSE;  // Stop the thread
            new_event =  g_array_index(temp_thread->event_list, EventData*, (i-1));
            preempt_begin = new_event->event_time;// Save the time of the preemption (time of the suspend event
          }
          // Thread is suspended and want to restart, delete the thread_switch and the following event that should be thread_resume
          else{
            running = TRUE; // restart thread
            i++;
            new_event =  g_array_index(temp_thread->event_list, EventData*, i);
            preempt_time = ltt_time_add(preempt_time,ltt_time_sub(new_event->event_time,preempt_begin));// ignore the time spent in ready state
          }
        }
        // Thread going in overrun
        else if(new_event->name == LTT_EVENT_XENOLTT_TIMER_TICK){
          new_event->event_time = next_tick;
          next_tick = ltt_time_add(new_event->event_time,new_period);
          overruns++;  // If wait_period has not been called, this means we are going in overrun
          // Insert event in the period list
          g_array_append_val(temp_thread->new_event_list,new_event);
          //printf("\t%s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
        }

        if(new_event->name == LTT_EVENT_XENOLTT_THREAD_WAIT_PERIOD){
          new_event->event_time = ltt_time_add(last_write_event_time,delay); // New time of the event
          g_array_append_val(temp_thread->new_event_list,new_event);
          //printf("\t%s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
          //printf("END PERIOD\n");
          wait_period_called = TRUE;
          if(overruns > 0){
            EventData *missed_period_event = g_new(EventData, 1);
            missed_period_event->event_time = new_event->event_time;  // Same time ??
            missed_period_event->name = LTT_EVENT_XENOLTT_THREAD_MISSED_PERIOD;
            g_array_append_val(temp_thread->new_event_list,missed_period_event);
            //printf("\t%s - TIME: %lu.%lu\n", g_quark_to_string(missed_period_event->name),missed_period_event->event_time.tv_sec,missed_period_event->event_time.tv_nsec);
          }
          overruns = 0;
          // Period is finished
          running = FALSE;
        }
         
        if(new_event->name == LTT_EVENT_XENOLTT_THREAD_DELETE){
          // Insert event in the period list
          new_event->event_time = ltt_time_add(last_write_event_time,delay); // New time of the event
          g_array_append_val(temp_thread->new_event_list,new_event);
          //printf("\t%s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
          break;
        }
      }
    }
    // For other events, simply save them with new time
    else{
      if (new_event->name != LTT_EVENT_XENOLTT_THREAD_MISSED_PERIOD){
        new_event->event_time = ltt_time_add(last_write_event_time,delay); // New time of the event
        g_array_append_val(temp_thread->new_event_list,new_event);
        //printf("NO_PERIOD %s - TIME: %lu.%lu - %lu.%lu\n", g_quark_to_string(new_event->name),original_event_time.tv_sec,original_event_time.tv_nsec,new_event->event_time.tv_sec,new_event->event_time.tv_nsec);
      }
    }
  }
  
//  printf("fin phase 1\n");
  // Now we have a full list of events representing the simulation of the current task
  // Last step consist of checking if this thread will be preempted by others
  // To see that, we will check in the running_thread list to find some free time space

  // Iterate on the event_list and check for every thread_switch
  gboolean not_running = TRUE;
  overruns = 0;
  wait_period_called = TRUE;
  j=0;
  delay = ltt_time_zero;
  
  // We will iterate on all event of this thread
  for(i=0;i<temp_thread->new_event_list->len;i++){
    event = g_array_index(temp_thread->new_event_list, EventData*, i);
    if (event->name == LTT_EVENT_XENOLTT_THREAD_SWITCH){
      // If thread is a switch in
      if (not_running){
        not_running = FALSE;
        // Check if cpu is free at this time, look for the nearest begin execution time period(before current time)
        for(;j<running_thread->len;j++){
          temp_running_thread = g_array_index(running_thread, RunningThread*, j);

          if (ltt_time_compare(event->event_time,temp_running_thread->begin_time) >= 0){
            if (ltt_time_compare(event->event_time,temp_running_thread->end_time) <= 0){
              // Compute delay to insert in all following events
              delay = ltt_time_add(delay,ltt_time_sub(temp_running_thread->end_time,event->event_time));
              // new event time is the time of the next switch out
              event->event_time = temp_running_thread->end_time;
              // event_time we be tested on next entry
            }
          }
          else{
            break;
          }
        }
        
        // At this time we should have found a free starting position
        RunningThread *new_running = g_new(RunningThread,1);
        new_running->thread = temp_thread;
        new_running->begin_time = event->event_time;        
//        printf("Begin: %lu.%lu\n",new_running->begin_time.tv_sec,new_running->begin_time.tv_nsec);
        
        for(i++;i<temp_thread->new_event_list->len;i++){
          event =  g_array_index(temp_thread->new_event_list, EventData*, i);
          
          // Don't delay Timer_Tick          
          if (event->name == LTT_EVENT_XENOLTT_TIMER_TICK){
            // Beginning of a period
            if (wait_period_called){
              wait_period_called = FALSE;
              overruns = 0;
              delay = ltt_time_zero;
            }
            // We are going in overrun
            else{
              overruns++;
            }
          }
          
          // On Switch_Out event, save the thread running time in the running_thread list
          else if(event->name == LTT_EVENT_XENOLTT_THREAD_SWITCH){
              new_running->end_time = event->event_time;
              g_array_insert_val(running_thread,j,new_running);
              not_running = TRUE;
              break;
          }
          // All other events may be preempt by another task including the thread_switch
          else{
            // insert the delay due to previous preemption in this period
            event->event_time = ltt_time_add(event->event_time,delay);


            // We must check the if the next running thread is beginning before the event_time
            // Note that temp_running_thread can be the last running_thread of the list
            // or the next running thread if we have found a free time space between
            // two threads
            if(temp_running_thread != NULL && running_thread->len > 0){
              // Another task is alreday running we will insert a thread switch out
              if(ltt_time_compare(event->event_time,temp_running_thread->begin_time) >= 0 &&
                  ltt_time_compare(event->event_time,temp_running_thread->end_time) < 0){
                // If running task ends before the event_time, no delay is needed
                // but if taks finishes after the event, we must move the event at the end time
                if(ltt_time_compare(event->event_time,temp_running_thread->end_time) < 0)
                  delay = ltt_time_add(delay,ltt_time_sub(temp_running_thread->end_time,event->event_time));

                // Insert a thread switch in that will be check at next iteration
                new_event = g_new(EventData, 1);
                new_event->event_time = temp_running_thread->end_time;
                new_event->name = LTT_EVENT_XENOLTT_THREAD_SWITCH;
                g_array_insert_val(temp_thread->new_event_list,i,new_event);
            
                // Thread switch out
                new_event = g_new(EventData, 1);
                new_event->event_time = temp_running_thread->begin_time;
                new_event->name = LTT_EVENT_XENOLTT_THREAD_SWITCH;
                g_array_insert_val(temp_thread->new_event_list,i,new_event);

                // Insert the thread in the running thread
                new_running->end_time = temp_running_thread->begin_time;
                g_array_insert_val(running_thread,j,new_running);
//                printf("%lu.%lu\n",new_running->begin_time.tv_sec,new_running->begin_time.tv_nsec);
                not_running = TRUE;      
                
                break;
              } 
            }                          
          }
        }
      }
    }
  }

        
 
/*  
  //Print the new thread simulation
  for(j=0;j<new_event_list->len;j++){
    event =  g_array_index(new_event_list, EventData*, j);
    printf("%s - TIME: %lu.%lu\n", g_quark_to_string(event->name),event->event_time.tv_sec,event->event_time.tv_nsec);
  }
*/
}

void simulate_high_priority_thread(ThreadEventData* thread){
  EventData *event;
  gboolean running = FALSE;
  RunningThread *run_thread  = g_new(RunningThread, 1);
  RunningThread *temp_thread;
  int i,j; 
  LttTime begin_time = ltt_time_zero;
  LttTime end_time = ltt_time_zero;
  gboolean inserted;

  for(i=0;i<thread->event_list->len;i++){
    event =  g_array_index(thread->event_list, EventData*, i);
    
    if(event->name == LTT_EVENT_XENOLTT_THREAD_SWITCH){
      if(running){
        running = FALSE;
        end_time = event->event_time;
        run_thread  = g_new(RunningThread, 1);
        run_thread->thread = thread;
        run_thread->begin_time = begin_time;
        run_thread->end_time = end_time;
        
        inserted = FALSE;
        for(j=0;j<running_thread->len;j++){
          temp_thread =  g_array_index(running_thread, RunningThread*, j);
          if (ltt_time_compare(temp_thread->begin_time,run_thread->begin_time) > 0){
            g_array_insert_val(running_thread,j,run_thread);
            inserted = TRUE;
            break;
          }
        }
        if (!inserted) g_array_append_val(running_thread,run_thread);
      }
      else{
        running = TRUE;
        begin_time = event->event_time;
      }
    }   
  }
  thread->new_event_list = thread->event_list;
}

GArray* get_thread_list(){
  return thread_event_list;
}

void compute_simulation(guint index, guint period, FILE *a_file){
  
  int i,j;
  ThreadEventData *temp_thread;
  RunningThread *run_thread;  
  EventData *event;
  
  
  // First, set the new period of the thread
  temp_thread = g_array_index(thread_event_list, ThreadEventData*, index);
  temp_thread->period = period;
  
  running_thread = g_array_new(FALSE, FALSE, sizeof(RunningThread*));  
    
  fprintf(a_file,"<EVENTS_LIST>\n");  
  
  /* 
   First, we need to ignore all task with higher priority 
   than the task we want to simulate that's why we begin the simulation
   from the thread index which we will modify the period
  */
  for(i=0;i<index;i++){
    temp_thread = g_array_index(thread_event_list, ThreadEventData*, i);
    simulate_high_priority_thread(temp_thread);    
    fprintf(a_file,"\t<TASK NAME=\"%s\" ADDRESS=\"%p\" PRIORITY=\"%u\" PERIOD=\"%u\">\n",g_quark_to_string(temp_thread->name),(void *) temp_thread->address, temp_thread->prio, temp_thread->period);    
    for(j=0; j<temp_thread->new_event_list->len;j++){
      event =  g_array_index(temp_thread->new_event_list, EventData*, j);   
      fprintf(a_file,"\t\t<EVENT NAME=\"%s\" TIME=\"%lu.%lu\">\n",g_quark_to_string(event->name), event->event_time.tv_sec,event->event_time.tv_nsec);    
    }
    fprintf(a_file,"\t</TASK>\n"); 
  }
  
  for(i=index;i<thread_event_list->len;i++){
    temp_thread = g_array_index(thread_event_list, ThreadEventData*, i);
    
    // We will simulate this thread considering all higher priority threads
    calculate_event_time(i, temp_thread);
    fprintf(a_file,"\t<TASK NAME=\"%s\" ADDRESS=\"%p\" PRIORITY=\"%u\" PERIOD=\"%u\">\n",g_quark_to_string(temp_thread->name), (void *) temp_thread->address, temp_thread->prio, temp_thread->period);        
    for(j=0; j<temp_thread->new_event_list->len;j++){
      event =  g_array_index(temp_thread->new_event_list, EventData*, j);   
      fprintf(a_file,"\t\t<EVENT NAME=\"%s\" TIME=\"%lu.%lu\">\n",g_quark_to_string(event->name), event->event_time.tv_sec,event->event_time.tv_nsec);    
    }
    fprintf(a_file,"\t</TASK>\n"); 
  }  
  

  fprintf(a_file,"</EVENTS_LIST>\n");    

  fprintf(a_file,"<RUNNING_TASK>\n");    
  
  for(i=0;i<running_thread->len;i++){
    run_thread = g_array_index(running_thread, RunningThread*, i);
    fprintf(a_file,"\t<TASK NAME=\"%s\" FROM=\"%lu.%lu\" TO=\"%lu.%lu\">\n",g_quark_to_string(run_thread->thread->name),run_thread->begin_time.tv_sec,run_thread->begin_time.tv_nsec,
                                                                                      run_thread->end_time.tv_sec,run_thread->end_time.tv_nsec);
  }  

  fprintf(a_file,"</RUNNING_TASK>\n");    
  
}

gboolean save_event(void *hook_data, void *call_data){
  LttvTraceHookByFacility *thf = (LttvTraceHookByFacility*)hook_data;
  LttvTracefileStats *tfcs = (LttvTracefileStats *)call_data;
  LttvTraceState *ts = (LttvTraceState*)tfcs->parent.parent.t_context;
  guint cpu = tfcs->parent.cpu;
  LttvXenoThreadState *thread_info;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);  
  GQuark event_name = ltt_eventtype_name(ltt_event_eventtype(e));  
  LttTime evtime = ltt_event_time(e);
  
  if (event_name == LTT_EVENT_XENOLTT_TIMER_TICK){
    gulong timer_address = ltt_event_get_long_unsigned(e, thf->f1);
    thread_info = lttv_xeno_state_find_thread_from_timer(ts,cpu,timer_address);
  }
  else{
    gulong address = ltt_event_get_long_unsigned(e, thf->f1);  
    // First we need to lookup for the current thread in the list
    thread_info = lttv_xeno_state_find_thread(ts,cpu,address);
  }
  
  if (thread_info != NULL){
    ThreadEventData *thread = lookup_or_create_thread(thread_info->address, thread_info->prio, thread_info->creation_time, thread_info->name);
    if (event_name == LTT_EVENT_XENOLTT_THREAD_SET_PERIOD) thread->period = thread_info->period;
    //Thread is found in the table, we can insert the new event in the list
    EventData *new_event = g_new(EventData, 1);
    new_event->event_time = evtime;
    new_event->name = event_name;
    new_event->event = e;
    g_array_append_val(thread->event_list,new_event); 
  }

  return FALSE;
}

/****************************************************************************************************************************/




static void module_init()
{
  // Initialization of the 2 main lists used in this module
  thread_event_list = g_array_new(FALSE, FALSE, sizeof(ThreadEventData*));
  running_thread = g_array_new(FALSE, FALSE, sizeof(RunningThread*));  
}

static void module_destroy() 
{
}


LTTV_MODULE("xenoltt_sim", "Compute Xenomai Tasks simulation", \
    "Simulate a task execution with a different period", \
    module_init, module_destroy, "state");
