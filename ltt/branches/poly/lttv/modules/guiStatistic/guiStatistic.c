#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <lttv/module.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/processTrace.h>
#include <lttv/hook.h>
#include <lttv/common.h>
#include <lttv/state.h>
#include <lttv/stats.h>

#include <ltt/ltt.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/trace.h>

#include <string.h>

#include "../icons/hGuiStatisticInsert.xpm"

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

#define PATH_LENGTH        256

static LttvModule *statistic_main_win_module;

/** Array containing instanced objects. Used when module is unloaded */
static GSList *g_statistic_viewer_data_list = NULL ;

typedef struct _StatisticViewerData StatisticViewerData;

//! Statistic Viewer's constructor hook
GtkWidget *h_gui_statistic(MainWindow *parent_window, LttvTracesetSelector * s, char* key);
//! Statistic Viewer's constructor
StatisticViewerData *gui_statistic(MainWindow *parent_window,LttvTracesetSelector * s, char* key);
//! Statistic Viewer's destructor
void gui_statistic_destructor(StatisticViewerData *statistic_viewer_data);
void gui_statistic_free(StatisticViewerData *statistic_viewer_data);

void grab_focus(GtkWidget *widget, gpointer data);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);

void statistic_destroy_hash_key(gpointer key);
void statistic_destroy_hash_data(gpointer data);

void show_traceset_stats(StatisticViewerData * statistic_viewer_data);
void show_tree(StatisticViewerData * statistic_viewer_data,
	       LttvAttribute* stats,  GtkTreeIter* parent);
void show_statistic(StatisticViewerData * statistic_viewer_data,
		    LttvAttribute* stats, GtkTextBuffer* buf);


/** hook functions for update time interval, current time ... */
gboolean statistic_update_time_window(void * hook_data, void * call_data);
gboolean statistic_show_viewer(void * hook_data, void * call_data);
gboolean statistic_traceset_changed(void * hook_data, void * call_data);
void statistic_add_context_hooks(StatisticViewerData * statistic_viewer_data, 
		       LttvTracesetContext * tsc);
void statistic_remove_context_hooks(StatisticViewerData * statistic_viewer_data, 
			  LttvTracesetContext * tsc);

enum
{
   NAME_COLUMN,
   N_COLUMNS
};

struct _StatisticViewerData{
  MainWindow * mw;
  LttvTracesetStats * stats;

  TimeInterval time_span;
  gboolean     shown;       //indicate if the statistic is shown or not
  char *       filter_key;

  GtkWidget    * hpaned_v;
  GtkTreeStore * store_m;
  GtkWidget    * tree_v;

  //scroll window containing Tree View
  GtkWidget * scroll_win_tree;

  GtkWidget    * text_v;  
  //scroll window containing Text View
  GtkWidget * scroll_win_text;

  // Selection handler 
  GtkTreeSelection *select_c;
  
  //hash 
  GHashTable *statistic_hash;
};


/**
 * plugin's init function
 *
 * This function initializes the Statistic Viewer functionnality through the
 * gtkTraceSet API.
 */
G_MODULE_EXPORT void init(LttvModule *self, int argc, char *argv[]) {

  statistic_main_win_module = lttv_module_require(self, "mainwin", argc, argv);
  
  if(statistic_main_win_module == NULL){
      g_critical("Can't load Statistic Viewer : missing mainwin\n");
      return;
  }
	
  /* Register the toolbar insert button */
  toolbar_item_reg(hGuiStatisticInsert_xpm, "Insert Statistic Viewer", h_gui_statistic);
  
  /* Register the menu item insert entry */
  menu_item_reg("/", "Insert Statistic Viewer", h_gui_statistic);
  
}

void statistic_destroy_walk(gpointer data, gpointer user_data)
{
  gui_statistic_destructor((StatisticViewerData*)data);
}

/**
 * plugin's destroy function
 *
 * This function releases the memory reserved by the module and unregisters
 * everything that has been registered in the gtkTraceSet API.
 */
G_MODULE_EXPORT void destroy() {
  int i;
  
  if(g_statistic_viewer_data_list){
    g_slist_foreach(g_statistic_viewer_data_list, statistic_destroy_walk, NULL );    
    g_slist_free(g_statistic_viewer_data_list);
  }

  /* Unregister the toolbar insert button */
  toolbar_item_unreg(h_gui_statistic);
	
  /* Unregister the menu item insert entry */
  menu_item_unreg(h_gui_statistic);
}


void
gui_statistic_free(StatisticViewerData *statistic_viewer_data)
{ 
  if(statistic_viewer_data){
    unreg_update_time_window(statistic_update_time_window,statistic_viewer_data, statistic_viewer_data->mw);
    unreg_show_viewer(statistic_show_viewer,statistic_viewer_data, statistic_viewer_data->mw);
    unreg_update_traceset(statistic_traceset_changed,statistic_viewer_data, statistic_viewer_data->mw);

    g_hash_table_destroy(statistic_viewer_data->statistic_hash);
    g_free(statistic_viewer_data->filter_key);
    g_statistic_viewer_data_list = g_slist_remove(g_statistic_viewer_data_list, statistic_viewer_data);
    g_free(statistic_viewer_data);
  }
}

void
gui_statistic_destructor(StatisticViewerData *statistic_viewer_data)
{
  /* May already been done by GTK window closing */
  if(GTK_IS_WIDGET(statistic_viewer_data->hpaned_v)){
    gtk_widget_destroy(statistic_viewer_data->hpaned_v);
    statistic_viewer_data = NULL;
  }
  //gui_statistic_free(statistic_viewer_data);
}


/**
 * Statistic Viewer's constructor hook
 *
 * This constructor is given as a parameter to the menuitem and toolbar button
 * registration. It creates the list.
 * @param parent_window A pointer to the parent window.
 * @return The widget created.
 */
GtkWidget *
h_gui_statistic(MainWindow * parent_window, LttvTracesetSelector * s, char* key)
{
  StatisticViewerData* statistic_viewer_data = gui_statistic(parent_window, s, key) ;

  if(statistic_viewer_data)
    return statistic_viewer_data->hpaned_v;
  else return NULL;
	
}

/**
 * Statistic Viewer's constructor
 *
 * This constructor is used to create StatisticViewerData data structure.
 * @return The Statistic viewer data created.
 */
StatisticViewerData *
gui_statistic(MainWindow *parent_window, LttvTracesetSelector * s, char* key)
{
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  StatisticViewerData* statistic_viewer_data = g_new(StatisticViewerData,1);

  statistic_viewer_data->mw     = parent_window;
  statistic_viewer_data->stats  = get_traceset_stats_api(statistic_viewer_data->mw);

  reg_update_time_window(statistic_update_time_window,statistic_viewer_data, statistic_viewer_data->mw);
  reg_show_viewer(statistic_show_viewer,statistic_viewer_data, statistic_viewer_data->mw);
  reg_update_traceset(statistic_traceset_changed,statistic_viewer_data, statistic_viewer_data->mw);

  statistic_viewer_data->statistic_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
								statistic_destroy_hash_key,
								statistic_destroy_hash_data);

  statistic_viewer_data->hpaned_v  = gtk_hpaned_new();
  statistic_viewer_data->store_m = gtk_tree_store_new (N_COLUMNS, G_TYPE_STRING);
  statistic_viewer_data->tree_v  = gtk_tree_view_new_with_model (GTK_TREE_MODEL (statistic_viewer_data->store_m));
  g_object_unref (G_OBJECT (statistic_viewer_data->store_m));

  g_signal_connect (G_OBJECT (statistic_viewer_data->tree_v), "grab-focus",
		    G_CALLBACK (grab_focus),
		    statistic_viewer_data);

  // Setup the selection handler
  statistic_viewer_data->select_c = gtk_tree_view_get_selection (GTK_TREE_VIEW (statistic_viewer_data->tree_v));
  gtk_tree_selection_set_mode (statistic_viewer_data->select_c, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (statistic_viewer_data->select_c), "changed",
		    G_CALLBACK (tree_selection_changed_cb),
		    statistic_viewer_data);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Statistic Name",
						     renderer,
						     "text", NAME_COLUMN,
						     NULL);
  gtk_tree_view_column_set_alignment (column, 0.0);
  //  gtk_tree_view_column_set_fixed_width (column, 45);
  gtk_tree_view_append_column (GTK_TREE_VIEW (statistic_viewer_data->tree_v), column);


  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (statistic_viewer_data->tree_v), FALSE);

  statistic_viewer_data->scroll_win_tree = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(statistic_viewer_data->scroll_win_tree), 
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (statistic_viewer_data->scroll_win_tree), statistic_viewer_data->tree_v);
  gtk_paned_pack1(GTK_PANED(statistic_viewer_data->hpaned_v),statistic_viewer_data->scroll_win_tree, TRUE, FALSE);
  gtk_paned_set_position(GTK_PANED(statistic_viewer_data->hpaned_v), 160);

  statistic_viewer_data->scroll_win_text = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(statistic_viewer_data->scroll_win_text), 
				 GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);

  statistic_viewer_data->text_v = gtk_text_view_new ();
  g_signal_connect (G_OBJECT (statistic_viewer_data->text_v), "grab-focus",
		    G_CALLBACK (grab_focus),
		    statistic_viewer_data);
  
  gtk_text_view_set_editable(GTK_TEXT_VIEW(statistic_viewer_data->text_v),FALSE);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(statistic_viewer_data->text_v),FALSE);
  gtk_container_add (GTK_CONTAINER (statistic_viewer_data->scroll_win_text), statistic_viewer_data->text_v);
  gtk_paned_pack2(GTK_PANED(statistic_viewer_data->hpaned_v), statistic_viewer_data->scroll_win_text, TRUE, FALSE);

  gtk_widget_show(statistic_viewer_data->scroll_win_tree);
  gtk_widget_show(statistic_viewer_data->scroll_win_text);
  gtk_widget_show(statistic_viewer_data->tree_v);
  gtk_widget_show(statistic_viewer_data->text_v);
  gtk_widget_show(statistic_viewer_data->hpaned_v);

  //get the life span of the traceset and set the upper of the scroll bar
  get_traceset_time_span(statistic_viewer_data->mw, &statistic_viewer_data->time_span);

  statistic_viewer_data->shown = FALSE;  
  statistic_viewer_data->filter_key = g_strdup(key);
  g_object_set_data(
		    G_OBJECT(statistic_viewer_data->hpaned_v),
		    statistic_viewer_data->filter_key,
		    s);

  g_object_set_data(
		    G_OBJECT(statistic_viewer_data->hpaned_v),
		    TRACESET_TIME_SPAN,
		    &statistic_viewer_data->time_span);

  g_object_set_data_full(
			G_OBJECT(statistic_viewer_data->hpaned_v),
			"statistic_viewer_data",
			statistic_viewer_data,
			(GDestroyNotify)gui_statistic_free);

  /* Add the object's information to the module's array */
  g_statistic_viewer_data_list = g_slist_append(
			g_statistic_viewer_data_list,
			statistic_viewer_data);

  return statistic_viewer_data;
}

void grab_focus(GtkWidget *widget, gpointer data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData *)data;
  MainWindow * mw = statistic_viewer_data->mw;
  set_focused_pane(mw, gtk_widget_get_parent(statistic_viewer_data->hpaned_v));
}

static void
tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*)data;
  GtkTreeIter iter;
  GtkTreeModel *model = GTK_TREE_MODEL(statistic_viewer_data->store_m);
  gchar *event;
  GtkTextBuffer* buf;
  gchar * str;
  GtkTreePath * path;
  GtkTextIter   text_iter;
  LttvAttribute * stats;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, NAME_COLUMN, &event, -1);

      path = gtk_tree_model_get_path(GTK_TREE_MODEL(model),&iter);
      str = gtk_tree_path_to_string (path);
      stats = (LttvAttribute*)g_hash_table_lookup (statistic_viewer_data->statistic_hash,str);
      g_free(str);
      
      buf =  gtk_text_view_get_buffer((GtkTextView*)statistic_viewer_data->text_v);
      gtk_text_buffer_set_text(buf,"Statistic for  '", -1);
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, event, strlen(event));      
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, "' :\n\n",5);
      
      show_statistic(statistic_viewer_data, stats, buf);

      g_free (event);
    }
}

void statistic_destroy_hash_key(gpointer key)
{
  g_free(key);
}

void statistic_destroy_hash_data(gpointer data)
{
  //  g_free(data);
}

void show_traceset_stats(StatisticViewerData * statistic_viewer_data)
{
  int i, nb;
  LttvTraceset *ts;
  LttvTraceStats *tcs;
  LttSystemDescription *desc;
  LttvTracesetStats * tscs = statistic_viewer_data->stats;
  gchar * str, trace_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = statistic_viewer_data->store_m;

  if(tscs->stats == NULL) return;

  gtk_tree_store_append (store, &iter, NULL);  
  gtk_tree_store_set (store, &iter,
		      NAME_COLUMN, "Traceset statistics",
		      -1);  
  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store),	&iter);
  str = gtk_tree_path_to_string (path);
  g_hash_table_insert(statistic_viewer_data->statistic_hash,
		      (gpointer)str, tscs->stats);
  show_tree(statistic_viewer_data, tscs->stats, &iter);

  //show stats for all traces
  ts = tscs->parent.parent.ts;
  nb = lttv_traceset_number(ts);
  
  for(i = 0 ; i < nb ; i++) {
    tcs = (LttvTraceStats *)(LTTV_TRACESET_CONTEXT(tscs)->traces[i]);
    desc = ltt_trace_system_description(tcs->parent.parent.t);    
    sprintf(trace_str, "Trace on system %s at time %d secs", 
      	    ltt_trace_system_description_node_name(desc),
	    (ltt_trace_system_description_trace_start_time(desc)).tv_sec);
    
    gtk_tree_store_append (store, &iter, NULL);  
    gtk_tree_store_set (store, &iter,NAME_COLUMN,trace_str,-1);  
    path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
    str = gtk_tree_path_to_string (path);
    g_hash_table_insert(statistic_viewer_data->statistic_hash,
			(gpointer)str,tcs->stats);
    show_tree(statistic_viewer_data, tcs->stats, &iter);
  }
}

void show_tree(StatisticViewerData * statistic_viewer_data,
	       LttvAttribute* stats,  GtkTreeIter* parent)
{
  int i, nb;
  LttvAttribute *subtree;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
  gchar * str, dir_str[PATH_LENGTH];
  GtkTreePath * path;
  GtkTreeIter   iter;
  GtkTreeStore * store = statistic_viewer_data->store_m;

  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value);
    switch(type) {
     case LTTV_GOBJECT:
        if(LTTV_IS_ATTRIBUTE(*(value.v_gobject))) {
	  sprintf(dir_str, "%s", g_quark_to_string(name));
          subtree = (LttvAttribute *)*(value.v_gobject);
	  gtk_tree_store_append (store, &iter, parent);  
	  gtk_tree_store_set (store, &iter,NAME_COLUMN,dir_str,-1);  
	  path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	  str = gtk_tree_path_to_string (path);
	  g_hash_table_insert(statistic_viewer_data->statistic_hash,
			      (gpointer)str, subtree);
          show_tree(statistic_viewer_data, subtree, &iter);
        }
        break;
      default:
	break;
    }
  }    
}

void show_statistic(StatisticViewerData * statistic_viewer_data,
		    LttvAttribute* stats, GtkTextBuffer* buf)
{
  int i, nb , flag;
  LttvAttribute *subtree;
  LttvAttributeName name;
  LttvAttributeValue value;
  LttvAttributeType type;
  gchar type_name[PATH_LENGTH], type_value[PATH_LENGTH];
  GtkTextIter   text_iter;
  
  flag = 0;
  nb = lttv_attribute_get_number(stats);
  for(i = 0 ; i < nb ; i++) {
    type = lttv_attribute_get(stats, i, &name, &value);
    sprintf(type_name,"%s", g_quark_to_string(name));
    type_value[0] = '\0';
    switch(type) {
      case LTTV_INT:
        sprintf(type_value, " :  %d\n", *value.v_int);
        break;
      case LTTV_UINT:
        sprintf(type_value, " :  %u\n", *value.v_uint);
        break;
      case LTTV_LONG:
        sprintf(type_value, " :  %ld\n", *value.v_long);
        break;
      case LTTV_ULONG:
        sprintf(type_value, " :  %lu\n", *value.v_ulong);
        break;
      case LTTV_FLOAT:
        sprintf(type_value, " :  %f\n", (double)*value.v_float);
        break;
      case LTTV_DOUBLE:
        sprintf(type_value, " :  %f\n", *value.v_double);
        break;
      case LTTV_TIME:
        sprintf(type_value, " :  %10u.%09u\n", value.v_time->tv_sec, 
            value.v_time->tv_nsec);
        break;
      case LTTV_POINTER:
        sprintf(type_value, " :  POINTER\n");
        break;
      case LTTV_STRING:
        sprintf(type_value, " :  %s\n", *value.v_string);
        break;
      default:
        break;
    }
    if(strlen(type_value)){
      flag = 1;
      strcat(type_name,type_value);
      gtk_text_buffer_get_end_iter(buf, &text_iter);
      gtk_text_buffer_insert(buf, &text_iter, type_name, strlen(type_name));
    }
  }

  if(flag == 0){
    sprintf(type_value, "No statistic information in this directory.\nCheck in subdirectories please.\n");
    gtk_text_buffer_get_end_iter(buf, &text_iter);
    gtk_text_buffer_insert(buf, &text_iter, type_value, strlen(type_value));

  }
}

gboolean statistic_update_time_window(void * hook_data, void * call_data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*) hook_data;
  LttvTracesetContext * tsc = get_traceset_context(statistic_viewer_data->mw);

  if(statistic_viewer_data->shown == FALSE){    
    statistic_add_context_hooks(statistic_viewer_data, tsc);
  }
  return FALSE;
}

gboolean statistic_show_viewer(void * hook_data, void * call_data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*) hook_data;
  LttvTracesetContext * tsc = get_traceset_context(statistic_viewer_data->mw);

  if(statistic_viewer_data->shown == FALSE){
    statistic_viewer_data->shown = TRUE;
    show_traceset_stats(statistic_viewer_data);
    statistic_remove_context_hooks(statistic_viewer_data,tsc);
  }

  return FALSE;
}

gboolean statistic_traceset_changed(void * hook_data, void * call_data)
{
  StatisticViewerData *statistic_viewer_data = (StatisticViewerData*) hook_data;
  
  //  gtk_tree_store_clear (statistic_viewer_data->store_m);  
  //  statistic_viewer_data->shown = FALSE;

  return FALSE;
}

void statistic_add_context_hooks(StatisticViewerData * statistic_viewer_data, 
		       LttvTracesetContext * tsc)
{
  gint i, j, nbi, nb_tracefile, nb_control, nb_per_cpu;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  LttvTracesetSelector  * ts_s;
  LttvTraceSelector     * t_s;
  LttvTracefileSelector * tf_s;
  gboolean selected;

  ts_s = (LttvTracesetSelector*)g_object_get_data(G_OBJECT(statistic_viewer_data->hpaned_v), 
						  statistic_viewer_data->filter_key);

  //if there are hooks for traceset, add them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, add them here

    nb_control = ltt_trace_control_tracefile_number(trace);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(trace);
    nb_tracefile = nb_control + nb_per_cpu;
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      
      if(j < nb_control)
	tfc = tc->control_tracefiles[j];
      else
	tfc = tc->per_cpu_tracefiles[j - nb_control];
      
      //if there are hooks for tracefile, add them here
      //      lttv_tracefile_context_add_hooks(tfc, NULL,NULL,NULL,NULL,
      //				       statistic_viewer_data->before_event_hooks,NULL);
    }
  }  

  //add state and stats hooks
  state_add_event_hooks_api(statistic_viewer_data->mw);
  stats_add_event_hooks_api(statistic_viewer_data->mw);
  
}

void statistic_remove_context_hooks(StatisticViewerData * statistic_viewer_data, 
			  LttvTracesetContext * tsc)
{
  gint i, j, nbi, nb_tracefile, nb_control, nb_per_cpu;
  LttTrace *trace;
  LttvTraceContext *tc;
  LttvTracefileContext *tfc;
  LttvTracesetSelector  * ts_s;
  LttvTraceSelector     * t_s;
  LttvTracefileSelector * tf_s;
  gboolean selected;

  ts_s = (LttvTracesetSelector*)g_object_get_data(G_OBJECT(statistic_viewer_data->hpaned_v), 
						  statistic_viewer_data->filter_key);

  //if there are hooks for traceset, remove them here
  
  nbi = lttv_traceset_number(tsc->ts);
  for(i = 0 ; i < nbi ; i++) {
    t_s = lttv_traceset_selector_get(ts_s,i);
    selected = lttv_trace_selector_get_selected(t_s);
    if(!selected) continue;
    tc = tsc->traces[i];
    trace = tc->t;
    //if there are hooks for trace, remove them here

    nb_control = ltt_trace_control_tracefile_number(trace);
    nb_per_cpu = ltt_trace_per_cpu_tracefile_number(trace);
    nb_tracefile = nb_control + nb_per_cpu;
    
    for(j = 0 ; j < nb_tracefile ; j++) {
      tf_s = lttv_trace_selector_get(t_s,j);
      selected = lttv_tracefile_selector_get_selected(tf_s);
      if(!selected) continue;
      
      if(j < nb_control)
	tfc = tc->control_tracefiles[j];
      else
	tfc = tc->per_cpu_tracefiles[j - nb_control];
      
      //if there are hooks for tracefile, remove them here
      //      lttv_tracefile_context_remove_hooks(tfc, NULL,NULL,NULL,NULL,
      //					  statistic_viewer_data->before_event_hooks,NULL);
    }
  }

  //remove state and stats hooks
  state_remove_event_hooks_api(statistic_viewer_data->mw);
  stats_remove_event_hooks_api(statistic_viewer_data->mw);
}


