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

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "cfv.h"
#include "drawing.h"
#include "processlist.h"
#include "eventhooks.h"
#include "cfv-private.h"
#include <lttv/lttv.h>

extern GSList *g_control_flow_data_list;

static void control_flow_grab_focus(GtkWidget *widget, gpointer data){
  ControlFlowData * control_flow_data = (ControlFlowData *)data;
  Tab * tab = control_flow_data->tab;
  lttvwindow_report_focus(tab, guicontrolflow_get_widget(control_flow_data));
  //g_assert(GTK_WIDGET_CAN_FOCUS(widget));
  //gtk_widget_grab_focus(widget);
  g_debug("FOCUS GRABBED");
}


/*****************************************************************************
 *                     Control Flow Viewer class implementation              *
 *****************************************************************************/
/**
 * Control Flow Viewer's constructor
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the drawing widget.
 * @param ParentWindow A pointer to the parent window.
 * @return The widget created.
 */
ControlFlowData *
guicontrolflow(void)
{
  GtkWidget *process_list_widget, *drawing_widget, *drawing_area;

  ControlFlowData* control_flow_data = g_new(ControlFlowData,1) ;

  control_flow_data->v_adjust = 
    GTK_ADJUSTMENT(gtk_adjustment_new(  0.0,  /* Value */
              0.0,  /* Lower */
              0.0,  /* Upper */
              0.0,  /* Step inc. */
              0.0,  /* Page inc. */
              0.0));  /* page size */

  /* Create the drawing */
  control_flow_data->drawing = drawing_construct(control_flow_data);
  
  drawing_widget = 
    drawing_get_widget(control_flow_data->drawing);
  
  drawing_area = 
    drawing_get_drawing_area(control_flow_data->drawing);

  control_flow_data->number_of_process = 0;

  /* Create the Process list */
  control_flow_data->process_list = processlist_construct();
  
  process_list_widget = 
    processlist_get_widget(control_flow_data->process_list);
  
  gtk_tree_view_set_vadjustment(GTK_TREE_VIEW(process_list_widget),
                                GTK_ADJUSTMENT(
                                   control_flow_data->v_adjust));

  
  control_flow_data->h_paned = gtk_hpaned_new();
  control_flow_data->box = gtk_event_box_new();
  control_flow_data->top_widget = control_flow_data->box;
  gtk_container_add(GTK_CONTAINER(control_flow_data->box),
                    control_flow_data->h_paned);
      
  gtk_paned_pack1(GTK_PANED(control_flow_data->h_paned),
                  process_list_widget, FALSE, TRUE);
  gtk_paned_pack2(GTK_PANED(control_flow_data->h_paned),
                  drawing_widget, TRUE, TRUE);
 
  /* Set the size of the drawing area */
  //drawing_Resize(drawing, h, w);

  /* Get trace statistics */
  //control_flow_data->Trace_Statistics = get_trace_statistics(Trace);

  gtk_widget_show(drawing_widget);
  gtk_widget_show(process_list_widget);
  gtk_widget_show(control_flow_data->h_paned);
  gtk_widget_show(control_flow_data->box);
  
  g_object_set_data_full(
      G_OBJECT(control_flow_data->top_widget),
      "control_flow_data",
      control_flow_data,
      (GDestroyNotify)guicontrolflow_destructor);
    
  g_object_set_data(
      G_OBJECT(drawing_area),
      "control_flow_data",
      control_flow_data);
        
  g_control_flow_data_list = g_slist_append(
      g_control_flow_data_list,
      control_flow_data);

  //WARNING : The widget must be 
  //inserted in the main window before the drawing area
  //can be configured (and this must happend bedore sending
  //data)

  //g_signal_connect (G_OBJECT (process_list_widget), "grab-focus",
  //g_signal_connect (G_OBJECT (control_flow_data->scrolled_window), 
  //      "button-press-event",
  //      G_CALLBACK (control_flow_grab_focus),
  //      control_flow_data);
  
  return control_flow_data;

}

/* Destroys widget also */
void
guicontrolflow_destructor_full(ControlFlowData *control_flow_data)
{
  g_info("CFV.c : guicontrolflow_destructor_full, %p", control_flow_data);
  /* May already have been done by GTK window closing */
  if(GTK_IS_WIDGET(guicontrolflow_get_widget(control_flow_data)))
    gtk_widget_destroy(guicontrolflow_get_widget(control_flow_data));
  //control_flow_data->mw = NULL;
  //FIXME guicontrolflow_destructor(control_flow_data);
}

/* When this destructor is called, the widgets are already disconnected */
void
guicontrolflow_destructor(ControlFlowData *control_flow_data)
{
  guint index;
  Tab *tab = control_flow_data->tab;
  
  g_info("CFV.c : guicontrolflow_destructor, %p", control_flow_data);
  g_info("%p, %p, %p", update_time_window_hook, control_flow_data, tab);
  if(GTK_IS_WIDGET(guicontrolflow_get_widget(control_flow_data)))
    g_info("widget still exists");
  
  /* Process List is removed with it's widget */
  //ProcessList_destroy(control_flow_data->process_list);
  if(tab != NULL)
  {
      /* Delete reading hooks */
    lttvwindow_unregister_traceset_notify(tab,
        traceset_notify,
        control_flow_data);
    
    lttvwindow_unregister_time_window_notify(tab,
        update_time_window_hook,
        control_flow_data);
  
    lttvwindow_unregister_current_time_notify(tab,
        update_current_time_hook,
        control_flow_data);

    lttvwindow_unregister_redraw_notify(tab, redraw_notify, control_flow_data);
    lttvwindow_unregister_continue_notify(tab,
                                          continue_notify,
                                          control_flow_data);
    
    lttvwindow_events_request_remove_all(control_flow_data->tab,
                                         control_flow_data);

    lttvwindowtraces_background_notify_remove(control_flow_data);
  }
  g_control_flow_data_list = 
         g_slist_remove(g_control_flow_data_list,control_flow_data);

  g_info("CFV.c : guicontrolflow_destructor end, %p", control_flow_data);
  g_free(control_flow_data);
 
}

GtkWidget *guicontrolflow_get_widget(ControlFlowData *control_flow_data)
{
  return control_flow_data->top_widget ;
}

ProcessList *guicontrolflow_get_process_list
    (ControlFlowData *control_flow_data)
{
    return control_flow_data->process_list ;
}


