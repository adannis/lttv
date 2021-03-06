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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301, USA.
 */

#define _GNU_SOURCE
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <glib.h>
#include <lttv/lttv.h>
#include <lttv/module.h>
#include <lttv/state.h>
#include <lttv/compiler.h>
#include <lttv/traceset.h>
#include <lttv/traceset-process.h>
#include <lttv/trace.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <babeltrace/babeltrace.h>

#define PREALLOCATED_EXECUTION_STACK 10

/* Channel Quarks */

GQuark
	LTT_CHANNEL_FD_STATE,
	LTT_CHANNEL_GLOBAL_STATE,
	LTT_CHANNEL_IRQ_STATE,
	LTT_CHANNEL_MODULE_STATE,
	LTT_CHANNEL_NETIF_STATE,
	LTT_CHANNEL_SOFTIRQ_STATE,
	LTT_CHANNEL_SWAP_STATE,
	LTT_CHANNEL_SYSCALL_STATE,
	LTT_CHANNEL_TASK_STATE,
	LTT_CHANNEL_VM_STATE,
	LTT_CHANNEL_KPROBE_STATE,
	LTT_CHANNEL_FS,
	LTT_CHANNEL_KERNEL,
	LTT_CHANNEL_MM,
	LTT_CHANNEL_USERSPACE,
	LTT_CHANNEL_BLOCK;

/* Events Quarks */

GQuark 
	LTT_EVENT_SYSCALL_ENTRY,
	LTT_EVENT_SYSCALL_EXIT,
	LTT_EVENT_PAGE_FAULT_NOSEM_ENTRY,
	LTT_EVENT_PAGE_FAULT_NOSEM_EXIT,
	LTT_EVENT_PAGE_FAULT_ENTRY,
	LTT_EVENT_PAGE_FAULT_EXIT,
	//LTT_EVENT_TRAP_ENTRY,
	//LTT_EVENT_TRAP_EXIT,
	LTT_EVENT_IRQ_ENTRY,
	LTT_EVENT_IRQ_EXIT,
	LTT_EVENT_SOFT_IRQ_RAISE,
	LTT_EVENT_SOFT_IRQ_ENTRY,
	LTT_EVENT_SOFT_IRQ_EXIT,
	LTT_EVENT_SCHED_SCHEDULE,
	LTT_EVENT_SCHED_TRY_WAKEUP,
	LTT_EVENT_PROCESS_FORK,
	LTT_EVENT_KTHREAD_CREATE,
	LTT_EVENT_PROCESS_EXIT,
	LTT_EVENT_PROCESS_FREE,
	LTT_EVENT_EXEC,
	LTT_EVENT_PROCESS_STATE,
	LTT_EVENT_STATEDUMP_END,
	LTT_EVENT_FUNCTION_ENTRY,
	LTT_EVENT_FUNCTION_EXIT,
	LTT_EVENT_REQUEST_ISSUE,
	LTT_EVENT_REQUEST_COMPLETE,
	LTT_EVENT_LIST_INTERRUPT,
	LTT_EVENT_SYS_CALL_TABLE,
	LTT_EVENT_SOFTIRQ_VEC,
	LTT_EVENT_KPROBE_TABLE,
	LTT_EVENT_KPROBE,
	LTT_EVENT_OPEN,
	LTT_EVENT_READ,
	LTT_EVENT_POLL_EVENT;

/* Fields Quarks */

GQuark 
	LTT_FIELD_SYSCALL_ID,
	LTT_FIELD_TRAP_ID,
	LTT_FIELD_IRQ_ID,
	LTT_FIELD_SOFT_IRQ_ID,
	LTT_FIELD_PREV_PID,
	LTT_FIELD_NEXT_PID,
	LTT_FIELD_PREV_STATE,
	LTT_FIELD_PARENT_PID,
	LTT_FIELD_CHILD_PID,
	LTT_FIELD_PID,
	LTT_FIELD_TGID,
	LTT_FIELD_CHILD_TGID,
	LTT_FIELD_FILENAME,
	LTT_FIELD_NAME,
	LTT_FIELD_TYPE,
	LTT_FIELD_MODE,
	LTT_FIELD_SUBMODE,
	LTT_FIELD_STATUS,
	LTT_FIELD_THIS_FN,
	LTT_FIELD_CALL_SITE,
	LTT_FIELD_MINOR,
	LTT_FIELD_MAJOR,
	LTT_FIELD_OPERATION,
	LTT_FIELD_ACTION,
	LTT_FIELD_ID,
	LTT_FIELD_ADDRESS,
	LTT_FIELD_SYMBOL,
	LTT_FIELD_IP,
	LTT_FIELD_FD,
	LTT_FIELD_STATE,
	LTT_FIELD_CPU_ID;

LttvExecutionMode
	LTTV_STATE_MODE_UNKNOWN,
	LTTV_STATE_USER_MODE,
	LTTV_STATE_MAYBE_USER_MODE,
	LTTV_STATE_SYSCALL,
	LTTV_STATE_MAYBE_SYSCALL,
	LTTV_STATE_TRAP,
	LTTV_STATE_MAYBE_TRAP,
	LTTV_STATE_IRQ,
	LTTV_STATE_SOFT_IRQ;

LttvExecutionSubmode
	LTTV_STATE_SUBMODE_UNKNOWN,
	LTTV_STATE_SUBMODE_NONE;

LttvProcessStatus
	LTTV_STATE_UNNAMED,
	LTTV_STATE_WAIT_FORK,
	LTTV_STATE_WAIT_CPU,
	LTTV_STATE_EXIT,
	LTTV_STATE_ZOMBIE,
	LTTV_STATE_WAIT,
	LTTV_STATE_RUN,
	LTTV_STATE_DEAD;

LttvProcessType
	LTTV_STATE_USER_THREAD,
	LTTV_STATE_KERNEL_THREAD;

LttvCPUMode
	LTTV_CPU_UNKNOWN,
	LTTV_CPU_IDLE,
	LTTV_CPU_BUSY,
	LTTV_CPU_IRQ,
	LTTV_CPU_SOFT_IRQ,
	LTTV_CPU_TRAP;

LttvIRQMode
	LTTV_IRQ_UNKNOWN,
	LTTV_IRQ_IDLE,
	LTTV_IRQ_BUSY;

LttvBdevMode
	LTTV_BDEV_UNKNOWN,
	LTTV_BDEV_IDLE,
	LTTV_BDEV_BUSY_READING,
	LTTV_BDEV_BUSY_WRITING;

static GQuark
	LTTV_STATE_TRACEFILES,
	LTTV_STATE_PROCESSES,
	LTTV_STATE_PROCESS,
	LTTV_STATE_RUNNING_PROCESS,
	LTTV_STATE_POSITION,
	LTTV_STATE_SAVED_STATES,
	LTTV_STATE_SAVED_STATES_TIME,
	LTTV_STATE_TIME,
	LTTV_STATE_HOOKS,
	LTTV_STATE_NAME_TABLES,
	LTTV_STATE_TRACE_STATE_USE_COUNT,
	LTTV_STATE_RESOURCE_CPUS,
	LTTV_STATE_RESOURCE_CPUS_COUNT,
	LTTV_STATE_RESOURCE_IRQS,
	LTTV_STATE_RESOURCE_SOFT_IRQS,
	LTTV_STATE_RESOURCE_TRAPS,
	LTTV_STATE_RESOURCE_BLKDEVS;

static void create_max_time(LttvTraceState *tcs);

static void get_max_time(LttvTraceState *tcs);

static void free_max_time(LttvTraceState *tcs);

static void create_name_tables(LttvTraceState *tcs);

static void get_name_tables(LttvTraceState *tcs);

static void free_name_tables(LttvTraceState *tcs);

static void free_saved_state(LttvTraceState *tcs);

static void lttv_state_free_process_table(GHashTable *processes);
#ifdef BABEL_CLEANUP
static void lttv_trace_states_read_raw(LttvTraceState *tcs, FILE *fp,
		GPtrArray *quarktable);
#endif
/* Resource function prototypes */
#ifdef BABEL_CLEANUP
static LttvBdevState *get_hashed_bdevstate(LttvTraceState *ts, guint32 devcode);
static LttvBdevState *bdevstate_new(void);
static void bdevstate_free(LttvBdevState *);
static void bdevstate_free_cb(gpointer key, gpointer value, gpointer user_data);
static LttvBdevState *bdevstate_copy(LttvBdevState *bds);
#endif
void lttv_state_add_event_hooks(LttvTraceset *traceset);

#if (__WORDSIZE == 32)
guint guint64_hash(gconstpointer key)
{
	guint64 ukey = *(const guint64 *)key;

	return (guint)ukey ^ (guint)(ukey >> 32);
}

gboolean guint64_equal(gconstpointer a, gconstpointer b)
{
	guint64 ua = *(const guint64 *)a;
	guint64 ub = *(const guint64 *)b;

	return ua == ub;
}
#endif

guint process_hash(gconstpointer key) 
{
	guint pid = ((const LttvProcessState *)key)->pid;
	return (pid>>8 ^ pid>>4 ^ pid>>2 ^ pid) ;
}


/* If the hash table hash function is well distributed,
 * the process_equal should compare different pid */
gboolean process_equal(gconstpointer a, gconstpointer b)
{
	const LttvProcessState *process_a, *process_b;
	gboolean ret = TRUE;

	process_a = (const LttvProcessState *)a;
	process_b = (const LttvProcessState *)b;

	if(likely(process_a->pid != process_b->pid)) ret = FALSE;
	else if(likely(process_a->pid == 0 &&
			process_a->cpu != process_b->cpu)) ret = FALSE;

	return ret;
}

gboolean rettrue(gpointer key, gpointer value, gpointer user_data)
{
	return TRUE;
}

static guint check_expand(nb, id)
{
	if(likely(nb > id))
		return nb;
	else
		return max(id + 1, nb * 2);
}

static void expand_name_table(LttvTraceState *ts, GQuark **table,
	guint nb, guint new_nb)
{
	/* Expand an incomplete table */
	GQuark *old_table = *table;
	*table = g_new(GQuark, new_nb);
	memcpy(*table, old_table, nb * sizeof(GQuark));
	g_free(old_table);
}

static void fill_name_table(LttvTraceState *ts, GQuark *table, guint nb,
		guint new_nb, const char *def_string)
{
	guint i;
	GString *fe_name = g_string_new("");
	for(i = nb; i < new_nb; i++) {
		g_string_printf(fe_name, "%s %d", def_string, i);
		table[i] = g_quark_from_string(fe_name->str);
	}
	g_string_free(fe_name, TRUE);
}
#ifdef BABEL_CLEANUP
static void expand_kprobe_table(LttvTraceState *ts, guint64 ip, char *symbol)
{
	LttvNameTables *nt = ts->name_tables;
#if (__WORDSIZE == 32)
	guint64 *ip_ptr = g_new(guint64, 1);
	g_hash_table_insert(nt->kprobe_hash, ip_ptr,
			(gpointer)(glong)g_quark_from_string(symbol));
#else
	g_hash_table_insert(nt->kprobe_hash, (gpointer)ip,
			(gpointer)(glong)g_quark_from_string(symbol));
#endif
}

static void expand_trap_table(LttvTraceState *ts, int id)
{
	LttvNameTables *nt = ts->name_tables;
	LttvTrapState *old_table;
	guint new_nb, i;

	new_nb = check_expand(nt->nb_traps, id);
	if(likely(new_nb == nt->nb_traps))
		return;

	expand_name_table(ts, &nt->trap_names, nt->nb_traps, new_nb);
	fill_name_table(ts, nt->trap_names, nt->nb_traps, new_nb, "trap");

	old_table = ts->trap_states;
	ts->trap_states = g_new(LttvTrapState, new_nb);
	memcpy(ts->trap_states, old_table, nt->nb_traps * sizeof(LttvTrapState));
	g_free(old_table);
	for(i = nt->nb_traps; i < new_nb; i++)
		ts->trap_states[i].running = 0;

	/* Update the table size */
	nt->nb_traps = new_nb;
}
#endif
static void expand_irq_table(LttvTraceState *ts, int id)
{
	LttvNameTables *nt = ts->name_tables;
	LttvIRQState *old_table;
	guint new_nb, i;

	new_nb = check_expand(nt->nb_irqs, id);
	if(likely(new_nb == nt->nb_irqs))
		return;

	expand_name_table(ts, &nt->irq_names, nt->nb_irqs, new_nb);
	fill_name_table(ts, nt->irq_names, nt->nb_irqs, new_nb, "irq");

	old_table = ts->irq_states;
	ts->irq_states = g_new(LttvIRQState, new_nb);
	memcpy(ts->irq_states, old_table, nt->nb_irqs * sizeof(LttvIRQState));
	g_free(old_table);
	for(i = nt->nb_irqs; i < new_nb; i++)
		ts->irq_states[i].mode_stack =
				g_array_new(FALSE, FALSE, sizeof(LttvIRQMode));

	/* Update the table size */
	nt->nb_irqs = new_nb;
}

static void expand_soft_irq_table(LttvTraceState *ts, int id)
{
	LttvNameTables *nt = ts->name_tables;
	LttvSoftIRQState *old_table;
	guint new_nb, i;

	new_nb = check_expand(nt->nb_soft_irqs, id);
	if(likely(new_nb == nt->nb_soft_irqs))
		return;

	expand_name_table(ts, &nt->soft_irq_names, nt->nb_soft_irqs, new_nb);
	fill_name_table(ts, nt->soft_irq_names, nt->nb_soft_irqs, new_nb, "softirq");

	old_table = ts->soft_irq_states;
	ts->soft_irq_states = g_new(LttvSoftIRQState, new_nb);
	memcpy(ts->soft_irq_states, old_table,
		nt->nb_soft_irqs * sizeof(LttvSoftIRQState));
	g_free(old_table);
	for(i = nt->nb_soft_irqs; i < new_nb; i++)
		ts->soft_irq_states[i].running = 0;

	/* Update the table size */
	nt->nb_soft_irqs = new_nb;
}

static void restore_init_state(LttvTraceState *self)
{
	guint i, nb_cpus, nb_irqs, nb_soft_irqs, nb_traps;

	//LttvTracefileState *tfcs;

	LttTime start_time;

	/* Free the process tables */
	if(self->processes != NULL) lttv_state_free_process_table(self->processes);
	self->processes = g_hash_table_new(process_hash, process_equal);
	self->nb_event = 0;

	/* Seek time to beginning */
	// Mathieu : fix : don't seek traceset here : causes inconsistency in seek
	// closest. It's the tracecontext job to seek the trace to the beginning
	// anyway : the init state might be used at the middle of the trace as well...
	//g_tree_destroy(self->parent.ts_context->pqueue);
	//self->parent.ts_context->pqueue = g_tree_new(compare_tracefile);

	start_time = ltt_time_from_uint64(
			    bt_trace_handle_get_timestamp_begin(self->trace->traceset->context, 
								self->trace->id,BT_CLOCK_REAL));


	//lttv_process_trace_seek_time(&self->parent, ltt_time_zero);

	nb_cpus = lttv_trace_get_num_cpu(self->trace);
	nb_irqs = self->name_tables->nb_irqs;
	nb_soft_irqs = self->name_tables->nb_soft_irqs;
	nb_traps = self->name_tables->nb_traps;

	/* Put the per cpu running_process to beginning state : process 0. */
	for(i=0; i< nb_cpus; i++) {
		LttvExecutionState *es;
		self->running_process[i] = lttv_state_create_process(self, NULL, i, 0, 0,
				LTTV_STATE_UNNAMED, &start_time);
		/* We are not sure is it's a kernel thread or normal thread, put the
		 * bottom stack state to unknown */
		self->running_process[i]->execution_stack =
				g_array_set_size(self->running_process[i]->execution_stack, 1);
		es = self->running_process[i]->state =
				&g_array_index(self->running_process[i]->execution_stack,
				LttvExecutionState, 0);
		es->t = LTTV_STATE_MODE_UNKNOWN;
		es->s = LTTV_STATE_UNNAMED;

		//self->running_process[i]->state->s = LTTV_STATE_RUN;
		self->running_process[i]->cpu = i;

		/* reset cpu states */
		if(self->cpu_states[i].mode_stack->len > 0) {
			g_array_remove_range(self->cpu_states[i].mode_stack, 0,
					self->cpu_states[i].mode_stack->len);
			if(self->cpu_states[i].irq_stack->len)
				g_array_remove_range(self->cpu_states[i].irq_stack, 0,
						self->cpu_states[i].irq_stack->len);
			if(self->cpu_states[i].softirq_stack->len)
				g_array_remove_range(self->cpu_states[i].softirq_stack, 0,
						self->cpu_states[i].softirq_stack->len);
			if(self->cpu_states[i].trap_stack->len)
				g_array_remove_range(self->cpu_states[i].trap_stack, 0,
						self->cpu_states[i].trap_stack->len);
		}
	}

	/* reset irq states */
	for(i=0; i<nb_irqs; i++) {
		if(self->irq_states[i].mode_stack->len > 0)
			g_array_remove_range(self->irq_states[i].mode_stack, 0,
					self->irq_states[i].mode_stack->len);
	}

	/* reset softirq states */
	for(i=0; i<nb_soft_irqs; i++) {
		self->soft_irq_states[i].pending = 0;
		self->soft_irq_states[i].running = 0;
	}

	/* reset trap states */
	for(i=0; i<nb_traps; i++) {
		self->trap_states[i].running = 0;
	}

#ifdef BABEL_CLEANUP
	/* reset bdev states */
	g_hash_table_foreach(self->bdev_states, bdevstate_free_cb, NULL);
	//g_hash_table_steal_all(self->bdev_states);
	g_hash_table_foreach_steal(self->bdev_states, rettrue, NULL);
#endif 

#if 0
	nb_tracefile = self->parent.tracefiles->len;

	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
			LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
					LttvTracefileContext*, i));
		ltt_trace_time_span_get(self->parent.t, &tfcs->parent.timestamp, NULL);
//		tfcs->saved_position = 0;
		tfcs->process = lttv_state_create_process(tfcs, NULL,0);
		tfcs->process->state->s = LTTV_STATE_RUN;
		tfcs->process->last_cpu = tfcs->cpu_name;
		tfcs->process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfcs)->tf);
	}
#endif //0
}

//static LttTime time_zero = {0,0};

#ifdef BABEL_CLEANUP

#define MAX_STRING_LEN 4096

static void state_load_saved_states(LttvTraceState *tcs)
{
	FILE *fp;
	GPtrArray *quarktable;
	const char *trace_path;
	char path[PATH_MAX];
	guint count;
	guint i;
	tcs->has_precomputed_states = FALSE;
	GQuark q;
	gchar *string;
	gint hdr;
	gchar buf[MAX_STRING_LEN];
	guint len;
	size_t res;

	trace_path = g_quark_to_string(ltt_trace_name(tcs->parent.t));
	strncpy(path, trace_path, PATH_MAX-1);
	count = strnlen(trace_path, PATH_MAX-1);
	// quarktable : open, test
	strncat(path, "/precomputed/quarktable", PATH_MAX-count-1);
	fp = fopen(path, "r");
	if(!fp) return;
	quarktable = g_ptr_array_sized_new(4096);

	/* Index 0 is null */
	hdr = fgetc(fp);
	if(hdr == EOF) return;
	g_assert(hdr == HDR_QUARKS);
	q = 1;
	do {
		hdr = fgetc(fp);
		if(hdr == EOF) break;
		g_assert(hdr == HDR_QUARK);
		g_ptr_array_set_size(quarktable, q+1);
		i=0;
		while(1) {
			res = fread(&buf[i], sizeof(gchar), 1, fp);
			g_assert(res == 1);
			if(buf[i] == '\0' || feof(fp)) break;
			i++;
		}
		len = strnlen(buf, MAX_STRING_LEN-1);
		g_ptr_array_index (quarktable, q) = g_new(gchar, len+1);
		strncpy(g_ptr_array_index (quarktable, q), buf, len+1);
		q++;
	} while(1);

	fclose(fp);

	// saved_states : open, test
	strncpy(path, trace_path, PATH_MAX-1);
	count = strnlen(trace_path, PATH_MAX-1);
	strncat(path, "/precomputed/states", PATH_MAX-count-1);
	fp = fopen(path, "r");
	if(!fp) return;

	hdr = fgetc(fp);
	if(hdr != HDR_TRACE) goto end;

	lttv_trace_states_read_raw(tcs, fp, quarktable);

	tcs->has_precomputed_states = TRUE;

end:
	fclose(fp);

	/* Free the quarktable */
	for(i=0; i<quarktable->len; i++) {
		string = g_ptr_array_index (quarktable, i);
		g_free(string);
	}
	g_ptr_array_free(quarktable, TRUE);
	return;
}
#endif /* BABEL_CLEANUP */

void lttv_trace_state_init(LttvTraceState *trace_state, LttvTrace *trace)
{
	guint j, nb_cpu;
	guint64 nb_irq;
	LttvAttributeValue v;

	trace_state->trace = trace;

	trace_state->save_interval = LTTV_STATE_SAVE_INTERVAL;
	lttv_attribute_find(lttv_trace_attribute(trace), LTTV_STATE_TRACE_STATE_USE_COUNT,
			LTTV_UINT, &v);
	(*v.v_uint)++;

	if (*(v.v_uint) == 1) {
		create_name_tables(trace_state);
		create_max_time(trace_state);
	}
	get_name_tables(trace_state);
	get_max_time(trace_state);

	nb_cpu = lttv_trace_get_num_cpu(trace);
	nb_irq = trace_state->name_tables->nb_irqs;
	trace_state->processes = NULL;
	trace_state->running_process = g_new(LttvProcessState*, nb_cpu);

	/* init cpu resource stuff */
	trace_state->cpu_states = g_new(LttvCPUState, nb_cpu);
	for (j = 0; j < nb_cpu; j++) {
		trace_state->cpu_states[j].mode_stack = g_array_new(FALSE, FALSE, sizeof(LttvCPUMode));
		trace_state->cpu_states[j].irq_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		trace_state->cpu_states[j].softirq_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		trace_state->cpu_states[j].trap_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		g_assert(trace_state->cpu_states[j].mode_stack != NULL);
	}

	/* init irq resource stuff */
	trace_state->irq_states = g_new(LttvIRQState, nb_irq);
	for (j = 0; j < nb_irq; j++) {
		trace_state->irq_states[j].mode_stack = g_array_new(FALSE, FALSE, sizeof(LttvIRQMode));
		g_assert(trace_state->irq_states[j].mode_stack != NULL);
	}

	/* init soft irq stuff */
	/* the kernel has a statically fixed max of 32 softirqs */
	trace_state->soft_irq_states = g_new(LttvSoftIRQState, trace_state->name_tables->nb_soft_irqs);

	/* init trap stuff */
	trace_state->trap_states = g_new(LttvTrapState, trace_state->name_tables->nb_traps);

	/* init bdev resource stuff */
	trace_state->bdev_states = g_hash_table_new(g_int_hash, g_int_equal);

	restore_init_state(trace_state);

	/* See if the trace has saved states */
	//state_load_saved_states(trace_state);
}

void lttv_trace_state_fini(LttvTraceState *trace_state)
{
	LttvTrace *trace = trace_state->trace;
	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(trace), LTTV_STATE_TRACE_STATE_USE_COUNT,
			LTTV_UINT, &v);

	g_assert(*(v.v_uint) != 0);
	(*v.v_uint)--;

	if (*(v.v_uint) == 0) {
		free_name_tables(trace_state);
		free_max_time(trace_state);
		free_saved_state(trace_state);
	}
	g_free(trace_state->running_process);
	trace_state->running_process = NULL;
	lttv_state_free_process_table(trace_state->processes);
	trace_state->processes = NULL;
}

#ifdef BABEL_CLEANUP

/* Write the process state of the trace */

static void write_process_state(gpointer key, gpointer value,
		gpointer user_data)
{
	LttvProcessState *process;

	LttvExecutionState *es;

	FILE *fp = (FILE *)user_data;

	guint i;
	guint64 address;

	process = (LttvProcessState *)value;
	fprintf(fp,"  <PROCESS CORE=%p PID=%u TGID=%u PPID=%u TYPE=\"%s\" CTIME_S=%lu CTIME_NS=%lu ITIME_S=%lu ITIME_NS=%lu NAME=\"%s\" CPU=\"%u\" FREE_EVENTS=\"%u\">\n",
			process, process->pid, process->tgid, process->ppid,
			g_quark_to_string(process->type),
			process->creation_time.tv_sec,
			process->creation_time.tv_nsec,
			process->insertion_time.tv_sec,
			process->insertion_time.tv_nsec,
			g_quark_to_string(process->name),
			process->cpu, process->free_events);

	for(i = 0 ; i < process->execution_stack->len; i++) {
		es = &g_array_index(process->execution_stack, LttvExecutionState, i);
		fprintf(fp, "    <ES MODE=\"%s\" SUBMODE=\"%s\" ENTRY_S=%lu ENTRY_NS=%lu",
				g_quark_to_string(es->t), g_quark_to_string(es->n),
				es->entry.tv_sec, es->entry.tv_nsec);
		fprintf(fp, " CHANGE_S=%lu CHANGE_NS=%lu STATUS=\"%s\"/>\n",
				es->change.tv_sec, es->change.tv_nsec, g_quark_to_string(es->s));
	}

	for(i = 0 ; i < process->user_stack->len; i++) {
		address = g_array_index(process->user_stack, guint64, i);
		fprintf(fp, "    <USER_STACK ADDRESS=\"%" PRIu64 "\"/>\n", address);
	}

	fprintf(fp, "  </PROCESS>\n");
}


void lttv_state_write(LttvTraceState *self, LttTime t, FILE *fp)
{
	guint i, nb_tracefile, nb_block, offset;
	guint64 tsc;

	LttvTracefileState *tfcs;

	LttTracefile *tf;

	LttEventPosition *ep;

	guint nb_cpus;

	ep = ltt_event_position_new();

	fprintf(fp,"<PROCESS_STATE TIME_S=%lu TIME_NS=%lu>\n", t.tv_sec, t.tv_nsec);

	g_hash_table_foreach(self->processes, write_process_state, fp);

	nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
	for(i=0;i<nb_cpus;i++) {
		fprintf(fp,"  <CPU NUM=%u RUNNING_PROCESS=%u>\n",
				i, self->running_process[i]->pid);
	}

	nb_tracefile = self->parent.tracefiles->len;

	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
				LttvTracefileContext*, i));
		fprintf(fp, "  <TRACEFILE TIMESTAMP_S=%lu TIMESTAMP_NS=%lu",
				tfcs->parent.timestamp.tv_sec,
				tfcs->parent.timestamp.tv_nsec);
		LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
		if(e == NULL) fprintf(fp,"/>\n");
		else {
			ltt_event_position(e, ep);
			ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
			fprintf(fp, " BLOCK=%u OFFSET=%u TSC=%" PRIu64 "/>\n", nb_block, offset,
					tsc);
		}
	}
	g_free(ep);
	fprintf(fp,"</PROCESS_STATE>\n");
}


static void write_process_state_raw(gpointer key, gpointer value,
		gpointer user_data)
{
	LttvProcessState *process;

	LttvExecutionState *es;

	FILE *fp = (FILE *)user_data;

	guint i;
	guint64 address;

	process = (LttvProcessState *)value;
	fputc(HDR_PROCESS, fp);
	//fwrite(&header, sizeof(header), 1, fp);
	//fprintf(fp, "%s", g_quark_to_string(process->type));
	//fputc('\0', fp);
	fwrite(&process->type, sizeof(process->type), 1, fp);
	//fprintf(fp, "%s", g_quark_to_string(process->name));
	//fputc('\0', fp);
	fwrite(&process->name, sizeof(process->name), 1, fp);
	//fputc('\0', fp);
	fwrite(&process->pid, sizeof(process->pid), 1, fp);
	fwrite(&process->free_events, sizeof(process->free_events), 1, fp);
	fwrite(&process->tgid, sizeof(process->tgid), 1, fp);
	fwrite(&process->ppid, sizeof(process->ppid), 1, fp);
	fwrite(&process->cpu, sizeof(process->cpu), 1, fp);
	fwrite(&process->creation_time, sizeof(process->creation_time), 1, fp);
	fwrite(&process->insertion_time, sizeof(process->insertion_time), 1, fp);

#if 0
	fprintf(fp,"  <PROCESS CORE=%p PID=%u TGID=%u PPID=%u TYPE=\"%s\" CTIME_S=%lu CTIME_NS=%lu ITIME_S=%lu ITIME_NS=%lu NAME=\"%s\" CPU=\"%u\" PROCESS_TYPE=%u>\n",
			process, process->pid, process->tgid, process->ppid,
			g_quark_to_string(process->type),
			process->creation_time.tv_sec,
			process->creation_time.tv_nsec,
			process->insertion_time.tv_sec,
			process->insertion_time.tv_nsec,
			g_quark_to_string(process->name),
			process->cpu);
#endif //0

	for(i = 0 ; i < process->execution_stack->len; i++) {
		es = &g_array_index(process->execution_stack, LttvExecutionState, i);

		fputc(HDR_ES, fp);
		//fprintf(fp, "%s", g_quark_to_string(es->t));
		//fputc('\0', fp);
		fwrite(&es->t, sizeof(es->t), 1, fp);
		//fprintf(fp, "%s", g_quark_to_string(es->n));
		//fputc('\0', fp);
		fwrite(&es->n, sizeof(es->n), 1, fp);
		//fprintf(fp, "%s", g_quark_to_string(es->s));
		//fputc('\0', fp);
		fwrite(&es->s, sizeof(es->s), 1, fp);
		fwrite(&es->entry, sizeof(es->entry), 1, fp);
		fwrite(&es->change, sizeof(es->change), 1, fp);
		fwrite(&es->cum_cpu_time, sizeof(es->cum_cpu_time), 1, fp);
#if 0
		fprintf(fp, "    <ES MODE=\"%s\" SUBMODE=\"%s\" ENTRY_S=%lu ENTRY_NS=%lu",
				g_quark_to_string(es->t), g_quark_to_string(es->n),
				es->entry.tv_sec, es->entry.tv_nsec);
		fprintf(fp, " CHANGE_S=%lu CHANGE_NS=%lu STATUS=\"%s\"/>\n",
				es->change.tv_sec, es->change.tv_nsec, g_quark_to_string(es->s));
#endif //0
	}

	for(i = 0 ; i < process->user_stack->len; i++) {
		address = g_array_index(process->user_stack, guint64, i);
		fputc(HDR_USER_STACK, fp);
		fwrite(&address, sizeof(address), 1, fp);
#if 0
		fprintf(fp, "    <USER_STACK ADDRESS=\"%llu\"/>\n", address);
#endif //0
	}
}


void lttv_state_write_raw(LttvTraceState *self, LttTime t, FILE *fp)
{
	guint i, nb_tracefile, nb_block, offset;
	guint64 tsc;

	LttvTracefileState *tfcs;

	LttTracefile *tf;

	LttEventPosition *ep;

	guint nb_cpus;

	ep = ltt_event_position_new();

	//fprintf(fp,"<PROCESS_STATE TIME_S=%lu TIME_NS=%lu>\n", t.tv_sec, t.tv_nsec);
	fputc(HDR_PROCESS_STATE, fp);
	fwrite(&t, sizeof(t), 1, fp);

	g_hash_table_foreach(self->processes, write_process_state_raw, fp);

	nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
	for(i=0;i<nb_cpus;i++) {
		fputc(HDR_CPU, fp);
		fwrite(&i, sizeof(i), 1, fp); /* cpu number */
		fwrite(&self->running_process[i]->pid,
				sizeof(self->running_process[i]->pid), 1, fp);
		//fprintf(fp,"  <CPU NUM=%u RUNNING_PROCESS=%u>\n",
		//		i, self->running_process[i]->pid);
	}

	nb_tracefile = self->parent.tracefiles->len;

	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
						LttvTracefileContext*, i));
	//	fprintf(fp, "  <TRACEFILE TIMESTAMP_S=%lu TIMESTAMP_NS=%lu",
	//			tfcs->parent.timestamp.tv_sec,
	//			tfcs->parent.timestamp.tv_nsec);
		fputc(HDR_TRACEFILE, fp);
		fwrite(&tfcs->parent.timestamp, sizeof(tfcs->parent.timestamp), 1, fp);
		/* Note : if timestamp if LTT_TIME_INFINITE, there will be no
		 * position following : end of trace */
		LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
		if(e != NULL) {
			ltt_event_position(e, ep);
			ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
			//fprintf(fp, " BLOCK=%u OFFSET=%u TSC=%llu/>\n", nb_block, offset,
			//		tsc);
			fwrite(&nb_block, sizeof(nb_block), 1, fp);
			fwrite(&offset, sizeof(offset), 1, fp);
			fwrite(&tsc, sizeof(tsc), 1, fp);
		}
	}
	g_free(ep);
}


/* Read process state from a file */

/* Called because a HDR_PROCESS was found */
static void read_process_state_raw(LttvTraceState *self, FILE *fp,
		GPtrArray *quarktable)
{
	LttvExecutionState *es;
	LttvProcessState *process, *parent_process;
	LttvProcessState tmp;
	GQuark tmpq;
	size_t res;

	guint64 *address;

	res = fread(&tmp.type, sizeof(tmp.type), 1, fp);
	res += fread(&tmp.name, sizeof(tmp.name), 1, fp);
	res += fread(&tmp.pid, sizeof(tmp.pid), 1, fp);
	res += fread(&tmp.free_events, sizeof(tmp.free_events), 1, fp);
	res += fread(&tmp.tgid, sizeof(tmp.tgid), 1, fp);
	res += fread(&tmp.ppid, sizeof(tmp.ppid), 1, fp);
	res += fread(&tmp.cpu, sizeof(tmp.cpu), 1, fp);
	res += fread(&tmp.creation_time, sizeof(tmp.creation_time), 1, fp);
	res += fread(&tmp.insertion_time, sizeof(tmp.insertion_time), 1, fp);
	g_assert(res == 10);

	if(tmp.pid == 0) {
		process = lttv_state_find_process(self, tmp.cpu, tmp.pid);
	} else {
		/* We must link to the parent */
		parent_process = lttv_state_find_process_or_create(self, ANY_CPU, tmp.ppid,
				&ltt_time_zero);
		process = lttv_state_find_process(self, ANY_CPU, tmp.pid);
		if(process == NULL) {
			process = lttv_state_create_process(self, parent_process, tmp.cpu,
					tmp.pid, tmp.tgid,
					g_quark_from_string((gchar*)g_ptr_array_index(quarktable, tmp.name)),
					&tmp.creation_time);
		}
	}
	process->insertion_time = tmp.insertion_time;
	process->creation_time = tmp.creation_time;
	process->type = g_quark_from_string(
			(gchar*)g_ptr_array_index(quarktable, tmp.type));
	process->tgid = tmp.tgid;
	process->ppid = tmp.ppid;
	process->name =
			g_quark_from_string((gchar*)g_ptr_array_index(quarktable, tmp.name));
	process->free_events = tmp.free_events;

	do {
		if(feof(fp) || ferror(fp)) goto end_loop;

		gint hdr = fgetc(fp);
		if(hdr == EOF) goto end_loop;

		switch(hdr) {
			case HDR_ES:
				process->execution_stack =
					g_array_set_size(process->execution_stack,
							process->execution_stack->len + 1);
				es = &g_array_index(process->execution_stack, LttvExecutionState,
						process->execution_stack->len-1);
				process->state = es;

				res = fread(&es->t, sizeof(es->t), 1, fp);
				g_assert(res == 1);
				es->t = g_quark_from_string(
						(gchar*)g_ptr_array_index(quarktable, es->t));
				res = fread(&es->n, sizeof(es->n), 1, fp);
				g_assert(res == 1);
				es->n = g_quark_from_string(
						(gchar*)g_ptr_array_index(quarktable, es->n));
				res = fread(&es->s, sizeof(es->s), 1, fp);
				g_assert(res == 1);
				es->s = g_quark_from_string(
						(gchar*)g_ptr_array_index(quarktable, es->s));
				res = fread(&es->entry, sizeof(es->entry), 1, fp);
				res += fread(&es->change, sizeof(es->change), 1, fp);
				res += fread(&es->cum_cpu_time, sizeof(es->cum_cpu_time), 1, fp);
				g_assert(res == 3);
				break;

			case HDR_USER_STACK:
				process->user_stack = g_array_set_size(process->user_stack,
						process->user_stack->len + 1);
				address = &g_array_index(process->user_stack, guint64,
						process->user_stack->len-1);
				res = fread(address, sizeof(address), 1, fp);
				g_assert(res == 1);
				process->current_function = *address;
				break;

			default:
				ungetc(hdr, fp);
				goto end_loop;
		};
	} while(1);
end_loop:
	return;
}


/* Called because a HDR_PROCESS_STATE was found */
/* Append a saved state to the trace states */
void lttv_state_read_raw(LttvTraceState *self, FILE *fp, GPtrArray *quarktable)
{
	guint i, nb_tracefile, nb_block, offset;
	guint64 tsc;
	LttvTracefileState *tfcs;

	LttEventPosition *ep;

	guint nb_cpus;

	int hdr;
	size_t res;

	LttTime t;

	LttvAttribute *saved_states_tree, *saved_state_tree;

	LttvAttributeValue value;
	GTree *pqueue = self->parent.ts_context->pqueue;
	ep = ltt_event_position_new();

	restore_init_state(self);

	res = fread(&t, sizeof(t), 1, fp);
	g_assert(res == 1);

	do {
		if(feof(fp) || ferror(fp)) goto end_loop;
		hdr = fgetc(fp);
		if(hdr == EOF) goto end_loop;

		switch(hdr) {
			case HDR_PROCESS:
				/* Call read_process_state_raw */
				read_process_state_raw(self, fp, quarktable);
				break;
			case HDR_TRACEFILE:
			case HDR_TRACESET:
			case HDR_TRACE:
			case HDR_QUARKS:
			case HDR_QUARK:
			case HDR_ES:
			case HDR_USER_STACK:
			case HDR_PROCESS_STATE:
			case HDR_CPU:
				ungetc(hdr, fp);
				goto end_loop;
				break;
			default:
				g_error("Error while parsing saved state file : unknown data header %d",
						hdr);
		};
	} while(1);
end_loop:

	nb_cpus = ltt_trace_get_num_cpu(self->parent.t);
	for(i=0;i<nb_cpus;i++) {
		int cpu_num;
		hdr = fgetc(fp);
		g_assert(hdr == HDR_CPU);
		res = fread(&cpu_num, sizeof(cpu_num), 1, fp); /* cpu number */
		g_assert(res == 1);
		g_assert(i == cpu_num);
		res = fread(&self->running_process[i]->pid,
				sizeof(self->running_process[i]->pid), 1, fp);
		g_assert(res == 1);
	}

	nb_tracefile = self->parent.tracefiles->len;

	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
						LttvTracefileContext*, i));
	//	fprintf(fp, "  <TRACEFILE TIMESTAMP_S=%lu TIMESTAMP_NS=%lu",
	//			tfcs->parent.timestamp.tv_sec,
	//			tfcs->parent.timestamp.tv_nsec);
		g_tree_remove(pqueue, &tfcs->parent);
		hdr = fgetc(fp);
		g_assert(hdr == HDR_TRACEFILE);
		res = fread(&tfcs->parent.timestamp, sizeof(tfcs->parent.timestamp), 1, fp);
		g_assert(res == 1);
		/* Note : if timestamp if LTT_TIME_INFINITE, there will be no
		 * position following : end of trace */
		if(ltt_time_compare(tfcs->parent.timestamp, ltt_time_infinite) != 0) {
			res = fread(&nb_block, sizeof(nb_block), 1, fp);
			res += fread(&offset, sizeof(offset), 1, fp);
			res += fread(&tsc, sizeof(tsc), 1, fp);
			g_assert(res == 3);
			ltt_event_position_set(ep, tfcs->parent.tf, nb_block, offset, tsc);
			gint ret = ltt_tracefile_seek_position(tfcs->parent.tf, ep);
			g_assert(ret == 0);
			g_tree_insert(pqueue, &tfcs->parent, &tfcs->parent);
		}
	}
	g_free(ep);

	saved_states_tree = lttv_attribute_find_subdir(self->parent.t_a,
			LTTV_STATE_SAVED_STATES);
	saved_state_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
	value = lttv_attribute_add(saved_states_tree,
			lttv_attribute_get_number(saved_states_tree), LTTV_GOBJECT);
	*(value.v_gobject) = (GObject *)saved_state_tree;
	value = lttv_attribute_add(saved_state_tree, LTTV_STATE_TIME, LTTV_TIME);
	*(value.v_time) = t;
	lttv_state_save(self, saved_state_tree);
	g_debug("Saving state at time %lu.%lu", t.tv_sec,
			t.tv_nsec);

	*(self->max_time_state_recomputed_in_seek) = t;

}

/* Called when a HDR_TRACE is found */
void lttv_trace_states_read_raw(LttvTraceState *tcs, FILE *fp,
		GPtrArray *quarktable)
{
	int hdr;

	do {
		if(feof(fp) || ferror(fp)) goto end_loop;
		hdr = fgetc(fp);
		if(hdr == EOF) goto end_loop;

		switch(hdr) {
			case HDR_PROCESS_STATE:
				/* Call read_process_state_raw */
				lttv_state_read_raw(tcs, fp, quarktable);
				break;
			case HDR_TRACEFILE:
			case HDR_TRACESET:
			case HDR_TRACE:
			case HDR_QUARKS:
			case HDR_QUARK:
			case HDR_ES:
			case HDR_USER_STACK:
			case HDR_PROCESS:
			case HDR_CPU:
				g_error("Error while parsing saved state file :"
						" unexpected data header %d",
						hdr);
				break;
			default:
				g_error("Error while parsing saved state file : unknown data header %d",
						hdr);
		};
	} while(1);
end_loop:
	*(tcs->max_time_state_recomputed_in_seek) = tcs->parent.time_span.end_time;
	restore_init_state(tcs);
	lttv_process_trace_seek_time(&tcs->parent, ltt_time_zero);
	return;
}
#endif /* BABEL_CLEANUP */


/* Copy each process from an existing hash table to a new one */

static void copy_process_state(gpointer key, gpointer value,gpointer user_data)
{
	LttvProcessState *process, *new_process;

	GHashTable *new_processes = (GHashTable *)user_data;

	guint i;

	process = (LttvProcessState *)value;
	new_process = g_new(LttvProcessState, 1);
	*new_process = *process;
	new_process->execution_stack = g_array_sized_new(FALSE, FALSE,
			sizeof(LttvExecutionState), PREALLOCATED_EXECUTION_STACK);
	new_process->execution_stack =
			g_array_set_size(new_process->execution_stack,
			process->execution_stack->len);
	for(i = 0 ; i < process->execution_stack->len; i++) {
		g_array_index(new_process->execution_stack, LttvExecutionState, i) =
				g_array_index(process->execution_stack, LttvExecutionState, i);
	}
	new_process->state = &g_array_index(new_process->execution_stack,
			LttvExecutionState, new_process->execution_stack->len - 1);
#ifdef BABEL_CLEANUP
	new_process->user_stack = g_array_sized_new(FALSE, FALSE,
			sizeof(guint64), 0);
	new_process->user_stack = g_array_set_size(new_process->user_stack,
			process->user_stack->len);
	for(i = 0 ; i < process->user_stack->len; i++) {
		g_array_index(new_process->user_stack, guint64, i) =
				g_array_index(process->user_stack, guint64, i);
	}
	new_process->current_function = process->current_function;
#endif /* BABEL_CLEANUP */

	/* fd hash table stuff */
	{
		GHashTableIter it;
		gpointer key;
		gpointer value;

		/* copy every item in the hash table */
		new_process->fds = g_hash_table_new(g_direct_hash, g_direct_equal);

		g_hash_table_iter_init(&it, process->fds);
		while (g_hash_table_iter_next (&it, (void *)&key, (void *)&value)) {
			g_hash_table_insert(new_process->fds, key, value);
		}
	}

	/* When done creating the new process state, insert it in the
	 * hash table */
	g_hash_table_insert(new_processes, new_process, new_process);
}


static GHashTable *lttv_state_copy_process_table(GHashTable *processes)
{
	GHashTable *new_processes = g_hash_table_new(process_hash, process_equal);

	g_hash_table_foreach(processes, copy_process_state, new_processes);
	return new_processes;
}

static LttvCPUState *lttv_state_copy_cpu_states(LttvCPUState *states, guint n)
{
	guint i,j;
	LttvCPUState *retval;

	retval = g_new(LttvCPUState, n);

	for(i=0; i<n; i++) {
		retval[i].irq_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_set_size(retval[i].irq_stack, states[i].irq_stack->len);
		for(j=0; j<states[i].irq_stack->len; j++) {
			g_array_index(retval[i].irq_stack, gint, j) =
					g_array_index(states[i].irq_stack, gint, j);
		}

		retval[i].softirq_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_set_size(retval[i].softirq_stack, states[i].softirq_stack->len);
		for(j=0; j<states[i].softirq_stack->len; j++) {
			g_array_index(retval[i].softirq_stack, gint, j) =
					g_array_index(states[i].softirq_stack, gint, j);
		}

		retval[i].trap_stack = g_array_new(FALSE, FALSE, sizeof(gint));
		g_array_set_size(retval[i].trap_stack, states[i].trap_stack->len);
		for(j=0; j<states[i].trap_stack->len; j++) {
			g_array_index(retval[i].trap_stack, gint, j) =
					g_array_index(states[i].trap_stack, gint, j);
		}

		retval[i].mode_stack = g_array_new(FALSE, FALSE, sizeof(LttvCPUMode));
		g_array_set_size(retval[i].mode_stack, states[i].mode_stack->len);
		for(j=0; j<states[i].mode_stack->len; j++) {
			g_array_index(retval[i].mode_stack, GQuark, j) =
					g_array_index(states[i].mode_stack, GQuark, j);
		}
	}

	return retval;
}

static void lttv_state_free_cpu_states(LttvCPUState *states, guint n)
{
	guint i;

	for(i=0; i<n; i++) {
		g_array_free(states[i].mode_stack, TRUE);
		g_array_free(states[i].irq_stack, TRUE);
		g_array_free(states[i].softirq_stack, TRUE);
		g_array_free(states[i].trap_stack, TRUE);
	}

	g_free(states);
}

static LttvIRQState *lttv_state_copy_irq_states(LttvIRQState *states, guint n)
{
	guint i,j;
	LttvIRQState *retval;

	retval = g_new(LttvIRQState, n);

	for(i=0; i<n; i++) {
		retval[i].mode_stack = g_array_new(FALSE, FALSE, sizeof(LttvIRQMode));
		g_array_set_size(retval[i].mode_stack, states[i].mode_stack->len);
		for(j=0; j<states[i].mode_stack->len; j++) {
			g_array_index(retval[i].mode_stack, GQuark, j) =
					g_array_index(states[i].mode_stack, GQuark, j);
		}
	}

	return retval;
}

static void lttv_state_free_irq_states(LttvIRQState *states, guint n)
{
	guint i;

	for(i=0; i<n; i++) {
		g_array_free(states[i].mode_stack, TRUE);
	}

	g_free(states);
}

static LttvSoftIRQState *
lttv_state_copy_soft_irq_states(LttvSoftIRQState *states, guint n)
{
	guint i;
	LttvSoftIRQState *retval;

	retval = g_new(LttvSoftIRQState, n);

	for(i=0; i<n; i++) {
		retval[i].pending = states[i].pending;
		retval[i].running = states[i].running;
	}

	return retval;
}

static void lttv_state_free_soft_irq_states(LttvSoftIRQState *states, guint n)
{
	g_free(states);
}

static LttvTrapState *
lttv_state_copy_trap_states(LttvTrapState *states, guint n)
{
	guint i;
	LttvTrapState *retval;

	retval = g_new(LttvTrapState, n);

	for(i=0; i<n; i++) {
		retval[i].running = states[i].running;
	}

	return retval;
}

static void lttv_state_free_trap_states(LttvTrapState *states, guint n)
{
	g_free(states);
}
#ifdef BABEL_CLEANUP
/* bdevstate stuff */

static LttvBdevState *get_hashed_bdevstate(LttvTraceState *ts, guint32 devcode)
{
	gint devcode_gint = devcode;
	gpointer bdev = g_hash_table_lookup(ts->bdev_states, &devcode_gint);
	if(bdev == NULL) {
		LttvBdevState *bdevstate = g_new(LttvBdevState, 1);
		bdevstate->mode_stack = g_array_new(FALSE, FALSE, sizeof(GQuark));

		gint * key = g_new(gint, 1);
		*key = devcode;
		g_hash_table_insert(ts->bdev_states, key, bdevstate);

		bdev = bdevstate;
	}

	return bdev;
}
#endif
static LttvBdevState *bdevstate_new(void)
{
	LttvBdevState *retval;
	retval = g_new(LttvBdevState, 1);
	retval->mode_stack = g_array_new(FALSE, FALSE, sizeof(GQuark));

	return retval;
}

static void bdevstate_free(LttvBdevState *bds)
{
	g_array_free(bds->mode_stack, TRUE);
	g_free(bds);
}

static void bdevstate_free_cb(gpointer key, gpointer value, gpointer user_data)
{
	LttvBdevState *bds = (LttvBdevState *) value;

	bdevstate_free(bds);
}

static LttvBdevState *bdevstate_copy(LttvBdevState *bds)
{
	LttvBdevState *retval;

	retval = bdevstate_new();
	g_array_insert_vals(retval->mode_stack, 0, bds->mode_stack->data,
			bds->mode_stack->len);

	return retval;
}

static void insert_and_copy_bdev_state(gpointer k, gpointer v, gpointer u)
{
	//GHashTable *ht = (GHashTable *)u;
	LttvBdevState *bds = (LttvBdevState *)v;
	LttvBdevState *newbds;

	newbds = bdevstate_copy(bds);

	g_hash_table_insert(u, k, newbds);
}

static GHashTable *lttv_state_copy_blkdev_hashtable(GHashTable *ht)
{
	GHashTable *retval;

	retval = g_hash_table_new(g_int_hash, g_int_equal);

	g_hash_table_foreach(ht, insert_and_copy_bdev_state, retval);

	return retval;
}

/* Free a hashtable and the LttvBdevState structures its values
 * point to. */

static void lttv_state_free_blkdev_hashtable(GHashTable *ht)
{
	g_hash_table_foreach(ht, bdevstate_free_cb, NULL);
	g_hash_table_destroy(ht);
}

/* The saved state for each trace contains a member "processes", which
   stores a copy of the process table, and a member "tracefiles" with
   one entry per tracefile. Each tracefile has a "process" member pointing
   to the current process and a "position" member storing the tracefile
   position (needed to seek to the current "next" event. */

void lttv_state_save(LttvTraceState *self, LttvAttribute *container)
{
	guint i, nb_cpus, nb_irqs, nb_soft_irqs, nb_traps;

	guint *running_process;

	LttvAttributeValue value;

	value = lttv_attribute_add(container, LTTV_STATE_PROCESSES,
			LTTV_POINTER);
	*(value.v_pointer) = lttv_state_copy_process_table(self->processes);

	/* Add the currently running processes array */
	nb_cpus = lttv_trace_get_num_cpu(self->trace);
	running_process = g_new(guint, nb_cpus);
	for(i=0;i<nb_cpus;i++) {
		running_process[i] = self->running_process[i]->pid;
	}
	value = lttv_attribute_add(container, LTTV_STATE_RUNNING_PROCESS,
			LTTV_POINTER);
	*(value.v_pointer) = running_process;

	g_info("State save");

	/* Save the current position */
	value = lttv_attribute_add(container, LTTV_STATE_POSITION,
			LTTV_POINTER);
	*(value.v_pointer) = lttv_traceset_create_current_position(lttv_trace_get_traceset(self->trace));

#ifdef BABEL_CLEANUP
	nb_tracefile = self->parent.tracefiles->len;
	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
						LttvTracefileContext*, i));
		tracefile_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
		value = lttv_attribute_add(tracefiles_tree, i,
				LTTV_GOBJECT);
		*(value.v_gobject) = (GObject *)tracefile_tree;
#if 0
		value = lttv_attribute_add(tracefile_tree, LTTV_STATE_PROCESS,
				LTTV_UINT);
		*(value.v_uint) = tfcs->process->pid;
#endif //0
		value = lttv_attribute_add(tracefile_tree, LTTV_STATE_EVENT,
				LTTV_POINTER);
		/* Only save the position if the tfs has not infinite time. */
		//if(!g_tree_lookup(self->parent.ts_context->pqueue, &tfcs->parent)
		//		&& current_tfcs != tfcs) {
		if(ltt_time_compare(tfcs->parent.timestamp, ltt_time_infinite) == 0) {
			*(value.v_pointer) = NULL;
		} else {
			LttEvent *e = ltt_tracefile_get_event(tfcs->parent.tf);
			ep = ltt_event_position_new();
			ltt_event_position(e, ep);
			*(value.v_pointer) = ep;

			guint nb_block, offset;
			guint64 tsc;
			LttTracefile *tf;
			ltt_event_position_get(ep, &tf, &nb_block, &offset, &tsc);
			g_info("Block %u offset %u tsc %" PRIu64 " time %lu.%lu", nb_block,
					offset, tsc, tfcs->parent.timestamp.tv_sec,
					tfcs->parent.timestamp.tv_nsec);
		}
	}
#endif /* BABEL_CLEANUP */

	/* save the cpu state */
	{
		value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_CPUS_COUNT,
				LTTV_UINT);
		*(value.v_uint) = nb_cpus;

		value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_CPUS,
				LTTV_POINTER);
		*(value.v_pointer) = lttv_state_copy_cpu_states(self->cpu_states, nb_cpus);
	}

	/* save the irq state */
	nb_irqs = self->name_tables->nb_irqs;
	{
		value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_IRQS,
				LTTV_POINTER);
		*(value.v_pointer) = lttv_state_copy_irq_states(self->irq_states, nb_irqs);
	}

	/* save the soft irq state */
	nb_soft_irqs = self->name_tables->nb_soft_irqs;
	{
		value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_SOFT_IRQS,
				LTTV_POINTER);
		*(value.v_pointer) = lttv_state_copy_soft_irq_states(self->soft_irq_states, nb_soft_irqs);
	}

	/* save the trap state */
	nb_traps = self->name_tables->nb_traps;
	{
		value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_TRAPS,
				LTTV_POINTER);
		*(value.v_pointer) = lttv_state_copy_trap_states(self->trap_states, nb_traps);
	}

	/* save the blkdev states */
	value = lttv_attribute_add(container, LTTV_STATE_RESOURCE_BLKDEVS,
				LTTV_POINTER);
	*(value.v_pointer) = lttv_state_copy_blkdev_hashtable(self->bdev_states);
}

void lttv_state_restore(LttvTraceState *self, LttvAttribute *container)
{
	guint i, pid, nb_cpus, nb_irqs, nb_soft_irqs, nb_traps;
	guint *running_process;
	LttvAttributeType type;
	LttvAttributeValue value;

	type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES,
			&value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_process_table(self->processes);
	self->processes = lttv_state_copy_process_table(*(value.v_pointer));

	/* Add the currently running processes array */
	nb_cpus = lttv_trace_get_num_cpu(self->trace);
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RUNNING_PROCESS,
				&value);
	g_assert(type == LTTV_POINTER);
	running_process = *(value.v_pointer);
	for(i=0;i<nb_cpus;i++) {
		pid = running_process[i];
		self->running_process[i] = lttv_state_find_process(self, i, pid);
		g_assert(self->running_process[i] != NULL);
	}

	//nb_tracefile = self->parent.tracefiles->len;

	//g_tree_destroy(tsc->pqueue);
	//tsc->pqueue = g_tree_new(compare_tracefile);

	/* restore cpu resource states */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_CPUS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_cpu_states(self->cpu_states, nb_cpus);
	self->cpu_states = lttv_state_copy_cpu_states(*(value.v_pointer), nb_cpus);
 
	/* restore irq resource states */
	nb_irqs = self->name_tables->nb_irqs;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_IRQS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_irq_states(self->irq_states, nb_irqs);
	self->irq_states = lttv_state_copy_irq_states(*(value.v_pointer), nb_irqs);
 
	/* restore soft irq resource states */
	nb_soft_irqs = self->name_tables->nb_soft_irqs;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_SOFT_IRQS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_soft_irq_states(self->soft_irq_states, nb_soft_irqs);
	self->soft_irq_states = lttv_state_copy_soft_irq_states(*(value.v_pointer), nb_soft_irqs);
 
	/* restore trap resource states */
	nb_traps = self->name_tables->nb_traps;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_TRAPS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_trap_states(self->trap_states, nb_traps);
	self->trap_states = lttv_state_copy_trap_states(*(value.v_pointer), nb_traps);
 
	/* restore the blkdev states */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_BLKDEVS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_blkdev_hashtable(self->bdev_states);
	self->bdev_states = lttv_state_copy_blkdev_hashtable(*(value.v_pointer));

#ifdef BABEL_CLEANUP
	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(g_array_index(self->parent.tracefiles,
						LttvTracefileContext*, i));
		type = lttv_attribute_get(tracefiles_tree, i, &name, &value, &is_named);
		g_assert(type == LTTV_GOBJECT);
		tracefile_tree = *((LttvAttribute **)(value.v_gobject));
#if 0
		type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_PROCESS,
				&value);
		g_assert(type == LTTV_UINT);
		pid = *(value.v_uint);
		tfcs->process = lttv_state_find_process_or_create(tfcs, pid);
#endif //0
		type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT,
				&value);
		g_assert(type == LTTV_POINTER);
		//g_assert(*(value.v_pointer) != NULL);
		ep = *(value.v_pointer);
		g_assert(tfcs->parent.t_context != NULL);

		tfcs->cpu_state = &self->cpu_states[tfcs->cpu];

		LttvTracefileContext *tfc = LTTV_TRACEFILE_CONTEXT(tfcs);
		g_tree_remove(tsc->pqueue, tfc);

		if(ep != NULL) {
			retval= ltt_tracefile_seek_position(tfc->tf, ep);
			g_assert_cmpint(retval, ==, 0);
			tfc->timestamp = ltt_event_time(ltt_tracefile_get_event(tfc->tf));
			g_assert_cmpint(ltt_time_compare(tfc->timestamp, ltt_time_infinite),
					!=, 0);
			g_tree_insert(tsc->pqueue, tfc, tfc);
			g_info("Restoring state for a tf at time %lu.%lu",
					tfc->timestamp.tv_sec, tfc->timestamp.tv_nsec);
		} else {
			tfc->timestamp = ltt_time_infinite;
		}
	}
#endif /* BABEL_CLEANUP */
}

/*
 * Note: the position must be explicitely set on the entire traceset to
 * match the trace states.
 */
LttvTracesetPosition *lttv_trace_state_get_position(LttvAttribute *container)
{
	LttvAttributeType type;
	LttvAttributeValue value;

	type = lttv_attribute_get_by_name(container, LTTV_STATE_POSITION, &value);
	g_assert(type == LTTV_POINTER);
	return *(value.v_pointer);
}

void lttv_state_saved_free(LttvTraceState *self, LttvAttribute *container)
{
	guint nb_cpus, nb_irqs, nb_soft_irqs;
	guint *running_process;
	LttvAttributeType type;
	LttvAttributeValue value;

	type = lttv_attribute_get_by_name(container, LTTV_STATE_PROCESSES,
			&value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_process_table(*(value.v_pointer));
	*(value.v_pointer) = NULL;
	lttv_attribute_remove_by_name(container, LTTV_STATE_PROCESSES);

	/* Free running processes array */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RUNNING_PROCESS,
			&value);
	g_assert(type == LTTV_POINTER);
	running_process = *(value.v_pointer);
	g_free(running_process);

	/* free cpu resources */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_CPUS_COUNT, &value);
	g_assert(type == LTTV_UINT);
	nb_cpus = *value.v_uint;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_CPUS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_cpu_states(*(value.v_pointer), nb_cpus);

	/* free irq resource states */
	nb_irqs = self->name_tables->nb_irqs;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_IRQS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_irq_states(*(value.v_pointer), nb_irqs);

	/* free softirq resource states */
	nb_soft_irqs = self->name_tables->nb_soft_irqs;
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_SOFT_IRQS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_soft_irq_states(*(value.v_pointer), nb_soft_irqs);

	/* free the blkdev states */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_RESOURCE_BLKDEVS, &value);
	g_assert(type == LTTV_POINTER);
	lttv_state_free_blkdev_hashtable(*(value.v_pointer));

	/* remove the position */
	type = lttv_attribute_get_by_name(container, LTTV_STATE_POSITION, &value);
	g_assert(type == LTTV_POINTER);
	lttv_traceset_destroy_position(*(value.v_pointer));

#ifdef BABEL_CLEANUP
	nb_tracefile = self->parent.tracefiles->len;

	for(i = 0 ; i < nb_tracefile ; i++) {
	
		type = lttv_attribute_get(tracefiles_tree, i, &name, &value, &is_named);
		g_assert(type == LTTV_GOBJECT);
		tracefile_tree = *((LttvAttribute **)(value.v_gobject));

		type = lttv_attribute_get_by_name(tracefile_tree, LTTV_STATE_EVENT,
				&value);
		g_assert(type == LTTV_POINTER);
		if(*(value.v_pointer) != NULL) g_free(*(value.v_pointer));
	}
	g_object_unref(G_OBJECT(tracefiles_tree));
#endif /* BABEL_CLEANUP */
}


static void free_saved_state(LttvTraceState *self)
{
	guint i, nb;
	LttvAttributeType type;
	LttvAttributeValue value;
	LttvAttributeName name;
	gboolean is_named;
	LttvAttribute *saved_states;

	saved_states = lttv_attribute_find_subdir(lttv_trace_attribute(self->trace),
			LTTV_STATE_SAVED_STATES);

	nb = lttv_attribute_get_number(saved_states);
	for(i = 0 ; i < nb ; i++) {
		type = lttv_attribute_get(saved_states, i, &name, &value, &is_named);
		g_assert(type == LTTV_GOBJECT);
		lttv_state_saved_free(self, *((LttvAttribute **)value.v_gobject));
	}

	lttv_attribute_remove_by_name(lttv_trace_attribute(self->trace),
		LTTV_STATE_SAVED_STATES);
}


static void create_max_time(LttvTraceState *trace_state)
{
	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(trace_state->trace),
			LTTV_STATE_SAVED_STATES_TIME,
			LTTV_POINTER, &v);
	g_assert(*(v.v_pointer) == NULL);
	*(v.v_pointer) = g_new(LttTime,1);
	*((LttTime *)*(v.v_pointer)) = ltt_time_zero;
}


static void get_max_time(LttvTraceState *trace_state)
{
	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(trace_state->trace),
			LTTV_STATE_SAVED_STATES_TIME,
			LTTV_POINTER, &v);
	g_assert(*(v.v_pointer) != NULL);
	trace_state->max_time_state_recomputed_in_seek = (LttTime *)*(v.v_pointer);
}


static void free_max_time(LttvTraceState *trace_state)
{
	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(trace_state->trace),
			LTTV_STATE_SAVED_STATES_TIME,
			LTTV_POINTER, &v);
	g_free(*(v.v_pointer));
	*(v.v_pointer) = NULL;
}

static void create_name_tables(LttvTraceState *tcs)
{
	int i;

	GString *fe_name = g_string_new("");

	LttvNameTables *name_tables = g_new(LttvNameTables, 1);

	LttvAttributeValue v;

	//	GArray *hooks;

	lttv_attribute_find(lttv_trace_attribute(tcs->trace), LTTV_STATE_NAME_TABLES,
			LTTV_POINTER, &v);
	g_assert(*(v.v_pointer) == NULL);
	*(v.v_pointer) = name_tables;

#ifdef BABEL_CLEANUP
	hooks = g_array_sized_new(FALSE, FALSE, sizeof(LttvTraceHook), 1);

	if(!lttv_trace_find_hook(tcs->parent.t,
			LTT_CHANNEL_KERNEL,
			LTT_EVENT_SYSCALL_ENTRY,
			FIELD_ARRAY(LTT_FIELD_SYSCALL_ID),
			NULL, NULL, &hooks)) {

//		th = lttv_trace_hook_get_first(&th);
//
//		t = ltt_field_type(lttv_trace_get_hook_field(th, 0));
//		nb = ltt_type_element_number(t);
//
//		name_tables->syscall_names = g_new(GQuark, nb);
//		name_tables->nb_syscalls = nb;
//
//		for(i = 0 ; i < nb ; i++) {
//			name_tables->syscall_names[i] = ltt_enum_string_get(t, i);
//			if(!name_tables->syscall_names[i]) {
//				GString *string = g_string_new("");
//				g_string_printf(string, "syscall %u", i);
//				name_tables->syscall_names[i] = g_quark_from_string(string->str);
//				g_string_free(string, TRUE);
//			}
//		}
#endif /* BABEL_CLEANUP */

		name_tables->nb_syscalls = PREALLOC_NB_SYSCALLS;
		name_tables->syscall_names = g_new(GQuark, name_tables->nb_syscalls);
		for(i = 0 ; i < name_tables->nb_syscalls; i++) {
			g_string_printf(fe_name, "syscall %d", i);
			name_tables->syscall_names[i] = g_quark_from_string(fe_name->str);
		}
#ifdef BABEL_CLEANUP
	} else {
		name_tables->syscall_names = NULL;
		name_tables->nb_syscalls = 0;
	}
	lttv_trace_hook_remove_all(&hooks);
#endif /* BABEL_CLEANUP */

#ifdef BABEL_CLEANUP
	if(!lttv_trace_find_hook(tcs->parent.t,
				LTT_CHANNEL_KERNEL,
				LTT_EVENT_TRAP_ENTRY,
				FIELD_ARRAY(LTT_FIELD_TRAP_ID),
				NULL, NULL, &hooks) ||
		 !lttv_trace_find_hook(tcs->parent.t,
				LTT_CHANNEL_KERNEL,
				LTT_EVENT_PAGE_FAULT_ENTRY,
				FIELD_ARRAY(LTT_FIELD_TRAP_ID),
				NULL, NULL, &hooks)) {

//		th = lttv_trace_hook_get_first(&th);
//
//		t = ltt_field_type(lttv_trace_get_hook_field(th, 0));
//		//nb = ltt_type_element_number(t);
//
//		name_tables->trap_names = g_new(GQuark, nb);
//		for(i = 0 ; i < nb ; i++) {
//			name_tables->trap_names[i] = g_quark_from_string(
//					ltt_enum_string_get(t, i));
//		}

#endif /* BABEL_CLEANUP */
		name_tables->nb_traps = PREALLOC_NB_TRAPS;
		name_tables->trap_names = g_new(GQuark, name_tables->nb_traps);
		for(i = 0 ; i < name_tables->nb_traps; i++) {
			g_string_printf(fe_name, "trap %d", i);
			name_tables->trap_names[i] = g_quark_from_string(fe_name->str);
		}
#ifdef BABEL_CLEANUP
	} else {
		name_tables->trap_names = NULL;
		name_tables->nb_traps = 0;
	}
	lttv_trace_hook_remove_all(&hooks);
#endif /* BABEL_CLEANUP */

#ifdef BABEL_CLEANUP
	if(!lttv_trace_find_hook(tcs->parent.t,
				LTT_CHANNEL_KERNEL,
				LTT_EVENT_IRQ_ENTRY,
				FIELD_ARRAY(LTT_FIELD_IRQ_ID),
				NULL, NULL, &hooks)) {

		/*
		name_tables->irq_names = g_new(GQuark, nb);
		for(i = 0 ; i < nb ; i++) {
			name_tables->irq_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
		}
		*/
		/* FIXME: LttvIRQState *irq_states should become a g_array */

#endif /* BABEL_CLEANUP */
		name_tables->nb_irqs = PREALLOC_NB_IRQS;
		name_tables->irq_names = g_new(GQuark, name_tables->nb_irqs);
		for(i = 0 ; i < name_tables->nb_irqs; i++) {
			g_string_printf(fe_name, "irq %d", i);
			name_tables->irq_names[i] = g_quark_from_string(fe_name->str);
		}
#ifdef BABEL_CLEANUP
	} else {
		name_tables->nb_irqs = 0;
		name_tables->irq_names = NULL;
	}
	lttv_trace_hook_remove_all(&hooks);
#endif /* BABEL_CLEANUP */
	/*
	name_tables->soft_irq_names = g_new(GQuark, nb);
	for(i = 0 ; i < nb ; i++) {
		name_tables->soft_irq_names[i] = g_quark_from_string(ltt_enum_string_get(t, i));
	}
	*/

	name_tables->nb_soft_irqs = PREALLOC_NB_SOFT_IRQS;
	name_tables->soft_irq_names = g_new(GQuark, name_tables->nb_soft_irqs);
	for(i = 0 ; i < name_tables->nb_soft_irqs ; i++) {
		g_string_printf(fe_name, "softirq %d", i);
		name_tables->soft_irq_names[i] = g_quark_from_string(fe_name->str);
	}
	//	g_array_free(hooks, TRUE);

	g_string_free(fe_name, TRUE);

#if (__WORDSIZE == 32)
	name_tables->kprobe_hash = g_hash_table_new_full(guint64_hash, guint64_equal,
			g_free, NULL);
#else
	name_tables->kprobe_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
#endif
}


static void get_name_tables(LttvTraceState *tcs)
{
	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(tcs->trace), LTTV_STATE_NAME_TABLES,
			LTTV_POINTER, &v);
	g_assert(*(v.v_pointer) != NULL);
	tcs->name_tables = (LttvNameTables *)*(v.v_pointer);
}


static void free_name_tables(LttvTraceState *tcs)
{
	LttvNameTables *name_tables;

	LttvAttributeValue v;

	lttv_attribute_find(lttv_trace_attribute(tcs->trace), LTTV_STATE_NAME_TABLES,
			LTTV_POINTER, &v);
	name_tables = (LttvNameTables *)*(v.v_pointer);
	*(v.v_pointer) = NULL;

 // g_free(name_tables->eventtype_names);
	if(name_tables->syscall_names) g_free(name_tables->syscall_names);
	if(name_tables->trap_names) g_free(name_tables->trap_names);
	if(name_tables->irq_names) g_free(name_tables->irq_names);
	if(name_tables->soft_irq_names) g_free(name_tables->soft_irq_names);
	g_hash_table_destroy(name_tables->kprobe_hash);
	g_free(name_tables);
} 

#ifdef HASH_TABLE_DEBUG

static void test_process(gpointer key, gpointer value, gpointer user_data)
{
	LttvProcessState *process = (LttvProcessState *)value;

	/* Test for process corruption */
	guint stack_len = process->execution_stack->len;
}

static void hash_table_check(GHashTable *table)
{
	g_hash_table_foreach(table, test_process, NULL);
}


#endif

/* clears the stack and sets the state passed as argument */
static void cpu_set_base_mode(LttvCPUState *cpust, LttvCPUMode state)
{
	g_array_set_size(cpust->mode_stack, 1);
	((GQuark *)cpust->mode_stack->data)[0] = state;
}

static void cpu_push_mode(LttvCPUState *cpust, LttvCPUMode state)
{
	g_array_set_size(cpust->mode_stack, cpust->mode_stack->len + 1);
	((GQuark *)cpust->mode_stack->data)[cpust->mode_stack->len - 1] = state;
}

static void cpu_pop_mode(LttvCPUState *cpust)
{
	if(cpust->mode_stack->len <= 1)
		cpu_set_base_mode(cpust, LTTV_CPU_UNKNOWN);
	else
		g_array_set_size(cpust->mode_stack, cpust->mode_stack->len - 1);
}
#ifdef BABEL_CLEANUP
/* clears the stack and sets the state passed as argument */
static void bdev_set_base_mode(LttvBdevState *bdevst, LttvBdevMode state)
{
	g_array_set_size(bdevst->mode_stack, 1);
	((GQuark *)bdevst->mode_stack->data)[0] = state;
}

static void bdev_push_mode(LttvBdevState *bdevst, LttvBdevMode state)
{
	g_array_set_size(bdevst->mode_stack, bdevst->mode_stack->len + 1);
	((GQuark *)bdevst->mode_stack->data)[bdevst->mode_stack->len - 1] = state;
}

static void bdev_pop_mode(LttvBdevState *bdevst)
{
	if(bdevst->mode_stack->len <= 1)
		bdev_set_base_mode(bdevst, LTTV_BDEV_UNKNOWN);
	else
		g_array_set_size(bdevst->mode_stack, bdevst->mode_stack->len - 1);
}
#endif
static void irq_set_base_mode(LttvIRQState *irqst, LttvIRQMode state)
{
	g_array_set_size(irqst->mode_stack, 1);
	((GQuark *)irqst->mode_stack->data)[0] = state;
}

static void irq_push_mode(LttvIRQState *irqst, LttvIRQMode state)
{
	g_array_set_size(irqst->mode_stack, irqst->mode_stack->len + 1);
	((GQuark *)irqst->mode_stack->data)[irqst->mode_stack->len - 1] = state;
}

static void irq_pop_mode(LttvIRQState *irqst)
{
	if(irqst->mode_stack->len <= 1)
		irq_set_base_mode(irqst, LTTV_IRQ_UNKNOWN);
	else
		g_array_set_size(irqst->mode_stack, irqst->mode_stack->len - 1);
}

static void push_state(LttvEvent *event,
		LttvTraceState *ts, LttvExecutionMode t, 
		guint state_id)
{
	LttvExecutionState *es;
	gint cpu;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	g_assert(cpu >= 0);

#ifdef HASH_TABLE_DEBUG
	hash_table_check(ts->processes);
#endif
	LttvProcessState *process = ts->running_process[cpu];

	guint depth = process->execution_stack->len;

	process->execution_stack =
		g_array_set_size(process->execution_stack, depth + 1);
	/* Keep in sync */
	process->state =
		&g_array_index(process->execution_stack, LttvExecutionState, depth - 1);

	es = &g_array_index(process->execution_stack, LttvExecutionState, depth);
	es->t = t;
	es->n = state_id;
	es->entry = es->change = lttv_event_get_timestamp(event);
	es->cum_cpu_time = ltt_time_zero;
	es->s = process->state->s;
	process->state = es;
}

/* pop state
 * return 1 when empty, else 0 */
int 
lttv_state_pop_state_cleanup(LttvProcessState *process, LttvEvent *event)
{ 
	guint depth = process->execution_stack->len;

	if(depth == 1){
		return 1;
	}

	process->execution_stack =
			g_array_set_size(process->execution_stack, depth - 1);
	process->state = &g_array_index(process->execution_stack, LttvExecutionState,
			depth - 2);
	process->state->change = lttv_event_get_timestamp(event);

	return 0;
}

static void pop_state(LttvEvent *event,
		LttvTraceState *ts, LttvExecutionMode t)
{
	LttvProcessState *process;
	gint cpu;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	g_assert(cpu >= 0);
	process = ts->running_process[cpu];

	guint depth = process->execution_stack->len;

	if(process->state->t != t){
		g_info("Different execution mode type: ignore it\n"
		       );
		
		g_info("process state has %s when pop_int is %s\n",
				g_quark_to_string(process->state->t),
				g_quark_to_string(t));
		g_info("{ %u, %u, %s, %s }\n",
				process->pid,
				process->ppid,
				g_quark_to_string(process->name),
				g_quark_to_string(process->state->s));
		return;
	}

	if(depth == 1){
		//TODO ybrosseau readd debug
		//g_info("Trying to pop last state on stack (%lu.%09lu): ignore it\n",
		//		tfs->parent.timestamp.tv_sec, tfs->parent.timestamp.tv_nsec);
		g_info("Trying to pop last state on stack: ignore it\n");
		return;
	}

	process->execution_stack =
			g_array_set_size(process->execution_stack, depth - 1);

	process->state = &g_array_index(process->execution_stack, LttvExecutionState,
			depth - 2);
	process->state->change = lttv_event_get_timestamp(event);

	if((process->state->t == LTTV_STATE_MODE_UNKNOWN) && (t == LTTV_STATE_SYSCALL)) {
		//Force state at running
		process->state->t = LTTV_STATE_USER_MODE;
		process->state->s = LTTV_STATE_RUN;
	}
}

struct search_result {
	const LttTime *time;	/* Requested time */
	LttTime *best;	/* Best result */
};

/* Return a new and initialized LttvProcessState structure */

LttvProcessState *lttv_state_create_process(LttvTraceState *tcs,
		LttvProcessState *parent, guint cpu, guint pid,
		guint tgid, GQuark name, const LttTime *timestamp)
{
	LttvProcessState *process = g_new(LttvProcessState, 1);

	LttvExecutionState *es;

	char buffer[128];

	process->pid = pid;
	process->tgid = tgid;
	process->cpu = cpu;
	process->name = name;
	//process->last_cpu = tfs->cpu_name;
	//process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfs)->tf);
	process->type = LTTV_STATE_USER_THREAD;

	g_info("Process %u, core %p", process->pid, process);
	g_hash_table_insert(tcs->processes, process, process);

	if(parent) {
		process->ppid = parent->pid;
		process->creation_time = *timestamp;
	}

	/* No parent. This process exists but we are missing all information about
	   its creation. The birth time is set to zero but we remember the time of
	   insertion */

	else {
		process->ppid = 0;
		process->creation_time = ltt_time_zero;
	}

	process->insertion_time = *timestamp;
	sprintf(buffer,"%d-%lu.%lu",pid, process->creation_time.tv_sec,
			process->creation_time.tv_nsec);
	process->pid_time = g_quark_from_string(buffer);
	process->cpu = cpu;
	process->free_events = 0;
	//process->last_cpu = tfs->cpu_name;
	//process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)tfs)->tf);
	process->execution_stack = g_array_sized_new(FALSE, FALSE,
			sizeof(LttvExecutionState), PREALLOCATED_EXECUTION_STACK);
	process->execution_stack = g_array_set_size(process->execution_stack, 2);
	es = process->state = &g_array_index(process->execution_stack,
			LttvExecutionState, 0);
	es->t = LTTV_STATE_USER_MODE;
	es->n = LTTV_STATE_SUBMODE_NONE;
	es->entry = *timestamp;
	//g_assert(timestamp->tv_sec != 0);
	es->change = *timestamp;
	es->cum_cpu_time = ltt_time_zero;
	es->s = LTTV_STATE_RUN;

	es = process->state = &g_array_index(process->execution_stack,
			LttvExecutionState, 1);
	es->t = LTTV_STATE_SYSCALL;
	es->n = LTTV_STATE_SUBMODE_NONE;
	es->entry = *timestamp;
	//g_assert(timestamp->tv_sec != 0);
	es->change = *timestamp;
	es->cum_cpu_time = ltt_time_zero;
	es->s = LTTV_STATE_WAIT_FORK;

	/* Allocate an empty function call stack. If it's empty, use 0x0. */
#ifdef BABEL_CLEANUP       
	//process->user_stack = g_array_sized_new(FALSE, FALSE,
	//		sizeof(guint64), 0);
#endif
	process->fds = g_hash_table_new(g_direct_hash, g_direct_equal);

	return process;
}

LttvProcessState *
lttv_state_find_process(LttvTraceState *ts, guint cpu, guint pid)
{
	LttvProcessState key;
	LttvProcessState *process;

	key.pid = pid;
	key.cpu = cpu;
	process = g_hash_table_lookup(ts->processes, &key);
	return process;
}

LttvProcessState *lttv_state_find_process_or_create(LttvTraceState *ts,
		guint cpu, guint pid, const LttTime *timestamp)
{
	LttvProcessState *process = lttv_state_find_process(ts, cpu, pid);
	LttvExecutionState *es;

	/* Put ltt_time_zero creation time for unexisting processes */
	if(unlikely(process == NULL)) {
		process = lttv_state_create_process(ts,
				NULL, cpu, pid, 0, LTTV_STATE_UNNAMED, timestamp);
		/* We are not sure is it's a kernel thread or normal thread, put the
		 * bottom stack state to unknown */
		process->execution_stack =
				g_array_set_size(process->execution_stack, 1);
		process->state = es =
				&g_array_index(process->execution_stack, LttvExecutionState, 0);
		es->t = LTTV_STATE_MODE_UNKNOWN;
		es->s = LTTV_STATE_UNNAMED;
	}
	return process;
}

/* FIXME : this function should be called when we receive an event telling that
 * release_task has been called in the kernel. In happens generally when
 * the parent waits for its child termination, but may also happens in special
 * cases in the child's exit : when the parent ignores its children SIGCCHLD or
 * has the flag SA_NOCLDWAIT. It can also happen when the child is part
 * of a killed thread group, but isn't the leader.
 */
static int exit_process(LttvEvent *event, LttvProcessState *process) 
{
	LttvTraceState *ts = event->state;
	LttvProcessState key;

	/* Wait for both schedule with exit dead and process free to happen.
	 * They can happen in any order. */
	if (++(process->free_events) < 2)
		return 0;

	key.pid = process->pid;
	key.cpu = process->cpu;
	g_hash_table_remove(ts->processes, &key);
	g_array_free(process->execution_stack, TRUE);

	/* the following also clears the content */
	g_hash_table_destroy(process->fds);

	g_free(process);
	return 1;
}


static void free_process_state(gpointer key, gpointer value,gpointer user_data)
{
	g_array_free(((LttvProcessState *)value)->execution_stack, TRUE);

	/* the following also clears the content */
	g_hash_table_destroy(((LttvProcessState *)value)->fds);

	g_free(value);
}


static void lttv_state_free_process_table(GHashTable *processes)
{
	g_hash_table_foreach(processes, free_process_state, NULL);
	g_hash_table_destroy(processes);
}


static gboolean syscall_entry(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;
	LttvProcessState *process;
	LttvExecutionSubmode submode;
	char syscall_name[200];

	event = (LttvEvent *) call_data;
	if (strncmp(lttv_traceset_get_name_from_event(event),
			"sys_", sizeof("sys_") - 1) != 0)
		return FALSE;

	strncpy(syscall_name,lttv_traceset_get_name_from_event(event)+4,200);
	
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];

	submode = g_quark_from_string(syscall_name);
	/* There can be no system call from PID 0 : unknown state */
	if(process->pid != 0)
		push_state(event, ts, LTTV_STATE_SYSCALL, submode);
	return FALSE;
}


static gboolean syscall_exit(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;
	LttvProcessState *process;


	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"exit_syscall") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];

	/* There can be no system call from PID 0 : unknown state */
	if (process->pid != 0)
		pop_state(event, ts, LTTV_STATE_SYSCALL);
	return FALSE;
}

#ifdef BABEL_CLEANUP
static gboolean trap_entry(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState *)s->parent.t_context;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	struct marker_field *f = lttv_trace_get_hook_field(th, 0);
	LttvNameTables *nt = ((LttvTraceState *)(s->parent.t_context))->name_tables;

	LttvExecutionSubmode submode;

	guint64 trap = ltt_event_get_long_unsigned(e, f);

	expand_trap_table(ts, trap);

	submode = nt->trap_names[trap];

	push_state(s, LTTV_STATE_TRAP, submode);

	/* update cpu status */
	cpu_push_mode(s->cpu_state, LTTV_CPU_TRAP);

	/* update trap status */
	g_array_append_val(s->cpu_state->trap_stack, trap);
	ts->trap_states[trap].running++;

	return FALSE;
}

static gboolean trap_exit(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState *)s->parent.t_context;

	pop_state(s, LTTV_STATE_TRAP);

	/* update cpu status */
	cpu_pop_mode(s->cpu_state);

	/* update trap status */
	if (s->cpu_state->trap_stack->len > 0) {
		gint last = g_array_index(s->cpu_state->trap_stack, gint,
				s->cpu_state->trap_stack->len-1);
		if(ts->trap_states[last].running)
			ts->trap_states[last].running--;
		g_array_remove_index(s->cpu_state->trap_stack,
				s->cpu_state->trap_stack->len-1);
	}
	return FALSE;
}
#endif /* BABEL_CLEANUP */

static gboolean irq_entry(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;

	LttvExecutionSubmode submode;
	LttvNameTables *nt;
	guint64 irq;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"irq_handler_entry") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;

	nt = ts->name_tables;
	irq = lttv_event_get_long(event, "irq");

	expand_irq_table(ts, irq);

	submode = nt->irq_names[irq];

	/* Do something with the info about being in user or system mode when int? */
	push_state(event, ts, LTTV_STATE_IRQ, submode);

	/* update cpu status */
	cpu_push_mode(&(ts->cpu_states[cpu]), LTTV_CPU_IRQ);

	/* update irq status */
	g_array_append_val(ts->cpu_states[cpu].irq_stack, irq);
	irq_push_mode(&ts->irq_states[irq], LTTV_IRQ_BUSY);

	return FALSE;
}

static gboolean soft_irq_exit(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;

	LttvCPUState *cpu_state;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"softirq_exit") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;

	cpu_state = &(ts->cpu_states[cpu]);
	pop_state(event, ts, LTTV_STATE_SOFT_IRQ);

	/* update cpu status */
	cpu_pop_mode(cpu_state);

	/* update softirq status */
	if (cpu_state->softirq_stack->len > 0) {
		gint last = g_array_index(cpu_state->softirq_stack, gint, cpu_state->softirq_stack->len-1);
		if(ts->soft_irq_states[last].running)
			ts->soft_irq_states[last].running--;
		g_array_remove_index(cpu_state->softirq_stack, cpu_state->softirq_stack->len-1);
	}
	return FALSE;
}

static gboolean irq_exit(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;

	LttvCPUState *cpu_state;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"irq_handler_exit") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	cpu_state = &(ts->cpu_states[cpu]);

	pop_state(event, ts, LTTV_STATE_IRQ);

	/* update cpu status */
	cpu_pop_mode(cpu_state);

	/* update irq status */
	if (cpu_state->irq_stack->len > 0) {
		gint last = g_array_index(cpu_state->irq_stack, gint, cpu_state->irq_stack->len-1);
		g_array_remove_index(cpu_state->irq_stack, cpu_state->irq_stack->len-1);
		irq_pop_mode(&ts->irq_states[last]);
	}

	return FALSE;
}

static gboolean soft_irq_raise(void *hook_data, void *call_data)
{
	LttvEvent *event;
	//guint cpu;
	LttvTraceState *ts;


	guint64 softirq;
	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"softirq_raise") != 0)
		return FALSE;

	//cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	softirq = lttv_event_get_long_unsigned(event, "vec");

	expand_soft_irq_table(ts, softirq);

	/* update softirq status */
	/* a soft irq raises are not cumulative */
	ts->soft_irq_states[softirq].pending=1;

	return FALSE;
}

static gboolean soft_irq_entry(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;
	LttvExecutionSubmode submode;
	LttvNameTables *nt;
	guint64 softirq;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"softirq_entry") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;




	softirq = lttv_event_get_long_unsigned(event, "vec");
	expand_soft_irq_table(ts, softirq);
	nt = ts->name_tables;
	submode = nt->soft_irq_names[softirq];

	/* Do something with the info about being in user or system mode when int? */
	push_state(event, ts, LTTV_STATE_SOFT_IRQ, submode);

	/* update cpu status */
	cpu_push_mode(&(ts->cpu_states[cpu]), LTTV_CPU_SOFT_IRQ);

	/* update softirq status */
	g_array_append_val(ts->cpu_states[cpu].softirq_stack, softirq);
	if (ts->soft_irq_states[softirq].pending)
		ts->soft_irq_states[softirq].pending--;
	ts->soft_irq_states[softirq].running++;

	return FALSE;
}

static gboolean enum_interrupt(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;	

	LttvNameTables *nt;


	GQuark action;
	guint irq;
	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"lttng_statedump_interrupt") != 0)
		return FALSE;
	ts = event->state;

	nt = ts->name_tables;
	irq = lttv_event_get_long_unsigned(event, "irq");
	action = g_quark_from_string(lttv_event_get_string(event,
							   "action"));
	expand_irq_table(ts, irq);
	nt->irq_names[irq] = action;

	return FALSE;
}

#ifdef BABEL_CLEANUP
static gboolean bdev_request_issue(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;

	guint major;
	guint minor;
	guint oper;
	guint32 devcode;
	gpointer bdev;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"block_rq_issue") != 0)
		return FALSE;

	ts = event->state;
	major = lttv_event_get_long_unsigned(event,);
		
	minor = lttv_event_get_long_unsigned(event,);
		
	oper = lttv_event_get_long_unsigned(event,);
		
	devcode = MKDEV(major,minor);

	/* have we seen this block device before? */
	bdev = get_hashed_bdevstate(ts, devcode);
	if(oper == 0)
		bdev_push_mode(bdev, LTTV_BDEV_BUSY_READING);
	else
		bdev_push_mode(bdev, LTTV_BDEV_BUSY_WRITING);

	return FALSE;
}

static gboolean bdev_request_complete(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState *)s->parent.t_context;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;

	guint major = ltt_event_get_long_unsigned(e,
			lttv_trace_get_hook_field(th, 0));
	guint minor = ltt_event_get_long_unsigned(e,
			lttv_trace_get_hook_field(th, 1));
	//guint oper = ltt_event_get_long_unsigned(e,
	//		lttv_trace_get_hook_field(th, 2));
	guint32 devcode = MKDEV(major,minor);

	/* have we seen this block device before? */
	gpointer bdev = get_hashed_bdevstate(ts, devcode);

	/* update block device */
	bdev_pop_mode(bdev);

	return FALSE;
}
#endif
#ifdef BABEL_CLEANUP
// We dont have the syscall table in LTTng 2.0
static gboolean dump_syscall(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
	LttvNameTables *nt = ts->name_tables;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	guint id;
	char *symbol;

	id = ltt_event_get_unsigned(e, lttv_trace_get_hook_field(th, 0));
	symbol = ltt_event_get_string(e, lttv_trace_get_hook_field(th, 2));

	expand_syscall_table(ts, id);
	nt->syscall_names[id] = g_quark_from_string(symbol);

	return FALSE;
}

static gboolean dump_kprobe(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	guint64 ip;
	char *symbol;

	ip = ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
	symbol = ltt_event_get_string(e, lttv_trace_get_hook_field(th, 1));

	expand_kprobe_table(ts, ip, symbol);

	return FALSE;
}
#endif
#ifdef BABEL_CLEANUP
static gboolean dump_softirq(void *hook_data, void *call_data)
{

	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
	LttvNameTables *nt = ts->name_tables;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	guint id;
	char *symbol;

	id = ltt_event_get_unsigned(e, lttv_trace_get_hook_field(th, 0));
	symbol = ltt_event_get_string(e, lttv_trace_get_hook_field(th, 2));

	expand_soft_irq_table(ts, id);
	nt->soft_irq_names[id] = g_quark_from_string(symbol);

	return FALSE;

}
#endif
static gboolean sched_try_wakeup(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	LttvProcessState *process;
	gint woken_pid;
	guint woken_cpu;
	LttTime timestamp;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sched_wakeup") != 0)
		return FALSE;

	ts = event->state;

	woken_pid = lttv_event_get_long(event, "tid");
	woken_cpu = lttv_event_get_long(event, "target_cpu");

	timestamp = lttv_event_get_timestamp(event);
	process = lttv_state_find_process_or_create(
						    ts,
						    woken_cpu, woken_pid,
						    &timestamp);

	if (process->state->s == LTTV_STATE_WAIT || process->state->s == LTTV_STATE_WAIT_FORK)
	{
		process->state->s = LTTV_STATE_WAIT_CPU;
		process->state->change = timestamp;
	}

	g_debug("Wakeup: process %d on CPU %u\n", woken_pid, woken_cpu);

	return FALSE;
}

static gboolean schedchange(void *hook_data, void *call_data)
{
	LttvEvent *event;
	guint cpu;
	LttvTraceState *ts;
	LttvProcessState *process;


	//LttvProcessState *old_process = ts->running_process[cpu];

	guint pid_in, pid_out;
	gint64 state_out;
	//TODO ybrosseau 2012-07-13: manage this 20 in a constact or dynamically
	char next_comm[20];
	LttTime timestamp;
	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sched_switch") != 0)
		return FALSE;

	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;	
	process = ts->running_process[cpu];
	pid_out = lttv_event_get_long(event, "prev_tid");
	pid_in = lttv_event_get_long(event, "next_tid");
	state_out = lttv_event_get_long(event, "prev_state");

	strncpy(next_comm, lttv_event_get_string(event, "next_comm"), 20);
	next_comm[20-1] = '\0';

	timestamp = lttv_event_get_timestamp(event);
	
	if(likely(process != NULL)) {

		/* We could not know but it was not the idle process executing.
		   This should only happen at the beginning, before the first schedule
		   event, and when the initial information (current process for each CPU)
		   is missing. It is not obvious how we could, after the fact, compensate
		   the wrongly attributed statistics. */

		//This test only makes sense once the state is known and if there is no
		//missing events. We need to silently ignore schedchange coming after a
		//process_free, or it causes glitches. (FIXME)
		//if(unlikely(process->pid != pid_out)) {
		//	g_assert(process->pid == 0);
		//}
		if(process->pid == 0
			&& process->state->t == LTTV_STATE_MODE_UNKNOWN) {
			if(pid_out == 0) {
				/*
				 * Scheduling out of pid 0 at beginning of the trace.
				 * We are typically in system call mode at this point although
				 * (FIXME) we might be in a trap handler.
				 */
				g_assert(process->execution_stack->len == 1);
				process->state->t = LTTV_STATE_SYSCALL;
				process->state->s = LTTV_STATE_WAIT;
				process->state->change = timestamp;
				process->state->entry = timestamp;
			}
		} else {
			if(unlikely(process->state->s == LTTV_STATE_EXIT)) {
				process->state->s = LTTV_STATE_ZOMBIE;
				process->state->change = timestamp;
			} else {
				if(unlikely(state_out == 0)) process->state->s = LTTV_STATE_WAIT_CPU;
				else process->state->s = LTTV_STATE_WAIT;
				process->state->change = timestamp;
			}

			if(state_out == 32 || state_out == 64) { /* EXIT_DEAD || TASK_DEAD */
				/* see sched.h for states */
				if (!exit_process(event, process)) {
					process->state->s = LTTV_STATE_DEAD;
					process->state->change = timestamp;
	}
			}
		}
	}
	process = ts->running_process[cpu] = lttv_state_find_process_or_create(
			ts,
			cpu, pid_in,
			&timestamp);
	process->state->s = LTTV_STATE_RUN;
	process->cpu = cpu;
 // process->last_cpu_index = ltt_tracefile_num(((LttvTracefileContext*)s)->tf);
	process->state->change = timestamp;
	process->name = g_quark_from_string(next_comm);

	/* update cpu status */
	if(pid_in == 0)
		/* going to idle task */
		cpu_set_base_mode(&(ts->cpu_states[cpu]), LTTV_CPU_IDLE);
	else {
		/* scheduling a real task.
		 * we must be careful here:
		 * if we just schedule()'ed to a process that is
		 * in a trap, we must put the cpu in trap mode
		 */
		cpu_set_base_mode(&(ts->cpu_states[cpu]), LTTV_CPU_BUSY);
		if(process->state->t == LTTV_STATE_TRAP)
			cpu_push_mode(&(ts->cpu_states[cpu]), LTTV_CPU_TRAP);
	}

	return FALSE;
}

static gboolean process_fork(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	LttvProcessState *process;
	LttvProcessState *child_process;	
	guint child_pid;   /* In the Linux Kernel, there is one PID per thread. */
	guint child_tgid;  /* tgid in the Linux kernel is the "real" POSIX PID. */
	//LttvProcessState *zombie_process;
	guint cpu;
	LttTime timestamp;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sched_process_fork") != 0)
		return FALSE;
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];
	timestamp = lttv_event_get_timestamp(event);
		
	/* Skip Parent PID param */

	/* Child PID */
	child_pid = lttv_event_get_long(event, "child_tid");
	//ts->target_pid = child_pid;

	/* Child TGID */
	
	child_tgid = 0;

	/* Mathieu : it seems like the process might have been scheduled in before the
	 * fork, and, in a rare case, might be the current process. This might happen
	 * in a SMP case where we don't have enough precision on the clocks.
	 *
	 * Test reenabled after precision fixes on time. (Mathieu) */
#if 0 
	zombie_process = lttv_state_find_process(ts, ANY_CPU, child_pid);

	if(unlikely(zombie_process != NULL)) {
		/* Reutilisation of PID. Only now we are sure that the old PID
		 * has been released. FIXME : should know when release_task happens instead.
		 */
		guint num_cpus = ltt_trace_get_num_cpu(ts->parent.t);
		guint i;
		for(i=0; i< num_cpus; i++) {
			g_assert(zombie_process != ts->running_process[i]);
		}

		exit_process(s, zombie_process);
	}
#endif //0
	g_assert(process->pid != child_pid);
	// FIXME : Add this test in the "known state" section
	// g_assert(process->pid == parent_pid);
	child_process = lttv_state_find_process(ts, ANY_CPU, child_pid);
	if(child_process == NULL) {
		child_process = lttv_state_create_process(ts, process, cpu,
				child_pid, child_tgid,
				LTTV_STATE_UNNAMED, &timestamp);
	} else {
		/* The process has already been created :  due to time imprecision between
		 * multiple CPUs : it has been scheduled in before creation. Note that we
		 * shouldn't have this kind of imprecision.
		 *
		 * Simply put a correct parent.
		 */
		g_error("Process %u has been created at [%lu.%09lu] "
				"and inserted at [%lu.%09lu] before \n"
				"fork on cpu %u[%lu.%09lu].\n"
				"Probably an unsynchronized TSC problem on the traced machine.",
				child_pid,
				child_process->creation_time.tv_sec,
				child_process->creation_time.tv_nsec,
				child_process->insertion_time.tv_sec,
				child_process->insertion_time.tv_nsec,
				cpu, timestamp.tv_sec, timestamp.tv_nsec);
		//g_assert(0); /* This is a problematic case : the process has been created
		//                before the fork event */
		child_process->ppid = process->pid;
		child_process->tgid = child_tgid;
	}
	g_assert(child_process->name == LTTV_STATE_UNNAMED);
	child_process->name = process->name;

	return FALSE;
}

#ifdef BABEL_CLEANUP
//NO KTHREAD_CREATE in LTTng 2.0
/* We stamp a newly created process as kernel_thread.
 * The thread should not be running yet. */
static gboolean process_kernel_thread(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	guint pid;
	LttvTraceState *ts = (LttvTraceState*)s->parent.t_context;
	LttvProcessState *process;
	LttvExecutionState *es;

	/* PID */
	pid = (guint)ltt_event_get_long_unsigned(e, lttv_trace_get_hook_field(th, 0));
	//s->parent.target_pid = pid;

	process = lttv_state_find_process_or_create(ts, ANY_CPU, pid,
			&ltt_time_zero);
	if (process->state->s != LTTV_STATE_DEAD) {
		process->execution_stack =
			g_array_set_size(process->execution_stack, 1);
		es = process->state =
			&g_array_index(process->execution_stack, LttvExecutionState, 0);
		es->t = LTTV_STATE_SYSCALL;
	}
	process->type = LTTV_STATE_KERNEL_THREAD;

	return FALSE;
}
#endif
static gboolean process_exit(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	guint pid;
	guint cpu;
	LttvProcessState *process; // = ts->running_process[cpu];

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sched_process_exit") != 0)
		return FALSE;
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];

	pid = lttv_event_get_long(event, "tid");
	//s->parent.target_pid = pid;

	// FIXME : Add this test in the "known state" section
	// g_assert(process->pid == pid);

	process = lttv_state_find_process(ts, ANY_CPU, pid);
	if(likely(process != NULL)) {
		process->state->s = LTTV_STATE_EXIT;
	}
	return FALSE;
}

static gboolean process_free(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	guint cpu;
	guint release_pid;
	LttvProcessState *process;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sched_process_free") != 0)
		return FALSE;
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];

	/* PID of the process to release */
	release_pid = lttv_event_get_long(event, "_tid");
	//s->parent.target_pid = release_pid;

	g_assert(release_pid != 0);

	process = lttv_state_find_process(ts, ANY_CPU, release_pid);
	if(likely(process != NULL))
		exit_process(event, process);
	return FALSE;
//DISABLED
#if 0
	if(likely(process != NULL)) {
		/* release_task is happening at kernel level : we can now safely release
		 * the data structure of the process */
		//This test is fun, though, as it may happen that
		//at time t : CPU 0 : process_free
		//at time t+150ns : CPU 1 : schedule out
		//Clearly due to time imprecision, we disable it. (Mathieu)
		//If this weird case happen, we have no choice but to put the
		//Currently running process on the cpu to 0.
		//I re-enable it following time precision fixes. (Mathieu)
		//Well, in the case where an process is freed by a process on another CPU
		//and still scheduled, it happens that this is the schedchange that will
		//drop the last reference count. Do not free it here!
		guint num_cpus = ltt_trace_get_num_cpu(ts->parent.t);
		guint i;
		for(i=0; i< num_cpus; i++) {
			//g_assert(process != ts->running_process[i]);
			if(process == ts->running_process[i]) {
				//ts->running_process[i] = lttv_state_find_process(ts, i, 0);
				break;
			}
		}
		if(i == num_cpus) /* process is not scheduled */
			exit_process(s, process);
	}

	return FALSE;
#endif //DISABLED
}


static gboolean process_exec(void *hook_data, void *call_data)
{	
	LttvEvent *event;
	LttvTraceState *ts;
	guint cpu;
	//gchar *name;
	LttvProcessState *process;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"sys_execve") != 0)
		return FALSE;
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];

#if 0//how to use a sequence that must be transformed in a string
	/* PID of the process to release */
	guint64 name_len = ltt_event_field_element_number(e,
		lttv_trace_get_hook_field(th, 0));
	//name = ltt_event_get_string(e, lttv_trace_get_hook_field(th, 0));
	LttField *child = ltt_event_field_element_select(e,
		lttv_trace_get_hook_field(th, 0), 0);
	gchar *name_begin =
		(gchar*)(ltt_event_data(e)+ltt_event_field_offset(e, child));
	gchar *null_term_name = g_new(gchar, name_len+1);
	memcpy(null_term_name, name_begin, name_len);
	null_term_name[name_len] = '\0';
	process->name = g_quark_from_string(null_term_name);
#endif //0

	process->name = g_quark_from_string(lttv_event_get_string(event,
								  "filename"));
	//g_free(null_term_name);
	return FALSE;
}
#if 0
	// TODO We only have sys_open, without the FD
	// manage to do somehting better
static gboolean fs_open(void *hook_data, void *call_data)
{
	LttvTracefileState *s = (LttvTracefileState *)call_data;
	LttvTraceState *ts = (LttvTraceState *)s->parent.t_context;
	LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	LttvTraceHook *th = (LttvTraceHook *)hook_data;
	struct marker_field *f;
	guint cpu = s->cpu;
	int fd;
	char *filename;
	LttvProcessState *process = ts->running_process[cpu];

	f = lttv_trace_get_hook_field(th, 0);
	fd = ltt_event_get_int(e, f);

	f = lttv_trace_get_hook_field(th, 1);
	filename = ltt_event_get_string(e, f);

	g_hash_table_insert(process->fds, (gpointer)(long)fd,
			(gpointer)(unsigned long)g_quark_from_string(filename));

	return FALSE;
}
#endif
static void print_stack(LttvProcessState *process)
{
	LttvExecutionState *es;
	int i;

	g_debug("Execution stack for process %u %s:\n",
			process->pid, g_quark_to_string(process->name));

	for (i = 0; i < process->execution_stack->len; i++) {
		es = &g_array_index(process->execution_stack,
				LttvExecutionState, i);
		g_debug("Depth %d mode %s submode %s status %s\n",
				i, g_quark_to_string(es->t),
				g_quark_to_string(es->n),
				g_quark_to_string(es->s));
	}

}

static void fix_process(gpointer key, gpointer value, gpointer user_data)
{
	LttvProcessState *process;
	LttvExecutionState *es;
	process = (LttvProcessState *)value;
	LttTime *timestamp = (LttTime*)user_data;

	print_stack(process);

	if(process->type == LTTV_STATE_KERNEL_THREAD) {
		es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
		if(es->t == LTTV_STATE_MAYBE_SYSCALL) {
			es->t = LTTV_STATE_SYSCALL;
			es->n = LTTV_STATE_SUBMODE_NONE;
			es->entry = *timestamp;
			es->change = *timestamp;
			es->cum_cpu_time = ltt_time_zero;
			if(es->s == LTTV_STATE_UNNAMED)
				es->s = LTTV_STATE_WAIT;
		}
	} else {
		es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
		if(es->t == LTTV_STATE_MAYBE_USER_MODE) {
			es->t = LTTV_STATE_USER_MODE;
			es->n = LTTV_STATE_SUBMODE_NONE;
			es->entry = *timestamp;
			//g_assert(timestamp->tv_sec != 0);
			es->change = *timestamp;
			es->cum_cpu_time = ltt_time_zero;
			if(es->s == LTTV_STATE_UNNAMED)
				es->s = LTTV_STATE_RUN;

			if(process->execution_stack->len == 1) {
				/* Still in bottom unknown mode, means we either:
				 * - never did a system call
				 * - are scheduled out from user mode.
				 * May be either in user mode, syscall mode, running or waiting.*/
				/* CHECK : we may be tagging syscall mode when being user mode
				 * (should be fixed now) */
				if (es->s == LTTV_STATE_WAIT_CPU) {
					/* nothing to do: scheduled out from userspace */
				} else {
					process->execution_stack =
							g_array_set_size(process->execution_stack, 2);
					es = process->state = &g_array_index(process->execution_stack,
							LttvExecutionState, 1);
					es->t = LTTV_STATE_SYSCALL;
					es->n = LTTV_STATE_SUBMODE_NONE;
					es->entry = *timestamp;
					//g_assert(timestamp->tv_sec != 0);
					es->change = *timestamp;
					es->cum_cpu_time = ltt_time_zero;
					if(es->s == LTTV_STATE_WAIT_FORK)
						es->s = LTTV_STATE_WAIT;
				}
			}
		}
	}
}

static gboolean statedump_end(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	LttTime timestamp;
	//LttEvent *e = ltt_tracefile_get_event(s->parent.tf);
	//LttvTraceHook *th = (LttvTraceHook *)hook_data;
	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"lttng_statedump_end") != 0)
		return FALSE;

	ts = event->state;
	timestamp = lttv_event_get_timestamp(event);

	/* For all processes */
		/* if kernel thread, if stack[0] is unknown, set to syscall mode, wait */
		/* else, if stack[0] is unknown, set to user mode, running */

	g_hash_table_foreach(ts->processes, fix_process, &timestamp);

	return FALSE;
}

static gboolean enum_process_state(void *hook_data, void *call_data)
{
	LttvEvent *event;
	LttvTraceState *ts;
	LttTime timestamp;
	guint parent_pid;
	guint pid;
	guint tgid;
	gchar * command;
	guint cpu;
	LttvProcessState *parent_process;
	LttvProcessState *process;

	guint type;
	LttvExecutionState *es;
	guint i, nb_cpus;

	event = (LttvEvent *) call_data;
	if (strcmp(lttv_traceset_get_name_from_event(event),
			"lttng_statedump_process_state") != 0)
		return FALSE;
	cpu = lttv_traceset_get_cpuid_from_event(event);
	ts = event->state;
	process = ts->running_process[cpu];
	timestamp = lttv_event_get_timestamp(event);

	/* PID */
	pid = lttv_event_get_long(event, "tid");
	//s->parent.target_pid = pid;

	/* Parent PID */
	parent_pid = lttv_event_get_long(event, "ppid");

	/* Command name */
	command = lttv_event_get_string(event, "name");

	/* type */
	
	type = lttv_event_get_long(event, "type");

	//FIXME: type is rarely used, enum must match possible types.

	/* Skip mode 4th param */

	/* Skip submode 5th param */

	/* Skip status 6th param */
	/* TGID */
	tgid = lttv_event_get_long(event, "pid");
	
	if(pid == 0) {
		nb_cpus = lttv_trace_get_num_cpu(ts->trace);
		for(i=0; i<nb_cpus; i++) {
			process = lttv_state_find_process(ts, i, pid);
			g_assert(process != NULL);

			process->ppid = parent_pid;
			process->tgid = tgid;
			process->name = g_quark_from_string(command);
			es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
			process->type = LTTV_STATE_KERNEL_THREAD;
		}

	} else {
		/* The process might exist if a process was forked while performing the
		 * state dump. */
		process = lttv_state_find_process(ts, ANY_CPU, pid);
		if(process == NULL) {
			parent_process = lttv_state_find_process(ts, ANY_CPU, parent_pid);
			process = lttv_state_create_process(ts, parent_process, cpu,
					pid, tgid, g_quark_from_string(command),
					&timestamp);

			/* Keep the stack bottom : a running user mode */
			/* Disabled because of inconsistencies in the current statedump states. */
			//if(type == LTTV_STATE_KERNEL_THREAD) {
			if(type == 1) {
				/* Only keep the bottom
				 * FIXME Kernel thread : can be in syscall or interrupt or trap. */
				/* Will cause expected trap when in fact being syscall (even after end of
				 * statedump event)
				 * Will cause expected interrupt when being syscall. (only before end of
				 * statedump event) */
				// This will cause a "popping last state on stack, ignoring it."
				process->execution_stack = g_array_set_size(process->execution_stack, 1);
				es = process->state = &g_array_index(process->execution_stack,
						LttvExecutionState, 0);
				process->type = LTTV_STATE_KERNEL_THREAD;
				es->t = LTTV_STATE_MAYBE_SYSCALL;
				es->s = LTTV_STATE_UNNAMED;
				es->n = LTTV_STATE_SUBMODE_UNKNOWN;
				//es->s = status;
				//es->n = submode;
			} else {
				/* User space process :
				 * bottom : user mode
				 * either currently running or scheduled out.
				 * can be scheduled out because interrupted in (user mode or in syscall)
				 * or because of an explicit call to the scheduler in syscall. Note that
				 * the scheduler call comes after the irq_exit, so never in interrupt
				 * context. */
				// temp workaround : set size to 1 : only have user mode bottom of stack.
				// will cause g_info message of expected syscall mode when in fact being
				// in user mode. Can also cause expected trap when in fact being user
				// mode in the event of a page fault reenabling interrupts in the handler.
				// Expected syscall and trap can also happen after the end of statedump
				// This will cause a "popping last state on stack, ignoring it."
				process->execution_stack = g_array_set_size(process->execution_stack, 1);
				es = process->state = &g_array_index(process->execution_stack,
						LttvExecutionState, 0);
				es->t = LTTV_STATE_MAYBE_USER_MODE;
				es->s = LTTV_STATE_UNNAMED;
				es->n = LTTV_STATE_SUBMODE_UNKNOWN;
				//es->s = status;
				//es->n = submode;
			}
	#if 0
			/* UNKNOWN STATE */
			{
				es = process->state = &g_array_index(process->execution_stack,
						LttvExecutionState, 1);
				es->t = LTTV_STATE_MODE_UNKNOWN;
				es->s = LTTV_STATE_UNNAMED;
				es->n = LTTV_STATE_SUBMODE_UNKNOWN;
			}
	#endif //0
		} else {
			/* The process has already been created :
			 * Probably was forked while dumping the process state or
			 * was simply scheduled in prior to get the state dump event.
			 */
			process->ppid = parent_pid;
			process->tgid = tgid;
			process->name = g_quark_from_string(command);
			process->type = type;
			es = &g_array_index(process->execution_stack, LttvExecutionState, 0);
#if 0
			if(es->t == LTTV_STATE_MODE_UNKNOWN) {
				if(type == LTTV_STATE_KERNEL_THREAD)
					es->t = LTTV_STATE_SYSCALL;
				else
					es->t = LTTV_STATE_USER_MODE;
			}
#endif //0
			/* Don't mess around with the stack, it will eventually become
			 * ok after the end of state dump. */
		}
	}

	return FALSE;
}



gint lttv_state_hook_add_event_hooks(void *hook_data, void *call_data)
{
	LttvTraceset *traceset = (LttvTraceset *)(call_data);

	lttv_state_add_event_hooks(traceset);

	return 0;
}

void lttv_state_add_event_hooks(LttvTraceset *traceset)
{
	LttvHooks*event_hook;

	//Use traceset hooks 
	event_hook = lttv_traceset_get_hooks(traceset);
	g_assert(event_hook);

	lttv_hooks_add(event_hook,syscall_entry , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,syscall_exit , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,irq_entry , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,irq_exit , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,soft_irq_raise , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,soft_irq_entry , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,soft_irq_exit , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,schedchange , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,sched_try_wakeup , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,process_exit , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,process_free , NULL, LTTV_PRIO_STATE);
	lttv_hooks_add(event_hook,process_fork , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,process_exec , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,enum_process_state , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,statedump_end , NULL, LTTV_PRIO_STATE);	
	lttv_hooks_add(event_hook,enum_interrupt , NULL, LTTV_PRIO_STATE);	

}

gint lttv_state_hook_remove_event_hooks(void *hook_data, void *call_data)
{

	LttvTraceset *traceset = (LttvTraceset*)(call_data);

	lttv_state_remove_event_hooks(traceset);

	return 0;
}

void lttv_state_remove_event_hooks(LttvTraceset *traceset)
{

	LttvHooks*event_hook;

	event_hook = lttv_traceset_get_hooks(traceset);

	g_assert(event_hook);


	lttv_hooks_remove(event_hook,syscall_entry );
	lttv_hooks_remove(event_hook,syscall_exit);
	lttv_hooks_remove(event_hook,irq_entry );
	lttv_hooks_remove(event_hook,irq_exit );
	lttv_hooks_remove(event_hook,soft_irq_raise);
	lttv_hooks_remove(event_hook,soft_irq_entry);
	lttv_hooks_remove(event_hook,soft_irq_exit);
	lttv_hooks_remove(event_hook,schedchange);
	lttv_hooks_remove(event_hook,sched_try_wakeup);
	lttv_hooks_remove(event_hook,process_exit);
	lttv_hooks_remove(event_hook,process_free);
	lttv_hooks_remove(event_hook,process_exec);
	lttv_hooks_remove(event_hook,enum_process_state);
	lttv_hooks_remove(event_hook,statedump_end);
	lttv_hooks_remove(event_hook,enum_interrupt);
}


static gboolean state_save_event_hook(void *hook_data, void *call_data)
{
	guint *event_count = (guint*)hook_data;

	/* Only save at LTTV_STATE_SAVE_INTERVAL */
	if(likely((*event_count)++ < LTTV_STATE_SAVE_INTERVAL))
		return FALSE;
	else
		*event_count = 0;

	LttvEvent *event = (LttvEvent *)call_data;

	LttvTraceset *traceset = lttv_trace_get_traceset(event->state->trace);

	LttvAttribute *saved_states_tree, *saved_state_tree;

	LttvAttributeValue value;
	LttTime currentTime;
	guint i;
	currentTime = lttv_event_get_timestamp(event);
	int nb_trace = lttv_traceset_number(traceset);
	for(i = 0 ; i < nb_trace ; i++) {

		LttvTrace *trace = lttv_traceset_get(traceset, i);
		LttvTraceState *tstate = trace->state;
		saved_states_tree = lttv_attribute_find_subdir(trace->a,
							       LTTV_STATE_SAVED_STATES);

		saved_state_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
		value = lttv_attribute_add(saved_states_tree,
					   lttv_attribute_get_number(saved_states_tree), LTTV_GOBJECT);
		*(value.v_gobject) = (GObject *)saved_state_tree;


		value = lttv_attribute_add(saved_state_tree, LTTV_STATE_TIME, LTTV_TIME);
		*(value.v_time) = currentTime;
		
		lttv_state_save(tstate, saved_state_tree);
		g_debug("Saving state at time %lu.%lu", currentTime.tv_sec,
			currentTime.tv_nsec);

		*(tstate->max_time_state_recomputed_in_seek) = currentTime;
	}
	return FALSE;
}

static gboolean state_save_after_trace_hook(void *hook_data, void *call_data)
{
#warning "Would we move max_time to traceset"
	LttvTrace *trace = (LttvTrace *)(call_data);
	LttvTraceState *tcs = trace->state;
	TimeInterval time_span = lttv_traceset_get_time_span_real(lttv_trace_get_traceset(trace));

	*(tcs->max_time_state_recomputed_in_seek) = time_span.end_time;

	return FALSE;
}
#ifdef BABEL_CLEANUP
guint lttv_state_current_cpu(LttvTracefileState *tfs)
{
	return tfs->cpu;
}

#endif //BABEL_CLEANUP

#if 0
static gboolean block_start(void *hook_data, void *call_data)
{
	LttvTracefileState *self = (LttvTracefileState *)call_data;

	LttvTracefileState *tfcs;

	LttvTraceState *tcs = (LttvTraceState *)(self->parent.t_context);

	LttEventPosition *ep;

	guint i, nb_block, nb_event, nb_tracefile;

	LttTracefile *tf;

	LttvAttribute *saved_states_tree, *saved_state_tree;

	LttvAttributeValue value;

	ep = ltt_event_position_new();

	nb_tracefile = tcs->parent.tracefiles->len;

	/* Count the number of events added since the last block end in any
		 tracefile. */

	for(i = 0 ; i < nb_tracefile ; i++) {
		tfcs =
				LTTV_TRACEFILE_STATE(&g_array_index(tcs->parent.tracefiles,
				LttvTracefileContext, i));
		ltt_event_position(tfcs->parent.e, ep);
		ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
		tcs->nb_event += nb_event - tfcs->saved_position;
		tfcs->saved_position = nb_event;
	}
	g_free(ep);

	if(tcs->nb_event >= tcs->save_interval) {
		saved_states_tree = lttv_attribute_find_subdir(tcs->parent.t_a,
				LTTV_STATE_SAVED_STATES);
		saved_state_tree = g_object_new(LTTV_ATTRIBUTE_TYPE, NULL);
		value = lttv_attribute_add(saved_states_tree,
				lttv_attribute_get_number(saved_states_tree), LTTV_GOBJECT);
		*(value.v_gobject) = (GObject *)saved_state_tree;
		value = lttv_attribute_add(saved_state_tree, LTTV_STATE_TIME, LTTV_TIME);
		*(value.v_time) = self->parent.timestamp;
		lttv_state_save(tcs, saved_state_tree);
		tcs->nb_event = 0;
		g_debug("Saving state at time %lu.%lu", self->parent.timestamp.tv_sec,
				self->parent.timestamp.tv_nsec);
	}
	*(tcs->max_time_state_recomputed_in_seek) = self->parent.timestamp;
	return FALSE;
}
#endif //0

#if 0
static gboolean block_end(void *hook_data, void *call_data)
{
	LttvTracefileState *self = (LttvTracefileState *)call_data;

	LttvTraceState *tcs = (LttvTraceState *)(self->parent.t_context);

	LttTracefile *tf;

	LttEventPosition *ep;

	guint nb_block, nb_event;

	ep = ltt_event_position_new();
	ltt_event_position(self->parent.e, ep);
	ltt_event_position_get(ep, &nb_block, &nb_event, &tf);
	tcs->nb_event += nb_event - self->saved_position + 1;
	self->saved_position = 0;
	*(tcs->max_time_state_recomputed_in_seek) = self->parent.timestamp;
	g_free(ep);

	return FALSE;
}
#endif //0
#if 0
void lttv_state_save_add_event_hooks(LttvTracesetState *self)
{
	LttvTraceset *traceset = self->parent.ts;

	guint i, j, nb_trace, nb_tracefile;

	LttvTraceState *ts;

	LttvTracefileState *tfs;

	LttvTraceHook hook_start, hook_end;

	nb_trace = lttv_traceset_number(traceset);
	for(i = 0 ; i < nb_trace ; i++) {
		ts = (LttvTraceState *)self->parent.traces[i];

		lttv_trace_find_hook(ts->parent.t, "core","block_start",NULL,
				NULL, NULL, block_start, &hook_start);
		lttv_trace_find_hook(ts->parent.t, "core","block_end",NULL,
				NULL, NULL, block_end, &hook_end);

		nb_tracefile = ts->parent.tracefiles->len;

		for(j = 0 ; j < nb_tracefile ; j++) {
			tfs =
					LTTV_TRACEFILE_STATE(&g_array_index(ts->parent.tracefiles,
					LttvTracefileContext, j));
			lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id,
					hook_start.id), hook_start.h, NULL, LTTV_PRIO_STATE);
			lttv_hooks_add(lttv_hooks_by_id_find(tfs->parent.event_by_id,
					hook_end.id), hook_end.h, NULL, LTTV_PRIO_STATE);
		}
	}
}
#endif //0

void lttv_state_save_add_event_hooks(LttvTraceset *traceset)
{

	if(!traceset->has_precomputed_states) {
		guint *event_count = g_new(guint, 1);
		
		*event_count = 0;
		lttv_hooks_add(traceset->event_hooks,
			       state_save_event_hook,
			       event_count,
			       LTTV_PRIO_STATE);
	
	lttv_process_traceset_begin(traceset,
				    NULL, NULL, NULL);
	}

}

gint lttv_state_save_hook_add_event_hooks(void *hook_data, void *call_data)
{
	LttvTraceset *ts = (LttvTraceset*)(call_data);

	lttv_state_save_add_event_hooks(ts);

	return 0;
}


void lttv_state_save_remove_event_hooks(LttvTraceset *traceset)
{

	LttvHooks *after_trace = lttv_hooks_new();
	guint *event_count = NULL;

	lttv_hooks_add(after_trace,
			state_save_after_trace_hook,
			NULL,
			LTTV_PRIO_STATE);


	lttv_process_traceset_end(traceset,
			NULL, after_trace, NULL);
 
	lttv_hooks_destroy(after_trace);

	//nb_trace = lttv_traceset_number(traceset);

	event_count = lttv_hooks_remove(traceset->event_hooks,
					state_save_event_hook);
	
	if(event_count) g_free(event_count);

}

gint lttv_state_save_hook_remove_event_hooks(void *hook_data, void *call_data)
{
	LttvTraceset *ts = (LttvTraceset*)(call_data);

	lttv_state_save_remove_event_hooks(ts);

	return 0;
}


void lttv_state_traceset_seek_time(LttvTraceset *traceset, LttTime t)
{
      lttv_state_traceset_seek_time_closest(traceset,
          t);
      lttv_process_traceset_middle(traceset, t, G_MAXUINT,
                                   NULL);
}

void lttv_state_traceset_seek_position(LttvTraceset *traceset, LttvTracesetPosition *position)
{
	LttTime t = lttv_traceset_position_get_time(position);
	
	lttv_state_traceset_seek_time_closest(traceset,
					      t);
	lttv_process_traceset_middle(traceset, 
				     ltt_time_infinite, 
				     G_MAXUINT,
				     position);
}

void lttv_state_traceset_seek_time_closest(LttvTraceset *traceset, LttTime t)
{
	guint i, nb_trace;

	int min_pos, mid_pos, max_pos;

	guint resto_start = 0;
	guint resto_at = 0;

	LttvAttributeValue value;

	LttvAttributeType type;

	LttvAttributeName name;

	gboolean is_named;

	LttvAttribute *saved_states_tree, *saved_state_tree, *closest_tree = NULL;
	
	LttTime closest_tree_time, restored_time;
	guint first_restored_time = 1;

	//g_tree_destroy(self->parent.pqueue);
	//self->parent.pqueue = g_tree_new(compare_tracefile);

	g_debug("Entering seek_time_closest for time %lu.%lu", t.tv_sec, t.tv_nsec);

	nb_trace = lttv_traceset_number(traceset);
	for(i = 0 ; i < nb_trace ; i++) {

		LttvTrace *trace = lttv_traceset_get(traceset, i);
		LttvTraceState *tstate = trace->state;

		if(ltt_time_compare(t, *(tstate->max_time_state_recomputed_in_seek)) < 0) {
			saved_states_tree = lttv_attribute_find_subdir(trace->a,
					LTTV_STATE_SAVED_STATES);
			min_pos = -1;

			if(saved_states_tree) {
				max_pos = lttv_attribute_get_number(saved_states_tree) - 1;
				mid_pos = max_pos / 2;
				while(min_pos < max_pos) {
					type = lttv_attribute_get(saved_states_tree, mid_pos,
							&name, &value, &is_named);
					g_assert(type == LTTV_GOBJECT);
					saved_state_tree = *((LttvAttribute **)(value.v_gobject));
					type = lttv_attribute_get_by_name(saved_state_tree,
							LTTV_STATE_TIME, &value);
					g_assert(type == LTTV_TIME);
					if(ltt_time_compare(*(value.v_time), t) < 0) {
						min_pos = mid_pos;
						closest_tree = saved_state_tree;
						closest_tree_time = *(value.v_time);
					}
					else max_pos = mid_pos - 1;

					mid_pos = (min_pos + max_pos + 1) / 2;
				}
			}

			/* restore the closest earlier saved state */
			if(min_pos != -1) {
				if(first_restored_time || (ltt_time_compare(restored_time, closest_tree_time) == 0)) {
					first_restored_time = 0;
					lttv_state_restore(tstate, closest_tree);
					
					restored_time = closest_tree_time;
				} else {
					g_debug("State: restored time mismatch between traces");
					resto_start = 1;
					break;
				}
				
			}

			/* There is no saved state, yet we want to have it. Restart at T0 */
			else {
				resto_start = 1;
				break;
			}
		}
		/* We want to seek quickly without restoring/updating the state */
		else {
			resto_at = 1;
			break;
		}
	}

	if(resto_start || resto_at) {
		// Restore init state and seek so
		for(i = 0 ; i < nb_trace ; i++) {

			LttvTrace *trace = lttv_traceset_get(traceset, i);
			LttvTraceState *tstate = trace->state;
			
			restore_init_state(tstate);	
		}

		// If t > max saved state
		if(resto_at) {
			lttv_process_traceset_seek_time(traceset, t);
		} else if (resto_start) {
			// If no saved state
			lttv_process_traceset_seek_time(traceset, ltt_time_zero);
		} else {
			g_assert(FALSE);
		}
		g_info("NOT Calling restore");

	} else {
		// Seek at checkpoint		
		lttv_process_traceset_seek_time(traceset, restored_time);
					

	}


}

#ifdef BABEL_CLEANUP
static void traceset_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void traceset_state_finalize (LttvTracesetState *self)
{
	G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACESET_CONTEXT_TYPE))->
			finalize(G_OBJECT(self));
}


static void traceset_state_class_init (LttvTracesetContextClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = (void (*)(GObject *self)) traceset_state_finalize;
	klass->init = (void (*)(LttvTracesetContext *self, LttvTraceset *ts))init;
	klass->fini = (void (*)(LttvTracesetContext *self))fini;
	klass->new_traceset_context = new_traceset_context;
	klass->new_trace_context = new_trace_context;
	klass->new_tracefile_context = new_tracefile_context;
}


GType lttv_traceset_state_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (LttvTracesetStateClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) traceset_state_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LttvTracesetState),
			0,      /* n_preallocs */
			(GInstanceInitFunc) traceset_state_instance_init,    /* instance_init */
			NULL    /* value handling */
		};

		type = g_type_register_static (LTTV_TRACESET_CONTEXT_TYPE, "LttvTracesetStateType",
				&info, 0);
	}
	return type;
}

#endif
#if BABEL_CLEANUP
static void trace_state_instance_init (GTypeInstance *instance, gpointer g_class)
{
}


static void trace_state_finalize (LttvTraceState *self)
{
	G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACE_CONTEXT_TYPE))->
			finalize(G_OBJECT(self));
}


static void trace_state_class_init (LttvTraceStateClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = (void (*)(GObject *self)) trace_state_finalize;
	klass->state_save = state_save;
	klass->state_restore = state_restore;
	klass->state_saved_free = state_saved_free;
}


GType lttv_trace_state_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (LttvTraceStateClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) trace_state_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LttvTraceState),
			0,      /* n_preallocs */
			(GInstanceInitFunc) trace_state_instance_init,    /* instance_init */
			NULL    /* value handling */
		};

		type = g_type_register_static (LTTV_TRACE_CONTEXT_TYPE,
				"LttvTraceStateType", &info, 0);
	}
	return type;
}


static void tracefile_state_instance_init (GTypeInstance *instance,
		gpointer g_class)
{
}


static void tracefile_state_finalize (LttvTracefileState *self)
{
	G_OBJECT_CLASS(g_type_class_peek(LTTV_TRACEFILE_CONTEXT_TYPE))->
			finalize(G_OBJECT(self));
}


static void tracefile_state_class_init (LttvTracefileStateClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

	gobject_class->finalize = (void (*)(GObject *self)) tracefile_state_finalize;
}


GType lttv_tracefile_state_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (LttvTracefileStateClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			(GClassInitFunc) tracefile_state_class_init,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (LttvTracefileState),
			0,      /* n_preallocs */
			(GInstanceInitFunc) tracefile_state_instance_init,    /* instance_init */
			NULL    /* value handling */
		};

		type = g_type_register_static (LTTV_TRACEFILE_CONTEXT_TYPE,
				"LttvTracefileStateType", &info, 0);
	}
	return type;
}

#endif
static void module_init(void)
{
	LTTV_STATE_UNNAMED = g_quark_from_string("");
	LTTV_STATE_MODE_UNKNOWN = g_quark_from_string("MODE_UNKNOWN");
	LTTV_STATE_USER_MODE = g_quark_from_string("USER_MODE");
	LTTV_STATE_MAYBE_USER_MODE = g_quark_from_string("MAYBE_USER_MODE");
	LTTV_STATE_SYSCALL = g_quark_from_string("SYSCALL");
	LTTV_STATE_MAYBE_SYSCALL = g_quark_from_string("MAYBE_SYSCALL");
	LTTV_STATE_TRAP = g_quark_from_string("TRAP");
	LTTV_STATE_MAYBE_TRAP = g_quark_from_string("MAYBE_TRAP");
	LTTV_STATE_IRQ = g_quark_from_string("IRQ");
	LTTV_STATE_SOFT_IRQ = g_quark_from_string("SOFTIRQ");
	LTTV_STATE_SUBMODE_UNKNOWN = g_quark_from_string("UNKNOWN");
	LTTV_STATE_SUBMODE_NONE = g_quark_from_string("NONE");
	LTTV_STATE_WAIT_FORK = g_quark_from_string("WAIT_FORK");
	LTTV_STATE_WAIT_CPU = g_quark_from_string("WAIT_CPU");
	LTTV_STATE_EXIT = g_quark_from_string("EXIT");
	LTTV_STATE_ZOMBIE = g_quark_from_string("ZOMBIE");
	LTTV_STATE_WAIT = g_quark_from_string("WAIT");
	LTTV_STATE_RUN = g_quark_from_string("RUN");
	LTTV_STATE_DEAD = g_quark_from_string("DEAD");
	LTTV_STATE_USER_THREAD = g_quark_from_string("USER_THREAD");
	LTTV_STATE_KERNEL_THREAD = g_quark_from_string("KERNEL_THREAD");
	LTTV_STATE_TRACEFILES = g_quark_from_string("tracefiles");
	LTTV_STATE_PROCESSES = g_quark_from_string("processes");
	LTTV_STATE_PROCESS = g_quark_from_string("process");
	LTTV_STATE_RUNNING_PROCESS = g_quark_from_string("running_process");
	LTTV_STATE_POSITION = g_quark_from_string("position");
	LTTV_STATE_SAVED_STATES = g_quark_from_string("saved states");
	LTTV_STATE_SAVED_STATES_TIME = g_quark_from_string("saved states time");
	LTTV_STATE_TIME = g_quark_from_string("time");
	LTTV_STATE_HOOKS = g_quark_from_string("saved state hooks");
	LTTV_STATE_NAME_TABLES = g_quark_from_string("name tables");
	LTTV_STATE_TRACE_STATE_USE_COUNT =
			g_quark_from_string("trace_state_use_count");
	LTTV_STATE_RESOURCE_CPUS = g_quark_from_string("cpu resource states");
	LTTV_STATE_RESOURCE_CPUS = g_quark_from_string("cpu count");
	LTTV_STATE_RESOURCE_IRQS = g_quark_from_string("irq resource states");
	LTTV_STATE_RESOURCE_SOFT_IRQS = g_quark_from_string("soft irq resource states");
	LTTV_STATE_RESOURCE_TRAPS = g_quark_from_string("trap resource states");
	LTTV_STATE_RESOURCE_BLKDEVS = g_quark_from_string("blkdevs resource states");

	LTT_CHANNEL_FD_STATE         = g_quark_from_string("fd_state");
	LTT_CHANNEL_GLOBAL_STATE     = g_quark_from_string("global_state");
	LTT_CHANNEL_IRQ_STATE        = g_quark_from_string("irq_state");
	LTT_CHANNEL_MODULE_STATE     = g_quark_from_string("module_state");
	LTT_CHANNEL_NETIF_STATE      = g_quark_from_string("netif_state");
	LTT_CHANNEL_SOFTIRQ_STATE    = g_quark_from_string("softirq_state");
	LTT_CHANNEL_SWAP_STATE       = g_quark_from_string("swap_state");
	LTT_CHANNEL_SYSCALL_STATE    = g_quark_from_string("syscall_state");
	LTT_CHANNEL_TASK_STATE       = g_quark_from_string("task_state");
	LTT_CHANNEL_VM_STATE         = g_quark_from_string("vm_state");
	LTT_CHANNEL_KPROBE_STATE     = g_quark_from_string("kprobe_state");
	LTT_CHANNEL_FS        = g_quark_from_string("fs");
	LTT_CHANNEL_KERNEL    = g_quark_from_string("kernel");
	LTT_CHANNEL_MM        = g_quark_from_string("mm");
	LTT_CHANNEL_USERSPACE = g_quark_from_string("userspace");
	LTT_CHANNEL_BLOCK     = g_quark_from_string("block");

	LTT_EVENT_SYSCALL_ENTRY = g_quark_from_string("syscall_entry");
	LTT_EVENT_SYSCALL_EXIT  = g_quark_from_string("syscall_exit");
	//LTT_EVENT_TRAP_ENTRY    = g_quark_from_string("trap_entry");
	//LTT_EVENT_TRAP_EXIT     = g_quark_from_string("trap_exit");
	LTT_EVENT_PAGE_FAULT_ENTRY    = g_quark_from_string("page_fault_entry");
	LTT_EVENT_PAGE_FAULT_EXIT     = g_quark_from_string("page_fault_exit");
	LTT_EVENT_PAGE_FAULT_NOSEM_ENTRY = g_quark_from_string("page_fault_nosem_entry");
	LTT_EVENT_PAGE_FAULT_NOSEM_EXIT  = g_quark_from_string("page_fault_nosem_exit");
	LTT_EVENT_IRQ_ENTRY     = g_quark_from_string("irq_entry");
	LTT_EVENT_IRQ_EXIT      = g_quark_from_string("irq_exit");
	LTT_EVENT_SOFT_IRQ_RAISE     = g_quark_from_string("softirq_raise");
	LTT_EVENT_SOFT_IRQ_ENTRY     = g_quark_from_string("softirq_entry");
	LTT_EVENT_SOFT_IRQ_EXIT      = g_quark_from_string("softirq_exit");
	LTT_EVENT_SCHED_SCHEDULE   = g_quark_from_string("sched_schedule");
	LTT_EVENT_SCHED_TRY_WAKEUP = g_quark_from_string("sched_try_wakeup");
	LTT_EVENT_PROCESS_FORK          = g_quark_from_string("process_fork");
	LTT_EVENT_KTHREAD_CREATE = g_quark_from_string("kthread_create");
	LTT_EVENT_PROCESS_EXIT          = g_quark_from_string("process_exit");
	LTT_EVENT_PROCESS_FREE          = g_quark_from_string("process_free");
	LTT_EVENT_EXEC          = g_quark_from_string("exec");
	LTT_EVENT_PROCESS_STATE  = g_quark_from_string("process_state");
	LTT_EVENT_STATEDUMP_END  = g_quark_from_string("statedump_end");
	LTT_EVENT_FUNCTION_ENTRY  = g_quark_from_string("function_entry");
	LTT_EVENT_FUNCTION_EXIT  = g_quark_from_string("function_exit");
	LTT_EVENT_REQUEST_ISSUE = g_quark_from_string("_blk_request_issue");
	LTT_EVENT_REQUEST_COMPLETE = g_quark_from_string("_blk_request_complete");
	LTT_EVENT_LIST_INTERRUPT = g_quark_from_string("interrupt");
	LTT_EVENT_SYS_CALL_TABLE = g_quark_from_string("sys_call_table");
	LTT_EVENT_SOFTIRQ_VEC = g_quark_from_string("softirq_vec");
	LTT_EVENT_KPROBE_TABLE = g_quark_from_string("kprobe_table");
	LTT_EVENT_KPROBE = g_quark_from_string("kprobe");
	LTT_EVENT_OPEN = g_quark_from_string("open");
	LTT_EVENT_READ = g_quark_from_string("read");
	LTT_EVENT_POLL_EVENT = g_quark_from_string("poll_event");

	LTT_FIELD_SYSCALL_ID    = g_quark_from_string("syscall_id");
	LTT_FIELD_TRAP_ID       = g_quark_from_string("trap_id");
	LTT_FIELD_IRQ_ID        = g_quark_from_string("irq_id");
	LTT_FIELD_SOFT_IRQ_ID        = g_quark_from_string("softirq_id");
	LTT_FIELD_PREV_PID            = g_quark_from_string("prev_pid");
	LTT_FIELD_NEXT_PID           = g_quark_from_string("next_pid");
	LTT_FIELD_PREV_STATE     = g_quark_from_string("prev_state");
	LTT_FIELD_PARENT_PID    = g_quark_from_string("parent_pid");
	LTT_FIELD_CHILD_PID     = g_quark_from_string("child_pid");
	LTT_FIELD_PID           = g_quark_from_string("pid");
	LTT_FIELD_TGID          = g_quark_from_string("tgid");
	LTT_FIELD_CHILD_TGID    = g_quark_from_string("child_tgid");
	LTT_FIELD_FILENAME      = g_quark_from_string("filename");
	LTT_FIELD_NAME          = g_quark_from_string("name");
	LTT_FIELD_TYPE          = g_quark_from_string("type");
	LTT_FIELD_MODE          = g_quark_from_string("mode");
	LTT_FIELD_SUBMODE       = g_quark_from_string("submode");
	LTT_FIELD_STATUS        = g_quark_from_string("status");
	LTT_FIELD_THIS_FN       = g_quark_from_string("this_fn");
	LTT_FIELD_CALL_SITE     = g_quark_from_string("call_site");
	LTT_FIELD_MAJOR     = g_quark_from_string("major");
	LTT_FIELD_MINOR     = g_quark_from_string("minor");
	LTT_FIELD_OPERATION     = g_quark_from_string("direction");
	LTT_FIELD_ACTION        = g_quark_from_string("action");
	LTT_FIELD_ID            = g_quark_from_string("id");
	LTT_FIELD_ADDRESS       = g_quark_from_string("address");
	LTT_FIELD_SYMBOL        = g_quark_from_string("symbol");
	LTT_FIELD_IP            = g_quark_from_string("ip");
	LTT_FIELD_FD            = g_quark_from_string("fd");
	LTT_FIELD_STATE         = g_quark_from_string("state");
	LTT_FIELD_CPU_ID        = g_quark_from_string("cpu_id");

	LTTV_CPU_UNKNOWN = g_quark_from_string("unknown");
	LTTV_CPU_IDLE = g_quark_from_string("idle");
	LTTV_CPU_BUSY = g_quark_from_string("busy");
	LTTV_CPU_IRQ = g_quark_from_string("irq");
	LTTV_CPU_SOFT_IRQ = g_quark_from_string("softirq");
	LTTV_CPU_TRAP = g_quark_from_string("trap");

	LTTV_IRQ_UNKNOWN = g_quark_from_string("unknown");
	LTTV_IRQ_IDLE = g_quark_from_string("idle");
	LTTV_IRQ_BUSY = g_quark_from_string("busy");

	LTTV_BDEV_UNKNOWN = g_quark_from_string("unknown");
	LTTV_BDEV_IDLE = g_quark_from_string("idle");
	LTTV_BDEV_BUSY_READING = g_quark_from_string("busy_reading");
	LTTV_BDEV_BUSY_WRITING = g_quark_from_string("busy_writing");
}

static void module_destroy() 
{
}


LTTV_MODULE("state", "State computation", \
		"Update the system state, possibly saving it at intervals", \
		module_init, module_destroy)



