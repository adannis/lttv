/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Xiangxiu Yang
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

/*
This file is what every viewer plugin writer should refer to.

- streamline the rest.

A viewer plugin is, before anything, a plugin. It thus has an init and 
a destroy function called whenever it is loaded/initialized and 
unloaded/destroyed. A viewer depends on lttvwindow and thus uses its init and
destroy functions to register viewer related hooks defined in this file.

The lifetime of a viewer is as follows. The viewer constructor function is
called each time an instance view is created (one subwindow of this viewer
type is created by the user). Thereafter, the viewer gets hooks called for
different purposes by the window containing it. These hooks are detailed
below.

show: called initially once the trace, position and time window are known.
      Do the drawing or register hooks
process_trace for show: hooks called
show_end: remove the hooks

background_init: prepare for background computation (comes after show_end).
process_trace for background: done in small chunks in gtk_idle, hooks called.
background_end: remove the hooks and perhaps update the window.

update: called when the windows is exposed/resized, the time window is changed,
        the current time is changed or the traceset is changed (different
        traceset or traces added/removed. Redraw or register hooks.
process_trace for update: hooks called
update_end: remove the hooks.

There may be different versions of update functions for the different cases,
or the type of update may be described in an argument. The expose method
normally provides the exposed region. This should be used to register hooks
to process_traceset but also tell about the time interval for which it is
required. Then, a expose_end will be required to remove the hooks and finish
the display as needed.

In most cases, the enclosing window knows about updates such as a new trace
added to a traceset, time window updates caused by scrolling and even
expose events. There are a few cases, however, where updates are caused by
actions known by a view instance. For example, clicking in a view may update
the current time; all viewers within the same window must be told about the
new current time to change the currently highlighted time point. A viewer
reports such events by calling report_update on its lttvwindow. The lttvwindow
will thereafter call update for each of its contained viewers.


Things that can happen to a viewer:

update_time_window
update_current_time
update_traceset
update_filter
show_viewer
update_dividor
?? Reshape, damage as gtk methods ??

Things that a viewer can do:

update_status
lttvwindow_report_time_window
set_current_time
update_traceset -> not actually
update_filter -> not actually
show_viewer -> makes no sense.
set_focused_pane
set_hpane_dividor
*/




/*! \file viewer.h
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

#include <gtk/gtk.h>
#include <ltt/ltt.h>
#include <lttv/hook.h>
#include <lttvwindow/common.h>
#include <lttv/stats.h>
//FIXME (not ready yet) #include <lttv/filter.h>

/**
 * Function to register a view constructor so that main window can generate
 * a toolbar item for the viewer in order to generate a new instance easily. 
 * It will be called by init function of the module.
 * @param ButtonPixmap image shown on the toolbar item.
 * @param tooltip tooltip of the toolbar item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_toolbar(char ** pixmap, char *tooltip, lttvwindow_viewer_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by pixmap, tooltip and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the pixmap and tooltip are.
 */

void lttvwindow_unregister_toolbar(lttvwindow_viewer_constructor view_constructor);


/**
 * Function to register a view constructor so that main window can generate
 * a menu item for the viewer in order to generate a new instance easily.
 * It will be called by init function of the module.
 * @param menu_path path of the menu item.
 * @param menu_text text of the menu item.
 * @param view_constructor constructor of the viewer. 
 */

void lttvwindow_register_menu(char *menu_path, char *menu_text, lttvwindow_viewer_constructor view_constructor);


/**
 * Function to unregister the viewer's constructor, release the space 
 * occupied by menu_path, menu_text and constructor of the viewer.
 * It will be called when a module is unloaded.
 * @param view_constructor constructor of the viewer which is used as 
 * a reference to find out where the menu_path and menu_text are.
 */

void lttvwindow_unregister_menu(lttvwindow_viewer_constructor view_constructor);


/**
 * Function to register a hook function for a viewer to update its
 * time interval.
 * This register function is typically called by the constructor of the viewer.
 * @param hook hook function of the viewer. Takes a TimeInterval* as call_data.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

typedef struct _TimeWindowNotifyData {
  TimeWindow *new_time_window;
  TimeWindow *old_time_window;
} TimeWindowNotifyData;

void lttvwindow_register_time_window_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the time interval of the viewer.
 * This unregister function is typically called by the destructor of the viewer.
 * @param hook hook function of the viewer. Takes a TimeInterval as call_data.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_time_window_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to set/update its 
 * traceset.
 * This register function is typically called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_traceset_notify(MainWindow * main_win,
    LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the traceset of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_traceset_notify(MainWindow * main_win,
              LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to set/update its 
 * filter.
 * This register function is typically called by the constructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_filter_notify(MainWindow *main_win,
      LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the filter of the viewer.
 * This unregistration is called by the destructor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_filter_notify(LttvHook hook,  gpointer hook_data,
		       MainWindow * main_win);


/**
 * Function to register a hook function for a viewer to set/update its 
 * current time.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_current_time_notify(MainWindow *main_win,
            LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update the current time of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_current_time_notify(MainWindow * main_win,
            LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to show 
 * its content.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_show(MainWindow *main_win,
          LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * show the content of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_show(MainWindow * main_win,
              LttvHook hook, gpointer hook_data);


/**
 * Function to register a hook function for a viewer to set/update the 
 * dividor of the hpane. It provides a way to make the horizontal
 * dividors of all the viewers linked together.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_register_dividor(MainWindow *main_win,
                    LttvHook hook, gpointer hook_data);


/**
 * Function to unregister a viewer's hook function which is used to 
 * set/update hpane's dividor of the viewer.
 * @param hook hook function of the viewer.
 * @param hook_data hook data associated with the hook function.
 * @param main_win the main window the viewer belongs to.
 */

void lttvwindow_unregister_dividor(MainWindow *main_win,
                    LttvHook hook, gpointer hook_data);



/**
 * Update the status bar whenever something changed in the viewer.
 * @param main_win the main window the viewer belongs to.
 * @param info the message which will be shown in the status bar.
 */

void lttvwindow_report_status(MainWindow *main_win, char *info);


/**
 * Function to set the time interval of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the move_slider signal
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval is stored.
 */

void lttvwindow_report_time_window(MainWindow *main_win, TimeWindow *time_window);

/**
 * Function to set the current time/event of the current tab.
 * It will be called by a viewer's signal handle associated with 
 * the button-release-event signal
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time is stored.
 */

void lttvwindow_report_current_time(MainWindow *main_win, LttTime *time);


/**
 * Function to set the position of the hpane's dividor (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the motion_notify_event event/signal
 * @param main_win the main window the viewer belongs to.
 * @param position position of the hpane's dividor.
 */

void lttvwindow_report_dividor(MainWindow *main_win, gint position);

/**
 * Function to set the focused pane (viewer).
 * It will be called by a viewer's signal handle associated with 
 * the grab_focus signal
 * @param main_win the main window the viewer belongs to.
 * @param paned a pointer to a pane where the viewer is contained.
 */
//FIXME : can we do this through normal GTK signals ?
void lttvwindow_report_focus(MainWindow *main_win, gpointer paned);

/**
 * Function to get the life span of the traceset
 * @param main_win the main window the viewer belongs to.
 * @param start start time of the traceset.
 * @param end end time of the traceset.
 */

const TimeInterval *lttvwindow_get_time_span(MainWindow *main_win);

/**
 * Function to get the current time window of the current tab.
 * @param main_win the main window the viewer belongs to.
 * @param time_interval a pointer where time interval will be stored.
 */

const TimeWindow *lttvwindow_get_time_window(MainWindow *main_win);


/**
 * Function to get the current time/event of the current tab.
 * @param main_win the main window the viewer belongs to.
 * @param time a pointer where time will be stored.
 */

const LttTime *lttvwindow_get_current_time(MainWindow *main_win);

/**
 * Function to get the traceset from the current tab.
 * @param main_win the main window the viewer belongs to.
 * @param traceset a pointer to a traceset.
 */

const LttvTraceset *lttvwindow_get_traceset(MainWindow *main_win);


/**
 * Function to get the filter of the current tab.
 * @param main_win, the main window the viewer belongs to.
 * @param filter, a pointer to a filter.
 */

//FIXME
typedef void lttv_filter;
//FIXME
const lttv_filter *lttvwindow_get_filter(MainWindow *main_win);


/**
 * Function to get the stats of the traceset 
 * It must be non const so the viewer can modify it. //FIXME really ?
 * @param main_win the main window the viewer belongs to.
 */

LttvTracesetStats* lttvwindow_get_traceset_stats(MainWindow *main_win);

/**
 * Function to get the context of the traceset 
 * It must be non const so the viewer can add and remove hooks from it.
 * @param main_win the main window the viewer belongs to.
 */


LttvTracesetContext* lttvwindow_get_traceset_context(MainWindow *main_win);


