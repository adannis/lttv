/* Event_hooks.c defines the hooks that are given to processTrace as parameter.
 * These hooks call the Drawing API to draw the information on the screen,
 * using information from Context, but mostly state (running, waiting...).
 */


#ifndef _EVENT_HOOKS_H
#define _EVENT_HOOKS_H

#include <gtk/gtk.h>
#include <lttv/mainWindow.h>
#include <ltt/time.h>
#include "Process_List.h"
#include "Drawing.h"
#include "CFV.h"


/* Structure used to store and use information relative to one events refresh
 * request. Typically filled in by the expose event callback, then passed to the
 * library call, then used by the drawing hooks. Then, once all the events are
 * sent, it is freed by the hook called after the reading.
 */
typedef struct _EventRequest
{
	ControlFlowData *Control_Flow_Data;
	LttTime time_begin, time_end;
	gint	x_begin, x_end;
	/* Fill the Events_Context during the initial expose, before calling for
	 * events.
	 */
	//GArray Events_Context; //FIXME
} EventRequest ;





void send_test_data(ProcessList *Process_List, Drawing_t *Drawing);

GtkWidget *h_guicontrolflow(MainWindow *pmParentWindow, LttvTracesetSelector * s, char * key);

int event_selected_hook(void *hook_data, void *call_data);

/* Hook called before drawing. Gets the initial context at the beginning of the
 * drawing interval and copy it to the context in Event_Request.
 */
int draw_before_hook(void *hook_data, void *call_data);

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
int draw_event_hook(void *hook_data, void *call_data);

int draw_after_hook(void *hook_data, void *call_data);

void draw_closure(gpointer key, gpointer value, gpointer user_data);

int  after_data_request(void *hook_data, void *call_data);


gint update_time_window_hook(void *hook_data, void *call_data);
gint update_current_time_hook(void *hook_data, void *call_data);




#endif // _EVENT_HOOKS_H
