/* This module inserts a hook in the program main loop. This hook processes 
   all the events in the main tracefile. */


#include <lttv/lttv.h>
#include <lttv/attribute.h>
#include <lttv/hook.h>
#include <lttv/option.h>
#include <lttv/module.h>
#include <lttv/processTrace.h>
#include <lttv/state.h>
#include <lttv/stats.h>

static LttvTraceset *traceset;

static LttvHooks
  *before_traceset,
  *after_traceset,
  *before_trace,
  *after_trace,
  *before_tracefile,
  *after_tracefile,
  *before_event,
  *after_event,
  *main_hooks;

static char *a_trace;

static gboolean a_stats;

void lttv_trace_option(void *hook_data)
{ 
  LttTrace *trace;

  trace = ltt_trace_open(a_trace);
  if(trace == NULL) g_critical("cannot open trace %s", a_trace);
  lttv_traceset_add(traceset, trace);
}


static gboolean process_traceset(void *hook_data, void *call_data)
{
  LttvTracesetStats *tscs;

  LttvTracesetContext *tc;

  LttTime start, end;

  g_info("BatchAnalysis begin process traceset");

  tscs = g_object_new(LTTV_TRACESET_STATS_TYPE, NULL);
  tc = &tscs->parent.parent;

  g_info("BatchAnalysis initialize context");

  lttv_context_init(tc, traceset);
  lttv_state_add_event_hooks(&tscs->parent);
  if(a_stats) lttv_stats_add_event_hooks(tscs);

  lttv_traceset_context_add_hooks(tc,
  before_traceset, after_traceset, NULL, before_trace, after_trace,
  NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);

  start.tv_sec = 0;
  start.tv_nsec = 0;
  end.tv_sec = G_MAXULONG;
  end.tv_nsec = G_MAXULONG;

  g_info("BatchAnalysis process traceset");

  lttv_process_trace(start, end, traceset, tc, G_MAXULONG);

  g_info("BatchAnalysis destroy context");

  lttv_traceset_context_remove_hooks(tc,
  before_traceset, after_traceset, NULL, before_trace, after_trace,
  NULL, before_tracefile, after_tracefile, NULL, before_event, after_event);
  lttv_state_remove_event_hooks(&tscs->parent);
  if(a_stats) lttv_stats_remove_event_hooks(tscs);
  lttv_context_fini(tc);
  g_object_unref(tscs);

  g_info("BatchAnalysis end process traceset");
}


G_MODULE_EXPORT void init(LttvModule *self, int argc, char **argv)
{
  LttvAttributeValue value;

  LttvIAttribute *attributes = LTTV_IATTRIBUTE(lttv_global_attributes());

  g_info("Init batchAnalysis.c");

  lttv_option_add("trace", 't', 
      "add a trace to the trace set to analyse", 
      "pathname of the directory containing the trace", 
      LTTV_OPT_STRING, &a_trace, lttv_trace_option, NULL);

  a_stats = FALSE;
  lttv_option_add("stats", 's', 
      "write the traceset and trace statistics", 
      "", 
      LTTV_OPT_NONE, &a_stats, NULL, NULL);


  traceset = lttv_traceset_new();

  before_traceset = lttv_hooks_new();
  after_traceset = lttv_hooks_new();
  before_trace = lttv_hooks_new();
  after_trace = lttv_hooks_new();
  before_tracefile = lttv_hooks_new();
  after_tracefile = lttv_hooks_new();
  before_event = lttv_hooks_new();
  after_event = lttv_hooks_new();

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/traceset/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_traceset;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/trace/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_trace;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/tracefile/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_tracefile;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/before",
      LTTV_POINTER, &value));
  *(value.v_pointer) = before_event;
  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/event/after",
      LTTV_POINTER, &value));
  *(value.v_pointer) = after_event;

  g_assert(lttv_iattribute_find_by_path(attributes, "hooks/main/before",
      LTTV_POINTER, &value));
  g_assert((main_hooks = *(value.v_pointer)) != NULL);
  lttv_hooks_add(main_hooks, process_traceset, NULL);
}


G_MODULE_EXPORT void destroy()
{
  guint i, nb;

  g_info("Destroy batchAnalysis.c");

  lttv_option_remove("trace");
  lttv_option_remove("stats");

  lttv_hooks_destroy(before_traceset);
  lttv_hooks_destroy(after_traceset);
  lttv_hooks_destroy(before_trace);
  lttv_hooks_destroy(after_trace);
  lttv_hooks_destroy(before_tracefile);
  lttv_hooks_destroy(after_tracefile);
  lttv_hooks_destroy(before_event);
  lttv_hooks_destroy(after_event);
  lttv_hooks_remove_data(main_hooks, process_traceset, NULL);

  nb = lttv_traceset_number(traceset);
  for(i = 0 ; i < nb ; i++) {
    ltt_trace_close(lttv_traceset_get(traceset, i));
  }

  lttv_traceset_destroy(traceset); 
}

