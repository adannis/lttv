


#include <lttv/event.h>
#include <lttv/time.h>
#include <babeltrace/ctf/events.h>

LttTime lttv_event_get_timestamp(LttvEvent *event)
{
  return ltt_time_from_uint64(bt_ctf_get_timestamp(event->bt_event));
}

//TODO ybrosseau find a way to return an error code
unsigned long lttv_event_get_long_unsigned(LttvEvent *event, const char* field)
{
	const struct definition *scope;
	unsigned long timestamp;
	unsigned long data;
	struct bt_ctf_event *ctf_event = event->bt_event;

	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		printf("ERROR: lttv_event_get_long_unsigned - invalid event\n");
		return 0;
	}
	//scope = bt_ctf_get_top_level_scope(ctf_event, BT_STREAM_PACKET_CONTEXT);
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_EVENT_FIELDS);
	if (bt_ctf_field_get_error()) {
		printf("ERROR: lttv_event_get_long_unsigned - cannot get field scope\n");
		return 0;
	}
	data = bt_ctf_get_uint64(bt_ctf_get_field(ctf_event, scope, field));
	if (bt_ctf_field_get_error()) {
		printf("ERROR: lttv_event_get_long_unsigned - cannot get field data %s \n", field);
		return 0;
	} else {
		return data;
	}
}


char* lttv_event_get_string(LttvEvent *event, const char* field)
{
	const struct definition *scope;
	unsigned long timestamp;
	char* data;
	struct bt_ctf_event *ctf_event = event->bt_event;

	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		printf("ERROR: lttv_event_get_string - cannot read timestamps");
		return 0;
	}
	//scope = bt_ctf_get_top_level_scope(ctf_event, BT_STREAM_PACKET_CONTEXT);
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_EVENT_FIELDS);
	if (bt_ctf_field_get_error()) {
		printf("ERROR: lttv_event_get_string - cannot get field scope");
		return 0;
	}
	data = bt_ctf_get_char_array(bt_ctf_get_field(ctf_event, scope, field));
	if (bt_ctf_field_get_error()) {
		// Try get_string if we fail on char_array
		// TODO ybrosseau: properly detect right type
		data = bt_ctf_get_string(bt_ctf_get_field(ctf_event, scope, field));
		if (bt_ctf_field_get_error()) {
			

			printf("ERROR: lttv_event_get_string - cannot get field data %s", field);

			return 0;
		} else {
			return data;
		}
		
	} else {
		return data;
	}
}
long lttv_event_get_long(LttvEvent *event, const char* field)
{
	const struct definition *scope;
	unsigned long timestamp;
	long data;
	struct bt_ctf_event *ctf_event = event->bt_event;

	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		printf("ERROR: lttv_event_get_long - invalid event\n");
		return 0;
	}
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_EVENT_FIELDS);
	if (bt_ctf_field_get_error()) {
		printf("ERROR: lttv_event_get_long - cannot get field scope\n");
		return 0;
	}
	data = bt_ctf_get_int64(bt_ctf_get_field(ctf_event, scope, field));
	if (bt_ctf_field_get_error()) {
		printf("ERROR: lttv_event_get_long - cannot get field data %s \n", field);
		return 0;
	} else {
		return data;
	}
}
/*
unsigned int lttv_event_get_int_unsigned(LttvEvent *event, const char* field)
{
	struct definition *scope;
	unsigned long timestamp;
	char* data;
	struct bt_ctf_event *ctf_event = event->bt_event;

	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		return 0;
	}
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_STREAM_PACKET_CONTEXT);
	if (bt_ctf_field_get_error()) {
		return 0;
	}
	data = bt_ctf_get_char_array(bt_ctf_get_field(ctf_event, scope, field));
	if (bt_ctf_field_get_error()) {
		return 0;
	} else {
		return data;
	}
}
int lttv_event_get_int(LttvEvent *event, const char* field)
{
	struct definition *scope;
	unsigned long timestamp;
	char* data;
	struct bt_ctf_event *ctf_event = event->bt_event;

	timestamp = bt_ctf_get_timestamp(ctf_event);
	if (timestamp == -1ULL) {
		return 0;
	}
	scope = bt_ctf_get_top_level_scope(ctf_event, BT_STREAM_PACKET_CONTEXT);
	if (bt_ctf_field_get_error()) {
		return 0;
	}
	data = bt_ctf_get_char_array(bt_ctf_get_field(ctf_event, scope, field));
	if (bt_ctf_field_get_error()) {
		return 0;
	} else {
		return data;
	}
}
*/
