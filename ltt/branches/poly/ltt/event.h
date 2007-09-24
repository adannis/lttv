#ifndef _LTT_EVENT_H
#define _LTT_EVENT_H

#include <ltt/time.h>
#include <glib.h>
#include <stdint.h>
#include <sys/types.h>
#include <ltt/ltt.h>
#include <endian.h>

/*
 * Structure LttEvent and LttEventPosition must begin with the _exact_ same
 * fields in the exact same order. LttEventPosition is a parent of LttEvent.
 */
struct LttEvent {
	/* Begin of LttEventPosition fields */
	LttTracefile *tracefile;
	unsigned int block;
	unsigned int offset;

	/* Timekeeping */
	uint64_t tsc;		/* Current timestamp counter */
	
	/* End of LttEventPosition fields */
	guint32	timestamp;	/* truncated timestamp */

	guint16 event_id;

	LttTime event_time;

	void *data;		/* event data */
	guint data_size;
	guint event_size;	/* event_size field of the header :
				   used to verify data_size from facility. */
	uint32_t compact_data;

	int count;		/* the number of overflow of cycle count */
	gint64 overflow_nsec;	/* precalculated nsec for overflows */
};

struct LttEventPosition {
	LttTracefile *tracefile;
	unsigned int block;
	unsigned int offset;
	
	/* Timekeeping */
	uint64_t tsc;		 /* Current timestamp counter */
};

static inline guint16 ltt_event_id(struct LttEvent *event)
{
	return event->event_id;
}

static inline LttTime ltt_event_time(struct LttEvent *event)
{
	return event->event_time;
}

/* Obtain the position of the event within the tracefile. This
   is used to seek back to this position later or to seek to another
   position, computed relative to this position. The event position
   structure is opaque and contains several fields, only two
   of which are user accessible: block number and event index
   within the block. */

void ltt_event_position(LttEvent *e, LttEventPosition *ep);

LttEventPosition * ltt_event_position_new();

void ltt_event_position_get(LttEventPosition *ep, LttTracefile **tf,
        guint *block, guint *offset, guint64 *tsc);

void ltt_event_position_set(LttEventPosition *ep, LttTracefile *tf,
        guint block, guint offset, guint64 tsc);

gint ltt_event_position_compare(const LttEventPosition *ep1,
                                const LttEventPosition *ep2);

void ltt_event_position_copy(LttEventPosition *dest,
                             const LttEventPosition *src);

LttTracefile *ltt_event_position_tracefile(LttEventPosition *ep);

#endif //_LTT_EVENT_H
