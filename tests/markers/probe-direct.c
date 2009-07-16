/* probe.c
 *
 * Loads a function at a marker call site.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/linkage.h>


int value;
void *ptr;

/* function to install */
#define DO_MARK1_FORMAT "%d %p"
asmlinkage void do_mark1(const char *format, int pavalue, void *paptr)
{
	value = pavalue;
	ptr = paptr;
}

int init_module(void)
{
	int result;
	result = marker_set_probe("subsys_mark1", DO_MARK1_FORMAT,
			(marker_probe_func*)do_mark1);
	if(!result) goto end;

	return 0;

end:
	marker_remove_probe((marker_probe_func*)do_mark1);
	return -EPERM;
}

void cleanup_module(void)
{
	marker_remove_probe((marker_probe_func*)do_mark1);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Probe");
