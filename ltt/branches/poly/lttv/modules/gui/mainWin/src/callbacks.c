#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gmodule.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include <lttv/mainWindow.h>
#include <lttv/menu.h>
#include <lttv/toolbar.h>
#include <lttv/gtkTraceSet.h>
#include <lttv/module.h>
#include <lttv/gtkdirsel.h>
#include <lttv/iattribute.h>

#define PATH_LENGTH          256
#define DEFAULT_TIME_WIDTH_S   1

extern LttTrace *g_init_trace ;


/** Array containing instanced objects. */
extern GSList * g_main_window_list;

static int g_win_count = 0;

MainWindow * get_window_data_struct(GtkWidget * widget);
char * get_unload_module(char ** loaded_module_name, int nb_module);
char * get_remove_trace(char ** all_trace_name, int nb_trace);
char * get_selection(char ** all_name, int nb, char *title, char * column_title);
void * create_tab(GtkWidget* parent, GtkNotebook * notebook, char * label);

void insert_viewer(GtkWidget* widget, view_constructor constructor);

enum
{
  MODULE_COLUMN,
  N_COLUMNS
};


void
insert_viewer_wrap(GtkMenuItem *menuitem, gpointer user_data)
{
  guint val = 20;
  insert_viewer((GtkWidget*)menuitem, (view_constructor)user_data);
  //  selected_hook(&val);
}


/* internal functions */
void insert_viewer(GtkWidget* widget, view_constructor constructor)
{
  GtkCustom * custom;
  MainWindow * mw_data;  
  GtkWidget * viewer;

  mw_data = get_window_data_struct(widget);
  if(!mw_data->current_tab) return;
  custom = mw_data->current_tab->custom;

  viewer = (GtkWidget*)constructor(mw_data);
  if(viewer)
  {
    gtk_custom_widget_add(custom, viewer); 
    // Added by MD
    //    g_object_unref(G_OBJECT(viewer));
  }
}

void get_label_string (GtkWidget * text, gchar * label) 
{
  GtkEntry * entry = (GtkEntry*)text;
  if(strlen(gtk_entry_get_text(entry))!=0)
    strcpy(label,gtk_entry_get_text(entry)); 
}

void get_label(GtkWindow * mw, gchar * str, gchar* dialogue_title, gchar * label_str)
{
  GtkWidget * dialogue;
  GtkWidget * text;
  GtkWidget * label;
  gint id;

  dialogue = gtk_dialog_new_with_buttons(dialogue_title,NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 

  label = gtk_label_new(label_str);
  gtk_widget_show(label);

  text = gtk_entry_new();
  gtk_widget_show(text);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), label,TRUE, TRUE,0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), text,FALSE, FALSE,0);

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
      get_label_string(text,str);
      gtk_widget_destroy(dialogue);
      break;
    case GTK_RESPONSE_REJECT:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }
}

MainWindow * get_window_data_struct(GtkWidget * widget)
{
  GtkWidget * mw;
  MainWindow * mw_data;

  mw = lookup_widget(widget, "MWindow");
  if(mw == NULL){
    g_printf("Main window does not exist\n");
    return;
  }
  
  mw_data = (MainWindow *) g_object_get_data(G_OBJECT(mw),"mainWindow");
  if(mw_data == NULL){
    g_printf("Main window data does not exist\n");
    return;
  }
  return mw_data;
}

void create_new_window(GtkWidget* widget, gpointer user_data, gboolean clone)
{
  MainWindow * parent = get_window_data_struct(widget);

  if(clone){
    g_printf("Clone : use the same traceset\n");
    construct_main_window(parent, NULL, FALSE);
  }else{
    g_printf("Empty : traceset is set to NULL\n");
    construct_main_window(NULL, parent->win_creation_data, FALSE);
  }
}

void move_up_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_custom_widget_move_up(mw->current_tab->custom);
}

void move_down_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_custom_widget_move_down(mw->current_tab->custom);
}

void delete_viewer(GtkWidget * widget, gpointer user_data)
{
  MainWindow * mw = get_window_data_struct(widget);
  if(!mw->current_tab) return;
  gtk_custom_widget_delete(mw->current_tab->custom);
}

void open_traceset(GtkWidget * widget, gpointer user_data)
{
  char ** dir;
  gint id;
  LttvTraceset * traceset;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkFileSelection * file_selector = 
    (GtkFileSelection *)gtk_file_selection_new("Select a traceset");

  gtk_file_selection_hide_fileop_buttons(file_selector);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (file_selector);
      traceset = lttv_traceset_load(dir[0]);
      g_printf("Open a trace set %s\n", dir[0]); 
      //Not finished yet
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }

}

void add_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  char * dir;
  gint id;
  MainWindow * mw_data = get_window_data_struct(widget);
  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select a trace");
  gtk_dir_selection_hide_fileop_buttons(file_selector);
  
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (file_selector);
      trace = ltt_trace_open(dir);
      if(trace == NULL) g_critical("cannot open trace %s", dir);
      trace_v = lttv_trace_new(trace);
      traceset = mw_data->traceset_info->traceset;
      if(mw_data->traceset_info->traceset_context != NULL){
	lttv_context_fini(LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context));
	g_object_unref(mw_data->traceset_info->traceset_context);
      }
      lttv_traceset_add(traceset, trace_v);
      mw_data->traceset_info->traceset_context =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
      lttv_context_init(
	LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context),traceset);      
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }

  g_printf("add a trace to a trace set\n");
}

void remove_trace(GtkWidget * widget, gpointer user_data)
{
  LttTrace *trace;
  LttvTrace * trace_v;
  LttvTraceset * traceset;
  gint i, nb_trace;
  char ** name, *remove_trace_name;
  MainWindow * mw_data = get_window_data_struct(widget);
  
  nb_trace =lttv_traceset_number(mw_data->traceset_info->traceset); 
  name = g_new(char*,nb_trace);
  for(i = 0; i < nb_trace; i++){
    trace_v = lttv_traceset_get(mw_data->traceset_info->traceset, i);
    trace = lttv_trace(trace_v);
    name[i] = trace->pathname;
  }

  remove_trace_name = get_remove_trace(name, nb_trace);

  if(remove_trace_name){
    for(i=0; i<nb_trace; i++){
      if(strcmp(remove_trace_name,name[i]) == 0){
	traceset = mw_data->traceset_info->traceset;
	if(mw_data->traceset_info->traceset_context != NULL){
	  lttv_context_fini(LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context));
	  g_object_unref(mw_data->traceset_info->traceset_context);
	}
	lttv_traceset_remove(traceset, i);
	mw_data->traceset_info->traceset_context =
	  g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
	lttv_context_init(
	  LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context),traceset);      
	break;
      }
    }
  }

  g_free(name);
  g_printf("remove a trace from a trace set\n");
}

void save(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save\n");
}

void save_as(GtkWidget * widget, gpointer user_data)
{
  g_printf("Save as\n");
}

void zoom(GtkWidget * widget, double size)
{
  TimeInterval *time_span;
  TimeWindow time_window;
  LttTime    current_time, time_delta, time_s, time_e;
  MainWindow * mw_data = get_window_data_struct(widget);

  time_span = LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context)->Time_Span ;
  time_window =  mw_data->current_tab->time_window;
  current_time = mw_data->current_tab->current_time;
  
  time_delta = ltt_time_sub(time_span->endTime,time_span->startTime);
  if(size == 0){
    time_window.start_time = time_span->startTime;
    time_window.time_width = time_delta;
  }else{
    time_window.time_width = ltt_time_div(time_window.time_width, size);
    if(ltt_time_compare(time_window.time_width,time_delta) > 0)
      time_window.time_width = time_delta;        

    time_s = ltt_time_sub(current_time,ltt_time_div(time_window.time_width, 2));
    time_e = ltt_time_add(current_time,ltt_time_div(time_window.time_width, 2));
    if(ltt_time_compare(time_span->startTime, time_s) > 0){
      time_s = time_span->startTime;
    }else if(ltt_time_compare(time_span->endTime, time_e) < 0){
      time_e = time_span->endTime;
      time_s = ltt_time_sub(time_e,time_window.time_width);
    }
    time_window.start_time = time_s;    
  }
  set_time_window(mw_data, &time_window);
  gtk_custom_set_adjust(mw_data->current_tab->custom, FALSE);
}

void zoom_in(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 2);
}

void zoom_out(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 0.5);
}

void zoom_extended(GtkWidget * widget, gpointer user_data)
{
  zoom(widget, 0);
}

void go_to_time(GtkWidget * widget, gpointer user_data)
{
  g_printf("Go to time\n");
}

void show_time_frame(GtkWidget * widget, gpointer user_data)
{
  g_printf("Show time frame\n");  
}


/* callback function */

void
on_empty_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)menuitem, user_data, FALSE);
}


void
on_clone_traceset_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)menuitem, user_data, TRUE);
}


void
on_tab_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gchar label[PATH_LENGTH];
  GtkNotebook * notebook = (GtkNotebook *)lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }

  strcpy(label,"Page");
  get_label(NULL, label,"Get the name of the tab","Please input tab's name");

  create_tab ((GtkWidget*)menuitem, notebook, label);
}


void
on_open_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)menuitem, user_data);
}


void
on_close_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  main_window_destructor(mw_data);  
}


void
on_close_tab_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  int count = 0;
  GtkWidget * notebook;
  Tab * tmp;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  notebook = lookup_widget((GtkWidget*)menuitem, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  
  if(mw_data->tab == mw_data->current_tab){
    //    tmp = mw_data->current_tb;
    //    mw_data->tab = mw_data->current_tab->next;
    g_printf("The default TAB can not be deleted\n");
    return;
  }else{
    tmp = mw_data->tab;
    while(tmp != mw_data->current_tab){
      tmp = tmp->next;
      count++;
    }
  }

  gtk_notebook_remove_page((GtkNotebook*)notebook, count);  
}


void
on_add_trace_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)menuitem, user_data);
}


void
on_remove_trace_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)menuitem, user_data);
}


void
on_save_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save((GtkWidget*)menuitem, user_data);
}


void
on_save_as_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)menuitem, user_data);
}


void
on_quit_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_main_quit ();
}


void
on_cut_activate                        (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Cut\n");
}


void
on_copy_activate                       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Copye\n");
}


void
on_paste_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Paste\n");
}


void
on_delete_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Delete\n");
}


void
on_zoom_in_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_out_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)menuitem, user_data); 
}


void
on_zoom_extended_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)menuitem, user_data); 
}


void
on_go_to_time_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)menuitem, user_data); 
}


void
on_show_time_frame_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)menuitem, user_data); 
}


void
on_move_viewer_up_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)menuitem, user_data);
}


void
on_move_viewer_down_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)menuitem, user_data);
}


void
on_remove_viewer_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)menuitem, user_data);
}


void
on_load_module_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  char ** dir;
  gint id;
  char str[PATH_LENGTH], *str1;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  GtkFileSelection * file_selector = (GtkFileSelection *)gtk_file_selection_new("Select a module");
  gtk_file_selection_hide_fileop_buttons(file_selector);
  
  str[0] = '\0';
  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_file_selection_get_selections (file_selector);
      sprintf(str,dir[0]);
      str1 = strrchr(str,'/');
      if(str1)str1++;
      else{
	str1 = strrchr(str,'\\');
	str1++;
      }
      if(mw_data->win_creation_data)
	lttv_module_load(str1, mw_data->win_creation_data->argc,mw_data->win_creation_data->argv);
      else
	lttv_module_load(str1, 0,NULL);
      g_slist_foreach(g_main_window_list, insert_menu_toolbar_item, NULL);
      g_strfreev(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
  g_printf("Load module: %s\n", str);
}


void
on_unload_module_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  int i;
  char **name, *unload_module_name;
  guint nb;
  LttvModule ** modules, *module;
  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);
  
  modules = lttv_module_list(&nb);
  name  = g_new(char*, nb);
  for(i=0;i<nb;i++){
    module = modules[i];
    name[i] = lttv_module_name(module);
  }

  unload_module_name =get_unload_module(name,nb);
  
  if(unload_module_name){
    for(i=0;i<nb;i++){
      if(strcmp(unload_module_name, name[i]) == 0){
	lttv_module_unload(modules[i]);
	break;
      }
    }    
  }

  g_free(name);
}


void
on_add_module_search_path_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  GtkDirSelection * file_selector = (GtkDirSelection *)gtk_dir_selection_new("Select module path");
  char * dir;
  gint id;

  MainWindow * mw_data = get_window_data_struct((GtkWidget*)menuitem);

  id = gtk_dialog_run(GTK_DIALOG(file_selector));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      dir = gtk_dir_selection_get_dir (file_selector);
      lttv_module_path_add(dir);
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy((GtkWidget*)file_selector);
      break;
  }
}


void
on_color_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Color\n");
}


void
on_filter_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Filter\n");
}


void
on_save_configuration_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Save configuration\n");
}


void
on_content_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("Content\n");
}


void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  g_printf("About...\n");
}


void
on_button_new_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
  create_new_window((GtkWidget*)button, user_data, FALSE);
}


void
on_button_open_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  open_traceset((GtkWidget*)button, user_data);
}


void
on_button_add_trace_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  add_trace((GtkWidget*)button, user_data);
}


void
on_button_remove_trace_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
  remove_trace((GtkWidget*)button, user_data);
}


void
on_button_save_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  save((GtkWidget*)button, user_data);
}


void
on_button_save_as_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  save_as((GtkWidget*)button, user_data);
}


void
on_button_zoom_in_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_in((GtkWidget*)button, user_data); 
}


void
on_button_zoom_out_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_out((GtkWidget*)button, user_data); 
}


void
on_button_zoom_extended_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
   zoom_extended((GtkWidget*)button, user_data); 
}


void
on_button_go_to_time_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
   go_to_time((GtkWidget*)button, user_data); 
}


void
on_button_show_time_frame_clicked      (GtkButton       *button,
                                        gpointer         user_data)
{
   show_time_frame((GtkWidget*)button, user_data); 
}


void
on_button_move_up_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  move_up_viewer((GtkWidget*)button, user_data);
}


void
on_button_move_down_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  move_down_viewer((GtkWidget*)button, user_data);
}


void
on_button_delete_viewer_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  delete_viewer((GtkWidget*)button, user_data);
}

void
on_MWindow_destroy                     (GtkObject       *object,
                                        gpointer         user_data)
{
  MainWindow *Main_Window = (MainWindow*)user_data;
  
  g_printf("There are : %d windows\n",g_slist_length(g_main_window_list));

  g_win_count--;
  if(g_win_count == 0)
    gtk_main_quit ();
}


void
on_MNotebook_switch_page               (GtkNotebook     *notebook,
                                        GtkNotebookPage *page,
                                        guint            page_num,
                                        gpointer         user_data)
{
  MainWindow * mw = get_window_data_struct((GtkWidget*)notebook);
  Tab * tab = mw->tab;
 
  while(page_num){
    tab = tab->next;
    page_num--;
  }
  mw->current_tab = tab;
}

char * get_remove_trace(char ** all_trace_name, int nb_trace)
{
  return get_selection(all_trace_name, nb_trace, 
		       "Select a trace", "Trace pathname");
}
char * get_unload_module(char ** loaded_module_name, int nb_module)
{
  return get_selection(loaded_module_name, nb_module, 
		       "Select an unload module", "Module pathname");
}

char * get_selection(char ** loaded_module_name, int nb_module,
		     char *title, char * column_title)
{
  GtkWidget         * dialogue;
  GtkWidget         * scroll_win;
  GtkWidget         * tree;
  GtkListStore      * store;
  GtkTreeViewColumn * column;
  GtkCellRenderer   * renderer;
  GtkTreeSelection  * select;
  GtkTreeIter         iter;
  gint                id, i;
  char              * unload_module_name = NULL;

  dialogue = gtk_dialog_new_with_buttons(title,
					 NULL,
					 GTK_DIALOG_MODAL,
					 GTK_STOCK_OK,GTK_RESPONSE_ACCEPT,
					 GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT,
					 NULL); 
  gtk_window_set_default_size((GtkWindow*)dialogue, 500, 200);

  scroll_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show ( scroll_win);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_win), 
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  store = gtk_list_store_new (N_COLUMNS,G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));
  gtk_widget_show ( tree);
  g_object_unref (G_OBJECT (store));
		
  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes (column_title,
						     renderer,
						     "text", MODULE_COLUMN,
						     NULL);
  gtk_tree_view_column_set_alignment (column, 0.5);
  gtk_tree_view_column_set_fixed_width (column, 150);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  gtk_container_add (GTK_CONTAINER (scroll_win), tree);  

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialogue)->vbox), scroll_win,TRUE, TRUE,0);

  for(i=0;i<nb_module;i++){
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, MODULE_COLUMN,loaded_module_name[i],-1);
  }

  id = gtk_dialog_run(GTK_DIALOG(dialogue));
  switch(id){
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_OK:
      if (gtk_tree_selection_get_selected (select, (GtkTreeModel**)&store, &iter)){
	  gtk_tree_model_get ((GtkTreeModel*)store, &iter, MODULE_COLUMN, &unload_module_name, -1);
      }
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:
    default:
      gtk_widget_destroy(dialogue);
      break;
  }

  return unload_module_name;
}

void main_window_destroy_hash_key(gpointer key)
{
  g_free(key);
}

void main_window_destroy_hash_data(gpointer data)
{
}


void insert_menu_toolbar_item(MainWindow * mw, gpointer user_data)
{
  int i;
  GdkPixbuf *pixbuf;
  view_constructor constructor;
  LttvMenus * menu;
  LttvToolbars * toolbar;
  lttv_menu_closure *menu_item;
  lttv_toolbar_closure *toolbar_item;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  GtkWidget * tool_menu_title_menu, *insert_view, *pixmap, *tmp;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(menu){
    for(i=0;i<menu->len;i++){
      menu_item = &g_array_index(menu, lttv_menu_closure, i);
      tmp = g_hash_table_lookup(mw->hash_menu_item, g_strdup(menu_item->menuText));
      if(tmp)continue;
      constructor = menu_item->con;
      tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
      insert_view = gtk_menu_item_new_with_mnemonic (menu_item->menuText);
      gtk_widget_show (insert_view);
      gtk_container_add (GTK_CONTAINER (tool_menu_title_menu), insert_view);
      g_signal_connect ((gpointer) insert_view, "activate",
			G_CALLBACK (insert_viewer_wrap),
			constructor);  
      g_hash_table_insert(mw->hash_menu_item, g_strdup(menu_item->menuText),
			  insert_view);
    }
  }

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar){
    for(i=0;i<toolbar->len;i++){
      toolbar_item = &g_array_index(toolbar, lttv_toolbar_closure, i);
      tmp = g_hash_table_lookup(mw->hash_toolbar_item, g_strdup(toolbar_item->tooltip));
      if(tmp)continue;
      constructor = toolbar_item->con;
      tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar2");
      pixbuf = gdk_pixbuf_new_from_xpm_data ((const char**)toolbar_item->pixmap);
      pixmap = gtk_image_new_from_pixbuf(pixbuf);
      insert_view = gtk_toolbar_append_element (GTK_TOOLBAR (tool_menu_title_menu),
						GTK_TOOLBAR_CHILD_BUTTON,
						NULL,
						"",
						toolbar_item->tooltip, NULL,
						pixmap, NULL, NULL);
      gtk_label_set_use_underline (GTK_LABEL (((GtkToolbarChild*) (g_list_last (GTK_TOOLBAR (tool_menu_title_menu)->children)->data))->label), TRUE);
      gtk_widget_show (insert_view);
      gtk_container_set_border_width (GTK_CONTAINER (insert_view), 1);
      g_signal_connect ((gpointer) insert_view, "clicked",G_CALLBACK (insert_viewer_wrap),constructor);       
      g_hash_table_insert(mw->hash_toolbar_item, g_strdup(toolbar_item->tooltip),
			  insert_view);
    }
  }
}

void construct_main_window(MainWindow * parent, WindowCreationData * win_creation_data,
		      gboolean first_window)
{
  g_critical("construct_main_window()");
  GtkWidget  * new_window; /* New generated main window */
  MainWindow * new_m_window;/* New main window structure */
  GtkNotebook * notebook;
  LttvIAttribute *attributes =
	  LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  LttvAttributeValue value;
         
  new_m_window = g_new(MainWindow, 1);

  // Add the object's information to the module's array 
  g_main_window_list = g_slist_append(g_main_window_list, new_m_window);


  new_window  = create_MWindow();
  gtk_widget_show (new_window);
    
  new_m_window->attributes = attributes;
  
  new_m_window->traceset_info = g_new(TracesetInfo,1);
  new_m_window->traceset_info->path = NULL ;


  new_m_window->traceset_info->before_traceset = lttv_hooks_new();
  new_m_window->traceset_info->after_traceset = lttv_hooks_new();
  new_m_window->traceset_info->before_trace = lttv_hooks_new();
  new_m_window->traceset_info->after_trace = lttv_hooks_new();
  new_m_window->traceset_info->before_tracefile = lttv_hooks_new();
  new_m_window->traceset_info->after_tracefile = lttv_hooks_new();
  new_m_window->traceset_info->before_event = lttv_hooks_new();
  new_m_window->traceset_info->after_event = lttv_hooks_new();

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->before_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->after_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->before_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->after_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->before_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->after_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->before_event;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = new_m_window->traceset_info->after_event;
 
  
  new_m_window->mwindow = new_window;
  new_m_window->tab = NULL;
  new_m_window->current_tab = NULL;
  new_m_window->attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  if(parent){
    new_m_window->traceset_info->traceset = 
        lttv_traceset_copy(parent->traceset_info->traceset);
    
//FIXME copy not implemented in lower level
    new_m_window->traceset_info->traceset_context =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    lttv_context_init(
	LTTV_TRACESET_CONTEXT(new_m_window->traceset_info->traceset_context),
	new_m_window->traceset_info->traceset);
  //new_m_window->traceset_context = parent->traceset_context;
    new_m_window->win_creation_data = parent->win_creation_data;
  }else{
    new_m_window->traceset_info->traceset = lttv_traceset_new();

    /* Add the command line trace */
    if(g_init_trace != NULL && first_window)
      lttv_traceset_add(new_m_window->traceset_info->traceset, g_init_trace);
    /* NOTE : the context must be recreated if we change the traceset,
     * ie : adding/removing traces */
    new_m_window->traceset_info->traceset_context =
  	g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    lttv_context_init(
	LTTV_TRACESET_CONTEXT(new_m_window->traceset_info->traceset_context),
	new_m_window->traceset_info->traceset);

    new_m_window->win_creation_data = win_creation_data;
  }

  new_m_window->hash_menu_item = g_hash_table_new_full (g_str_hash, g_str_equal,
					      main_window_destroy_hash_key, 
					      main_window_destroy_hash_data);
  new_m_window->hash_toolbar_item = g_hash_table_new_full (g_str_hash, g_str_equal,
					      main_window_destroy_hash_key, 
					      main_window_destroy_hash_data);

  insert_menu_toolbar_item(new_m_window, NULL);
  
  g_object_set_data(G_OBJECT(new_window), "mainWindow", (gpointer)new_m_window);    

  //create a default tab
  notebook = (GtkNotebook *)lookup_widget(new_m_window->mwindow, "MNotebook");
  if(notebook == NULL){
    g_printf("Notebook does not exist\n");
    return;
  }
  //for now there is no name field in LttvTraceset structure
  //Use "Traceset" as the label for the default tab
  create_tab(new_m_window->mwindow, notebook,"Traceset");

  g_object_set_data_full(
			G_OBJECT(new_m_window->mwindow),
			"Main_Window_Data",
			new_m_window,
			(GDestroyNotify)main_window_free);

  g_win_count++;
}

void tab_destructor(Tab * tab_instance)
{
  if(tab_instance->attributes)
    g_object_unref(tab_instance->attributes);  

  if(tab_instance->mw->tab == tab_instance){
    tab_instance->mw->tab = tab_instance->next;
  }else{
    Tab * tmp1, *tmp = tab_instance->mw->tab;
    while(tmp != tab_instance){
      tmp1 = tmp;
      tmp = tmp->next;
    }
    tmp1->next = tab_instance->next;
  }
  g_free(tab_instance);
}

void * create_tab(GtkWidget* parent, GtkNotebook * notebook, char * label)
{
  GList * list;
  Tab * tmp_tab;
  MainWindow * mw_data;
  LttTime tmp_time;

  mw_data = get_window_data_struct(parent);
  tmp_tab = mw_data->tab;
  while(tmp_tab && tmp_tab->next) tmp_tab = tmp_tab->next;
  if(!tmp_tab){
    mw_data->current_tab = NULL;
    tmp_tab = g_new(Tab,1);
    mw_data->tab = tmp_tab;    
  }else{
    tmp_tab->next = g_new(Tab,1);
    tmp_tab = tmp_tab->next;
  }
  if(mw_data->current_tab){
 // Will have to read directly at the main window level, as we want
 // to be able to modify a traceset on the fly.
 //   tmp_tab->traceStartTime = mw_data->current_tab->traceStartTime;
 //   tmp_tab->traceEndTime   = mw_data->current_tab->traceEndTime;
    tmp_tab->time_window      = mw_data->current_tab->time_window;
    tmp_tab->current_time     = mw_data->current_tab->current_time;
  }else{
 // Will have to read directly at the main window level, as we want
 // to be able to modify a traceset on the fly.
  //  get_traceset_time_span(mw_data,&tmp_tab->traceStartTime, &tmp_tab->traceEndTime);
    tmp_tab->time_window.start_time   = 
	    LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context)->Time_Span->startTime;
    if(DEFAULT_TIME_WIDTH_S <
              LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context)->Time_Span->endTime.tv_sec)
      tmp_time.tv_sec = DEFAULT_TIME_WIDTH_S;
    else
      tmp_time.tv_sec =
              LTTV_TRACESET_CONTEXT(mw_data->traceset_info->traceset_context)->Time_Span->endTime.tv_sec;
    tmp_time.tv_nsec = 0;
    tmp_tab->time_window.time_width = tmp_time ;
    tmp_tab->current_time.tv_sec = tmp_time.tv_sec / 2;
    tmp_tab->current_time.tv_nsec = 0 ;
  }
  tmp_tab->attributes = LTTV_IATTRIBUTE(g_object_new(LTTV_ATTRIBUTE_TYPE, NULL));
  //  mw_data->current_tab = tmp_tab;
  tmp_tab->custom = (GtkCustom*)gtk_custom_new();
  tmp_tab->custom->mw = mw_data;
  gtk_widget_show((GtkWidget*)tmp_tab->custom);
  tmp_tab->next = NULL;    
  tmp_tab->mw   = mw_data;

  tmp_tab->label = gtk_label_new (label);
  gtk_widget_show (tmp_tab->label);

  g_object_set_data_full(
           G_OBJECT(tmp_tab->custom),
           "Tab_Info",
	   tmp_tab,
	   (GDestroyNotify)tab_destructor);
  
  gtk_notebook_append_page(notebook, (GtkWidget*)tmp_tab->custom, tmp_tab->label);  
  list = gtk_container_get_children(GTK_CONTAINER(notebook));
  gtk_notebook_set_current_page(notebook,g_list_length(list)-1);
}

void remove_menu_item(gpointer main_win, gpointer user_data)
{
  MainWindow * mw = (MainWindow *) main_win;
  lttv_menu_closure *menu_item = (lttv_menu_closure *)user_data;
  GtkWidget * tool_menu_title_menu, *insert_view;

  tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
  insert_view = (GtkWidget*)g_hash_table_lookup(mw->hash_menu_item,
						menu_item->menuText);
  if(insert_view){
    g_hash_table_remove(mw->hash_menu_item, menu_item->menuText);
    gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), insert_view);
  }
}

void remove_toolbar_item(gpointer main_win, gpointer user_data)
{
  MainWindow * mw = (MainWindow *) main_win;
  lttv_toolbar_closure *toolbar_item = (lttv_toolbar_closure *)user_data;
  GtkWidget * tool_menu_title_menu, *insert_view;


  tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar2");
  insert_view = (GtkWidget*)g_hash_table_lookup(mw->hash_toolbar_item,
						toolbar_item->tooltip);
  if(insert_view){
    g_hash_table_remove(mw->hash_toolbar_item, toolbar_item->tooltip);
    gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), insert_view);
  }
}

/**
 * Remove menu and toolbar item when a module unloaded
 */

void main_window_remove_menu_item(lttv_constructor constructor)
{
  int i;
  LttvMenus * menu;
  lttv_menu_closure *menu_item;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(menu){
    for(i=0;i<menu->len;i++){
      menu_item = &g_array_index(menu, lttv_menu_closure, i);
      if(menu_item->con != constructor) continue;
      if(g_main_window_list){
	g_slist_foreach(g_main_window_list, remove_menu_item, menu_item);
      }
      break;
    }
  }
  
}

void main_window_remove_toolbar_item(lttv_constructor constructor)
{
  int i;
  LttvToolbars * toolbar;
  lttv_toolbar_closure *toolbar_item;
  LttvAttributeValue value;
  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar){
    for(i=0;i<toolbar->len;i++){
      toolbar_item = &g_array_index(toolbar, lttv_toolbar_closure, i);
      if(toolbar_item->con != constructor) continue;
      if(g_main_window_list){
	g_slist_foreach(g_main_window_list, remove_toolbar_item, toolbar_item);
      }
      break;
    }
  }
}
