/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Peter Ho
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
/******************************************************************
 Each field of the interrupt viewer is summarized as follows:
 
- CPUID: processor ID

- IrqId: IRQ ID

- Frequency (Hz): the number of interrupts per second (Hz). 
                  We compute the total number of interrupts. Then 
		  we divide it by the time interval.

- Total Duration (nsec): the sum of each interrupt duration in nsec. 
	         For a given Irq ID, we sum the duration of each interrupt
		 to give us the total duration 

		 
*******************************************************************/
		 

#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <lttv/module.h>
#include <lttv/hook.h>
#include <lttv/tracecontext.h>
#include <lttv/state.h>
#include <lttv/filter.h>
#include <lttvwindow/lttvwindow.h>
#include <lttvwindow/lttv_plugin_tab.h>
#include <ltt/time.h>

#include "hTutorialInsert.xpm" 

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)
#define NO_ITEMS 0
  
typedef struct 
{
	guint cpu_id;
	guint id;
	guint TotalNumberOfInterrupts;
	LttTime total_duration;	
}Irq;

typedef struct 
{
	guint id;
	guint cpu_id;
	LttTime event_time;
}irq_entry;

enum type_t 
{
   IRQ_ENTRY,
   IRQ_EXIT		
};

/** Array containing instanced objects. Used when module is unloaded */
static GSList *interrupt_data_list = NULL ;
 

#define TRACE_NUMBER 0

typedef struct _InterruptEventData 
{

  /* Graphical Widgets */ 
  GtkWidget * ScrollWindow;
  GtkListStore *ListStore;
  GtkWidget *Hbox;
  GtkWidget *TreeView;
  GtkTreeSelection *SelectionTree;
  
  Tab 	     * tab; /* tab that contains this plug-in*/ 
  LttvPluginTab    * ptab;
  LttvHooks  * event_hooks;
  LttvHooks  * hooks_trace_after;
  LttvHooks  * hooks_trace_before;
  TimeWindow   time_window;
  LttvHooksById * event_by_id_hooks;
  GArray *IrqExit;
  GArray *IrqEntry;
} InterruptEventData ;


/* Function prototypes */
 
static gboolean interrupt_update_time_window(void * hook_data, void * call_data);
static GtkWidget *interrupts(LttvPlugin *plugin);
static InterruptEventData *system_info(LttvPluginTab *ptab);
void interrupt_destructor(InterruptEventData *event_viewer_data);
static void request_event(InterruptEventData *event_data );  
static guint64 get_interrupt_id(LttEvent *e);
static gboolean trace_header(void *hook_data, void *call_data);
static gboolean interrupt_display (void *hook_data, void *call_data);
static void calcul_duration(LttTime time_exit,  guint cpu_id,  InterruptEventData *event_data);
static void sum_interrupt_data(irq_entry *e, LttTime time_exit, GArray *interrupt_counters);
static gboolean irq_entry_callback(void *hook_data, void *call_data);
static gboolean irq_exit_callback(void *hook_data, void *call_data);
static void InterruptFree(InterruptEventData *event_viewer_data);
static int FrequencyInHZ(gint NumerofInterruptions, TimeWindow time_window);
/* Enumeration of the columns */
enum{
  CPUID_COLUMN,
  IRQ_ID_COLUMN,
  FREQUENCY_COLUMN,
  DURATION_COLUMN,
  N_COLUMNS
};
 
 
 
/**
 *  init function
 *
 * 
 * This is the entry point of the viewer.
 *
 */
static void init() 
{
  g_info("interrupts: init()");
  
  lttvwindow_register_constructor("tutorial",
                                  "/",
                                  "Insert  Interrupts View",
                                  hTutorialInsert_xpm,
                                  "Insert Interrupts View",
                                  interrupts);   
}


/**
 *  Constructor hook
 *
 */
static GtkWidget *interrupts(LttvPlugin *plugin)
{
  LttvPluginTab *ptab = LTTV_PLUGIN_TAB(plugin);
  InterruptEventData* event_data = system_info(ptab) ;
  if(event_data)
    return event_data->Hbox;
  else 
    return NULL; 
}

/**
 * This function initializes the Event Viewer functionnality through the
 * GTK  API. 
 */
InterruptEventData *system_info(LttvPluginTab *ptab)
{
  LttTime end;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  InterruptEventData* event_viewer_data = g_new(InterruptEventData,1) ;
  Tab *tab = ptab->tab;
   
  event_viewer_data->tab = tab;
  event_viewer_data->ptab = ptab;
  
  /*Get the current time frame from the main window */
  event_viewer_data->time_window  =  lttvwindow_get_time_window(tab);
  event_viewer_data->IrqExit = g_array_new(FALSE, FALSE, sizeof(Irq));
  event_viewer_data->IrqEntry   =  g_array_new(FALSE, FALSE, sizeof(irq_entry));
  
  /*Create tha main window for the viewer */					 	
  event_viewer_data->ScrollWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (event_viewer_data->ScrollWindow);
  gtk_scrolled_window_set_policy(
      GTK_SCROLLED_WINDOW(event_viewer_data->ScrollWindow), 
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
 
  /* Create a model for storing the data list */
  event_viewer_data->ListStore = gtk_list_store_new (
    N_COLUMNS,      /* Total number of columns     */
    G_TYPE_INT,     /* CPUID                       */
    G_TYPE_INT,     /* IRQ_ID                      */
    G_TYPE_INT,     /* Frequency 		   */
    G_TYPE_UINT64   /* Duration                    */
    );  
 
  event_viewer_data->TreeView = gtk_tree_view_new_with_model (GTK_TREE_MODEL (event_viewer_data->ListStore)); 
   
  g_object_unref (G_OBJECT (event_viewer_data->ListStore));
    
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("CPU ID",
                 renderer,
                 "text", CPUID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

   
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("IRQ ID",
                 renderer,
                 "text", IRQ_ID_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column,  220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Frequency (HZ)",
                 renderer,
                 "text", FREQUENCY_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 1.0);
  gtk_tree_view_column_set_fixed_width (column, 220);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Total Duration (nsec)",
                 renderer,
                 "text", DURATION_COLUMN,
                 NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  gtk_tree_view_column_set_fixed_width (column, 145);
  gtk_tree_view_append_column (GTK_TREE_VIEW (event_viewer_data->TreeView), column);

  event_viewer_data->SelectionTree = gtk_tree_view_get_selection (GTK_TREE_VIEW (event_viewer_data->TreeView));
  gtk_tree_selection_set_mode (event_viewer_data->SelectionTree, GTK_SELECTION_SINGLE);
   
  gtk_container_add (GTK_CONTAINER (event_viewer_data->ScrollWindow), event_viewer_data->TreeView);
   
  event_viewer_data->Hbox = gtk_hbox_new(0, 0);
  gtk_box_pack_start(GTK_BOX(event_viewer_data->Hbox), event_viewer_data->ScrollWindow, TRUE, TRUE, 0);
 
  gtk_widget_show(event_viewer_data->Hbox);
  gtk_widget_show(event_viewer_data->TreeView);

  interrupt_data_list = g_slist_append(interrupt_data_list, event_viewer_data);
  
  /* Registration for time notification */
  lttvwindow_register_time_window_notify(tab,
                                         interrupt_update_time_window,
                                         event_viewer_data);	
   
  g_object_set_data_full(G_OBJECT(event_viewer_data->Hbox),
      "event_data",
       event_viewer_data,
      (GDestroyNotify) InterruptFree);  					 
					 
  
  request_event(event_viewer_data );
  return event_viewer_data;
}

/**
 * 
 * For each trace in the traceset, this function:
 *  - registers a callback function to each hook
 *  - calls lttv_trace_find_hook() registers a hook function to event_by_id_hooks
 *  - calls lttvwindow_events_request() to request data in a specific 
 *    time interval to the main window
 * 
 */
static void request_event(InterruptEventData *event_data )
{
  guint i, k, l, nb_trace;
 
  LttvTraceHook *hook;
   
  guint ret; 
  
  LttvTraceState *ts;
    
  GArray *hooks;
   
  EventsRequest *events_request;
  
  LttvTraceHookByFacility *thf;
  
  LttvTracesetContext *tsc = lttvwindow_get_traceset_context(event_data->tab);
  
  
  /* Get the traceset */
  LttvTraceset *traceset = tsc->ts;
 
  nb_trace = lttv_traceset_number(traceset);
  
  /* There are many traces in a traceset. Iteration for each trace. */  
  for(i = 0; i<MIN(TRACE_NUMBER+1, nb_trace);i++)
  {
        events_request = g_new(EventsRequest, 1); 
	
      	hooks = g_array_new(FALSE, FALSE, sizeof(LttvTraceHook));
      	
	hooks = g_array_set_size(hooks, 2);
    
	event_data->hooks_trace_before = lttv_hooks_new();
	
  	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_before, trace_header, event_data, LTTV_PRIO_DEFAULT);	

  	event_data->hooks_trace_after = lttv_hooks_new();
  	/* Registers a hook function */
	lttv_hooks_add(event_data->hooks_trace_after, interrupt_display, event_data, LTTV_PRIO_DEFAULT);
 	/* Get a trace state */
	ts = (LttvTraceState *)tsc->traces[i];
	/* Create event_by_Id hooks */
  	event_data->event_by_id_hooks = lttv_hooks_by_id_new();
  
 	/*Register event_by_id_hooks with a callback function*/ 
          ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_ENTRY,
		LTT_FIELD_IRQ_ID, 0, 0,
		irq_entry_callback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 0));
	 
	 ret = lttv_trace_find_hook(ts->parent.t,
		LTT_FACILITY_KERNEL, LTT_EVENT_IRQ_EXIT,
		LTT_FIELD_IRQ_ID, 0, 0,
		irq_exit_callback,
		events_request,
		&g_array_index(hooks, LttvTraceHook, 1));
		
	  g_assert(!ret);
 	 
	/*iterate through the facility list*/
	for(k = 0 ; k < hooks->len; k++) 
	{ 
	        hook = &g_array_index(hooks, LttvTraceHook, k);
		for(l=0; l<hook->fac_list->len; l++) 
		{
			thf = g_array_index(hook->fac_list, LttvTraceHookByFacility*, l); 
			lttv_hooks_add(lttv_hooks_by_id_find(event_data->event_by_id_hooks, thf->id),
				thf->h,
				event_data,
				LTTV_PRIO_DEFAULT);
			 
		}
	}
	
	/* Initalize the EventsRequest structure */
	events_request->owner       = event_data; 
	events_request->viewer_data = event_data; 
	events_request->servicing   = FALSE;     
	events_request->start_time  = event_data->time_window.start_time; 
	events_request->start_position  = NULL;
	events_request->stop_flag	   = FALSE;
	events_request->end_time 	   = event_data->time_window.end_time;
	events_request->num_events  	   = G_MAXUINT;      
	events_request->end_position       = NULL; 
	events_request->trace 	   = i;    
	
	events_request->hooks = hooks;
	
	events_request->before_chunk_traceset = NULL; 
	events_request->before_chunk_trace    = event_data->hooks_trace_before; 
	events_request->before_chunk_tracefile= NULL; 
	events_request->event		        = NULL;  
	events_request->event_by_id		= event_data->event_by_id_hooks; 
	events_request->after_chunk_tracefile = NULL; 
	events_request->after_chunk_trace     = NULL;   
	events_request->after_chunk_traceset	= NULL; 
	events_request->before_request	= NULL; 
	events_request->after_request		= event_data->hooks_trace_after; 
	
	lttvwindow_events_request(event_data->tab, events_request);   
   }
   
}

/**
 *  This function is called whenever an irq_entry event occurs.  
 *  
 */ 
static gboolean irq_entry_callback(void *hook_data, void *call_data)
{
  
  LttTime  event_time; 
  unsigned cpu_id;
  irq_entry entry;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray* IrqEntry  = event_data->IrqEntry; 
  LttEvent *e = ltt_tracefile_get_event(tfc->tf); 
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
   
  if ((ltt_time_compare(event_time,event_data->time_window.start_time) == TRUE) &&    
     (ltt_time_compare(event_data->time_window.end_time,event_time) == TRUE))
  { 	 
	entry.id =get_interrupt_id(e);	  
	entry.cpu_id = cpu_id;
	entry.event_time =  event_time;		
	g_array_append_val (IrqEntry, entry);
  } 
  return FALSE;
}

/**
 *  This function gets the id of the interrupt. The id is stored in a dynamic structure. 
 *  Refer to the print.c file for howto extract data from a dynamic structure.
 */ 
static guint64 get_interrupt_id(LttEvent *e)
{
  guint i, num_fields;
  LttEventType *event_type;
  LttField *element;  
  LttField *field;
   guint64  irq_id;
  event_type = ltt_event_eventtype(e);
  num_fields = ltt_eventtype_num_fields(event_type);
  for(i = 0 ; i < num_fields-1 ; i++) 
  {   
        field = ltt_eventtype_field(event_type, i);
  	irq_id = ltt_event_get_long_unsigned(e,field);
  }
  return  irq_id;

} 
/**
 *  This function is called whenever an irq_exit event occurs.  
 *  
 */ 
gboolean irq_exit_callback(void *hook_data, void *call_data)
{
  LttTime  event_time; 
  unsigned cpu_id;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttvTracefileState *tfs = (LttvTracefileState *)call_data;
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttEvent *e = ltt_tracefile_get_event(tfc->tf);
  LttEventType *type = ltt_event_eventtype(e);
  event_time = ltt_event_time(e);
  cpu_id = ltt_event_cpu_id(e);
  if ((ltt_time_compare(event_time,event_data->time_window.start_time) == TRUE) &&    
     (ltt_time_compare(event_data->time_window.end_time,event_time) == TRUE))
     {
     	 calcul_duration( event_time,  cpu_id, event_data);
	 
     }
   return FALSE;
}

/**
 *  This function calculates the duration of an interrupt.  
 *  
 */ 
static void calcul_duration(LttTime time_exit,  guint cpu_id,InterruptEventData *event_data){
  
  gint i, irq_id;
  irq_entry *element; 
  LttTime duration;
  GArray *IrqExit = event_data->IrqExit;
  GArray *IrqEntry = event_data->IrqEntry;
  for(i = 0; i < IrqEntry->len; i++){
    element = &g_array_index(IrqEntry,irq_entry,i);
    if(element->cpu_id == cpu_id)
    {
      sum_interrupt_data(element,time_exit,  IrqExit);    
      g_array_remove_index(IrqEntry, i);
      break;
    }
  }
}
/**
 *  This function calculates the total duration of an interrupt.  
 *  
 */ 
static void sum_interrupt_data(irq_entry *e, LttTime time_exit, GArray *IrqExit)
{
  Irq irq;
  Irq *element; 
  guint i;
  LttTime duration;
  gboolean  notFound = FALSE;
  memset ((void*)&irq, 0,sizeof(Irq));
  
  
  if(IrqExit->len == NO_ITEMS)
  {
    irq.cpu_id = e->cpu_id;
    irq.id    =  e->id;
    irq.TotalNumberOfInterrupts++;
    irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
    g_array_append_val (IrqExit, irq);
  }
  else{
    for(i = 0; i < IrqExit->len; i++)
    {
      element = &g_array_index(IrqExit, Irq, i);
      if(element->id == e->id){
	notFound = TRUE;
	duration =  ltt_time_sub(time_exit, e->event_time);
	element->total_duration = ltt_time_add(element->total_duration, duration);
	element->TotalNumberOfInterrupts++;
      }
    }
    
    if(!notFound)
    {
      irq.cpu_id = e->cpu_id;
      irq.id    =  e->id;
      irq.TotalNumberOfInterrupts++;
      irq.total_duration =  ltt_time_sub(time_exit, e->event_time);
      g_array_append_val (IrqExit, irq);
    }
  } 
}

/**
 *  This function displays the result on the viewer 
 *  
 */ 
static gboolean interrupt_display(void *hook_data, void *call_data)
{
  
  gint i;	
  Irq element; 
  LttTime average_duration;
  GtkTreeIter    iter;
  guint64 real_data;
  int FrequencyHZ =  0; 
  
  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  GArray *IrqExit = event_data->IrqExit;  
  gtk_list_store_clear(event_data->ListStore);
  for(i = 0; i < IrqExit->len; i++)
  {  
  
    element = g_array_index(IrqExit,Irq,i);  
    real_data = element.total_duration.tv_sec;
    real_data *= NANOSECONDS_PER_SECOND;
    real_data += element.total_duration.tv_nsec;
    
    FrequencyHZ =  FrequencyInHZ(element.TotalNumberOfInterrupts,event_data->time_window);
   
    gtk_list_store_append (event_data->ListStore, &iter);
    
    gtk_list_store_set (event_data->ListStore, &iter,
      CPUID_COLUMN, element.cpu_id,
      IRQ_ID_COLUMN,  element.id,
      FREQUENCY_COLUMN, FrequencyHZ,
      DURATION_COLUMN, real_data,
      -1);
     
  } 
   
  if(event_data->IrqExit->len)
     g_array_remove_range (event_data->IrqExit,0,event_data->IrqExit->len);
   
  if(event_data->IrqEntry->len)
    g_array_remove_range (event_data->IrqEntry,0,event_data->IrqEntry->len);
  return FALSE;
}

/**
 *  This function converts the number of interrupts over a time window to
 *  frequency in HZ
 */ 
static int FrequencyInHZ(gint NumerofInterruptions, TimeWindow time_window)
{
  guint64 frequencyHz = 0;
  double timeSec;  // time in second
  double result; 
  result  = ltt_time_to_double(time_window.time_width);
  timeSec = (result/NANOSECONDS_PER_SECOND);  //time in second
  frequencyHz = NumerofInterruptions / timeSec;  
  return  frequencyHz;
}


/*
 * This function is called by the main window
 * when the time interval needs to be updated.
 **/ 
gboolean interrupt_update_time_window(void * hook_data, void * call_data)
{
  InterruptEventData *event_data = (InterruptEventData *) hook_data;
  const TimeWindowNotifyData *time_window_nofify_data =  ((const TimeWindowNotifyData *)call_data);
  event_data->time_window = *time_window_nofify_data->new_time_window;
  g_info("interrupts: interrupt_update_time_window()\n");
  Tab *tab = event_data->tab;
  lttvwindow_events_request_remove_all(tab, event_data);
  request_event(event_data );
  return FALSE;
}


gboolean trace_header(void *hook_data, void *call_data)
{

  InterruptEventData *event_data = (InterruptEventData *)hook_data;
  LttvTracefileContext *tfc = (LttvTracefileContext *)call_data;
  LttEvent *e;
  LttTime event_time;
  return FALSE;
}

void interrupt_destroy_walk(gpointer data, gpointer user_data)
{
  g_info("interrupt_destroy_walk");
  interrupt_destructor((InterruptEventData*)data);

}

void interrupt_destructor(InterruptEventData *event_viewer_data)
{
  /* May already been done by GTK window closing */
  g_info("enter interrupt_destructor \n");
  if(GTK_IS_WIDGET(event_viewer_data->Hbox))
  {
    gtk_widget_destroy(event_viewer_data->Hbox);
  }
}

/**
    This function is called when the viewer is destroyed to free hooks and memory
*/
static void InterruptFree(InterruptEventData *event_viewer_data)
{
  Tab *tab = event_viewer_data->tab;
  if(tab != NULL)
  {
     g_array_free(event_viewer_data->IrqExit, TRUE);
     
     g_array_free(event_viewer_data->IrqEntry, TRUE);
     
     lttvwindow_unregister_time_window_notify(tab, interrupt_update_time_window, event_viewer_data);
     	
     lttvwindow_events_request_remove_all(event_viewer_data->tab,
                                          event_viewer_data);	
					  
     interrupt_data_list = g_slist_remove(interrupt_data_list, event_viewer_data);					  
      
  }
 	
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
static void destroy() {
  g_info("Destroy  interrupts");
  g_slist_foreach(interrupt_data_list, interrupt_destroy_walk, NULL );
  g_slist_free(interrupt_data_list); 
  lttvwindow_unregister_constructor(interrupts);
  
}

LTTV_MODULE("tutorial", "interrupts info view", \
    "Graphical module to display interrupts performance", \
	    init, destroy, "lttvwindow")
