/*! \file gtkTraceSet.h
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

#include <lttv/common.h>
#include <ltt/ltt.h>
#include <lttv/lttv.h>
#include <lttv/mainWindow.h>   
#include <lttv/gtkTraceSet.h>
#include <lttv/processTrace.h>
#include <lttv/toolbar.h>
#include <lttv/menu.h>

/**
 * Internal function parts
 */

/**
 * Function to remove toolbar from the GUI
 * @param view_constructor constructor of the viewer.
 */

void RemoveToolbar(lttv_constructor view_constructor)
{
  g_printf("Toolbar for the viewer will be removed\n");
}

/**
 * Function to remove menu entry from the GUI
 * @param view_constructor constructor of the viewer.
 */

void RemoveMenu(lttv_constructor view_constructor)
{
  g_printf("Menu entry for the viewer will be removed\n");
}


/**
 * Function to set/update traceset for the viewers
 * @param main_win main window 
 * @param traceset traceset of the main window.
 */

void SetTraceset(mainWindow * main_win, gpointer traceset)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
	   "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL)return;
  lttv_hooks_call(tmp,traceset);
}


/**
 * Function to set/update filter for the viewers
 * @param main_win main window 
 * @param filter filter of the main window.
 */

void SetFilter(mainWindow * main_win, gpointer filter)
{
  LttvHooks * tmp;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
	   "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp,filter);
}



/**
 * API parts
 */

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param ButtonPixmap image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void ToolbarItemReg(char ** pixmap, char *tooltip, lttv_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);

  if(toolbar == NULL){    
    toolbar = lttv_toolbars_new();
    *(value.v_pointer) = toolbar;
  }
  lttv_toolbars_add(toolbar, view_constructor, tooltip, pixmap);
}


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void ToolbarItemUnreg(lttv_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvToolbars * toolbar;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
	   "viewers/toolbar", LTTV_POINTER, &value));
  toolbar = (LttvToolbars*)*(value.v_pointer);
  
  if(lttv_toolbars_remove(toolbar, view_constructor))
    RemoveToolbar(view_constructor);
}


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void MenuItemReg(char *menu_path, char *menu_text, lttv_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
	   "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);
  
  if(menu == NULL){    
    menu = lttv_menus_new();
    *(value.v_pointer) = menu;
  }
  lttv_menus_add(menu, view_constructor, menu_path, menu_text);
}

/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void MenuItemUnreg(lttv_constructor view_constructor)
{
  LttvIAttribute *attributes_global = LTTV_IATTRIBUTE(lttv_global_attributes());
  LttvMenus * menu;
  LttvAttributeValue value;

  g_assert(lttv_iattribute_find_by_path(attributes_global,
                              "viewers/menu", LTTV_POINTER, &value));
  menu = (LttvMenus*)*(value.v_pointer);

  if(lttv_menus_remove(menu, view_constructor))  
    RemoveMenu(view_constructor);
}


/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void UpdateStatus(mainWindow *main_win, char *info)
{
}


/**
 * Function to get the current time interval of the current tab.
 * It will be called by a viewer's hook function to update the 
 * time interval of the viewer and also be called by the constructor
 * of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

void GetTimeInterval(mainWindow *main_win, TimeInterval *time_interval)
{
  time_interval->startTime = main_win->CurrentTab->startTime;
  time_interval->endTime = main_win->CurrentTab->endTime;
}


/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void SetTimeInterval(mainWindow *main_win, TimeInterval *time_interval)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  main_win->CurrentTab->startTime = time_interval->startTime;
  main_win->CurrentTab->endTime = time_interval->endTime;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/updatetimeinterval", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL)return;
  lttv_hooks_call(tmp, time_interval);
}


/**
 * Function to get the current time/event of the current tab.
 * It will be called by a viewer's hook function to update the 
 * current time/event of the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

void GetCurrentTime(mainWindow *main_win, LttTime *time)
{
  time = &main_win->CurrentTab->currentTime;
}


/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void SetCurrentTime(mainWindow *main_win, LttTime *time)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  main_win->CurrentTab->currentTime = *time;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);

  if(tmp == NULL)return;
  lttv_hooks_call(tmp, time);
}


/**
 * Function to get the traceset from the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its traceset.
 * @param main_win the main window the viewer belongs to.
 * @param traceset a pointer to a traceset.
 */
/*
void GetTraceset(mainWindow *main_win, Traceset *traceset)
{
}
*/

/**
 * Function to get the filter of the current tab.
 * It will be called by the constructor of the viewer and also be
 * called by a hook funtion of the viewer to update its filter.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */
/*
void GetFilter(mainWindow *main_win, Filter *filter)
{
}
*/

/**
 * Function to register a hook function for a viewer to set/update its
 * time interval.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateTimeInterval(LttvHook hook, gpointer hook_data,
			   mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/updatetimeinterval", LTTV_POINTER, &value));
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
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateTimeInterval(LttvHook hook, gpointer hook_data,
			     mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/updatetimeinterval", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateTraceset(LttvHook hook, gpointer hook_data,
		       mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
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
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateTraceset(LttvHook hook, gpointer hook_data,
			 mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
		       "hooks/updatetraceset", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateFilter(LttvHook hook, gpointer hook_data,
		     mainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
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
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateFilter(LttvHook hook, gpointer hook_data,
		       mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->Attributes,
		       "hooks/updatefilter", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateCurrentTime(LttvHook hook, gpointer hook_data, 
			  mainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
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
 * It will be called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void UnregUpdateCurrentTime(LttvHook hook, gpointer hook_data,
			    mainWindow * main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/updatecurrenttime", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */

void SetFocusedPane(mainWindow *main_win, gpointer paned)
{
  gtk_custom_set_focus((GtkWidget*)main_win->CurrentTab->custom,paned);  
}


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane.
 * It will be called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void RegUpdateDividor(LttvHook hook, gpointer hook_data, 
		      mainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
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

void UnregUpdateDividor(LttvHook hook, gpointer hook_data, 
			mainWindow *main_win)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_remove_data(tmp, hook, hook_data);
}


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void SetHPaneDividor(mainWindow *main_win, gint position)
{
  LttvAttributeValue value;
  LttvHooks * tmp;
  g_assert(lttv_iattribute_find_by_path(main_win->CurrentTab->Attributes,
		       "hooks/hpanedividor", LTTV_POINTER, &value));
  tmp = (LttvHooks*)*(value.v_pointer);
  if(tmp == NULL) return;
  lttv_hooks_call(tmp, &position);
}


/**
 * Function to process traceset. It will call lttv_process_trace, 
 * each view will call this api to get events.
 * @param main_win the main window the viewer belongs to.
 * @param start the start time of the first event to be processed.
 * @param end the end time of the last event to be processed.
 */

void processTraceset(mainWindow *main_win, LttTime start, LttTime end)
{
  lttv_process_trace(start, end, main_win->traceset, main_win->traceset_context);
}

/**
 * Function to add hooks into the context of a traceset,
 * before reading events from traceset, viewer will call this api to
 * register hooks
 * @param main_win the main window the viewer belongs to.
 * @param LttvHooks hooks to be registered.
 */

void contextAddHooks(mainWindow *main_win ,
		     LttvHooks *before_traceset, 
		     LttvHooks *after_traceset,
		     LttvHooks *check_trace, 
		     LttvHooks *before_trace, 
		     LttvHooks *after_trace, 
		     LttvHooks *check_tracefile,
		     LttvHooks *before_tracefile,
		     LttvHooks *after_tracefile,
		     LttvHooks *check_event, 
		     LttvHooks *before_event, 
		     LttvHooks *after_event)
{
  LttvTracesetContext * tsc = main_win->traceset_context;
  lttv_traceset_context_add_hooks(tsc,before_traceset,after_traceset,
				  check_trace,before_trace,after_trace,
				  check_tracefile,before_tracefile,after_tracefile,
				  check_event,before_event, after_event);
}


/**
 * Function to remove hooks from the context of a traceset,
 * before reading events from traceset, viewer will call this api to
 * unregister hooks
 * @param main_win the main window the viewer belongs to.
 * @param LttvHooks hooks to be registered.
 */

void contextRemoveHooks(mainWindow *main_win ,
			LttvHooks *before_traceset, 
			LttvHooks *after_traceset,
			LttvHooks *check_trace, 
			LttvHooks *before_trace, 
			LttvHooks *after_trace, 
			LttvHooks *check_tracefile,
			LttvHooks *before_tracefile,
			LttvHooks *after_tracefile,
			LttvHooks *check_event, 
			LttvHooks *before_event, 
			LttvHooks *after_event)
{
  LttvTracesetContext * tsc = main_win->traceset_context;
  lttv_traceset_context_remove_hooks(tsc,before_traceset,after_traceset,
				     check_trace,before_trace,after_trace,
				     check_tracefile,before_tracefile,after_tracefile,
				     check_event,before_event, after_event);
}
