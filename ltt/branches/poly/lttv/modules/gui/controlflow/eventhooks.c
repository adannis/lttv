/* This file is part of the Linux Trace Toolkit viewer
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, 
 * MA 02111-1307, USA.
 */


/*****************************************************************************
 *                       Hooks to be called by the main window               *
 *****************************************************************************/


//#define PANGO_ENABLE_BACKEND
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

//#include <pango/pango.h>

#include <ltt/event.h>
#include <ltt/time.h>
#include <ltt/type.h>

#include <lttv/lttv.h>
#include <lttv/hook.h>
#include <lttv/state.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>


#include "eventhooks.h"
#include "cfv.h"
#include "processlist.h"
#include "drawing.h"
#include "cfv-private.h"


#define MAX_PATH_LEN 256


/* Action to do when background computation completed.
 *
 * Eventually, will have to check that every requested traces are finished
 * before doing the redraw.
 */

gint background_ready(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData *)hook_data;
  LttvTrace *trace = (LttvTrace*)call_data;

  g_debug("control flow viewer : background computation data ready.");

  redraw_notify(control_flow_data, NULL);

  return 0;
}


/* Request background computation. Verify if it is in progress or ready first.
 *
 * Right now, for all loaded traces.
 *
 * Later : must be only for each trace in the tab's traceset.
 */
void request_background_data(ControlFlowData *control_flow_data)
{
  gint num_traces = lttvwindowtraces_get_number();
  gint i;
  LttvTrace *trace;

  LttvHooks *background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, control_flow_data,
      LTTV_PRIO_DEFAULT);
  
  for(i=0;i<num_traces;i++) {
    trace = lttvwindowtraces_get_trace(i);

    if(lttvwindowtraces_get_ready(g_quark_from_string("state"),trace)==FALSE) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("state"),
                                          trace) == FALSE) {
        /* We first remove requests that could have been done for the same
         * information. Happens when two viewers ask for it before servicing
         * starts.
         */
        lttvwindowtraces_background_request_remove(trace, "state");
        lttvwindowtraces_background_request_queue(trace,
                                                  "state");
        lttvwindowtraces_background_notify_queue(control_flow_data,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 background_ready_hook);
      } else { /* in progress */
      
        lttvwindowtraces_background_notify_current(control_flow_data,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   background_ready_hook);
      
      }
    }
  }

  lttv_hooks_destroy(background_ready_hook);
}




/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param tab A pointer to the parent tab.
 * @return The widget created.
 */
GtkWidget *
h_guicontrolflow(Tab *tab, LttvTracesetSelector * s, char * key)
{
  g_info("h_guicontrolflow, %p, %p, %s", tab, s, key);
  ControlFlowData *control_flow_data = guicontrolflow() ;
  
  control_flow_data->tab = tab;
  
  //g_debug("time width2 : %u",time_window->time_width);
  // Unreg done in the GuiControlFlow_Destructor
  lttvwindow_register_traceset_notify(tab,
        traceset_notify,
        control_flow_data);
    
  lttvwindow_register_time_window_notify(tab,
                                         update_time_window_hook,
                                         control_flow_data);
  lttvwindow_register_current_time_notify(tab,
                                          update_current_time_hook,
                                          control_flow_data);
  lttvwindow_register_redraw_notify(tab,
                                    redraw_notify,
                                    control_flow_data);
  lttvwindow_register_continue_notify(tab,
                                      continue_notify,
                                      control_flow_data);
  request_background_data(control_flow_data);
  

  return guicontrolflow_get_widget(control_flow_data) ;
  
}

int event_selected_hook(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  guint *event_number = (guint*) call_data;

  g_debug("DEBUG : event selected by main window : %u", *event_number);
  
//  control_flow_data->currently_Selected_Event = *event_number;
//  control_flow_data->Selected_Event = TRUE ;
  
//  tree_v_set_cursor(control_flow_data);

}

/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in event_request.
 */
int draw_before_hook(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  //EventsContext Events_Context = (EventsContext*)call_data;
  
  //event_request->Events_Context = Events_Context;

  return 0;
}

/*
 * The draw event hook is called by the reading API to have a
 * particular event drawn on the screen.
 * @param hook_data ControlFlowData structure of the viewer. 
 * @param call_data Event context.
 *
 * This function basically draw lines and icons. Two types of lines are drawn :
 * one small (3 pixels?) representing the state of the process and the second
 * type is thicker (10 pixels?) representing on which CPU a process is running
 * (and this only in running state).
 *
 * Extremums of the lines :
 * x_min : time of the last event context for this process kept in memory.
 * x_max : time of the current event.
 * y : middle of the process in the process list. The process is found in the
 * list, therefore is it's position in pixels.
 *
 * The choice of lines'color is defined by the context of the last event for this
 * process.
 */
int draw_event_hook(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *control_flow_data = 
                      (ControlFlowData*)events_request->viewer_data;
  Tab *tab = control_flow_data->tab;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts =(LttvTraceState *)LTTV_TRACEFILE_CONTEXT(tfs)->t_context;
  
  LttEvent *e;
  e = tfc->e;

  LttTime evtime = ltt_event_time(e);
  TimeWindow time_window = 
    lttvwindow_get_time_window(tab);

  LttTime end_time = ltt_time_add(time_window.start_time,
                                    time_window.time_width);
  //if(time < time_beg || time > time_end) return;
  if(ltt_time_compare(evtime, time_window.start_time) == -1
        || ltt_time_compare(evtime, end_time) == 1)
            return;

  if(strcmp(ltt_eventtype_name(ltt_event_eventtype(e)),"schedchange") == 0)
  {
    g_debug("schedchange!");
    
    /* Add process to process list (if not present) and get drawing "y" from
     * process position */
    guint pid_out, pid_in;
    LttvProcessState *process_out, *process_in;
    LttTime birth;
    guint y_in = 0, y_out = 0, height = 0, pl_height = 0;

    ProcessList *process_list =
      guicontrolflow_get_process_list(control_flow_data);


    LttField *f = ltt_event_field(e);
    LttField *element;
    element = ltt_field_member(f,0);
    pid_out = ltt_event_get_long_unsigned(e,element);
    element = ltt_field_member(f,1);
    pid_in = ltt_event_get_long_unsigned(e,element);
    g_debug("out : %u  in : %u", pid_out, pid_in);


    /* Find process pid_out in the list... */
    process_out = lttv_state_find_process(tfs, pid_out);
    if(process_out == NULL) return 0;
    g_debug("out : %s",g_quark_to_string(process_out->state->s));
    
    birth = process_out->creation_time;
    gchar *name = strdup(g_quark_to_string(process_out->name));
    HashedProcessData *hashed_process_data_out = NULL;

    if(processlist_get_process_pixels(process_list,
            pid_out,
            &birth,
            tfc->t_context->index,
            &y_out,
            &height,
            &hashed_process_data_out) == 1)
    {
    /* Process not present */
    processlist_add(process_list,
        pid_out,
        &birth,
        tfc->t_context->index,
        name,
        &pl_height,
        &hashed_process_data_out);
    processlist_get_process_pixels(process_list,
            pid_out,
            &birth,
            tfc->t_context->index,
            &y_out,
            &height,
            &hashed_process_data_out);
    drawing_insert_square( control_flow_data->drawing, y_out, height);
    }

    g_free(name);
    
    /* Find process pid_in in the list... */
    process_in = lttv_state_find_process(tfs, pid_in);
    if(process_in == NULL) return 0;
    g_debug("in : %s",g_quark_to_string(process_in->state->s));

    birth = process_in->creation_time;
    name = strdup(g_quark_to_string(process_in->name));
    HashedProcessData *hashed_process_data_in = NULL;

    if(processlist_get_process_pixels(process_list,
            pid_in,
            &birth,
            tfc->t_context->index,
            &y_in,
            &height,
            &hashed_process_data_in) == 1)
    {
    /* Process not present */
      processlist_add(process_list,
        pid_in,
        &birth,
        tfc->t_context->index,
        name,
        &pl_height,
        &hashed_process_data_in);
      processlist_get_process_pixels(process_list,
            pid_in,
            &birth,
            tfc->t_context->index,
            &y_in,
            &height,
            &hashed_process_data_in);

      drawing_insert_square( control_flow_data->drawing, y_in, height);
    }
    g_free(name);


    /* Find pixels corresponding to time of the event. If the time does
     * not fit in the window, show a warning, not supposed to happend. */
    guint x = 0;
    guint width = control_flow_data->drawing->drawing_area->allocation.width;

    LttTime time = ltt_event_time(e);

    LttTime window_end = ltt_time_add(time_window.time_width,
                          time_window.start_time);

    
    convert_time_to_pixels(
        time_window.start_time,
        window_end,
        time,
        width,
        &x);
    //assert(x <= width);
    
    /* draw what represents the event for outgoing process. */

    DrawContext *draw_context_out = hashed_process_data_out->draw_context;
    draw_context_out->current->modify_over->x = x;
    draw_context_out->current->modify_under->x = x;
    draw_context_out->current->modify_over->y = y_out;
    draw_context_out->current->modify_under->y = y_out+(height/2)+2;
    draw_context_out->drawable = control_flow_data->drawing->pixmap;
    draw_context_out->pango_layout = control_flow_data->drawing->pango_layout;
    GtkWidget *widget = control_flow_data->drawing->drawing_area;
    //draw_context_out->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
    //draw_context_out->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
    //gdk_gc_copy(draw_context_out->gc, widget->style->black_gc);
    //draw_context_out->gc = widget->style->black_gc;
    
    //draw_arc((void*)&prop_arc, (void*)draw_context_out);
    //test_draw_item(control_flow_data->drawing, control_flow_data->drawing->pixmap);
    
    /* Draw the line/background of the out process */
    if(draw_context_out->previous->middle->x == -1)
    {
      draw_context_out->previous->over->x =
                            control_flow_data->drawing->damage_begin;
      draw_context_out->previous->middle->x = 
                            control_flow_data->drawing->damage_begin;
      draw_context_out->previous->under->x =
                            control_flow_data->drawing->damage_begin;

      g_debug("out middle x_beg : %u",control_flow_data->drawing->damage_begin);
    }
  
    draw_context_out->current->middle->x = x;
    draw_context_out->current->over->x = x;
    draw_context_out->current->under->x = x;
    draw_context_out->current->middle->y = y_out + height/2;
    draw_context_out->current->over->y = y_out;
    draw_context_out->current->under->y = y_out + height;
    draw_context_out->previous->middle->y = y_out + height/2;
    draw_context_out->previous->over->y = y_out;
    draw_context_out->previous->under->y = y_out + height;

    draw_context_out->drawable = control_flow_data->drawing->pixmap;
    draw_context_out->pango_layout = control_flow_data->drawing->pango_layout;

    if(process_out->state->s == LTTV_STATE_RUN)
    {
      draw_context_out->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
      gdk_gc_copy(draw_context_out->gc, widget->style->black_gc);

      PropertiesBG prop_bg;
      prop_bg.color = g_new(GdkColor,1);
      
      switch(tfc->index) {
        case 0:
          prop_bg.color->red = 0x1515;
          prop_bg.color->green = 0x1515;
          prop_bg.color->blue = 0x8c8c;
          break;
        case 1:
          prop_bg.color->red = 0x4e4e;
          prop_bg.color->green = 0xa9a9;
          prop_bg.color->blue = 0xa4a4;
          break;
        case 2:
          prop_bg.color->red = 0x7a7a;
          prop_bg.color->green = 0x4a4a;
          prop_bg.color->blue = 0x8b8b;
          break;
        case 3:
          prop_bg.color->red = 0x8080;
          prop_bg.color->green = 0x7777;
          prop_bg.color->blue = 0x4747;
          break;
        default:
          prop_bg.color->red = 0xe7e7;
          prop_bg.color->green = 0xe7e7;
          prop_bg.color->blue = 0xe7e7;
      }
      
      g_debug("calling from draw_event");
      draw_bg((void*)&prop_bg, (void*)draw_context_out);
      g_free(prop_bg.color);
      gdk_gc_unref(draw_context_out->gc);
    }

    draw_context_out->gc = widget->style->black_gc;

    GdkColor colorfg_out = { 0, 0xffff, 0x0000, 0x0000 };
    GdkColor colorbg_out = { 0, 0x0000, 0x0000, 0x0000 };
    PropertiesText prop_text_out;
    prop_text_out.foreground = &colorfg_out;
    prop_text_out.background = &colorbg_out;
    prop_text_out.size = 6;
    prop_text_out.position = OVER;

    /* color of text : status of the process */
    if(process_out->state->s == LTTV_STATE_UNNAMED)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_text_out.foreground->red = 0x0fff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xfff0;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_EXIT)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0x0000;
      prop_text_out.foreground->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0x0000;
      prop_text_out.foreground->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_RUN)
    {
      prop_text_out.foreground->red = 0x0000;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0x0000;
    }
    else
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xffff;
    }
 
    
    /* Print status of the process : U, WF, WC, E, W, R */
    if(process_out->state->s == LTTV_STATE_UNNAMED)
      prop_text_out.text = "U->";
    else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
      prop_text_out.text = "WF->";
    else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
      prop_text_out.text = "WC->";
    else if(process_out->state->s == LTTV_STATE_EXIT)
      prop_text_out.text = "E->";
    else if(process_out->state->s == LTTV_STATE_WAIT)
      prop_text_out.text = "W->";
    else if(process_out->state->s == LTTV_STATE_RUN)
      prop_text_out.text = "R->";
    else
      prop_text_out.text = "U";
    
    draw_text((void*)&prop_text_out, (void*)draw_context_out);
    //gdk_gc_unref(draw_context_out->gc);

    draw_context_out->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
    gdk_gc_copy(draw_context_out->gc, widget->style->black_gc);

    PropertiesLine prop_line_out;
    prop_line_out.color = g_new(GdkColor,1);
    prop_line_out.line_width = 2;
    prop_line_out.style = GDK_LINE_SOLID;
    prop_line_out.position = MIDDLE;
    
    g_debug("out state : %s", g_quark_to_string(process_out->state->s));

    /* color of line : status of the process */
    if(process_out->state->s == LTTV_STATE_UNNAMED)
    {
      prop_line_out.color->red = 0xffff;
      prop_line_out.color->green = 0xffff;
      prop_line_out.color->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_line_out.color->red = 0x0fff;
      prop_line_out.color->green = 0xffff;
      prop_line_out.color->blue = 0xfff0;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_line_out.color->red = 0xffff;
      prop_line_out.color->green = 0xffff;
      prop_line_out.color->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_EXIT)
    {
      prop_line_out.color->red = 0xffff;
      prop_line_out.color->green = 0x0000;
      prop_line_out.color->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT)
    {
      prop_line_out.color->red = 0xffff;
      prop_line_out.color->green = 0x0000;
      prop_line_out.color->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_RUN)
    {
      prop_line_out.color->red = 0x0000;
      prop_line_out.color->green = 0xffff;
      prop_line_out.color->blue = 0x0000;
    }
    else
    {
      prop_line_out.color->red = 0xffff;
      prop_line_out.color->green = 0xffff;
      prop_line_out.color->blue = 0xffff;
    }
  
    draw_line((void*)&prop_line_out, (void*)draw_context_out);
    g_free(prop_line_out.color);
    gdk_gc_unref(draw_context_out->gc);
    /* Note : finishing line will have to be added when trace read over. */
      
    /* Finally, update the drawing context of the pid_in. */

    DrawContext *draw_context_in = hashed_process_data_in->draw_context;
    draw_context_in->current->modify_over->x = x;
    draw_context_in->current->modify_under->x = x;
    draw_context_in->current->modify_over->y = y_in;
    draw_context_in->current->modify_under->y = y_in+(height/2)+2;
    draw_context_in->drawable = control_flow_data->drawing->pixmap;
    draw_context_in->pango_layout = control_flow_data->drawing->pango_layout;
    widget = control_flow_data->drawing->drawing_area;
    //draw_context_in->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
    //draw_context_in->gc = widget->style->black_gc;
    //draw_context_in->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
    //gdk_gc_copy(draw_context_in->gc, widget->style->black_gc);
    
    //draw_arc((void*)&prop_arc, (void*)draw_context_in);
    //test_draw_item(control_flow_data->drawing, control_flow_data->drawing->pixmap);
      
    /* Draw the line/bg of the in process */
    if(draw_context_in->previous->middle->x == -1)
    {
      draw_context_in->previous->over->x =
                            control_flow_data->drawing->damage_begin;
      draw_context_in->previous->middle->x = 
                            control_flow_data->drawing->damage_begin;
      draw_context_in->previous->under->x =
                            control_flow_data->drawing->damage_begin;

      g_debug("in middle x_beg : %u",control_flow_data->drawing->damage_begin);

    }
  
    draw_context_in->current->middle->x = x;
    draw_context_in->current->over->x = x;
    draw_context_in->current->under->x = x;
    draw_context_in->current->middle->y = y_in + height/2;
    draw_context_in->current->over->y = y_in;
    draw_context_in->current->under->y = y_in + height;
    draw_context_in->previous->middle->y = y_in + height/2;
    draw_context_in->previous->over->y = y_in;
    draw_context_in->previous->under->y = y_in + height;
    
    draw_context_in->drawable = control_flow_data->drawing->pixmap;
    draw_context_in->pango_layout = control_flow_data->drawing->pango_layout;
 

    if(process_in->state->s == LTTV_STATE_RUN)
    {
      draw_context_in->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
      gdk_gc_copy(draw_context_in->gc, widget->style->black_gc);

      PropertiesBG prop_bg;
      prop_bg.color = g_new(GdkColor,1);
        
      switch(tfc->index) {
        case 0:
          prop_bg.color->red = 0x1515;
          prop_bg.color->green = 0x1515;
          prop_bg.color->blue = 0x8c8c;
          break;
        case 1:
          prop_bg.color->red = 0x4e4e;
          prop_bg.color->green = 0xa9a9;
          prop_bg.color->blue = 0xa4a4;
          break;
        case 2:
          prop_bg.color->red = 0x7a7a;
          prop_bg.color->green = 0x4a4a;
          prop_bg.color->blue = 0x8b8b;
          break;
        case 3:
          prop_bg.color->red = 0x8080;
          prop_bg.color->green = 0x7777;
          prop_bg.color->blue = 0x4747;
          break;
        default:
          prop_bg.color->red = 0xe7e7;
          prop_bg.color->green = 0xe7e7;
          prop_bg.color->blue = 0xe7e7;
      }
      

      draw_bg((void*)&prop_bg, (void*)draw_context_in);
      g_free(prop_bg.color);
      gdk_gc_unref(draw_context_in->gc);
    }

    draw_context_in->gc = widget->style->black_gc;

    GdkColor colorfg_in = { 0, 0x0000, 0xffff, 0x0000 };
    GdkColor colorbg_in = { 0, 0x0000, 0x0000, 0x0000 };
    PropertiesText prop_text_in;
    prop_text_in.foreground = &colorfg_in;
    prop_text_in.background = &colorbg_in;
    prop_text_in.size = 6;
    prop_text_in.position = OVER;

    g_debug("in state : %s", g_quark_to_string(process_in->state->s));
    /* foreground of text : status of the process */
    if(process_in->state->s == LTTV_STATE_UNNAMED)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_text_in.foreground->red = 0x0fff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xfff0;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_EXIT)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0x0000;
      prop_text_in.foreground->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0x0000;
      prop_text_in.foreground->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_RUN)
    {
      prop_text_in.foreground->red = 0x0000;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0x0000;
    }
    else
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xffff;
    }
  


    /* Print status of the process : U, WF, WC, E, W, R */
    if(process_in->state->s == LTTV_STATE_UNNAMED)
      prop_text_in.text = "U->";
    else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
      prop_text_in.text = "WF->";
    else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
      prop_text_in.text = "WC->";
    else if(process_in->state->s == LTTV_STATE_EXIT)
      prop_text_in.text = "E->";
    else if(process_in->state->s == LTTV_STATE_WAIT)
      prop_text_in.text = "W->";
    else if(process_in->state->s == LTTV_STATE_RUN)
      prop_text_in.text = "R->";
    else
      prop_text_in.text = "U";
    
    draw_text((void*)&prop_text_in, (void*)draw_context_in);
    //gdk_gc_unref(draw_context_in->gc);
   
    draw_context_in->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
    gdk_gc_copy(draw_context_in->gc, widget->style->black_gc);

    PropertiesLine prop_line_in;
    prop_line_in.color = g_new(GdkColor,1);
    prop_line_in.line_width = 2;
    prop_line_in.style = GDK_LINE_SOLID;
    prop_line_in.position = MIDDLE;

    /* color of line : status of the process */
    if(process_in->state->s == LTTV_STATE_UNNAMED)
    {
      prop_line_in.color->red = 0xffff;
      prop_line_in.color->green = 0xffff;
      prop_line_in.color->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_line_in.color->red = 0x0fff;
      prop_line_in.color->green = 0xffff;
      prop_line_in.color->blue = 0xfff0;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_line_in.color->red = 0xffff;
      prop_line_in.color->green = 0xffff;
      prop_line_in.color->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_EXIT)
    {
      prop_line_in.color->red = 0xffff;
      prop_line_in.color->green = 0x0000;
      prop_line_in.color->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT)
    {
      prop_line_in.color->red = 0xffff;
      prop_line_in.color->green = 0x0000;
      prop_line_in.color->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_RUN)
    {
      prop_line_in.color->red = 0x0000;
      prop_line_in.color->green = 0xffff;
      prop_line_in.color->blue = 0x0000;
    }
    else
    {
      prop_line_in.color->red = 0xffff;
      prop_line_in.color->green = 0xffff;
      prop_line_in.color->blue = 0xffff;
    }
  
    draw_line((void*)&prop_line_in, (void*)draw_context_in);
    g_free(prop_line_in.color);
    gdk_gc_unref(draw_context_in->gc);
  }

  return 0;

  /* Temp dump */
#ifdef DONTSHOW
  GString *string = g_string_new("");;
  gboolean field_names = TRUE, state = TRUE;

  lttv_event_to_string(e, tfc->tf, string, TRUE, field_names, tfs);
  g_string_append_printf(string,"\n");  

  if(state) {
    g_string_append_printf(string, " %s",
        g_quark_to_string(tfs->process->state->s));
  }

  g_info("%s",string->str);

  g_string_free(string, TRUE);
  
  /* End of text dump */
#endif //DONTSHOW

}


int draw_after_hook(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;

  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;

  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  LttvTraceState *ts =(LttvTraceState *)LTTV_TRACEFILE_CONTEXT(tfs)->t_context;

  
  LttEvent *e;
  e = tfc->e;

  LttTime evtime = ltt_event_time(e);
  TimeWindow time_window = 
    lttvwindow_get_time_window(control_flow_data->tab);

  LttTime end_time = ltt_time_add(time_window.start_time,
                                    time_window.time_width);
  //if(time < time_beg || time > time_end) return;
  if(ltt_time_compare(evtime, time_window.start_time) == -1
        || ltt_time_compare(evtime, end_time) == 1)
            return;


  if(strcmp(ltt_eventtype_name(ltt_event_eventtype(e)),"schedchange") == 0)
  {
    g_debug("schedchange!");
    
    /* Add process to process list (if not present) and get drawing "y" from
     * process position */
    guint pid_out, pid_in;
    LttvProcessState *process_out, *process_in;
    LttTime birth;
    guint y_in = 0, y_out = 0, height = 0, pl_height = 0;

    ProcessList *process_list =
      guicontrolflow_get_process_list(control_flow_data);


    LttField *f = ltt_event_field(e);
    LttField *element;
    element = ltt_field_member(f,0);
    pid_out = ltt_event_get_long_unsigned(e,element);
    element = ltt_field_member(f,1);
    pid_in = ltt_event_get_long_unsigned(e,element);
    //g_debug("out : %u  in : %u", pid_out, pid_in);


    /* Find process pid_out in the list... */
    process_out = lttv_state_find_process(tfs, pid_out);
    if(process_out == NULL) return 0;
    //g_debug("out : %s",g_quark_to_string(process_out->state->s));

    birth = process_out->creation_time;
    gchar *name = strdup(g_quark_to_string(process_out->name));
    HashedProcessData *hashed_process_data_out = NULL;

    if(processlist_get_process_pixels(process_list,
            pid_out,
            &birth,
            tfc->t_context->index,
            &y_out,
            &height,
            &hashed_process_data_out) == 1)
    {
    /* Process not present */
    processlist_add(process_list,
        pid_out,
        &birth,
        tfc->t_context->index,
        name,
        &pl_height,
        &hashed_process_data_out);
    processlist_get_process_pixels(process_list,
            pid_out,
            &birth,
            tfc->t_context->index,
            &y_out,
            &height,
            &hashed_process_data_out);
    drawing_insert_square( control_flow_data->drawing, y_out, height);
    }

    g_free(name);
    
    /* Find process pid_in in the list... */
    process_in = lttv_state_find_process(tfs, pid_in);
    if(process_in == NULL) return 0;
    //g_debug("in : %s",g_quark_to_string(process_in->state->s));

    birth = process_in->creation_time;
    name = strdup(g_quark_to_string(process_in->name));
    HashedProcessData *hashed_process_data_in = NULL;

    if(processlist_get_process_pixels(process_list,
            pid_in,
            &birth,
            tfc->t_context->index,
            &y_in,
            &height,
            &hashed_process_data_in) == 1)
    {
    /* Process not present */
      processlist_add(process_list,
        pid_in,
        &birth,
        tfc->t_context->index,
        name,
        &pl_height,
        &hashed_process_data_in);
      processlist_get_process_pixels(process_list,
            pid_in,
            &birth,
            tfc->t_context->index,
            &y_in,
            &height,
            &hashed_process_data_in);

      drawing_insert_square( control_flow_data->drawing, y_in, height);
    }
    g_free(name);


    /* Find pixels corresponding to time of the event. If the time does
     * not fit in the window, show a warning, not supposed to happend. */
    //guint x = 0;
    //guint width = control_flow_data->drawing->drawing_area->allocation.width;

    //LttTime time = ltt_event_time(e);

    //LttTime window_end = ltt_time_add(time_window->time_width,
    //                      time_window->start_time);

    
    //convert_time_to_pixels(
    //    time_window->start_time,
    //    window_end,
    //    time,
    //    width,
    //    &x);
    
    //assert(x <= width);
    
    /* draw what represents the event for outgoing process. */

    DrawContext *draw_context_out = hashed_process_data_out->draw_context;
    //draw_context_out->current->modify_over->x = x;
    draw_context_out->current->modify_over->y = y_out;
    draw_context_out->current->modify_under->y = y_out+(height/2)+2;
    draw_context_out->drawable = control_flow_data->drawing->pixmap;
    draw_context_out->pango_layout = control_flow_data->drawing->pango_layout;
    GtkWidget *widget = control_flow_data->drawing->drawing_area;
    //draw_context_out->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
 
    //draw_arc((void*)&prop_arc, (void*)draw_context_out);
    //test_draw_item(control_flow_data->drawing, control_flow_data->drawing->pixmap);
     
    /*if(process_out->state->s == LTTV_STATE_RUN)
    {
      draw_context_out->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
      gdk_gc_copy(draw_context_out->gc, widget->style->black_gc);
      PropertiesBG prop_bg;
      prop_bg.color = g_new(GdkColor,1);
      
      prop_bg.color->red = 0xffff;
      prop_bg.color->green = 0xffff;
      prop_bg.color->blue = 0xffff;
      
      draw_bg((void*)&prop_bg, (void*)draw_context_out);
      g_free(prop_bg.color);
      gdk_gc_unref(draw_context_out->gc);
    }*/

    draw_context_out->gc = widget->style->black_gc;

    GdkColor colorfg_out = { 0, 0xffff, 0x0000, 0x0000 };
    GdkColor colorbg_out = { 0, 0x0000, 0x0000, 0x0000 };
    PropertiesText prop_text_out;
    prop_text_out.foreground = &colorfg_out;
    prop_text_out.background = &colorbg_out;
    prop_text_out.size = 6;
    prop_text_out.position = OVER;

    /* color of text : status of the process */
    if(process_out->state->s == LTTV_STATE_UNNAMED)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_text_out.foreground->red = 0x0fff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xfff0;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_EXIT)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0x0000;
      prop_text_out.foreground->blue = 0xffff;
    }
    else if(process_out->state->s == LTTV_STATE_WAIT)
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0x0000;
      prop_text_out.foreground->blue = 0x0000;
    }
    else if(process_out->state->s == LTTV_STATE_RUN)
    {
      prop_text_out.foreground->red = 0x0000;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0x0000;
    }
    else
    {
      prop_text_out.foreground->red = 0xffff;
      prop_text_out.foreground->green = 0xffff;
      prop_text_out.foreground->blue = 0xffff;
    }
 
    /* Print status of the process : U, WF, WC, E, W, R */
    if(process_out->state->s == LTTV_STATE_UNNAMED)
      prop_text_out.text = "U";
    else if(process_out->state->s == LTTV_STATE_WAIT_FORK)
      prop_text_out.text = "WF";
    else if(process_out->state->s == LTTV_STATE_WAIT_CPU)
      prop_text_out.text = "WC";
    else if(process_out->state->s == LTTV_STATE_EXIT)
      prop_text_out.text = "E";
    else if(process_out->state->s == LTTV_STATE_WAIT)
      prop_text_out.text = "W";
    else if(process_out->state->s == LTTV_STATE_RUN)
      prop_text_out.text = "R";
    else
      prop_text_out.text = "U";
    
    draw_text((void*)&prop_text_out, (void*)draw_context_out);

    //gdk_gc_unref(draw_context_out->gc);
 
    draw_context_out->current->middle->y = y_out+height/2;
    draw_context_out->current->over->y = y_out;
    draw_context_out->current->under->y = y_out+height;
    draw_context_out->current->status = process_out->state->s;
    
    /* for pid_out : remove previous, Prev = current, new current (default) */
    g_free(draw_context_out->previous->modify_under);
    g_free(draw_context_out->previous->modify_middle);
    g_free(draw_context_out->previous->modify_over);
    g_free(draw_context_out->previous->under);
    g_free(draw_context_out->previous->middle);
    g_free(draw_context_out->previous->over);
    g_free(draw_context_out->previous);

    draw_context_out->previous = draw_context_out->current;
    
    draw_context_out->current = g_new(DrawInfo,1);
    draw_context_out->current->over = g_new(ItemInfo,1);
    draw_context_out->current->over->x = -1;
    draw_context_out->current->over->y = -1;
    draw_context_out->current->middle = g_new(ItemInfo,1);
    draw_context_out->current->middle->x = -1;
    draw_context_out->current->middle->y = -1;
    draw_context_out->current->under = g_new(ItemInfo,1);
    draw_context_out->current->under->x = -1;
    draw_context_out->current->under->y = -1;
    draw_context_out->current->modify_over = g_new(ItemInfo,1);
    draw_context_out->current->modify_over->x = -1;
    draw_context_out->current->modify_over->y = -1;
    draw_context_out->current->modify_middle = g_new(ItemInfo,1);
    draw_context_out->current->modify_middle->x = -1;
    draw_context_out->current->modify_middle->y = -1;
    draw_context_out->current->modify_under = g_new(ItemInfo,1);
    draw_context_out->current->modify_under->x = -1;
    draw_context_out->current->modify_under->y = -1;
    draw_context_out->current->status = LTTV_STATE_UNNAMED;
      
    /* Finally, update the drawing context of the pid_in. */

    DrawContext *draw_context_in = hashed_process_data_in->draw_context;
    //draw_context_in->current->modify_over->x = x;
    draw_context_in->current->modify_over->y = y_in;
    draw_context_in->current->modify_under->y = y_in+(height/2)+2;
    draw_context_in->drawable = control_flow_data->drawing->pixmap;
    draw_context_in->pango_layout = control_flow_data->drawing->pango_layout;
    widget = control_flow_data->drawing->drawing_area;
    //draw_context_in->gc = widget->style->fg_gc[GTK_WIDGET_STATE (widget)];
    
    //draw_arc((void*)&prop_arc, (void*)draw_context_in);
    //test_draw_item(control_flow_data->drawing, control_flow_data->drawing->pixmap);
     
    /*if(process_in->state->s == LTTV_STATE_RUN)
    {
      draw_context_in->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
      gdk_gc_copy(draw_context_in->gc, widget->style->black_gc);
      PropertiesBG prop_bg;
      prop_bg.color = g_new(GdkColor,1);
      
      prop_bg.color->red = 0xffff;
      prop_bg.color->green = 0xffff;
      prop_bg.color->blue = 0xffff;
      
      draw_bg((void*)&prop_bg, (void*)draw_context_in);
      g_free(prop_bg.color);
      gdk_gc_unref(draw_context_in->gc);
    }*/

    draw_context_in->gc = widget->style->black_gc;

    GdkColor colorfg_in = { 0, 0x0000, 0xffff, 0x0000 };
    GdkColor colorbg_in = { 0, 0x0000, 0x0000, 0x0000 };
    PropertiesText prop_text_in;
    prop_text_in.foreground = &colorfg_in;
    prop_text_in.background = &colorbg_in;
    prop_text_in.size = 6;
    prop_text_in.position = OVER;

    /* foreground of text : status of the process */
    if(process_in->state->s == LTTV_STATE_UNNAMED)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_text_in.foreground->red = 0x0fff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xfff0;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_EXIT)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0x0000;
      prop_text_in.foreground->blue = 0xffff;
    }
    else if(process_in->state->s == LTTV_STATE_WAIT)
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0x0000;
      prop_text_in.foreground->blue = 0x0000;
    }
    else if(process_in->state->s == LTTV_STATE_RUN)
    {
      prop_text_in.foreground->red = 0x0000;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0x0000;
    }
    else
    {
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xffff;
    }
  

    /* Print status of the process : U, WF, WC, E, W, R */
    if(process_in->state->s == LTTV_STATE_UNNAMED)
      prop_text_in.text = "U";
    else if(process_in->state->s == LTTV_STATE_WAIT_FORK)
      prop_text_in.text = "WF";
    else if(process_in->state->s == LTTV_STATE_WAIT_CPU)
      prop_text_in.text = "WC";
    else if(process_in->state->s == LTTV_STATE_EXIT)
      prop_text_in.text = "E";
    else if(process_in->state->s == LTTV_STATE_WAIT)
      prop_text_in.text = "W";
    else if(process_in->state->s == LTTV_STATE_RUN)
      prop_text_in.text = "R";
    else
      prop_text_in.text = "U";
    
    draw_text((void*)&prop_text_in, (void*)draw_context_in);
    

    if(process_in->state->s == LTTV_STATE_RUN)
    { 
      gchar tmp[255];
      prop_text_in.foreground = &colorfg_in;
      prop_text_in.background = &colorbg_in;
      prop_text_in.foreground->red = 0xffff;
      prop_text_in.foreground->green = 0xffff;
      prop_text_in.foreground->blue = 0xffff;
      prop_text_in.size = 6;
      prop_text_in.position = UNDER;

      prop_text_in.text = g_new(gchar, 260);
      strcpy(prop_text_in.text, "CPU ");
      snprintf(tmp, 255, "%u", tfc->index);
      strcat(prop_text_in.text, tmp);

      draw_text((void*)&prop_text_in, (void*)draw_context_in);
      g_free(prop_text_in.text);
    }

    
    draw_context_in->current->middle->y = y_in+height/2;
    draw_context_in->current->over->y = y_in;
    draw_context_in->current->under->y = y_in+height;
    draw_context_in->current->status = process_in->state->s;

    /* for pid_in : remove previous, Prev = current, new current (default) */
    g_free(draw_context_in->previous->modify_under);
    g_free(draw_context_in->previous->modify_middle);
    g_free(draw_context_in->previous->modify_over);
    g_free(draw_context_in->previous->under);
    g_free(draw_context_in->previous->middle);
    g_free(draw_context_in->previous->over);
    g_free(draw_context_in->previous);

    draw_context_in->previous = draw_context_in->current;
    
    draw_context_in->current = g_new(DrawInfo,1);
    draw_context_in->current->over = g_new(ItemInfo,1);
    draw_context_in->current->over->x = -1;
    draw_context_in->current->over->y = -1;
    draw_context_in->current->middle = g_new(ItemInfo,1);
    draw_context_in->current->middle->x = -1;
    draw_context_in->current->middle->y = -1;
    draw_context_in->current->under = g_new(ItemInfo,1);
    draw_context_in->current->under->x = -1;
    draw_context_in->current->under->y = -1;
    draw_context_in->current->modify_over = g_new(ItemInfo,1);
    draw_context_in->current->modify_over->x = -1;
    draw_context_in->current->modify_over->y = -1;
    draw_context_in->current->modify_middle = g_new(ItemInfo,1);
    draw_context_in->current->modify_middle->x = -1;
    draw_context_in->current->modify_middle->y = -1;
    draw_context_in->current->modify_under = g_new(ItemInfo,1);
    draw_context_in->current->modify_under->x = -1;
    draw_context_in->current->modify_under->y = -1;
    draw_context_in->current->status = LTTV_STATE_UNNAMED;
  
  }

  return 0;
}




gint update_time_window_hook(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;

  const TimeWindowNotifyData *time_window_nofify_data = 
                          ((const TimeWindowNotifyData *)call_data);

  TimeWindow *old_time_window = 
    time_window_nofify_data->old_time_window;
  TimeWindow *new_time_window = 
    time_window_nofify_data->new_time_window;
  
  /* Update the ruler */
  drawing_update_ruler(control_flow_data->drawing,
                       new_time_window);


  /* Two cases : zoom in/out or scrolling */
  
  /* In order to make sure we can reuse the old drawing, the scale must
   * be the same and the new time interval being partly located in the
   * currently shown time interval. (reuse is only for scrolling)
   */

  g_info("Old time window HOOK : %u, %u to %u, %u",
      old_time_window->start_time.tv_sec,
      old_time_window->start_time.tv_nsec,
      old_time_window->time_width.tv_sec,
      old_time_window->time_width.tv_nsec);

  g_info("New time window HOOK : %u, %u to %u, %u",
      new_time_window->start_time.tv_sec,
      new_time_window->start_time.tv_nsec,
      new_time_window->time_width.tv_sec,
      new_time_window->time_width.tv_nsec);

  if( new_time_window->time_width.tv_sec == old_time_window->time_width.tv_sec
  && new_time_window->time_width.tv_nsec == old_time_window->time_width.tv_nsec)
  {
    /* Same scale (scrolling) */
    g_info("scrolling");
    LttTime *ns = &new_time_window->start_time;
    LttTime *os = &old_time_window->start_time;
    LttTime old_end = ltt_time_add(old_time_window->start_time,
                                    old_time_window->time_width);
    LttTime new_end = ltt_time_add(new_time_window->start_time,
                                    new_time_window->time_width);
    //if(ns<os+w<ns+w)
    //if(ns<os+w && os+w<ns+w)
    //if(ns<old_end && os<ns)
    if(ltt_time_compare(*ns, old_end) == -1
        && ltt_time_compare(*os, *ns) == -1)
    {
      g_info("scrolling near right");
      /* Scroll right, keep right part of the screen */
      guint x = 0;
      guint width = control_flow_data->drawing->drawing_area->allocation.width;
      convert_time_to_pixels(
          *os,
          old_end,
          *ns,
          width,
          &x);

      /* Copy old data to new location */
      gdk_draw_drawable (control_flow_data->drawing->pixmap,
          control_flow_data->drawing->drawing_area->style->black_gc,
          control_flow_data->drawing->pixmap,
          x, 0,
          0, 0,
         control_flow_data->drawing->width-x+SAFETY, -1);
 
      if(drawing->damage_begin == drawing->damage_end)
        drawing->damage_begin = control_flow_data->drawing->width-x;
      else
        drawing->damage_begin = 0;

      drawing->damage_end = control_flow_data->drawing->width;

      /* Clear the data request background, but not SAFETY */
      gdk_draw_rectangle (control_flow_data->drawing->pixmap,
          //control_flow_data->drawing->drawing_area->style->black_gc,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin+SAFETY, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          control_flow_data->drawing->height+SAFETY);

       gtk_widget_queue_draw_area (drawing->drawing_area,
                                0,0,
                                control_flow_data->drawing->width,
                                control_flow_data->drawing->height);

      /* Get new data for the rest. */
      drawing_data_request(control_flow_data->drawing,
          &control_flow_data->drawing->pixmap,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,
          control_flow_data->drawing->height);
    } else { 
      //if(ns<os<ns+w)
      //if(ns<os && os<ns+w)
      //if(ns<os && os<new_end)
      if(ltt_time_compare(*ns,*os) == -1
          && ltt_time_compare(*os,new_end) == -1)
      {
        g_info("scrolling near left");
        /* Scroll left, keep left part of the screen */
        guint x = 0;
        guint width = control_flow_data->drawing->drawing_area->allocation.width;
        convert_time_to_pixels(
            *ns,
            new_end,
            *os,
            width,
            &x);
        

        /* Copy old data to new location */
        gdk_draw_drawable (control_flow_data->drawing->pixmap,
            control_flow_data->drawing->drawing_area->style->black_gc,
            control_flow_data->drawing->pixmap,
            0, 0,
            x, 0,
            -1, -1);
  
        *old_time_window = *new_time_window;

        if(drawing->damage_begin == drawing->damage_end)
          drawing->damage_end = x;
        else
          drawing->damage_end = 
            control_flow_data->drawing->drawing_area->allocation.width;

        drawing->damage_begin = 0;
        
        gdk_draw_rectangle (control_flow_data->drawing->pixmap,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          drawing->damage_begin, 0,
          drawing->damage_end - drawing->damage_begin,  // do not overlap
          control_flow_data->drawing->height+SAFETY);

        gtk_widget_queue_draw_area (drawing->drawing_area,
                                0,0,
                                control_flow_data->drawing->width,
                                control_flow_data->drawing->height);


        /* Get new data for the rest. */
        drawing_data_request(control_flow_data->drawing,
            &control_flow_data->drawing->pixmap,
            drawing->damage_begin, 0,
            drawing->damage_end - drawing->damage_begin,
            control_flow_data->drawing->height);
    
      } else {
        if(ltt_time_compare(*ns,*os) == 0)
        {
          g_info("not scrolling");
        } else {
          g_info("scrolling far");
          /* Cannot reuse any part of the screen : far jump */
          
          
          gdk_draw_rectangle (control_flow_data->drawing->pixmap,
            control_flow_data->drawing->drawing_area->style->black_gc,
            TRUE,
            0, 0,
            control_flow_data->drawing->width+SAFETY, // do not overlap
            control_flow_data->drawing->height+SAFETY);

          gtk_widget_queue_draw_area (drawing->drawing_area,
                                0,0,
                                control_flow_data->drawing->width,
                                control_flow_data->drawing->height);

          drawing->damage_begin = 0;
          drawing->damage_end = control_flow_data->drawing->width;

          drawing_data_request(control_flow_data->drawing,
              &control_flow_data->drawing->pixmap,
              0, 0,
              control_flow_data->drawing->width,
              control_flow_data->drawing->height);
      
        }
      }
    }
  } else {
    /* Different scale (zoom) */
    g_info("zoom");

    gdk_draw_rectangle (control_flow_data->drawing->pixmap,
          control_flow_data->drawing->drawing_area->style->black_gc,
          TRUE,
          0, 0,
          control_flow_data->drawing->width+SAFETY, // do not overlap
          control_flow_data->drawing->height+SAFETY);

    gtk_widget_queue_draw_area (drawing->drawing_area,
                                0,0,
                                control_flow_data->drawing->width,
                                control_flow_data->drawing->height);
  
    drawing->damage_begin = 0;
    drawing->damage_end = control_flow_data->drawing->width;

    drawing_data_request(control_flow_data->drawing,
        &control_flow_data->drawing->pixmap,
        0, 0,
        control_flow_data->drawing->width,
        control_flow_data->drawing->height);
  }


  
  return 0;
}

gint traceset_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  drawing->damage_begin = 0;
  drawing->damage_end = widget->allocation.width;


  // Clear the image
  gdk_draw_rectangle (drawing->pixmap,
        widget->style->black_gc,
        TRUE,
        0, 0,
        widget->allocation.width+SAFETY,
        widget->allocation.height+SAFETY);


  if(drawing->damage_begin < drawing->damage_end)
  {
    drawing_data_request(drawing,
                         &drawing->pixmap,
                         drawing->damage_begin,
                         0,
                         drawing->damage_end-drawing->damage_begin,
                         widget->allocation.height);
  }

  gtk_widget_queue_draw_area(drawing->drawing_area,
                             0,0,
                             drawing->width,
                             drawing->height);

  request_background_data(control_flow_data);
 
  return FALSE;
}

gint redraw_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  drawing->damage_begin = 0;
  drawing->damage_end = widget->allocation.width;


  // Clear the image
  gdk_draw_rectangle (drawing->pixmap,
        widget->style->black_gc,
        TRUE,
        0, 0,
        widget->allocation.width+SAFETY,
        widget->allocation.height+SAFETY);


  if(drawing->damage_begin < drawing->damage_end)
  {
    drawing_data_request(drawing,
                         &drawing->pixmap,
                         drawing->damage_begin,
                         0,
                         drawing->damage_end-drawing->damage_begin,
                         widget->allocation.height);
  }

  gtk_widget_queue_draw_area(drawing->drawing_area,
                             0,0,
                             drawing->width,
                             drawing->height);
 
  return FALSE;

}


gint continue_notify(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  Drawing_t *drawing = control_flow_data->drawing;
  GtkWidget *widget = drawing->drawing_area;

  //g_assert(widget->allocation.width == drawing->damage_end);

  if(drawing->damage_begin < drawing->damage_end)
  {
    drawing_data_request(drawing,
                         &drawing->pixmap,
                         drawing->damage_begin,
                         0,
                         drawing->damage_end-drawing->damage_begin,
                         widget->allocation.height);
  }

  return FALSE;
}


gint after_process_traceset_hook(void *hook_data, void *call_data)
{
  //ControlFlowData *control_flow_data = (ControlFlowData*) hook_data;
  EventsRequest *events_request = (EventsRequest *)hook_data;

  ControlFlowData *control_flow_data = 
                   (ControlFlowData*)events_request->viewer_data;


  drawing_data_request_end(events_request,
                           (LttvTracesetState*)call_data);
  return 0; 
}
 


gint update_current_time_hook(void *hook_data, void *call_data)
{
  ControlFlowData *control_flow_data = (ControlFlowData*)hook_data;
  Drawing_t *drawing = control_flow_data->drawing;

  LttTime current_time = *((LttTime*)call_data);
  
  TimeWindow time_window =
            lttvwindow_get_time_window(control_flow_data->tab);
  
  LttTime time_begin = time_window.start_time;
  LttTime width = time_window.time_width;
  LttTime half_width = ltt_time_div(width,2.0);
  LttTime time_end = ltt_time_add(time_begin, width);

  LttvTracesetContext * tsc =
        lttvwindow_get_traceset_context(control_flow_data->tab);
  
  LttTime trace_start = tsc->time_span.start_time;
  LttTime trace_end = tsc->time_span.end_time;
  
  g_info("New current time HOOK : %u, %u", current_time.tv_sec,
              current_time.tv_nsec);


  
  /* If current time is inside time interval, just move the highlight
   * bar */

  /* Else, we have to change the time interval. We have to tell it
   * to the main window. */
  /* The time interval change will take care of placing the current
   * time at the center of the visible area, or nearest possible if we are
   * at one end of the trace. */
  
  
  if(ltt_time_compare(current_time, time_begin) == -1)
  {
    TimeWindow new_time_window;

    if(ltt_time_compare(current_time,
          ltt_time_add(trace_start,half_width)) == -1)
      time_begin = trace_start;
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;

    lttvwindow_report_time_window(control_flow_data->tab, &new_time_window);
  }
  else if(ltt_time_compare(current_time, time_end) == 1)
  {
    TimeWindow new_time_window;

    if(ltt_time_compare(current_time, ltt_time_sub(trace_end, half_width)) == 1)
      time_begin = ltt_time_sub(trace_end,width);
    else
      time_begin = ltt_time_sub(current_time,half_width);
  
    new_time_window.start_time = time_begin;
    new_time_window.time_width = width;

    lttvwindow_report_time_window(control_flow_data->tab, &new_time_window);
    
  }
  //gtk_widget_queue_draw(control_flow_data->drawing->drawing_area);
  gtk_widget_queue_draw_area(drawing->drawing_area,
                             0,0,
                             drawing->width,
                             drawing->height);
  
  return 0;
}

typedef struct _ClosureData {
  EventsRequest *events_request;
  LttvTracesetState *tss;
} ClosureData;
  

void draw_closure(gpointer key, gpointer value, gpointer user_data)
{
  ProcessInfo *process_info = (ProcessInfo*)key;
  HashedProcessData *hashed_process_data = (HashedProcessData*)value;
  ClosureData *closure_data = (ClosureData*)user_data;
    
  ControlFlowData *control_flow_data =
    closure_data->events_request->viewer_data;
  
  GtkWidget *widget = control_flow_data->drawing->drawing_area;

  /* Get y position of process */
  gint y=0, height=0;
  
  processlist_get_pixels_from_data( control_flow_data->process_list,
          process_info,
          hashed_process_data,
          &y,
          &height);
  /* Get last state of process */
  LttvTraceContext *tc =
    ((LttvTracesetContext*)closure_data->tss)->traces[process_info->trace_num];
  LttvTracesetContext *tsc = (LttvTracesetContext *)closure_data->tss;

  LttvTraceState *ts = (LttvTraceState*)tc;
  LttvProcessState *process;

  /* We do not provide a cpu_name argument assuming that this is not the
     idle job (pid 0) and thus its pid is unique across all cpus */
  process = lttv_state_find_process_from_trace(ts, 0, process_info->pid);
  
  /* Draw the closing line */
  DrawContext *draw_context = hashed_process_data->draw_context;
  if(draw_context->previous->middle->x == -1)
  {
    draw_context->previous->over->x =
                          control_flow_data->drawing->damage_begin;
    draw_context->previous->middle->x = 
                          control_flow_data->drawing->damage_begin;
    draw_context->previous->under->x =
                          control_flow_data->drawing->damage_begin;

    g_debug("out middle x_beg : %u",control_flow_data->drawing->damage_begin);
  }

  /* Find pixels corresponding to current time . If the time does
   * not fit in the window, show a warning, not supposed to happend. */
  guint x = 0;
  guint width = control_flow_data->drawing->drawing_area->allocation.width;

  TimeWindow time_window =
            lttvwindow_get_time_window(control_flow_data->tab);

  LttTime time = lttv_traceset_context_get_current_tfc(tsc)->timestamp;

  LttTime window_end = ltt_time_add(time_window.time_width,
                        time_window.start_time);
  
  convert_time_to_pixels(
      time_window.start_time,
      window_end,
      time,
      width,
      &x);

  draw_context->current->middle->x = x;
  draw_context->current->over->x = x;
  draw_context->current->under->x = x;
  draw_context->current->middle->y = y + height/2;
  draw_context->current->over->y = y ;
  draw_context->current->under->y = y + height;
  draw_context->previous->middle->y = y + height/2;
  draw_context->previous->over->y = y ;
  draw_context->previous->under->y = y + height;
  draw_context->drawable = control_flow_data->drawing->pixmap;
  draw_context->pango_layout = control_flow_data->drawing->pango_layout;
  //draw_context->gc = widget->style->black_gc;
  draw_context->gc = gdk_gc_new(control_flow_data->drawing->pixmap);
  gdk_gc_copy(draw_context->gc, widget->style->black_gc);
 
  if(process != NULL && process->state->s == LTTV_STATE_RUN)
  {
    PropertiesBG prop_bg;
    prop_bg.color = g_new(GdkColor,1);

    /*switch(tfc->index) {
      case 0:
        prop_bg.color->red = 0x1515;
        prop_bg.color->green = 0x1515;
        prop_bg.color->blue = 0x8c8c;
        break;
      case 1:
        prop_bg.color->red = 0x4e4e;
        prop_bg.color->green = 0xa9a9;
        prop_bg.color->blue = 0xa4a4;
      break;
      case 2:
        prop_bg.color->red = 0x7a7a;
        prop_bg.color->green = 0x4a4a;
        prop_bg.color->blue = 0x8b8b;
        break;
      case 3:
        prop_bg.color->red = 0x8080;
        prop_bg.color->green = 0x7777;
        prop_bg.color->blue = 0x4747;
        break;
      default:
        prop_bg.color->red = 0xe7e7;
        prop_bg.color->green = 0xe7e7;
        prop_bg.color->blue = 0xe7e7;
    }
    */

    g_debug("calling from closure");
    //FIXME : I need the cpu number in process's state to draw this.
    //draw_bg((void*)&prop_bg, (void*)draw_context);
    g_free(prop_bg.color);
  }


  PropertiesLine prop_line;
  prop_line.color = g_new(GdkColor,1);
  prop_line.line_width = 2;
  prop_line.style = GDK_LINE_SOLID;
  prop_line.position = MIDDLE;

  /* color of line : status of the process */
  if(process != NULL)
  {
    if(process->state->s == LTTV_STATE_UNNAMED)
    {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0xffff;
    }
    else if(process->state->s == LTTV_STATE_WAIT_FORK)
    {
      prop_line.color->red = 0x0fff;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0xfff0;
    }
    else if(process->state->s == LTTV_STATE_WAIT_CPU)
    {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0x0000;
    }
    else if(process->state->s == LTTV_STATE_EXIT)
    {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0x0000;
      prop_line.color->blue = 0xffff;
    }
    else if(process->state->s == LTTV_STATE_WAIT)
    {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0x0000;
      prop_line.color->blue = 0x0000;
    }
    else if(process->state->s == LTTV_STATE_RUN)
    {
      prop_line.color->red = 0x0000;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0x0000;
    }
    else
    {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0xffff;
    }

  }
  else
  {
      prop_line.color->red = 0xffff;
      prop_line.color->green = 0xffff;
      prop_line.color->blue = 0xffff;
  }

  draw_line((void*)&prop_line, (void*)draw_context);
  g_free(prop_line.color);
  gdk_gc_unref(draw_context->gc);

  /* Reset draw_context of the process for next request */

  hashed_process_data->draw_context->drawable = NULL;
  hashed_process_data->draw_context->gc = NULL;
  hashed_process_data->draw_context->pango_layout = NULL;
  hashed_process_data->draw_context->current->over->x = -1;
  hashed_process_data->draw_context->current->over->y = -1;
  hashed_process_data->draw_context->current->middle->x = -1;
  hashed_process_data->draw_context->current->middle->y = -1;
  hashed_process_data->draw_context->current->under->x = -1;
  hashed_process_data->draw_context->current->under->y = -1;
  hashed_process_data->draw_context->current->modify_over->x = -1;
  hashed_process_data->draw_context->current->modify_over->y = -1;
  hashed_process_data->draw_context->current->modify_middle->x = -1;
  hashed_process_data->draw_context->current->modify_middle->y = -1;
  hashed_process_data->draw_context->current->modify_under->x = -1;
  hashed_process_data->draw_context->current->modify_under->y = -1;
  hashed_process_data->draw_context->current->status = LTTV_STATE_UNNAMED;
  hashed_process_data->draw_context->previous->over->x = -1;
  hashed_process_data->draw_context->previous->over->y = -1;
  hashed_process_data->draw_context->previous->middle->x = -1;
  hashed_process_data->draw_context->previous->middle->y = -1;
  hashed_process_data->draw_context->previous->under->x = -1;
  hashed_process_data->draw_context->previous->under->y = -1;
  hashed_process_data->draw_context->previous->modify_over->x = -1;
  hashed_process_data->draw_context->previous->modify_over->y = -1;
  hashed_process_data->draw_context->previous->modify_middle->x = -1;
  hashed_process_data->draw_context->previous->modify_middle->y = -1;
  hashed_process_data->draw_context->previous->modify_under->x = -1;
  hashed_process_data->draw_context->previous->modify_under->y = -1;
  hashed_process_data->draw_context->previous->status = LTTV_STATE_UNNAMED;

}

int  before_data_request(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  LttvTracesetState *tss = LTTV_TRACESET_STATE(call_data);

  drawing_data_request_begin(events_request, tss);

  return 0;
}


/*
 * for each process
 *    draw closing line
 *    new default prev and current
 * then finally remove reading hooks.
 */
int  after_data_request(void *hook_data, void *call_data)
{
  EventsRequest *events_request = (EventsRequest*)hook_data;
  ControlFlowData *control_flow_data = events_request->viewer_data;
  LttvTracesetState *tss = LTTV_TRACESET_STATE(call_data);
  
  ProcessList *process_list =
    guicontrolflow_get_process_list(control_flow_data);

  ClosureData closure_data;
  closure_data.events_request = (EventsRequest*)hook_data;
  closure_data.tss = tss;

  g_hash_table_foreach(process_list->process_hash, draw_closure,
                        (void*)&closure_data);

  /* Request expose */
  drawing_data_request_end(events_request, tss);

  return 0;
}

