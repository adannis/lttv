//FIXME by including ltt.h
#include <time.h>
typedef time_t ltt_time;

typedef struct _ltt_time_interval
{
	ltt_time time_begin, time_end;
} ltt_time_interval;

// ???


	/* Setup the hooks */
	Draw_Before_Hooks = lttv_hooks_new();
	Draw_Event_Hooks = lttv_hooks_new();
	Draw_After_Hooks = lttv_hooks_new();
	
	lttv_hooks_add(Draw_Before_Hooks, Draw_Before_Hook, NULL);
	lttv_hooks_add(Draw_Event_Hooks, Draw_Event_Hook, NULL);
	lttv_hooks_add(Draw_After_Hooks, Draw_After_Hook, NULL);
	
	/* Destroy the hooks */
	
	lttv_hooks_destroy(Draw_Before_Hooks);
	lttv_hooks_destroy(Draw_Event_Hooks);
	lttv_hooks_destroy(Draw_After_Hooks);
	



/*****************************************************************************
 *                         Definition of structures                          *
 *****************************************************************************/

/* Structure used to store and use information relative to one events refresh
 * request. Typically filled in by the expose event callback, then passed to the
 * library call, then used by the drawing hooks. Then, once all the events are
 * sent, it is freed by the hook called after the reading.
 */
typedef struct _EventRequest
{
	ControlFlowData *Control_Flow_Data;
	ltt_time time_begin, time_end;
	/* Fill the Events_Context during the initial expose, before calling for
	 * events.
	 */
	GArray Events_Context; //FIXME
} EventRequest ;



/*****************************************************************************
 *                         Function prototypes                               *
 *****************************************************************************/
//! Control Flow Viewer's constructor hook
GtkWidget *hGuiControlFlow(GtkWidget *pmParentWindow);
//! Control Flow Viewer's constructor
ControlFlowData *GuiControlFlow(void);
//! Control Flow Viewer's destructor
void GuiControlFlow_Destructor(ControlFlowData *Control_Flow_Data);


static int Event_Selected_Hook(void *hook_data, void *call_data);

static lttv_hooks
	*Draw_Before_Hooks,
	*Draw_Event_Hooks,
	*Draw_After_Hooks;

Draw_Before_Hook(void *hook_data, void *call_data)
Draw_Event_Hook(void *hook_data, void *call_data)
Draw_After_Hook(void *hook_data, void *call_data)


//void Tree_V_set_cursor(ControlFlowData *Control_Flow_Data);
//void Tree_V_get_cursor(ControlFlowData *Control_Flow_Data);

/* Prototype for selection handler callback */
//static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void v_scroll_cb (GtkAdjustment *adjustment, gpointer data);
//static void Tree_V_size_allocate_cb (GtkWidget *widget, GtkAllocation *alloc, gpointer data);
//static void Tree_V_size_request_cb (GtkWidget *widget, GtkRequisition *requisition, gpointer data);
//static void Tree_V_cursor_changed_cb (GtkWidget *widget, gpointer data);
//static void Tree_V_move_cursor_cb (GtkWidget *widget, GtkMovementStep arg1, gint arg2, gpointer data);

static void expose_event_cb (GtkWidget *widget, GdkEventExpose *expose, gpointer data);

void add_test_process(ControlFlowData *Control_Flow_Data);

static void get_test_data(guint Event_Number, guint List_Height, 
									 ControlFlowData *Control_Flow_Data);

void add_test_data(ControlFlowData *Control_Flow_Data);
void test_draw(ControlFlowData *Control_Flow_Data);

void Drawing_Area_Init(ControlFlowData *Control_Flow_Data);


/*\@}*/
