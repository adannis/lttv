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

#ifndef _MAIN_WINDOW_PRIVATE_
#define _MAIN_WINDOW_PRIVATE_

#include <gtk/gtk.h>

#include <lttv/attribute.h>
#include <lttv/traceset.h>

#include <lttv/hook.h>
#ifdef BABEL_CLEANUP
#include <lttv/tracecontext.h>
#include <lttv/stats.h>
#endif /* BABEL_CLEANUP */
#include <lttv/filter.h>
//#include <lttvwindow/gtkmultivpaned.h>
#include <lttvwindow/mainwindow.h>

#define SCROLL_STEP_PER_PAGE 10.0

struct _TracesetInfo {
  //FIXME? Traceset is the unique member of tracesetinfo
  LttvTraceset * traceset;
};

struct _MainWindow{
  GtkWidget*      mwindow;            /* Main Window */
  int             window_width;

  /* Status bar information */
  //  guint         MainSBarContextID;    /* Context ID of main status bar */
  //  guint         BegTimeSBarContextID; /* Context ID of BegTime status bar */
  //  guint         EndTimeSBarContextID; /* Context ID of EndTime status bar */
  
  /* Child windows */
  //openTracesetWindow*  OpenTracesetWindow;/* Window to get prof and proc file*/
  //viewTimeFrameWindow* ViewTimeFrameWindow;/*Window to select time frame */
  //gotoEventWindow*     GotoEventWindow; /*search for event description*/
  //openFilterWindow*    OpenFilterWindow; /* Open a filter selection window */
  GtkWidget*           help_contents;/* Window to display help contents */
 
  //  lttv_trace_filter * filter; /* trace filter associated with the window */

  /* Attributes for trace reading hooks local to the main window */
  LttvIAttribute * attributes;
  
  //Tab * tab;
  //Tab * current_tab;

};


struct _Tab{
  GtkWidget *label;
  GtkWidget *top_widget;
  GtkWidget *vbox; /* contains viewer_container and scrollbar */
  //GtkWidget *multivpaned;
  GtkWidget *viewer_container;
  GtkWidget *scrollbar;

  /* time bar */
  GtkWidget *MTimebar;

   
  // startTime is the left of the visible area. Corresponds to the scrollbar
  // value.
  // Time_Width is a zoom dependant value (corresponding to page size)
  TimeWindow time_window;
  gboolean time_manager_lock;
  
  // The current time is the time selected in the visible area by the user,
  // not the scrollbar value.
  LttTime current_time;
  gboolean current_time_manager_lock;

  LttvIAttribute * attributes;

  //struct _Tab * next;
  MainWindow  * mw;

  /* Traceset related information */
  TracesetInfo * traceset_info; 

  /* Filter to apply to the tab's traceset */
  LttvFilter *filter;

  /* A list of time requested for the next process trace */
  GSList *events_requests;
  gboolean events_request_pending;
  LttvAttribute *interrupted_state;
  gboolean stop_foreground;
};

#endif /* _MAIN_WINDOW_PRIVATE_ */


