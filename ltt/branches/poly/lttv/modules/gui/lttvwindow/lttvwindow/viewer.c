/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 XangXiu Yang
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

/*! \file lttvviewer.c
 * \brief API used by the graphical viewers to interact with their top window.
 * 
 * Main window (gui module) is the place to contain and display viewers. 
 * Viewers (lttv plugins) interacte with main window through this API and
 * events sent by gtk.
 * This header file should be included in each graphic module.
 * This library is used by graphical modules to interact with the
 * tracesetWindow.
 * 
 */

#include <ltt/ltt.h>
#include <lttv/lttv.h>
#include <lttv/state.h>
#include <lttv/stats.h>
#include <lttv/tracecontext.h>
#include <lttvwindow/common.h>
#include <lttvwindow/mainwindow.h>   
#include <lttvwindow/viewer.h>
#include <lttvwindow/toolbar.h>
#include <lttvwindow/menu.h>
#include <lttvwindow/callbacks.h> // for execute_time_requests


/**
 * Internal function parts
 */

/**
 * Function to set/update traceset for the viewers
 * @param main_win main window 
 * @param traceset traceset of the main window.
 * return value :
 * -1 : error
 *  0 : traceset updated
 *  1 : no traceset hooks to update; not an error.
 */

int SetTraceset(MainWindow * main_win, gpointer traceset)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  if( lttv_iattribute_find_by_path(main_win->attributes,
     "hooks/updatetraceset", LTTV_POINTER, &value) != 0)
    return -1;

  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return 1;
  

  lttv_hooks_call(tmp,traceset);
  
  return 0;
}


/**
 * Function to set/update filter for the viewers
 * @param main_win main window 
 * @param filter filter of the main window.
 * return value :
 * -1 : error
 *  0 : filters updated
 *  1 : no filter hooks to update; not an error.
 */

int SetFilter(MainWindow * main_win, gpointer filter)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  if(lttv_iattribute_find_by_path(main_win->attributes,
     "hooks/updatefilter", LTTV_POINTER, &value) != 0)
    return -1;

  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL) return 1;
  lttv_hooks_call(tmp,filter);

  return 0;
}

/**
 * Function to redraw each viewer belonging to the current tab 
 * @param main_win the main window the viewer belongs to.
 */

void update_traceset(MainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, NULL);
}

void set_time_window_adjustment(MainWindow * main_win, const TimeWindow* new_time_window)
{
  gtk_multi_vpaned_set_adjust(main_win->current_tab->multi_vpaned, new_time_window, FALSE);
}


void set_time_window(MainWindow * main_win, const TimeWindow *time_window)
{
  LttvAttributeValue value;
  LttvHooks * tmp;

  TimeWindowNotifyData time_window_notify_data;
  TimeWindow old_time_window = main_win->current_tab->time_window;
  time_window_notify_data.old_time_window = &old_time_window;
  main_win->current_tab->time_window = *time_window;
  time_window_notify_data.new_time_window = 
                          &(main_win->current_tab->time_window);

  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &time_window_notify_data);


}

void add_toolbar_constructor(MainWindow *mw, LttvToolbarClosure *toolbar_c)
{
  LttvIAttribute *attributes = LTTV_IATTRIBUTES(mw->attributes);
  LttvAttributeValue value;
  LttvToolbars * instance_toolbar;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *new_widget, *pixmap;
  GdkPixbuf *pixbuf;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    (LttvToolbars*)*(value.v_pointer) = lttv_toolbars_new();
  instance_toolbar = (LttvToolbars*)*(value.v_pointer);

  constructor = toolbar_c->con;
  tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar1");
  pixbuf = gdk_pixbuf_new_from_xpm_data((const char**)toolbar_c->pixmap);
  pixmap = gtk_image_new_from_pixbuf(pixbuf);
  new_widget =
     gtk_toolbar_append_element (GTK_TOOLBAR (tool_menu_title_menu),
        GTK_TOOLBAR_CHILD_BUTTON,
        NULL,
        "",
        toolbar_item->tooltip, NULL,
        pixmap, NULL, NULL);
  gtk_label_set_use_underline(
      GTK_LABEL (((GtkToolbarChild*) (
                       g_list_last (GTK_TOOLBAR 
                          (tool_menu_title_menu)->children)->data))->label),
      TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (new_widget), 1);
  g_signal_connect ((gpointer) new_widget,
      "clicked",
      G_CALLBACK (insert_viewer_wrap),
      constructor);       
  gtk_widget_show (new_widget);

  lttv_toolbars_add(instance_toolbar, toolbar_item_i->con, 
                    toolbar_item_i->tooltip,
                    toolbar_item_i->pixmap,
                    new_widget);

}

void add_menu_constructor(MainWindow *mw, LttvMenuClosure *menu_c)
{
  LttvIAttribute *attributes = LTTV_IATTRIBUTES(mw->attributes);
  LttvAttributeValue value;
  LttvToolbars * instance_menu;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *new_widget;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    (LttvMenus*)*(value.v_pointer) = lttv_menus_new();
  instance_menu = (LttvMenus*)*(value.v_pointer);


  constructor = menu_item->con;
  tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
  new_widget =
          gtk_menu_item_new_with_mnemonic (menu_c->menuText);
  gtk_container_add (GTK_CONTAINER (tool_menu_title_menu),
          new_widget);
  g_signal_connect ((gpointer) new_widget, "activate",
                      G_CALLBACK (insert_viewer_wrap),
                      constructor);
  gtk_widget_show (new_widget);
  lttv_menus_add(instance_menu, menu_c->con, 
                    menu_c->menu_path,
                    menu_c->menu_text,
                    new_widget);
}

void remove_toolbar_constructor(MainWindow *mw, lttvwindow_viewer_constructor viewer_constructor)
{
  LttvIAttribute *attributes = LTTV_IATTRIBUTES(mw->attributes);
  LttvAttributeValue value;
  LttvToolbars * instance_toolbar;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *widget;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/toolbar", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    (LttvToolbars*)*(value.v_pointer) = lttv_toolbars_new();
  instance_toolbar = (LttvToolbars*)*(value.v_pointer);

  tool_menu_title_menu = lookup_widget(mw->mwindow,"MToolbar1");
  widget = lttv_menus_remove(instance_toolbar, viewer_constructor);
  gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), 
                        widget);
}


void remove_menu_constructor(MainWindow *mw, lttvwindow_viewer_constructor viewer_constructor)
{
  LttvIAttribute *attributes = LTTV_IATTRIBUTES(mw->attributes);
  LttvAttributeValue value;
  LttvMenus * instance_menu;
  lttvwindow_viewer_constructor constructor;
  GtkWidget * tool_menu_title_menu, *widget;
  LttvMenuClosure *menu_item_i;

  g_assert(lttv_iattribute_find_by_path(attributes,
	   "viewers/menu", LTTV_POINTER, &value));
  if(*(value.v_pointer) == NULL)
    (LttvMenus*)*(value.v_pointer) = lttv_menus_new();
  instance_menu = (LttvMenus*)*(value.v_pointer);

  widget = lttv_menus_remove(instance_menu, viewer_constructor);
  tool_menu_title_menu = lookup_widget(mw->mwindow,"ToolMenuTitle_menu");
  gtk_container_remove (GTK_CONTAINER (tool_menu_title_menu), widget);
}


/**
 * API parts
 */


/**
 * Function to register a view constructor so that main window can generate
 * a menu item and a toolbar item for the viewer in order to generate a new
 * instance easily. A menu entry and toolbar item will be added to each main
 * window.
 * 
 * It should be called by init function of the module.
 * 
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param pixmap Image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_constructor
                            (char *  menu_path, 
                             char *  menu_text,
                             char ** pixmap,
                             char *  tooltip,
                             lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvMenus * menu;
  LttvToolbarClosure toolbar_c;
  LttvMenuClosure menu_c;
  LttvAttributeValue value;

  if(pixmap != NULL) {
    g_assert(lttv_iattribute_find_by_path(attributes_global,
       "viewers/toolbar", LTTV_POINTER, &value));
    toolbar = (LttvToolbars*)*(value.v_pointer);

    if(toolbar == NULL) {
      toolbar = lttv_toolbars_new();
      *(value.v_pointer) = toolbar;
    }
    toolbar_c = lttv_toolbars_add(toolbar, view_constructor, tooltip, pixmap);

    g_slist_foreach(g_main_window_list,
                    (gpointer)add_toolbar_constructor,
                    &toolbar_c);
  }

  if(menu_path != NULL) {
    g_assert(lttv_iattribute_find_by_path(attributes_global,
       "viewers/menu", LTTV_POINTER, &value));
    menu = (LttvMenus*)*(value.v_pointer);
    
    if(menu == NULL) {
      menu = lttv_menus_new();
      *(value.v_pointer) = menu;
    }
    menu_c = lttv_menus_add(menu, view_constructor, menu_path, menu_text);

    g_slist_foreach(g_main_window_list,
                    (gpointer)add_menu_constructor,
                    &menu_c);
  }
}


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text, pixmap, tooltip and constructor of the
 * viewer.
 * 
 * It will be called when a module is unloaded.
 * 
 * @param view_constructor constructor of the viewer.
 */


void lttvwindow_unregister_constructor
                  (lttvwindow_viewer_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);
  
  if(toolbar != NULL) {
    g_slist_foreach(g_main_window_list,
                    (gpointer)remove_toolbar_constructor,
                    view_constructor);
    lttv_toolbars_remove(toolbar, view_constructor);
  }

  g_assert(lttv_iattribute_find_by_path(attributes_global,
     "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);
  
  if(menu != NULL) {
    g_slist_foreach(g_main_window_list,
                    (gpointer)remove_menu_constructor,
                    view_constructor);
    lttv_menus_remove(menu, view_constructor);
  }
}


/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */
void lttvwindow_register_time_window_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook,hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_time_window_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetimewindow", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_traceset_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_traceset_notify(MainWindow * main_win,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_filter_notify(MainWindow *main_win,
      LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_filter_notify(MainWindow * main_win,
                                         LttvHook hook,
                                         gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->attributes,
           "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_current_time_notify(MainWindow *main_win,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_current_time_notify(MainWindow * main_win,
            LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to show 
 * the content of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_show_notify(MainWindow *main_win,
          LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * show the content of the viewer..
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_show_notify(MainWindow * main_win,
              LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/showviewer", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}

/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_dividor(MainWindow *main_win,
                    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL){    
    tmp = lttv_hooks_new();
    *(value.v_pointer) = tmp;
  }
  lttv_hooks_add(tmp, hook, hook_data);
}


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_dividor(MainWindow *main_win,
                    LttvHook hook, gpointer hook_data)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void lttvwindow_report_status(MainWindow *main_win, const char *info)
{ 
  //FIXME
  g_warning("update_status not implemented in viewer.c");
}

/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void lttvwindow_report_time_window(MainWindow *main_win,
                                   const TimeWindow *time_window)
{
  set_time_window(main_win, time_window);
  set_time_window_adjustment(main_win, time_window);
}


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void lttvwindow_report_current_time(MainWindow *main_win,
                                    const LttTime *time)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  main_win->current_tab->current_time = *time;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp, &main_win->current_tab->current_time);
}

/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void lttvwindow_report_dividor(MainWindow *main_win, gint position)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->current_tab->attributes,
           "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &position);
}

/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param top_widget the top widget containing all the other widgets of the
 *                   viewer.
 */

void lttvwindow_report_focus(MainWindow *main_win, GtkWidget *top_widget)
{
  gtk_multi_vpaned_set_focus((GtkWidget*)main_win->current_tab->multi_vpaned,
                             gtk_widget_get_parent(top_widget));  
}


/**
 * Function to request data in a specific time interval to the main window.
 * The main window will use this time interval and the others present
 * to get the data from the process trace.
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */

void lttvwindow_time_interval_request(MainWindow *main_win,
          TimeWindow time_requested, guint num_events,
          LttvHook after_process_traceset,
          gpointer after_process_traceset_data)
{
  TimeRequest time_request;

  time_request.time_window = time_requested;
  time_request.num_events = num_events;
  time_request.after_hook = after_process_traceset;
  time_request.after_hook_data = after_process_traceset_data;

  g_array_append_val(main_win->current_tab->time_requests, time_request);
  
  if(!main_win->current_tab->time_request_pending)
  {
    /* Redraw has +20 priority. We want a prio higher than that, so +19 */
    g_idle_add_full((G_PRIORITY_HIGH_IDLE + 19),
                    (GSourceFunc)execute_time_requests,
                    main_win,
                    NULL);
    main_win->current_tab->time_request_pending = TRUE;
  }
}

/**
 * Function to get the current time interval shown on the current tab.
 * It will be called by a viewer's hook function to update the 
 * shown time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

const TimeWindow *lttvwindow_get_time_window(MainWindow *main_win)
{
  //time_window->start_time = main_win->current_tab->time_window.start_time;
  //time_window->time_width = main_win->current_tab->time_window.time_width;
  return &(main_win->current_tab->time_window);
  
}


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

const LttTime *lttvwindow_get_current_time(MainWindow *main_win)
{
  return &(main_win->current_tab->current_time);
}


/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */
const lttv_filter *lttvwindow_get_filter(MainWindow *main_win)
{
  //FIXME
  g_warning("lttvwindow_get_filter not implemented in viewer.c");
}


/**
 * Function to get the stats of the traceset 
 * @param main_win the main window the viewer belongs to.
 */

LttvTracesetStats* lttvwindow_get_traceset_stats(MainWindow *main_win)
{
  return main_win->current_tab->traceset_info->traceset_context;
}


LttvTracesetContext* lttvwindow_get_traceset_context(MainWindow *main_win)
{
  return (LttvTracesetContext*)main_win->current_tab->traceset_info->traceset_context;
}
