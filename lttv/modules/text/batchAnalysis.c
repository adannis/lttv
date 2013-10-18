/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2003-2004 Michel Dagenais
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

/* This module inserts a hook in the program main loop. This hook processes 
   all the events in the main tracefile. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <unistd.h>
#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/traceset-process.h>
#include <lttv/state.h>
#ifdef BABEL_CLEANUP
#include <lttv/stats.h>
#include <lttv/filter.h>
#endif
#ifdef BABEL_CLEANUP_SYNC
#include <lttv/sync/sync_chain_lttv.h>
#endif
#include <babeltrace/context.h>

static LttvTraceset *traceset;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *before_tracefile,
  *after_tracefile,
  *event_hook,
  *main_hooks;

static char *trace_path;

static gboolean a_stats;
static gboolean a_live;
static int a_live_update_period;

#define DEFAULT_LIVE_UPDATE_PERIOD 1

void lttv_trace_option(void *hook_data)
{ 
  //LttTrace *trace;
  //trace_collection *trace;  
  //format *fmt = bt_lookup_format(g_quark_from_static_string("ctf"));
  /*
  if(a_live) {
    //trace = ltt_trace_open_live(a_trace);
  } else {
    bt_create_context();
    //trace = bt_add_trace(a_trace,"ctf");
  }
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  lttv_traceset_add(traceset, lttv_trace_new(trace));*/

  if(lttv_traceset_add_path(traceset, trace_path) < 0) {
    g_error("Cannot add trace %s", trace_path);
  }
}


static gboolean process_traceset(void *hook_data, void *call_data)
{
#ifdef BABEL_CLEANUP
  LttvAttributeValue value_expression, value_filter;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  LttvTracesetStats *tscs = NULL;

  LttvTracesetState *tss;

  LttvTracesetContext *tc;

  gboolean retval;
#endif
  LttTime start, end;


  g_info("BatchAnalysis begin process traceset");
#ifdef BABEL_CLEANUP
  if (a_stats) {
    tscs = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
    tss = &tscs->parent;
  } else {
    tss = g_object_new(LTTV_TRACESET_STATE_TYPE, NULL);
  }
  tc = &tss->parent;

  g_info("BatchAnalysis initialize context");

  lttv_context_init(tc, traceset);


  syncTraceset(tc);

  lttv_state_add_event_hooks(tss);
  if(a_stats) lttv_stats_add_event_hooks(tscs);


  retval= lttv_iattribute_find_by_path(attributes, "filter/expression",
    LTTV_POINTER, &value_expression);
  g_assert(retval);

  retval= lttv_iattribute_find_by_path(attributes, "filter/lttv_filter",
    LTTV_POINTER, &value_filter);
  g_assert(retval);

  /* Repeat the search for the first element, the second search might have
   * moved the first element (by creating the second element)
   */
  retval= lttv_iattribute_find_by_path(attributes, "filter/expression",
    LTTV_POINTER, &value_expression);
  g_assert(retval);

  *(value_filter.v_pointer) = lttv_filter_new();
  //g_debug("Filter string: %s",((GString*)*(value_expression.v_pointer))->str);

  lttv_filter_append_expression(*(value_filter.v_pointer),((GString*)*(value_expression.v_pointer))->str);
#endif  
  //lttv_traceset_context_add_hooks(tc,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);

  lttv_state_add_event_hooks(traceset);
  lttv_process_traceset_begin(traceset,
                              before_traceset,
                              before_trace,
                              event_hook);

  start.tv_sec = 0;
  start.tv_nsec = 0;
  end.tv_sec = G_MAXULONG;
  end.tv_nsec = G_MAXULONG;

  g_info("BatchAnalysis process traceset");
 
  lttv_process_traceset_seek_time(traceset, start);
  /* Read as long a we do not reach the end (0) */
  unsigned int count;
#ifdef BABEL_CLEANUP
  unsigned int updated_count;
#endif
  do {
	  count = lttv_process_traceset_middle(traceset,
							  end,
							  G_MAXULONG,
							  NULL);
	  
#ifdef BABEL_CLEANUP
	  updated_count = lttv_process_traceset_update(tc);
#endif
		
	  sleep(a_live_update_period);
  } while(count != 0
#ifdef BABEL_CLEANUP
      || updated_count > 0
#endif
      );


  //lttv_traceset_context_remove_hooks(tc,
  //before_traceset, after_traceset, NULL, before_trace, after_trace,
  //NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_process_traceset_end(traceset,
                            after_traceset,
                            after_trace,
                            event_hook);

  g_info("BatchAnalysis destroy context");
#ifdef BABEL_CLEANUP
  lttv_filter_destroy(*(value_filter.v_pointer));

  lttv_state_remove_event_hooks(tss);
  if(a_stats) lttv_stats_remove_event_hooks(tscs);

  lttv_context_fini(tc);
  if (a_stats)
    g_object_unref(tscs);
  else
    g_object_unref(tss);
#endif
  g_info("BatchAnalysis end process traceset");
  return FALSE;
}


static void init()
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());
  gboolean retval;

  g_info("Init batchAnalysis.c");

  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &trace_path, lttv_trace_option, NULL);

  a_stats = FALSE;
  lttv_option_add("stats", 's', 
      "write the traceset and trace statistics", 
      "", 
      LTTV_OPT_NONE, &a_stats, NULL, NULL);

  a_live = FALSE;
  lttv_option_add("live", 0,
      "define if the traceset is receiving live informations",
      "",
      LTTV_OPT_NONE, &a_live, NULL, NULL);
  
  a_live_update_period = DEFAULT_LIVE_UPDATE_PERIOD;
  lttv_option_add("live-period", 0,
		  "period to update a live trace",
		  "in seconds",
		  LTTV_OPT_INT,
		  &a_live_update_period,
		  NULL, NULL);


  traceset = lttv_traceset_new();

  before_traceset = lttv_hooks_new();
  after_traceset = lttv_hooks_new();
  before_trace = lttv_hooks_new();
  after_trace = lttv_hooks_new();
  before_tracefile = lttv_hooks_new();
  after_tracefile = lttv_hooks_new();
  //before_event = lttv_hooks_new();
  //after_event = lttv_hooks_new();
  event_hook = lttv_hooks_new();

  retval= lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = before_traceset;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = after_traceset;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = before_trace;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = after_trace;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/tracefile/before",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = before_tracefile;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/tracefile/after",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = after_tracefile;
  //g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
  //    LTTV_POINTER, &value));
  //*(value.v_pointer) = before_event;
  //g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
  //    LTTV_POINTER, &value));
  //*(value.v_pointer) = after_event;
  retval= lttv_iattribute_find_by_path(attributes, "hooks/event",
    LTTV_POINTER, &value);
  g_assert(retval);
  *(value.v_pointer) = event_hook;

  retval= lttv_iattribute_find_by_path(attributes, "hooks/main/before",
    LTTV_POINTER, &value);
  g_assert(retval);
  g_assert((main_hooks = *(value.v_pointer)) != NULL);
  lttv_hooks_add(main_hooks, process_traceset, NULL, LTTV_PRIO_DEFAULT);
}

static void destroy()
{
#ifdef BABEL_CLEANUP
  guint i, nb;

  LttvTrace *trace;
#endif

  g_info("Destroy batchAnalysis.c");

  lttv_option_remove("trace");
  lttv_option_remove("stats");
  lttv_option_remove("live");
  lttv_option_remove("live-period");

  lttv_hooks_destroy(before_traceset);
  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(before_trace);
  lttv_hooks_destroy(after_trace);
  lttv_hooks_destroy(before_tracefile);
  lttv_hooks_destroy(after_tracefile);
  //lttv_hooks_destroy(before_event);
  //lttv_hooks_destroy(after_event);
  lttv_hooks_destroy(event_hook);
  lttv_hooks_remove_data(main_hooks, process_traceset, NULL);

#ifdef BABEL_CLEANUP
  nb = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb ; i++) {
    trace = lttv_traceset_get(traceset, i);
    ltt_trace_close(lttv_trace(trace));
    lttv_trace_destroy(trace);
  }
#endif
  lttv_traceset_destroy(traceset); 
}

LTTV_MODULE("batchAnalysis", "Batch processing of a trace", \
    "Run through a trace calling all the registered hooks", \
    init, destroy, "state", "option")
//TODO ybrosseau 2012-05-15 reenable textFilter, stats, sync
