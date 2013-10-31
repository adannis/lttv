/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Mathieu Desnoyers and XangXiu Yang
 *               2005 Mathieu Desnoyers
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


//*! \defgroup GuiEvents libGuiEvents: The GUI Events display plugin */
/*\@{*/

/*! \file GuiEvents.c
 * \brief Graphical plugin for showing events.
 *
 * This plugin lists all the events contained in the current time interval
 * in a list.
 * 
 * This plugin adds a Events Viewer functionnality to Linux TraceToolkit
 * GUI when this plugin is loaded. The init and destroy functions add the
 * viewer's insertion menu item and toolbar icon by calling viewer.h's
 * API functions. Then, when a viewer's object is created, the constructor
 * creates ans register through API functions what is needed to interact
 * with the lttvwindow.
 *
 * Authors : Mathieu Desnoyers and XangXiu Yang, June to December 2003
 *           Inspired from original LTT, made by Karim Yaghmour
 *
 *           Mostly rewritten by Mathieu Desnoyers, August 2005.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <string.h>

#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/traceset.h>
#include <lttv/state.h>
#ifdef BABEL_CLEANUP
#include <lttv/filter.h>
#endif //babel_cleanup
#include <lttv/print.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttvwindowtraces.h>
#include <lttvwindow/lttv_plugin_tab.h>
#include <lttvwindow/support.h>
#include "lttv_plugin_evd.h"

#include "events.h"
#include "hGuiEventsInsert.xpm"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)

#ifndef g_debug
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#endif

#define abs(a) (((a)<0)?(-a):(a))
#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_event_viewer_data_list = NULL ;

typedef enum _ScrollDirection{
  SCROLL_STEP_UP,
  SCROLL_STEP_DOWN,
  SCROLL_PAGE_UP,
  SCROLL_PAGE_DOWN,
  SCROLL_JUMP,
  SCROLL_NONE
} ScrollDirection;

/** hook functions for update time interval, current time ... */
gboolean update_current_time(void * hook_data, void * call_data);
gboolean update_current_position(void * hook_data, void * call_data);
//gboolean show_event_detail(void * hook_data, void * call_data);
gboolean traceset_changed(void * hook_data, void * call_data);
gboolean timespan_changed(void * hook_data, void * call_data);
#ifdef BABEL_CLEANUP
gboolean filter_changed(void * hook_data, void * call_data);
#endif // babel_cleanup
static void request_background_data(EventViewerData *event_viewer_data);

//! Event Viewer's constructor hook
GtkWidget *h_gui_events(LttvPlugin *plugin);
//! Event Viewer's constructor
EventViewerData *gui_events(LttvPluginTab *ptab);
//! Event Viewer's destructor
void gui_events_destructor(gpointer data);
void gui_events_free(gpointer data);

static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data);

void tree_v_set_cursor(EventViewerData *event_viewer_data);
void tree_v_get_cursor(EventViewerData *event_viewer_data);
static void redraw(EventViewerData *event_viewer_data);

/* Prototype for selection handler callback */
static void tree_selection_changed_cb (GtkTreeSelection *selection,
    gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
static void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc,
    gpointer data);
static void tree_v_size_request_cb (GtkWidget *widget,
    GtkRequisition *requisition, gpointer data);
static void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data);
static void tree_v_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1,
    gint arg2, gpointer data);
static void        filter_button      (GtkToolButton *toolbutton,
                                          gpointer       user_data);
static gboolean tree_v_scroll_handler (GtkWidget *widget, GdkEventScroll *event, gpointer data);
static gboolean key_handler(GtkWidget *widget, GdkEventKey *event,
    gpointer user_data);

static void adjust_event_viewer(double time, EventViewerData *event_viewer_data);

int event_hook(void *hook_data, void *call_data);

/* Enumeration of the columns */
enum
{
  TRACE_NAME_COLUMN,
  CPUID_COLUMN,
  EVENT_COLUMN,
  TIME_S_COLUMN,
  TIME_NS_COLUMN,
  PID_COLUMN,
  EVENT_DESCR_COLUMN,
  POSITION_COLUMN,
  N_COLUMNS
};

/**
 * Event Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_gui_events(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  EventViewerData* event_viewer_data = gui_events(ptab) ;
  if(event_viewer_data)
    return event_viewer_data->top_widget;
  else return NULL;
  
}

/**
 * Event Viewer's constructor
 *
 * This constructor is used to create EventViewerData data structure.
 * @return The Event viewer data created.
 */
EventViewerData *
gui_events(LttvPluginTab *ptab)
{
  /*To disable the error bell when we reach the end(or the top) of the list of
	 event. Since we overload the management of the list*/
  GtkSettings* gtk_settings = gtk_settings_get_default();
  g_object_set(gtk_settings, "gtk-error-bell", FALSE, NULL);


  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  EventViewerData* event_viewer_data = g_new(EventViewerData,1);
  LttvPluginEVD *plugin_evd = g_object_new(LTTV_TYPE_PLUGIN_EVD, NULL);
  GtkTooltips *tooltips = gtk_tooltips_new();
  plugin_evd->evd = event_viewer_data;
  Tab *tab = ptab->tab;
  event_viewer_data->tab = tab;
  event_viewer_data->ptab = ptab;
  GtkWidget *tmp_toolbar_icon;
  
  LttvTraceset *ts =lttvwindow_get_traceset(event_viewer_data->tab);

  event_viewer_data->event_hooks = lttv_hooks_new();
  lttv_hooks_add(event_viewer_data->event_hooks,
                 event_hook,
                 event_viewer_data,
                 LTTV_PRIO_DEFAULT);

  lttvwindow_register_current_time_notify(tab, 
                update_current_time,event_viewer_data);
  lttvwindow_register_current_position_notify(tab, 
                update_current_position,event_viewer_data);
  lttvwindow_register_traceset_notify(tab, 
                traceset_changed,event_viewer_data);
  lttvwindow_register_timespan_notify(tab, 
                timespan_changed,event_viewer_data);
#ifdef BABEL_CLEANUP 
  lttvwindow_register_filter_notify(tab,
                filter_changed, event_viewer_data);
#endif
  lttvwindow_register_redraw_notify(tab,
                evd_redraw_notify, event_viewer_data);
  event_viewer_data->scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (event_viewer_data->scroll_win);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(event_viewer_data->scroll_win), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

  event_viewer_data->currently_selected_position =
				lttv_traceset_create_current_position(ts);
  event_viewer_data->first_event =
				lttv_traceset_create_current_position(ts);
  event_viewer_data->last_event =
				lttv_traceset_create_current_position(ts);
  event_viewer_data->main_win_filter = lttvwindow_get_filter(tab);

  event_viewer_data->update_cursor = TRUE;
  event_viewer_data->report_position = TRUE;

  event_viewer_data->last_tree_update_time = 0;

  event_viewer_data->init_done = 0;

  /* Create a model for storing the data list */
  event_viewer_data->store_m = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_STRING,  /* Trace name                  */
    G_TYPE_UINT,    /* CPUID                       */
    G_TYPE_STRING,  /* Event                       */
    G_TYPE_UINT,    /* Time s                      */
    G_TYPE_UINT,    /* Time ns                     */
    G_TYPE_INT,     /* PID                         */
    G_TYPE_STRING,  /* Event's description         */
    G_TYPE_POINTER);/* Position (not shown)        */
  
  event_viewer_data->pos = g_ptr_array_sized_new(10);
  
  /* Create the viewer widget for the columned list */
  event_viewer_data->tree_v =
    gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->store_m));
    
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "size-allocate",
        G_CALLBACK (tree_v_size_allocate_cb),
        event_viewer_data);
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "size-request",
        G_CALLBACK (tree_v_size_request_cb),
        event_viewer_data);
  
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "cursor-changed",
        G_CALLBACK (tree_v_cursor_changed_cb),
        event_viewer_data);
  
  g_signal_connect (G_OBJECT (event_viewer_data->tree_v), "move-cursor",
        G_CALLBACK (tree_v_move_cursor_cb),
        event_viewer_data);

  g_signal_connect (G_OBJECT(event_viewer_data->tree_v), "key-press-event",
      G_CALLBACK(key_handler),
      event_viewer_data);

  g_signal_connect (G_OBJECT(event_viewer_data->tree_v), "scroll-event",
      G_CALLBACK(tree_v_scroll_handler),
      event_viewer_data);



  // Use on each column!
  //gtk_tree_view_column_set_sizing(event_viewer_data->tree_v,
  //GTK_TREE_VIEW_COLUMN_FIXED);
  
  /* The view now holds a reference.  We can get rid of our own
   * reference */
  g_object_unref (G_OBJECT (event_viewer_data->store_m));
  

  /* Create a column, associating the "text" attribute of the
   * cell_renderer to the first column of the model */
  /* Columns alignment : 0.0 : Left    0.5 : Center   1.0 : Right */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Trace",
                 renderer,
                 "text", TRACE_NAME_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);

  event_viewer_data->button = column->button;

  g_signal_connect (G_OBJECT(event_viewer_data->button),
        "size-allocate",
        G_CALLBACK(header_size_allocate),
        (gpointer)event_viewer_data);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPUID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event",
                 renderer,
                 "text", EVENT_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time (s)",
                 renderer,
                 "text", TIME_S_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Time (ns)",
                 renderer,
                 "text", TIME_NS_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 120);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);


  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("PID",
                 renderer,
                 "text", PID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);
  
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Event Description",
                 renderer,
                 "text", EVENT_DESCR_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->tree_v),
      column);


  /* Setup the selection handler */
  event_viewer_data->select_c =
    gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->tree_v));
  gtk_tree_selection_set_mode (event_viewer_data->select_c,
      GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (event_viewer_data->select_c), "changed",
        G_CALLBACK (tree_selection_changed_cb),
        event_viewer_data);
  
  gtk_container_add (GTK_CONTAINER (event_viewer_data->scroll_win),
      event_viewer_data->tree_v);

  event_viewer_data->hbox_v = gtk_hbox_new(0, 0);
  event_viewer_data->top_widget = event_viewer_data->hbox_v;
  plugin_evd->parent.top_widget = event_viewer_data->hbox_v;

  event_viewer_data->toolbar = gtk_toolbar_new();
  gtk_toolbar_set_orientation(GTK_TOOLBAR(event_viewer_data->toolbar),
                              GTK_ORIENTATION_VERTICAL);

  tmp_toolbar_icon = create_pixmap (main_window_get_widget(tab),
      "guifilter16x16.png");
  gtk_widget_show(tmp_toolbar_icon);
  event_viewer_data->button_filter = gtk_tool_button_new(tmp_toolbar_icon,
      "Filter");
  g_signal_connect (G_OBJECT(event_viewer_data->button_filter),
        "clicked",
        G_CALLBACK (filter_button),
        (gpointer)plugin_evd);
  gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(event_viewer_data->button_filter), 
      tooltips, "Open the filter window", NULL);
  gtk_toolbar_insert(GTK_TOOLBAR(event_viewer_data->toolbar),
      event_viewer_data->button_filter,
      0);
  gtk_toolbar_set_style(GTK_TOOLBAR(event_viewer_data->toolbar),
      GTK_TOOLBAR_ICONS);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v),
      event_viewer_data->toolbar, FALSE, FALSE, 0);
  event_viewer_data->filter = NULL;

  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v),
      event_viewer_data->scroll_win, TRUE, TRUE, 0);

  gtk_container_set_border_width(GTK_CONTAINER(event_viewer_data->hbox_v), 1);

  /* Create vertical scrollbar and pack it */
  event_viewer_data->vscroll_vc = gtk_vscrollbar_new(NULL);
  gtk_range_set_update_policy (GTK_RANGE(event_viewer_data->vscroll_vc),
             GTK_UPDATE_CONTINUOUS);
             // Changed by MD : more user friendly :)
             //GTK_UPDATE_DISCONTINUOUS);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->hbox_v),
      event_viewer_data->vscroll_vc, FALSE, TRUE, 0);
  
  /* Get the vertical scrollbar's adjustment */
  event_viewer_data->vadjust_c =
    gtk_range_get_adjustment(GTK_RANGE(event_viewer_data->vscroll_vc));
  event_viewer_data->vtree_adjust_c = gtk_tree_view_get_vadjustment(
                    GTK_TREE_VIEW (event_viewer_data->tree_v));
  
  g_signal_connect (G_OBJECT (event_viewer_data->vadjust_c), "value-changed",
        G_CALLBACK (v_scroll_cb),
        event_viewer_data);
  //TODO ybrosseau 2011-01-06: Fix comment
  /* Set the upper bound to the last event number */
  event_viewer_data->previous_value = 0;
  event_viewer_data->vadjust_c->lower = 0.0;
    //event_viewer_data->vadjust_c->upper = event_viewer_data->number_of_events;

  LttTime time = lttvwindow_get_current_time(tab);
  TimeInterval time_span = lttv_traceset_get_time_span_real(ts);
  time = ltt_time_sub(time, time_span.start_time);
  //TODO ybrosseau 2011-01-06: Which one do we keep?
#ifdef BABEL_CLEANUP
  event_viewer_data->vadjust_c->value = ltt_time_to_double(time);
#endif //babel_cleanup
  event_viewer_data->vadjust_c->value = 0.0;
  event_viewer_data->vadjust_c->step_increment = 1.0;
  event_viewer_data->vadjust_c->page_increment = 2.0;
    //  event_viewer_data->vtree_adjust_c->upper;
  event_viewer_data->vadjust_c->page_size = 2.0;
  //    event_viewer_data->vtree_adjust_c->upper;
  /*  Raw event trace */
  gtk_widget_show(GTK_WIDGET(event_viewer_data->button_filter));
  gtk_widget_show(event_viewer_data->toolbar);
  gtk_widget_show(event_viewer_data->hbox_v);
  gtk_widget_show(event_viewer_data->tree_v);
  gtk_widget_show(event_viewer_data->vscroll_vc);

  /* Add the object's information to the module's array */
  g_event_viewer_data_list = g_slist_append(g_event_viewer_data_list,
      plugin_evd);

  event_viewer_data->num_visible_events = 1;

  //get the life span of the traceset and set the upper of the scroll bar
  
  end = ltt_time_sub(time_span.end_time, time_span.start_time);

  event_viewer_data->vadjust_c->upper = ltt_time_to_double(end);

  /* Set the Selected Event */
  //  tree_v_set_cursor(event_viewer_data);

 // event_viewer_data->current_time_updated = FALSE;
 //
  g_object_set_data_full(
      G_OBJECT(event_viewer_data->hbox_v),
      "plugin_data",
      plugin_evd,
      (GDestroyNotify)gui_events_free);

  g_object_set_data(
      G_OBJECT(event_viewer_data->hbox_v),
      "event_viewer_data",
      event_viewer_data);
  
  event_viewer_data->background_info_waiting = 0;


  request_background_data(event_viewer_data);
  return event_viewer_data;
}



static gint background_ready(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData *)hook_data;

  event_viewer_data->background_info_waiting--;

  if(event_viewer_data->background_info_waiting == 0) {
    g_message("event viewer : background computation data ready.");

    evd_redraw_notify(event_viewer_data, NULL);
  }

  return 0;
}


static void request_background_data(EventViewerData *event_viewer_data)
{
  LttvTraceset * ts =
        lttvwindow_get_traceset(event_viewer_data->tab);
  gint num_traces = ts->traces->len;
  gint i;
  LttvTrace *trace;

  LttvHooks *background_ready_hook = 
    lttv_hooks_new();
  lttv_hooks_add(background_ready_hook, background_ready, event_viewer_data,
      LTTV_PRIO_DEFAULT);
  event_viewer_data->background_info_waiting = 0;
    
  for(i=0;i<num_traces;i++) {
    trace = g_ptr_array_index(ts->traces,i);
#ifdef BABEL_CLEANUP 
    tstate = LTTV_TRACE_STATE(tsc->traces[i]);
#endif /*babel_cleanup*/

    if(lttvwindowtraces_get_ready(g_quark_from_string("state"),trace)==FALSE
        /*&& !tstate->has_precomputed_states*/) {

      if(lttvwindowtraces_get_in_progress(g_quark_from_string("state"),
                                          trace) == FALSE) {
        /* We first remove requests that could have been done for the same
         * information. Happens when two viewers ask for it before servicing
         * starts.
         */
        if(!lttvwindowtraces_background_request_find(trace, "state"))
          lttvwindowtraces_background_request_queue(
              main_window_get_widget(event_viewer_data->tab), trace, "state");
        lttvwindowtraces_background_notify_queue(event_viewer_data,
                                                 trace,
                                                 ltt_time_infinite,
                                                 NULL,
                                                 background_ready_hook);
        event_viewer_data->background_info_waiting++;
      } else { /* in progress */
      
        lttvwindowtraces_background_notify_current(event_viewer_data,
                                                   trace,
                                                   ltt_time_infinite,
                                                   NULL,
                                                   background_ready_hook);
        event_viewer_data->background_info_waiting++;
      }
    } else {
      /* Data ready. By its nature, this viewer doesn't need to have
       * its data ready hook called there, because a background
       * request is always linked with a redraw.
       */
    }
    
  }

  lttv_hooks_destroy(background_ready_hook);
}

static gboolean
header_size_allocate(GtkWidget *widget,
                        GtkAllocation *allocation,
                        gpointer user_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)user_data;

  event_viewer_data->header_height = allocation->height;

  return 0;
}


void tree_v_set_cursor(EventViewerData *event_viewer_data)
{
  g_debug("set cursor cb - do nothing");

}

void tree_v_get_cursor(EventViewerData *event_viewer_data)
{
  g_debug("get cursor cb -  do nothing");
  

}

/* Filter out the key repeats that come too fast */
static gboolean key_handler(GtkWidget *widget, GdkEventKey *event,
    gpointer user_data)
{
  EventViewerData *evd = (EventViewerData *)user_data;
 
  g_debug("event time : %u , last time : %u", event->time,
      evd->last_tree_update_time);
  
  if(guint32_before(event->time, evd->last_tree_update_time))
    return TRUE;
  else
    return FALSE;
}

void tree_v_move_cursor_cb (GtkWidget *widget,
                            GtkMovementStep stepType,
                            gint arg2,
                            gpointer data)
{
       
  GtkTreePath *path; // = gtk_tree_path_new();
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  
  g_debug("move cursor cb");
  //gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
  //                          &path, NULL);
  //if(path == NULL)
  //{
    /* No prior cursor, put it at beginning of page
     * and let the execution do */
  //  path = gtk_tree_path_new_from_indices(0, -1);
  //  gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
  //                              path, NULL, FALSE);
  //}

  //indices = gtk_tree_path_get_indices(path);
  
  //value = gtk_adjustment_get_value(event_viewer_data->vadjust_c);
 
  /* If events request pending, do nothing*/
  if(lttvwindow_events_request_pending(event_viewer_data->tab)) return;
  
  /* If no prior position... */


  g_debug("tree view move cursor : stepType is %u and arg2 is %d",
      (guint)stepType, arg2);

  switch(stepType) {
  case GTK_MOVEMENT_DISPLAY_LINES:
    if(arg2 == 1) {
      /* Move one line down */
      LttvTracesetPosition *end_pos = 0;
      if(event_viewer_data->pos->len > 0) {
	end_pos = 
          (LttvTracesetPosition*)g_ptr_array_index(
                                             event_viewer_data->pos,
                                             event_viewer_data->pos->len-1);
      }
      if(!end_pos || lttv_traceset_position_compare(end_pos, 
              event_viewer_data->currently_selected_position) == 0) {
          /* Must get down one event and select the last one */
          gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(event_viewer_data->tree_v)));
					event_viewer_data->update_cursor = FALSE;
					gtk_adjustment_set_value(event_viewer_data->vadjust_c,
							gtk_adjustment_get_value(event_viewer_data->vadjust_c) + 1);
					event_viewer_data->update_cursor = TRUE;
					if(event_viewer_data->pos->len > 0) {
						path = gtk_tree_path_new_from_indices(
								max(0, event_viewer_data->pos->len - 1), -1);
						if(path) {
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
											 path, NULL, FALSE);
							gtk_tree_path_free(path);
						}
					}
				}
		
    } else {
      LttvTracesetPosition *begin_pos = 0;
      
      if(event_viewer_data->pos->len > 0) {
        /* Move one line up */
        begin_pos =  
          (LttvTracesetPosition*)g_ptr_array_index(
	    event_viewer_data->pos,
	    0);
      }
      if(!begin_pos || lttv_traceset_position_compare(begin_pos, 
              event_viewer_data->currently_selected_position) == 0) 


      {
	/* Must get up one event and select the first one */
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(
					  GTK_TREE_VIEW(event_viewer_data->tree_v)));
	event_viewer_data->update_cursor = FALSE;
	gtk_adjustment_set_value(event_viewer_data->vadjust_c,
				gtk_adjustment_get_value(event_viewer_data->vadjust_c) - 1);
	event_viewer_data->update_cursor = TRUE;
	if(event_viewer_data->pos->len > 0) {
	  path = gtk_tree_path_new_from_indices(
	    0, -1);
	  if(path) {
	    gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
				    path, NULL, FALSE);
	    gtk_tree_path_free(path);
	  }
	}
      }

    }
    break;
  case GTK_MOVEMENT_PAGES:
    if(arg2 == 1) {
      /* Move one page down */
      LttvTracesetPosition *end_pos = 0;
      if(event_viewer_data->pos->len > 0) {
        end_pos = 
          (LttvTracesetPosition*)g_ptr_array_index(
                                             event_viewer_data->pos,
                                             event_viewer_data->pos->len-1);
      }
        if(!end_pos || lttv_traceset_position_compare(end_pos, 
              event_viewer_data->currently_selected_position) == 0) {
          /* Must get down one page and select the last one */
          gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(event_viewer_data->tree_v)));
					event_viewer_data->update_cursor = FALSE;
					gtk_adjustment_set_value(event_viewer_data->vadjust_c,
							gtk_adjustment_get_value(event_viewer_data->vadjust_c) + 2);
					event_viewer_data->update_cursor = TRUE;
					if(event_viewer_data->pos->len > 0) {
						path = gtk_tree_path_new_from_indices(
								event_viewer_data->pos->len - 1, -1);
						if(path) {
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
																			 path, NULL, FALSE);
							gtk_tree_path_free(path);
						}
					}
				}
    } else {
      /* Move one page up */
      LttvTracesetPosition *begin_pos = 0;

      if(event_viewer_data->pos->len > 0) {
        begin_pos = 
          (LttvTracesetPosition*)g_ptr_array_index(
                                             event_viewer_data->pos,
                                             0);
      }
        if(!begin_pos || lttv_traceset_position_compare(begin_pos, 
              event_viewer_data->currently_selected_position) == 0) {
          /* Must get up one page and select the first one */
          gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(
                GTK_TREE_VIEW(event_viewer_data->tree_v)));
					event_viewer_data->update_cursor = FALSE;
					gtk_adjustment_set_value(event_viewer_data->vadjust_c,
							gtk_adjustment_get_value(event_viewer_data->vadjust_c) - 2);
					event_viewer_data->update_cursor = TRUE;
					if(event_viewer_data->pos->len > 0) {
						path = gtk_tree_path_new_from_indices(
								0, -1);
						if(path) {
							gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
																			 path, NULL, FALSE);
							gtk_tree_path_free(path);
						}
					}
				}
    }
    break;
  default:
    break;
  }
  //gtk_tree_path_free(path);

}

static void        filter_button      (GtkToolButton *toolbutton,
                                          gpointer       user_data)
{
  LttvPluginEVD *plugin_evd = (LttvPluginEVD*)user_data;
  LttvAttribute *attribute;
  LttvAttributeValue value;
  gboolean ret;
  g_printf("Filter button clicked\n");

  attribute = LTTV_ATTRIBUTE(lttv_iattribute_find_subdir(
        LTTV_IATTRIBUTE(lttv_global_attributes()),
        LTTV_VIEWER_CONSTRUCTORS));
  g_assert(attribute);

  ret = lttv_iattribute_find_by_path(LTTV_IATTRIBUTE(attribute),
      "guifilter", LTTV_POINTER, &value);
  g_assert(ret);
  lttvwindow_viewer_constructor constructor =
    (lttvwindow_viewer_constructor)*(value.v_pointer);
  if(constructor) constructor(&plugin_evd->parent);
  else g_warning("Filter module not loaded.");

  //FIXME : viewer returned.
}

gboolean tree_v_scroll_handler (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	EventViewerData *event_viewer_data = (EventViewerData*) data;

	switch(event->direction) {
		case GDK_SCROLL_UP:
			gtk_adjustment_set_value(event_viewer_data->vadjust_c,
				gtk_adjustment_get_value(event_viewer_data->vadjust_c) - 1);
			break;
		case GDK_SCROLL_DOWN:
			gtk_adjustment_set_value(event_viewer_data->vadjust_c,
				gtk_adjustment_get_value(event_viewer_data->vadjust_c) + 1);
			break;
		default:
			g_error("Only scroll up and down expected");
	}
	return TRUE;
}

void tree_v_cursor_changed_cb (GtkWidget *widget, gpointer data)
{
        
  EventViewerData *event_viewer_data = (EventViewerData*) data;
 
  Tab *tab = event_viewer_data->tab;
  GtkTreeIter iter;
  GtkTreeModel* model = GTK_TREE_MODEL(event_viewer_data->store_m);
  GtkTreePath *path;
  LttvTracesetPosition *pos;

  g_debug("cursor changed cb");
 
  /* On cursor change, modify the currently selected event by calling
   * the right API function */
  if(event_viewer_data->report_position) {
    if(event_viewer_data->pos->len > 0) {
			gtk_tree_view_get_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
					&path, NULL);
			if(path) {
				if(gtk_tree_model_get_iter(model,&iter,path)){
					gtk_tree_model_get(model, &iter, POSITION_COLUMN, &pos, -1);
					
					if(lttv_traceset_position_compare(pos, 
									    event_viewer_data->currently_selected_position) != 0) {
						lttvwindow_report_current_position(tab, pos);
					}
				}else{
					g_warning("Can not get iter\n");
				}
				gtk_tree_path_free(path);
			}
		}
	}
}


static void tree_selection_changed_cb (GtkTreeSelection *selection,
    gpointer data)
{
  g_debug("tree sel changed cb - do nothing");


}

#if 0
static gint key_snooper(GtkWidget *grab_widget, GdkEventKey *event,
    gpointer func_data)
{
  return TRUE;
}
#endif //0

/* This callback may be recalled after a step up/down, but we don't want to lose
 * the exact position : what we do is that we only set the value if it has
 * changed : a step up/down that doesn't change the time value of the first
 * event won't trigger a scrollbar change. */

void v_scroll_cb (GtkAdjustment *adjustment, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  g_debug("SCROLL begin");
  g_debug("SCROLL values : %g , %g, %g",
      adjustment->value, event_viewer_data->previous_value,
      (adjustment->value - event_viewer_data->previous_value));

  LttTime new_time_off = ltt_time_from_double(adjustment->value);
  LttTime old_time_off = ltt_time_from_double(event_viewer_data->previous_value);
  g_debug("SCROLL time values %lu.%lu, %lu.%lu", new_time_off.tv_sec,
      new_time_off.tv_nsec, old_time_off.tv_sec, old_time_off.tv_nsec);
  /* If same value : nothing to update */
  if(ltt_time_compare(new_time_off, old_time_off) == 0)
    return;
  
  //LttTime old_time = event_viewer_data->first_event;
  

  //gint snoop = gtk_key_snooper_install(key_snooper, NULL);
  
  adjust_event_viewer(adjustment->value, event_viewer_data);

  //gtk_key_snooper_remove(snoop);

  g_debug("SCROLL end");
}

static __inline gint get_cell_height(GtkTreeView *TreeView)
{
  gint height;
  GtkTreeViewColumn *column = gtk_tree_view_get_column(TreeView, 0);
  
  gtk_tree_view_column_cell_get_size(column, NULL, NULL, NULL, NULL, &height);
  
  gint vertical_separator;
  gtk_widget_style_get (GTK_WIDGET (TreeView),
      "vertical-separator", &vertical_separator,
      NULL);
	
	height += vertical_separator;

  return height;
}

void tree_v_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data)
{
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
  gint last_num_visible_events = event_viewer_data->num_visible_events;
  gdouble exact_num_visible;
  
  exact_num_visible = ( alloc->height -
          event_viewer_data->header_height )
            / (double)cell_height ;
  
  event_viewer_data->num_visible_events = ceil(exact_num_visible) ;

  event_viewer_data->vadjust_c->page_increment = 
					floor(exact_num_visible);
  event_viewer_data->vadjust_c->page_size =
					floor(exact_num_visible);

 
  g_debug("size allocate %p : last_num_visible_events : %d",
           event_viewer_data, last_num_visible_events);
  g_debug("num_visible_events : %d, value %f",
           event_viewer_data->num_visible_events,
	   event_viewer_data->vadjust_c->value);

  if(event_viewer_data->num_visible_events != last_num_visible_events) {
    //TODO ybrosseau do a proper initialisation here, so we can use the redraw
    adjust_event_viewer(event_viewer_data->vadjust_c->value, event_viewer_data);
    //redraw(event_viewer_data);
  }
  

}

void tree_v_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data)
{
  gint h;
  EventViewerData *event_viewer_data = (EventViewerData*)data;
  gint cell_height = get_cell_height(GTK_TREE_VIEW(event_viewer_data->tree_v));
  
  h = cell_height + event_viewer_data->header_height;
  requisition->height = h;
  
}

static gboolean events_check_handler(guint count, gboolean *stop_flag,
  gpointer data)
{
  EventViewerData *evd = (EventViewerData*)data;
  if(count % CHECK_GDK_INTERVAL == 0) {
    GdkEvent *event;
    GtkWidget *widget;
    while((event = gdk_event_get()) != NULL) {
        widget = gtk_get_event_widget(event);
        if(widget == 
	  lookup_widget(main_window_get_widget(evd->tab),
	      "StopProcessingButton")
	  || widget == evd->vscroll_vc) {
          gtk_main_do_event(event);
	  gdk_window_process_all_updates();
	}
        gdk_event_free(event);
    }
    if(*stop_flag)
      return TRUE;
    else
      return FALSE;
  } else return FALSE;
}


/* This function will adjust the first event position for the viewer which
   will be used by the redraw to seek and display all the events
 */
static void adjust_event_viewer(double new_value, EventViewerData *event_viewer_data)
{
#ifdef BABEL_CLEANUP
  LttvTracesetStats *tss =
    lttvwindow_get_traceset_stats(event_viewer_data->tab);
#endif //babel_cleanup
  LttvTraceset *ts = lttvwindow_get_traceset(event_viewer_data->tab);

  gboolean seek_by_time;
 
  if(lttvwindow_preempt_count > 0) return;

  double value = new_value - event_viewer_data->previous_value;
  
  /* Set stop button status for foreground processing */
  event_viewer_data->tab->stop_foreground = FALSE;
  lttvwindow_events_request_disable();
  
  /* See where we have to scroll... */
  ScrollDirection direction;
  gint relative_position;
  
  if(value < -0.8) {
    if(value >= -1.0) direction = SCROLL_STEP_UP;
    else {
      if(value >= -2.0) direction = SCROLL_PAGE_UP;
      else direction = SCROLL_JUMP;
    }
  } else if(value > 0.8) {
    if(value <= 1.0) direction = SCROLL_STEP_DOWN;
    else {
      if(value <= 2.0) direction = SCROLL_PAGE_DOWN;
      else direction = SCROLL_JUMP;
    }
  } else direction = SCROLL_NONE; /* 0.0 */
 
  switch(direction) {
  case SCROLL_STEP_UP:
    g_debug("adjust_event_viewer : SCROLL_STEP_UP");
    relative_position = -1;
    seek_by_time = 0;
    break;
  case SCROLL_STEP_DOWN:
    g_debug("adjust_event_viewer : SCROLL_STEP_DOWN");
    relative_position = 1;
    seek_by_time = 0;
    break;
  case SCROLL_PAGE_UP:
    g_debug("adjust_event_viewer : SCROLL_PAGE_UP");
    relative_position = -(event_viewer_data->num_visible_events);
    seek_by_time = 0;
    break;
  case SCROLL_PAGE_DOWN:
    g_debug("adjust_event_viewer : SCROLL_PAGE_DOWN");
    relative_position = event_viewer_data->num_visible_events;
    seek_by_time = 0;
    break;
  case SCROLL_JUMP:
    g_debug("adjust_event_viewer : SCROLL_JUMP");
    relative_position = 0;
    seek_by_time = 1;
    break;
  case SCROLL_NONE:
    g_debug("adjust_event_viewer : SCROLL_NONE");
    relative_position = 0;
    seek_by_time = 0;
    break;
  }

  LttTime time = ltt_time_from_double(new_value);
  TimeInterval time_span = lttv_traceset_get_time_span_real(ts);
  time = ltt_time_add(time_span.start_time, time);
  
  LttvTracesetPosition *timePos = lttv_traceset_create_time_position(ts,time);
  

  if(!seek_by_time) {
 
    /* Remember the beginning position */
    if(event_viewer_data->pos->len > 0) {
      
      if(relative_position != 0)
      {
        LttvTracesetPosition *pos = 0;
        pos = (LttvTracesetPosition*)g_ptr_array_index(
                                                event_viewer_data->pos,
                                                0);
        lttv_state_traceset_seek_position(ts,pos);
      }
    } else {
      lttv_state_traceset_seek_time(ts,time);

    }

  /* Note that, as we mess with the tsc position, this function CANNOT be called
   * from a hook inside the lttv_process_traceset_middle. */
  /* As the lttvwindow API keeps a sync_position inside the tsc to go back at
   * the right spot after being interrupted, it's ok to change the tsc position,
   * as long as we do not touch the sync_position. */

  /* Get the beginning position of the read (with seek backward or seek forward)
   */
    if(relative_position > 0) {
      lttv_process_traceset_seek_n_forward(ts, relative_position,
          events_check_handler,
          &event_viewer_data->tab->stop_foreground,
          event_viewer_data->main_win_filter,
          event_viewer_data->filter, NULL, event_viewer_data);
    } else if(relative_position < 0) {
      /* Get an idea of currently shown event dispersion */
      guint64 first_event_time_stamp =
        lttv_traceset_position_get_timestamp(event_viewer_data->first_event);
      guint64 last_event_time_stamp =
        lttv_traceset_position_get_timestamp(event_viewer_data->last_event);
      guint64 timeSpan = (last_event_time_stamp - first_event_time_stamp);
      gdouble ratio =  (gdouble)timeSpan / (gdouble)event_viewer_data->num_visible_events;
      if(ratio <= 0){
        ratio = SEEK_BACK_DEFAULT_RATIO;
      }

      lttv_process_traceset_seek_n_backward(ts,
          (guint)abs(relative_position),
          ratio,
	  events_check_handler,
	  &event_viewer_data->tab->stop_foreground,
          event_viewer_data->main_win_filter,
          event_viewer_data->filter, NULL, event_viewer_data);
    } /* else 0 : do nothing : we are already at the beginning position */

    /* Save the first event position */

    event_viewer_data->first_event = lttv_traceset_create_current_position(ts);

    time = ltt_time_from_uint64(lttv_traceset_position_get_timestamp(
                                            event_viewer_data->first_event));
    //if(ltt_time_compare(time, tsc->time_span.end_time) > 0)
    //  time = tsc->time_span.end_time;

    LttTime time_val = ltt_time_sub(time,time_span.start_time);
    event_viewer_data->previous_value = ltt_time_to_double(time_val);


  } else {
    /* Seek by time */
    
    LttTime time_val = ltt_time_sub(time,time_span.start_time);
    event_viewer_data->previous_value = ltt_time_to_double(time_val);
    event_viewer_data->first_event = timePos;
  }
  lttvwindow_events_request_enable();

  redraw(event_viewer_data);
}

static void redraw(EventViewerData *event_viewer_data) {
  guint i;
  LttvTraceset *ts = lttvwindow_get_traceset(event_viewer_data->tab);
  
  g_debug("EventViewer redraw");

  //TODO ybrosseau verify its still required
  lttvwindow_events_request_disable();
  
  
  /* Clear the model (don't forget to free the TCS positions!) */
  gtk_list_store_clear(event_viewer_data->store_m);
  for(i=0;i<event_viewer_data->pos->len;i++) {
    LttvTracesetPosition *cur_pos = 
      (LttvTracesetPosition*)g_ptr_array_index(event_viewer_data->pos,
                                                      i);
    lttv_traceset_destroy_position(cur_pos);
  }
  g_ptr_array_set_size(event_viewer_data->pos, 0);
  
  lttv_state_traceset_seek_position(ts, event_viewer_data->first_event);

  /* Mathieu :
   * I make the choice not to use the mainwindow lttvwindow API here : the idle
   * loop might have a too low priority, and we want good update while
   * scrolling. However, we call the gdk loop to get events periodically so the
   * processing can be stopped.
   */
  
  lttv_process_traceset_begin(ts,
      NULL, NULL, event_viewer_data->event_hooks);

  event_viewer_data->num_events = 0;
  
  lttv_process_traceset_middle(ts, ltt_time_infinite, G_MAXUINT, NULL);
  
  lttv_process_traceset_end(ts, NULL, NULL, event_viewer_data->event_hooks);
  
  /* Get the end position */
  if(event_viewer_data->pos->len > 0) {
    LttvTracesetPosition *cur_pos = 
      (LttvTracesetPosition*)g_ptr_array_index(event_viewer_data->pos,
                                               event_viewer_data->pos->len - 1);
    event_viewer_data->last_event = cur_pos;
  } else
        event_viewer_data->last_event =lttv_traceset_create_current_position(ts);
  
  gtk_adjustment_set_value(event_viewer_data->vadjust_c,
      event_viewer_data->previous_value);
  
  //g_signal_emit_by_name(G_OBJECT (event_viewer_data->select_c),
  //    "changed");
  
  event_viewer_data->last_tree_update_time = 
    gdk_x11_get_server_time(
        gtk_widget_get_parent_window(event_viewer_data->tree_v));

  lttvwindow_events_request_enable();
  
  return;
}


int event_hook(void *hook_data, void *call_data)
{

  EventViewerData *event_viewer_data = (EventViewerData*)hook_data;
  
  LttvEvent * e = (LttvEvent *)call_data;

  if(event_viewer_data->num_events % CHECK_GDK_INTERVAL == 0) {
    GdkEvent *event;
    GtkWidget *widget;
    while((event = gdk_event_get()) != NULL) {
        widget = gtk_get_event_widget(event);
        if(widget == 
	  lookup_widget(main_window_get_widget(event_viewer_data->tab),
	      "StopProcessingButton")
	  || widget == event_viewer_data->vscroll_vc) {
          gtk_main_do_event(event);
	  gdk_window_process_all_updates();
	}
        gdk_event_free(event);
    }
    //gtk_main_iteration_do(FALSE);
    if(event_viewer_data->tab->stop_foreground)
      return TRUE;
  }
 
  event_viewer_data->num_events++;
#ifdef BABEL_CLEANUP
  LttvFilter *filter = event_viewer_data->main_win_filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  filter = event_viewer_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;
#endif //babel_cleanup
   
//  LttFacility *facility = ltt_event_facility(e);
//  LttEventType *event_type = ltt_event_eventtype(e);
  LttTime time = lttv_event_get_timestamp(e);
  gint cpu = lttv_traceset_get_cpuid_from_event(e);

  LttvTraceState *traceState = e->state;
  LttvProcessState *process = traceState->running_process[cpu];

  GtkTreeIter iter;

  GString *desc = g_string_new("");
  GString *name = g_string_new("");
  
  LttvTracesetPosition *pos = lttv_traceset_create_current_position(traceState->trace->traceset);

  lttv_event_to_string(e, desc, TRUE, FALSE);
  lttv_event_get_name(e,name);

  g_info("detail : %s", desc->str);

  gtk_list_store_append (event_viewer_data->store_m, &iter);

  gtk_list_store_set (event_viewer_data->store_m, &iter,
        TRACE_NAME_COLUMN, traceState->trace->short_name,
        CPUID_COLUMN, cpu,
        EVENT_COLUMN,name->str,
        TIME_S_COLUMN, time.tv_sec,
        TIME_NS_COLUMN, time.tv_nsec,
        PID_COLUMN, process->pid,
        EVENT_DESCR_COLUMN, desc->str,
        POSITION_COLUMN, pos,
        -1);

  g_ptr_array_add(event_viewer_data->pos, pos);
  
  g_string_free(desc, TRUE);
  g_string_free(name, TRUE);

  if(event_viewer_data->update_cursor) {
    if(lttv_traceset_position_compare(pos, 
          event_viewer_data->currently_selected_position) == 0) {
      GtkTreePath *path = gtk_tree_path_new_from_indices(
                          event_viewer_data->pos->len - 1, -1);
			if(path) {
	      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
  	                           path, NULL, FALSE);
    	  gtk_tree_path_free(path);
			}
    }
  }
  if(event_viewer_data->pos->len >= event_viewer_data->num_visible_events )
    return TRUE;
  else
    return FALSE;

}



static void event_update_selection(EventViewerData *event_viewer_data)
{
      
  guint i;
  GPtrArray *positions = event_viewer_data->pos;
  g_info("event_update_selection");
  
  int isFound = FALSE;
  

  for(i=0;i<positions->len;i++) {
    LttvTracesetPosition *cur_pos = 
		(LttvTracesetPosition*)g_ptr_array_index(positions, i);
    if(lttv_traceset_position_compare(cur_pos, 
          event_viewer_data->currently_selected_position) == 0) {
      GtkTreePath *path = gtk_tree_path_new_from_indices(i, -1);
			if(path) {
	      gtk_tree_view_set_cursor(GTK_TREE_VIEW(event_viewer_data->tree_v),
	                             path, NULL, FALSE);
              gtk_widget_grab_focus(event_viewer_data->tree_v );
	      gtk_tree_path_free(path);
              isFound = TRUE;
              break;
                        }               
    }
  } 
        if(isFound){
          return;
        }
  /* If the current selection is not in the currently displayed events*/
  LttTime currentTime = lttv_traceset_position_get_time(
                                event_viewer_data->currently_selected_position);
  LttTime time;
  
  LttvTraceset * ts = lttvwindow_get_traceset(event_viewer_data->tab);
  TimeInterval time_span = lttv_traceset_get_time_span_real(ts);
  time = ltt_time_sub(currentTime, time_span.start_time);
  gtk_adjustment_set_value(event_viewer_data->vadjust_c,
                                                ltt_time_to_double(time));
  gtk_widget_grab_focus(event_viewer_data->tree_v );
  
}
#ifdef BABEL_CLEANUP 
static int current_time_get_first_event_hook(void *hook_data, void *call_data)
{

  EventViewerData *event_viewer_data = (EventViewerData*)hook_data;
   
  LttvTracefileContext *tfc = (LttvTracefileContext*)call_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);

  LttvFilter *filter = event_viewer_data->main_win_filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  filter = event_viewer_data->filter;
  if(filter != NULL && filter->head != NULL)
    if(!lttv_filter_tree_parse(filter->head,e,tfc->tf,
          tfc->t_context->t,tfc,NULL,NULL))
      return FALSE;

  lttv_traceset_context_position_save(tfc->t_context->ts_context, 
      event_viewer_data->current_time_get_first);
  return TRUE;

 
}
#endif // BABEL_CLEANUP 

gboolean update_current_time(void * hook_data, void * call_data)
{
      
        //LttvEvent * e = (LttvEvent *)call_data;
  g_info("update_current_time");
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
 
  const LttTime * current_time = (LttTime*)call_data;
  LttvTraceset * ts = lttvwindow_get_traceset(event_viewer_data->tab);
  
  /* If the currently selected event time != current time, set the first event
   * with this time as currently selected. */
  LttTime pos_time = lttv_traceset_position_get_time(
                        event_viewer_data->currently_selected_position);
     
  if(ltt_time_compare(pos_time, *current_time) != 0) {
        /*create position*/
        //LttvTracesetPosition *currentPosition = 
        //                lttv_traceset_create_time_position(ts,*current_time );
        /*seek to current position*/
        lttv_state_traceset_seek_time(ts, *current_time);
    
	event_viewer_data->currently_selected_position = 
	  lttv_traceset_create_current_position(ts);
	g_debug("update_current_time: %p %lu", event_viewer_data->currently_selected_position,  event_viewer_data->currently_selected_position->timestamp);
  }

  event_viewer_data->report_position = FALSE;
  
    /* Simply update the current time : it is in the list */
  event_update_selection(event_viewer_data);
  event_viewer_data->report_position = TRUE;
  
  return FALSE;
}

gboolean update_current_position(void * hook_data, void * call_data)
{

  g_info("update_current_position");
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  const LttvTracesetPosition *current_pos = (LttvTracesetPosition*)call_data;
  
  if(lttv_traceset_position_compare(
        event_viewer_data->currently_selected_position, current_pos) != 0) {
    g_debug("Update current pos: %p, %lu", current_pos, current_pos->timestamp);
                event_viewer_data->currently_selected_position = current_pos;
      /* Simply update the current time : it is in the list */
      event_update_selection(event_viewer_data);
    }
  return FALSE;

}

gboolean timespan_changed(void * hook_data, void * call_data)
{
       
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTraceset * ts = lttvwindow_get_traceset(event_viewer_data->tab);
  TimeInterval time_span = lttv_traceset_get_time_span_real(ts);
 
  LttTime end;

  end = ltt_time_sub(time_span.end_time, time_span.start_time);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(end);

  if(event_viewer_data->pos->len < event_viewer_data->num_visible_events ) {

	  
    redraw(event_viewer_data);
	  
	  request_background_data(event_viewer_data);
  }
  return FALSE;
}

gboolean traceset_changed(void * hook_data, void * call_data)
{

  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;
  LttvTraceset * ts = lttvwindow_get_traceset(event_viewer_data->tab);
  TimeInterval time_span = lttv_traceset_get_time_span_real(ts);
  
  LttTime end;
  gtk_list_store_clear(event_viewer_data->store_m);
  g_ptr_array_set_size(event_viewer_data->pos, 0);
  
  end = ltt_time_sub(time_span.end_time, time_span.start_time);
  event_viewer_data->vadjust_c->upper = ltt_time_to_double(end);
#ifdef BABEL_CLEANUP
  /* Reset the positions */
  lttv_traceset_destroy_position(event_viewer_data->currently_selected_position);
  lttv_traceset_destroy_position(event_viewer_data->first_event);
  lttv_traceset_destroy_position(event_viewer_data->last_event);
  
  event_viewer_data->currently_selected_position =
                                lttv_traceset_create_current_position(ts);
  event_viewer_data->first_event =
                                lttv_traceset_create_current_position(ts);
  event_viewer_data->last_event =
                                lttv_traceset_create_current_position(ts);

  redraw(event_viewer_data);

#endif //babel_cleanup
  //  event_viewer_data->vadjust_c->value = 0;

	request_background_data(event_viewer_data);

  return FALSE;
}

gboolean filter_changed(void * hook_data, void * call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;

  event_viewer_data->main_win_filter = 
    (LttvFilter*)call_data;
  redraw(event_viewer_data);

  return FALSE;
}


gint evd_redraw_notify(void *hook_data, void *call_data)
{
  EventViewerData *event_viewer_data = (EventViewerData*) hook_data;

  redraw(event_viewer_data);
  return 0;
}

void gui_events_free(gpointer data)
{
#ifdef BABEL_CLEANUP
  LttvPluginEVD *plugin_evd = (LttvPluginEVD*)data;
  Tab *tab = plugin_evd->evd->tab;
  EventViewerData *event_viewer_data = plugin_evd->evd;
  guint i;
 
  lttv_filter_destroy(plugin_evd->evd->filter);

  if(tab != NULL){
    lttv_hooks_remove(event_viewer_data->event_hooks,event_hook);
    lttv_hooks_destroy(event_viewer_data->event_hooks);
    
    for(i=0;i<event_viewer_data->pos->len;i++) {
      LttvTracesetContextPosition *cur_pos = 
        (LttvTracesetContextPosition*)g_ptr_array_index(event_viewer_data->pos,
                                                        i);
      lttv_traceset_context_position_destroy(cur_pos);
    }
    lttv_traceset_context_position_destroy(
        event_viewer_data->currently_selected_position);
    lttv_traceset_context_position_destroy(
        event_viewer_data->first_event);
    lttv_traceset_context_position_destroy(
        event_viewer_data->last_event);
    g_ptr_array_free(event_viewer_data->pos, TRUE);
    
    lttvwindow_unregister_current_time_notify(tab,
                        update_current_time, event_viewer_data);
    lttvwindow_unregister_current_position_notify(tab,
                        update_current_position, event_viewer_data);
    //lttvwindow_unregister_show_notify(tab,
    //                    show_event_detail, event_viewer_data);
    lttvwindow_unregister_traceset_notify(tab,
                        traceset_changed, event_viewer_data);
    lttvwindow_unregister_timespan_notify(tab,
				    timespan_changed,event_viewer_data);
    lttvwindow_unregister_filter_notify(tab,
                        filter_changed, event_viewer_data);
    lttvwindow_unregister_redraw_notify(tab,
                evd_redraw_notify, event_viewer_data);

  }
  lttvwindowtraces_background_notify_remove(event_viewer_data);

  g_event_viewer_data_list = g_slist_remove(g_event_viewer_data_list,
      event_viewer_data);
  //g_free(event_viewer_data);
  g_object_unref(plugin_evd);
#endif // BABEL_CLEANUP
}



void gui_events_destructor(gpointer data)
{
  LttvPluginEVD *plugin_evd = (LttvPluginEVD*)data;
  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(plugin_evd->parent.top_widget)){
    gtk_widget_destroy(plugin_evd->parent.top_widget);
  }
}



/**
 * plugin's init function
 *
 * This function initializes the Event Viewer functionnality through the
 * gtkTraceSet API.
 */
static void init() {

  lttvwindow_register_constructor("guievents",
                                  "/",
                                  "Insert Event Viewer",
                                  hGuiEventsInsert_xpm,
                                  "Insert Event Viewer",
                                  h_gui_events);
}

void event_destroy_walk(gpointer data, gpointer user_data)
{
  gui_events_destructor((LttvPluginEVD*)data);
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  
  g_slist_foreach(g_event_viewer_data_list, event_destroy_walk, NULL );
  g_slist_free(g_event_viewer_data_list);

  lttvwindow_unregister_constructor(h_gui_events);
  
}




LTTV_MODULE("guievents", "Detailed events view", \
    "Graphical module to display a detailed event list", \
	    init, destroy, "lttvwindow", "print")
