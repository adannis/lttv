/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2005 Mathieu Desnoyers
 *
 * Complete rewrite from the original version made by XangXiu Yang.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <glib.h>
#include <malloc.h>
#include <sys/mman.h>

// For realpath
#include <limits.h>
#include <stdlib.h>


#include "parser.h"
#include <ltt/ltt.h>
#include "ltt-private.h"
#include <ltt/trace.h>
#include <ltt/facility.h>
#include <ltt/event.h>
#include <ltt/type.h>
#include <ltt/ltt-types.h>


/* Facility names used in this file */

GQuark LTT_FACILITY_NAME_HEARTBEAT,
       LTT_EVENT_NAME_HEARTBEAT;

#ifndef g_open
#define g_open open
#endif


#define __UNUSED__ __attribute__((__unused__))

#define g_info(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_INFO, format)
#define g_debug(format...) g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, format)

#define g_close close

/* obtain the time of an event */

static inline LttTime getEventTime(LttTracefile * tf);


/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void setFieldsOffset(LttTracefile *tf,LttEventType *evT,void *evD,LttTrace *t);

/* get the size of the field type according to the archtecture's
   size and endian type(info of the archecture) */
static inline gint getFieldtypeSize(LttTracefile * tf,
         LttEventType * evT, gint offsetRoot,
		     gint offsetParent, LttField *fld, void *evD, LttTrace* t);

/* map a fixed size or a block information from the file (fd) */
int map_block(LttTracefile * tf, unsigned int block_num);

/* calculate cycles per nsec for current block */
void getCyclePerNsec(LttTracefile * t);

/* go to the next event */
static int ltt_seek_next_event(LttTracefile *tf);

/* Functions to parse system.xml file (using glib xml parser) */
static void parser_start_element (GMarkupParseContext  __UNUSED__ *context,
				  const gchar          *element_name,
				  const gchar         **attribute_names,
				  const gchar         **attribute_values,
				  gpointer              user_data,
				  GError              **error)
{
  int i=0;
  LttSystemDescription* des = (LttSystemDescription* )user_data;
  if(strcmp("system", element_name)){
    *error = g_error_new(G_MARKUP_ERROR,
                         G_LOG_LEVEL_WARNING,
                         "This is not system.xml file");
    return;
  }
  
  while(attribute_names[i]){
    if(strcmp("node_name", attribute_names[i])==0){
	des->node_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("domainname", attribute_names[i])==0){
	des->domain_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("cpu", attribute_names[i])==0){
	des->nb_cpu = atoi(attribute_values[i]);      
    }else if(strcmp("arch_size", attribute_names[i])==0){
	if(strcmp(attribute_values[i],"LP32") == 0) des->size = LTT_LP32;
	else if(strcmp(attribute_values[i],"ILP32") == 0) des->size = LTT_ILP32;
	else if(strcmp(attribute_values[i],"LP64") == 0) des->size = LTT_LP64;
	else if(strcmp(attribute_values[i],"ILP64") == 0) des->size = LTT_ILP64;
	else if(strcmp(attribute_values[i],"UNKNOWN") == 0) des->size = LTT_UNKNOWN;
    }else if(strcmp("endian", attribute_names[i])==0){
	if(strcmp(attribute_values[i],"LITTLE_ENDIAN") == 0)
	  des->endian = LTT_LITTLE_ENDIAN;
	else if(strcmp(attribute_values[i],"BIG_ENDIAN") == 0) 
	  des->endian = LTT_BIG_ENDIAN;
    }else if(strcmp("kernel_name", attribute_names[i])==0){
	des->kernel_name = g_strdup(attribute_values[i]);      
    }else if(strcmp("kernel_release", attribute_names[i])==0){
	des->kernel_release = g_strdup(attribute_values[i]);      
    }else if(strcmp("kernel_version", attribute_names[i])==0){
	des->kernel_version = g_strdup(attribute_values[i]);      
    }else if(strcmp("machine", attribute_names[i])==0){
	des->machine = g_strdup(attribute_values[i]);      
    }else if(strcmp("processor", attribute_names[i])==0){
	des->processor = g_strdup(attribute_values[i]);      
    }else if(strcmp("hardware_platform", attribute_names[i])==0){
	des->hardware_platform = g_strdup(attribute_values[i]);      
    }else if(strcmp("operating_system", attribute_names[i])==0){
	des->operating_system = g_strdup(attribute_values[i]);      
    }else if(strcmp("ltt_major_version", attribute_names[i])==0){
	des->ltt_major_version = atoi(attribute_values[i]);      
    }else if(strcmp("ltt_minor_version", attribute_names[i])==0){
	des->ltt_minor_version = atoi(attribute_values[i]);      
    }else if(strcmp("ltt_block_size", attribute_names[i])==0){
	des->ltt_block_size = atoi(attribute_values[i]);      
    }else{
      *error = g_error_new(G_MARKUP_ERROR,
                           G_LOG_LEVEL_WARNING,
                           "Not a valid attribute");
      return;      
    }
    i++;
  }
}

static void  parser_characters   (GMarkupParseContext __UNUSED__ *context,
				  const gchar          *text,
				  gsize __UNUSED__      text_len,
				  gpointer              user_data,
				  GError __UNUSED__     **error)
{
  LttSystemDescription* des = (LttSystemDescription* )user_data;
  des->description = g_strdup(text);
}


/*****************************************************************************
 *Function name
 *    ltt_tracefile_open : open a trace file, construct a LttTracefile
 *Input params
 *    t                  : the trace containing the tracefile
 *    fileName           : path name of the trace file
 *    tf                 : the tracefile structure
 *Return value
 *                       : 0 for success, -1 otherwise.
 ****************************************************************************/ 

int ltt_tracefile_open(LttTrace *t, gchar * fileName, LttTracefile *tf)
{
  struct stat    lTDFStat;    /* Trace data file status */
  struct ltt_block_start_header *header;

  //open the file
  tf->name = g_quark_from_string(fileName);
  tf->trace = t;
  tf->fd = g_open(fileName, O_RDONLY, 0);
  if(tf->fd < 0){
    g_warning("Unable to open input data file %s\n", fileName);
    goto end;
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_warning("Unable to get the status of the input data file %s\n", fileName);
    goto close_file;
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < (off_t)(sizeof(BlockStart))){
    g_print("The input data file %s does not contain a trace\n", fileName);
    goto close_file;
  }
  
  /* Temporarily map the buffer start header to get trace information */
  /* Multiple of pages aligned head */
  tf->buffer.head = mmap(0, sizeof(struct ltt_block_start_header), PROT_READ, 
      tf->fd, 0);
  if(tf->buffer == NULL) {
    perror("Error in allocating memory for buffer of tracefile %s\n", fileName);
    goto close_file;
  }
  g_assert(tf->buffer.head & (8-1) == 0); // make sure it's aligned.
  
  header = (struct ltt_block_start_header*)tf->buffer.head;
  
  if(header->traceset.magic_number == LTT_MAGIC_NUMBER)
    tf->reverse_bo = 0;
  else if(header->traceset.magic_number == LTT_REV_MAGIC_NUMBER)
    tf->reverse_bo = 1;
  else  /* invalid magic number, bad tracefile ! */
    goto unmap_file;
    
  //store the size of the file
  tf->file_size = lTDFStat.st_size;
  tf->block_size = header->buf_size;
  tf->block_number = tf->file_size / tf->block_size;

  vfree(tf->buffer.head);
  tf->buffer.head = NULL;

  //read the first block
  if(map_block(tf,0)) {
    perror("Cannot map block %u for tracefile %s\n", 0, fileName);
    goto close_file;
  }
  
  return 0;

  /* Error */
unmap_file:
  munmap(tf->buffer.head, sizeof(struct ltt_block_start_header));
close_file:
  g_close(tf->fd);
end:
  return -1;
}


/*****************************************************************************
 *Open control and per cpu tracefiles
 ****************************************************************************/

void ltt_tracefile_open_cpu(LttTrace *t, gchar * tracefile_name)
{
  LttTracefile * tf;
  tf = ltt_tracefile_open(t,tracefile_name);
  if(!tf) return;
  t->per_cpu_tracefile_number++;
  g_ptr_array_add(t->per_cpu_tracefiles, tf);
}

gint ltt_tracefile_open_control(LttTrace *t, gchar * control_name)
{
  LttTracefile * tf;
  LttEvent ev;
  LttFacility * f;
  void * pos;
  FacilityLoad fLoad;
  unsigned int i;

  tf = ltt_tracefile_open(t,control_name);
  if(!tf) {
	  g_warning("ltt_tracefile_open_control : bad file descriptor");
    return -1;
  }
  t->control_tracefile_number++;
  g_ptr_array_add(t->control_tracefiles,tf);

  //parse facilities tracefile to get base_id
  if(strcmp(&control_name[strlen(control_name)-10],"facilities") ==0){
    while(1){
      if(!ltt_tracefile_read(tf,&ev)) return 0; // end of file

      if(ev.event_id == TRACE_FACILITY_LOAD){
	pos = ev.data;
	fLoad.name = (gchar*)pos;
	fLoad.checksum = *(LttChecksum*)(pos + strlen(fLoad.name));
	fLoad.base_code = *(guint32 *)(pos + strlen(fLoad.name) + sizeof(LttChecksum));

	for(i=0;i<t->facility_number;i++){
	  f = (LttFacility*)g_ptr_array_index(t->facilities,i);
	  if(strcmp(f->name,fLoad.name)==0 && fLoad.checksum==f->checksum){
	    f->base_id = fLoad.base_code;
	    break;
	  }
	}
	if(i==t->facility_number) {
	  g_warning("Facility: %s, checksum: %u is not found",
		  fLoad.name,(unsigned int)fLoad.checksum);
    return -1;
  }
      }else if(ev.event_id == TRACE_BLOCK_START){
	continue;
      }else if(ev.event_id == TRACE_BLOCK_END){
	break;
      }else {
        g_warning("Not valid facilities trace file");
        return -1;
      }
    }
  }
  return 0;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, 
 *Input params
 *    t                  : tracefile which will be closed
 ****************************************************************************/

void ltt_tracefile_close(LttTracefile *t)
{
  if(t->buffer.head != NULL)
    munmap(t->buffer.head, t->buf_size);
  g_close(t->fd);
}


/*****************************************************************************
 *Get system information
 ****************************************************************************/
gint getSystemInfo(LttSystemDescription* des, gchar * pathname)
{
  int fd;
  GIOChannel *iochan;
  gchar *buf = NULL;
  gsize length;

  GMarkupParseContext * context;
  GError * error = NULL;
  GMarkupParser markup_parser =
    {
      parser_start_element,
      NULL,
      parser_characters,
      NULL,  /*  passthrough  */
      NULL   /*  error        */
    };

  fd = g_open(pathname, O_RDONLY, 0);
  if(fd == -1){
    g_warning("Can not open file : %s\n", pathname);
    return -1;
  }
  
  iochan = g_io_channel_unix_new(fd);
  
  context = g_markup_parse_context_new(&markup_parser, 0, des,NULL);
  
  //while(fgets(buf,DIR_NAME_SIZE, fp) != NULL){
  while(g_io_channel_read_line(iochan, &buf, &length, NULL, &error)
      != G_IO_STATUS_EOF) {

    if(error != NULL) {
      g_warning("Can not read xml file: \n%s\n", error->message);
      g_error_free(error);
    }
    if(!g_markup_parse_context_parse(context, buf, length, &error)){
      if(error != NULL) {
        g_warning("Can not parse xml file: \n%s\n", error->message);
        g_error_free(error);
      }
      g_markup_parse_context_free(context);

      g_io_channel_shutdown(iochan, FALSE, &error); /* No flush */
      if(error != NULL) {
        g_warning("Can not close file: \n%s\n", error->message);
        g_error_free(error);
      }

      close(fd);
      return -1;
    }
  }
  g_markup_parse_context_free(context);

  g_io_channel_shutdown(iochan, FALSE, &error); /* No flush */
  if(error != NULL) {
    g_warning("Can not close file: \n%s\n", error->message);
    g_error_free(error);
  }

  g_close(fd);

  g_free(buf);
  return 0;
}

/*****************************************************************************
 *The following functions get facility/tracefile information
 ****************************************************************************/

gint getFacilityInfo(LttTrace *t, gchar* eventdefs)
{
  GDir * dir;
  const gchar * name;
  unsigned int i,j;
  LttFacility * f;
  LttEventType * et;
  gchar fullname[DIR_NAME_SIZE];
  GError * error = NULL;

  dir = g_dir_open(eventdefs, 0, &error);

  if(error != NULL) {
    g_warning("Can not open directory: %s, %s\n", eventdefs, error->message);
    g_error_free(error);
    return -1;
  }

  while((name = g_dir_read_name(dir)) != NULL){
    if(!g_pattern_match_simple("*.xml", name)) continue;
    strcpy(fullname,eventdefs);
    strcat(fullname,name);
    ltt_facility_open(t,fullname);
  }
  g_dir_close(dir);
  
  for(j=0;j<t->facility_number;j++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, j);
    for(i=0; i<f->event_number; i++){
      et = f->events[i];
      setFieldsOffset(NULL, et, NULL, t);
    }    
  }
  return 0;
}

/*****************************************************************************
 *A trace is specified as a pathname to the directory containing all the
 *associated data (control tracefiles, per cpu tracefiles, event 
 *descriptions...).
 *
 *When a trace is closed, all the associated facilities, types and fields
 *are released as well.
 */


/****************************************************************************
 * get_absolute_pathname
 *
 * return the unique pathname in the system
 * 
 * MD : Fixed this function so it uses realpath, dealing well with
 * forgotten cases (.. were not used correctly before).
 *
 ****************************************************************************/
void get_absolute_pathname(const gchar *pathname, gchar * abs_pathname)
{
  abs_pathname[0] = '\0';

  if ( realpath (pathname, abs_pathname) != NULL)
    return;
  else
  {
    /* error, return the original path unmodified */
    strcpy(abs_pathname, pathname);
    return;
  }
  return;
}

/* Search for something like : .*_.*
 *
 * The left side is the name, the right side is the number.
 */

int get_tracefile_name_number(const gchar *raw_name,
                              GQuark *name,
                              guint *num)
{
  guint raw_name_len = strlen(raw_name);
  gchar char_name[PATH_MAX]
  gchar *digit_begin;
  int i;
  int underscore_pos;
  long int cpu_num;
  gchar *endptr;

  for(i=raw_name_len-1;i>=0;i--) {
    if(raw_name[i] == '_') break;
  }
  if(i==0)  /* Either not found or name length is 0 */
    return -1;
  underscore_pos = i;

  cpu_num = strtol(raw_name+underscore_pos+1, &endptr, 10);

  if(endptr == raw_name+underscore_pos+1)
    return -1; /* No digit */
  if(cpu_num == LONG_MIN || cpu_num == LONG_MAX)
    return -1; /* underflow / overflow */
  
  char_name = strncpy(char_name, raw_name, underscore_pos);
  
  *name = g_quark_from_string(char_name);
  *num = cpu_num;
  
  return 0;
}


void ltt_tracefile_group_destroy(gpointer data)
{
  GArray *group = (GArray *)data;
  int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online)
      ltt_tracefile_close(tf);
  }
  g_array_free(group, TRUE);
}

gboolean ltt_tracefile_group_has_cpu_online(gpointer data)
{
  GArray *group = (GArray *)data;
  int i;
  LttTracefile *tf;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online) return 1;
  }
  return 0;
}


/* Open each tracefile under a specific directory. Put them in a
 * GData : permits to access them using their tracefile group pathname.
 * i.e. access control/modules tracefile group by index :
 * "control/module".
 *
 * A tracefile group is simply an array where all the per cpu tracefiles sits.
 */

static int open_tracefiles(LttTrace *trace, char *root_path, GData *tracefiles)
{
	DIR *dir = opendir(root_path);
	struct dirent *entry;
	struct stat stat_buf;
	int ret;
	char path[PATH_MAX];
	int path_len;
	char *path_ptr;

	if(channel_dir == NULL) {
		perror(subchannel_name);
		return ENOENT;
	}

	strncpy(path, root_path, PATH_MAX-1);
	path_len = strlen(path);
	path[path_len] = '/';
	path_len++;
	path_ptr = path + path_len;

	while((entry = readdir(channel_dir)) != NULL) {

		if(entry->d_name[0] == '.') continue;
		
		strncpy(path_ptr, entry->d_name, PATH_MAX - path_len);
		
		ret = stat(path, &stat_buf);
		if(ret == -1) {
			perror(path);
			continue;
		}
		
		g_debug("Tracefile file or directory : %s\n", path);
		
		if(S_ISDIR(stat_buf.st_mode)) {

			g_debug("Entering subdirectory...\n");
			ret = open_tracefiles(path, tracefiles);
			if(ret < 0) continue;
		} else if(S_ISREG(stat_buf.st_mode)) {
			g_debug("Opening file.\n");

			GQuark name;
      guint num;
      GArray *group;
      LttTracefile *tf;
      guint len;
      
      if(get_tracefile_name_number(path, &name, &num))
        continue; /* invalid name */

      group = g_datalist_get_data(tracefiles, name);
      if(group == NULL) {
        /* Elements are automatically cleared when the array is allocated.
         * It makes the cpu_online variable set to 0 : cpu offline, by default.
         */
        group = g_array_sized_new (FALSE, TRUE, sizeof(LttTracefile), 10);
        g_datalist_set_data_full(tracefiles, name,
                                 group, ltt_tracefile_group_destroy);
      }
      /* Add the per cpu tracefile to the named group */
      unsigned int old_len = group->len;
      if(num+1 > old_len)
        group = g_array_set_size(group, num+1);
      tf = &g_array_index (group, LttTracefile, num);

      if(ltt_tracefile_open(trace, path, tf)) {
        g_info("Error opening tracefile %s", path);
        g_array_set_size(group, old_len);

        if(!ltt_tracefile_group_has_cpu_online(group))
          g_datalist_remove_data(tracefiles, name);

        continue; /* error opening the tracefile : bad magic number ? */
      }
      tf->cpu_online = 1;
		}
	}
	
	closedir(dir);

	return 0;
}

/* ltt_get_facility_description
 *
 * Opens the trace corresponding to the requested facility (identified by fac_id
 * and checksum).
 *
 * The name searched is : %trace root%/eventdefs/facname_checksum.xml
 *
 * Returns 0 on success, or 1 on failure.
 */

static int ltt_get_facility_description(LttFacility *f, 
                                        LttTrace *t)
{
  char desc_file_name[PATH_MAX];
  char *text;
  guint textlen;
  gint err;

  text = g_quark_to_string(t->pathname);
  textlen = strlen(text);
  
  if(textlen >= PATH_MAX) goto name_error;
  strcpy(desc_file_name, text);

  text = "/eventdefs/";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);
  
  text = g_quark_to_string(f->name);
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);

  text = "_";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);

  err = snprintf(desc_file_name+textlen, PATH_MAX-textlen-1,
      "%u", f->checksum);
  if(err) goto name_error;

  textlen=strlen(desc_file_name);
  
  text = ".xml";
  textlen+=strlen(text);
  if(textlen >= PATH_MAX) goto name_error;
  strcat(desc_file_name, text);
 
  err = ltt_facility_open(f, t, desc_file_name);
  if(err) goto facility_error;
  
  return 0;

facility_error:
name_error:
  return 1;
}

static void ltt_tracefile_ids_destroy(gpointer data)
{
  GArray *fac_ids = (GArray *)data;
  int i;
  LttFacility *fac;

  for(i=0; i<group->len; i++) {
    fac = &g_array_index (fac_ids, LttFacility, i);
    ltt_facility_close(fac);
  }

  g_array_free(array, TRUE);
}


/* Presumes the tracefile is already seeked at the beginning. It makes sense,
 * because it must be done just after the opening */
static int ltt_process_facility_tracefile(LttTracefile *tf)
{
  int err;
  LttFacility *fac;
  GArray *fac_ids;
  
  while(1) {
    err = ltt_tracefile_read_seek(tf);
    if(err == EPERM) goto seek_error;;
    else if(err == ERANGE) break; /* End of tracefile */

    err = ltt_tracefile_read_update_event(tf);
    if(err) goto update_error;

    /* We are on a facility load/or facility unload/ or heartbeat event */
    /* The rules are :
     * * facility 0 is hardcoded : this is the core facility. It will be shown
     *   in the facility array though, and is shown as "loaded builtin" in the
     *   trace.
     * It contains event :
     *  0 : facility load
     *  1 : facility unload
     *  2 : state dump facility load
     * Facility 1 : (heartbeat)
     *  0 : heartbeat
     */
    if(tf->event.facility_id > 1) { /* Should only contain core and heartbeat
                                       facilities */
      g_warning("Error in processing facility file %s, "
          "should not contain facility id  %u.", g_quark_to_string(tf->name),
          tf->event.facility_id);
      err = EPERM;
      goto fac_id_error;
    } else if(tf->event.facility_id == 0) {
    
      // FIXME align
      switch((enum ltt_core_events)tf->event.event_id) {
        case LTT_EVENT_FACILITY_LOAD:
          struct LttFacilityLoad *fac_load_data =
            (struct LttFacilityLoad *)tf->event.data;
          char *fac_name = 
            (char*)(tf->event.data + sizeof(struct LttFacilityLoad));
          fac = &g_array_index (tf->facilities_by_num, LttTracefile,
              tf->event.);
          g_assert(fac->exists == 0);
          fac->name = g_quark_from_string(fac_name);
          fac->checksum = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->checksum);
          fac->id = ltt_get_uint8(LTT_GET_BO(tf), fac_load_data->id);
          fac->pointer_size = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->pointer_size);
          fac->size_t_size = ltt_get_uin32(LTT_GET_BO(tf),
                          fac_load_data->size_t_size);
          fac->alignment = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->alignment);

          if(ltt_get_facility_description(fac, tf->trace))
            goto facility_error;

          fac->exists = 1;

          fac_ids = g_datalist_get_data(tf->facilities_by_name, fac->name);
          if(fac_ids == NULL) {
            fac_ids = g_array_sized_new (FALSE, TRUE, sizeof(guint), 1);
            g_datalist_set_data_full(tf->facilities_by_name, fac->name,
                                     fac_ids, ltt_fac_ids_destroy);
          }
          g_array_append_val(fac_ids, fac->id);

          break;
        case LTT_EVENT_FACILITY_UNLOAD:
          /* We don't care about unload : facilities ID are valid for the whole
           * trace. They simply won't be used after the unload. */
          break;
        case LTT_EVENT_STATE_DUMP_FACILITY_LOAD:
          struct LttFacilityLoad *fac_load_data =
            (struct LttFacilityLoad *)tf->event.data;
          char *fac_name = 
            (char*)(tf->event.data + sizeof(struct LttFacilityLoad));
          fac = &g_array_index (tf->facilities_by_num, LttTracefile,
              tf->event.);
          g_assert(fac->exists == 0);
          fac->name = g_quark_from_string(fac_name);
          fac->checksum = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->checksum);
          fac->id = ltt_get_uint8(LTT_GET_BO(tf), fac_load_data->id);
          fac->pointer_size = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->pointer_size);
          fac->size_t_size = ltt_get_uin32(LTT_GET_BO(tf),
                          fac_load_data->size_t_size);
          fac->alignment = ltt_get_uint32(LTT_GET_BO(tf),
                          fac_load_data->alignment);
          fac->events;
          fac->named_types;
          fac->named_types_number;
          fac->exists = 1;
          
          fac_ids = g_datalist_get_data(tf->facilities_by_name, fac->name);
          if(fac_ids == NULL) {
            fac_ids = g_array_sized_new (FALSE, TRUE, sizeof(guint), 1);
            g_datalist_set_data_full(tf->facilities_by_name, fac->name,
                                     fac_ids, ltt_fac_ids_destroy);
          }
          g_array_append_val(fac_ids, fac->id);

          break;
        default:
          g_warning("Error in processing facility file %s, "
              "unknown event id %hhu in core facility.",
              g_quark_to_string(tf->name),
              tf->event.event_id);
          err = EPERM;
          goto event_id_error;
      }
    } else if(tf->event.facility_id == 1) {

      switch((enum ltt_heartbeat_events)tf->event.event_id) {
        case LTT_EVENT_HEARTBEAT:
          break;
        default:
          g_warning("Error in processing facility file %s, "
              "unknown event id %hhu in heartbeat facility.",
              g_quark_to_string(tf->name),
              tf->event.event_id);
          err = EPERM;
          goto event_id_error;
      }
    }
  }
  return 0;

  /* Error handling */
facility_error:
event_id_error:
fac_id_error:
update_error:
seek_error:
  return err;
}


LttTrace *ltt_trace_open(const gchar *pathname)
{
  gchar abs_path[PATH_MAX];
  LttTrace  * t;
  LttTracefile *tf;
  GArray *group;
  int i;
  
  LttTrace  * t = g_new(LttTrace, 1);
  if(!t) goto alloc_error;

  get_absolute_pathname(pathname, abs_path);
  t->pathname = g_quark_from_string(abs_path);

  /* Open all the tracefiles */
  g_datalist_init(t->tracefiles);
  if(open_tracefiles(t, abs_path, t->tracefiles))
    goto open_error;
  
  /* Prepare the facilities containers : array and mapping */
  /* Array is zeroed : the "exists" field is set to false by default */
  t->facilities_by_num = g_array_sized_new (FALSE, 
                                            TRUE, sizeof(LttFacility),
                                            NUM_FACILITIES);
  t->facilities_by_num = g_array_set_size(t->facilities_by_num, NUM_FACILITIES);

  g_datalist_init(t->tracefiles_by_name);
  
  /* Load trace XML event descriptions */
  //TODO
  
  /* Parse each trace control/facilitiesN files : get runtime fac. info */
  group = g_datalist_get_data(t->tracefiles, LTT_TRACEFILE_NAME_FACILITIES);
  if(group == NULL) {
    g_error("Trace %s has no facility tracefile", abs_path);
    goto facilities_error;
  }

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(ltt_process_facility_tracefile(tf))
      goto facilities_error;
  }

  
  
  return t;

  /* Error handling */
facilities_error:
  g_datalist_free(t->tracefiles_by_name);
  g_array_free(t->facilities_by_num, TRUE);
open_error:
  g_datalist_clear(t->tracefiles);
  g_free(t);
alloc_error:
  return NULL;

}

GQuark ltt_trace_name(LttTrace *t)
{
  return t->pathname;
}


/******************************************************************************
 * When we copy a trace, we want all the opening actions to happen again :
 * the trace will be reopened and totally independant from the original.
 * That's why we call ltt_trace_open.
 *****************************************************************************/
LttTrace *ltt_trace_copy(LttTrace *self)
{
  return ltt_trace_open(self->pathname);
}

//FIXME TODO
void ltt_trace_close(LttTrace *t)
{
  unsigned int i;
  LttTracefile * tf;
  LttFacility * f;

  g_free(t->pathname);
 
  //free system_description
  g_free(t->system_description->description);
  g_free(t->system_description->node_name);
  g_free(t->system_description->domain_name);
  g_free(t->system_description->kernel_name);
  g_free(t->system_description->kernel_release);
  g_free(t->system_description->kernel_version);
  g_free(t->system_description->machine);
  g_free(t->system_description->processor);
  g_free(t->system_description->hardware_platform);
  g_free(t->system_description->operating_system);
  g_free(t->system_description);

  //free control_tracefiles
  for(i=0;i<t->control_tracefile_number;i++){
    tf = (LttTracefile*)g_ptr_array_index(t->control_tracefiles,i);
    ltt_tracefile_close(tf);
  }
  g_ptr_array_free(t->control_tracefiles, TRUE);

  //free per_cpu_tracefiles
  for(i=0;i<t->per_cpu_tracefile_number;i++){
    tf = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles,i);
    ltt_tracefile_close(tf);
  }
  g_ptr_array_free(t->per_cpu_tracefiles, TRUE);

  //free facilities
  for(i=0;i<t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities,i);
    ltt_facility_close(f);
  }
  g_ptr_array_free(t->facilities, TRUE);

  g_free(t);
 
  g_blow_chunks();
}


/*****************************************************************************
 *Get the system description of the trace
 ****************************************************************************/

LttFacility *ltt_trace_facility_by_id(LttTrace *t, guint8 id)
{
  g_assert(index < t->facilities_by_num->len);
  return &g_array_index(t->facilities_by_num, LttFacility, id);
}

/* ltt_trace_facility_get_by_name
 *
 * Returns the GArray of facility indexes. All the fac_ids that matches the
 * requested facility name.
 *
 * If name is not found, returns NULL.
 */
GArray *ltt_trace_facility_get_by_name(LttTrace *t, GQuark name)
{
  return g_datalist_id_get_data(t->facilities_by_name, name);
}

/*****************************************************************************
 * Functions to discover all the event types in the trace 
 ****************************************************************************/

unsigned ltt_trace_eventtype_number(LttTrace *t)
{
  unsigned int i;
  unsigned count = 0;
  unsigned int num = t->facility_number;
  LttFacility * f;
  
  for(i=0;i<num;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    count += f->event_number;
  }
  return count;
}

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned evId)
{
  LttEventType *event_type;
  
  LttFacility * f;
  f = ltt_trace_facility_by_id(t,evId);

  if(unlikely(!f)) event_type = NULL;
  else event_type = f->events[evId - f->base_id];

  return event_type;
}

#if 0
/*****************************************************************************
 * ltt_trace_find_tracefile
 *
 * Find a tracefile by name and index in the group.
 *
 * Returns a pointer to the tracefiles, else NULL.
 ****************************************************************************/

LttTracefile *ltt_trace_find_tracefile(LttTrace *t, const gchar *name)
{
}
#endif //0

/*****************************************************************************
 * Get the start time and end time of the trace 
 ****************************************************************************/

static void ltt_tracefile_time_span_get(LttTracefile *tf,
                                        LttTime *start, LttTime *end)
{
  struct ltt_block_start_header * header;
  int err;

  err = map_block(tf, 0);
  if(unlikely(err)) {
    g_error("Can not map block");
    *start = { 0xFFFFFFFF, 0xFFFFFFFF };
  } else
    *start = tf->buffer.begin.timestamp;

  err = map_block(tf, tf->num_blocks - 1);  /* Last block */
  if(unlikely(err)) {
    g_error("Can not map block");
    *end = { 0, 0 };
  } else
    *end = tf->buffer.end.timestamp;
}

struct tracefile_time_span_get_args {
  LttTrace *t;
  LttTime *start;
  LttTime *end;
};

static void group_time_span_get(GQuark name, gpointer data, gpointer user_data)
{
  struct tracefile_time_span_get_args *args =
          (struct tracefile_time_span_get_args*)user_data;

  GArray *group = (GArray *)data;
  int i;
  LttTracefile *tf;
  LttTime tmp_start;
  LttTime tmp_end;

  for(i=0; i<group->len; i++) {
    tf = &g_array_index (group, LttTracefile, i);
    if(tf->cpu_online) {
      ltt_tracefile_time_span_get(tf, &tmp_start, &tmp_end);
      if(ltt_time_compare(*args->start, tmp_start)>0) *args->start = tmp_start;
      if(ltt_time_compare(*args->end, tmp_end)<0) *args->end = tmp_end;
    }
  }
}

void ltt_trace_time_span_get(LttTrace *t, LttTime *start, LttTime *end)
{
  LttTime min_start = { 0xFFFFFFFF, 0xFFFFFFFF };
  LttTime max_end = { 0, 0 };
  struct tracefile_time_span_get_args args = { t, &min_start, &max_end };

  g_datalist_foreach(t->tracefiles, &group_time_span_get, &args);
  
  if(start != NULL) *start = min_start;
  if(end != NULL) *end = max_end;
  
}


/*****************************************************************************
 *Get the name of a tracefile
 ****************************************************************************/

GQuark ltt_tracefile_name(LttTracefile *tf)
{
  return tf->name;
}

/*****************************************************************************
 * Get the number of blocks in the tracefile 
 ****************************************************************************/

guint ltt_tracefile_block_number(LttTracefile *tf)
{
  return tf->block_number; 
}


/* Seek to the first event in a tracefile that has a time equal or greater than
 * the time passed in parameter.
 *
 * If the time parameter is outside the tracefile time span, seek to the first
 * or the last event of the tracefile.
 *
 * If the time parameter is before the first event, we have to seek specially to
 * there.
 *
 * If the time is after the end of the trace, get the last event. 
 *
 * Do a binary search to find the right block, then a sequential search in the
 * block to find the event. 
 *
 * In the special case where the time requested fits inside a block that has no
 * event corresponding to the requested time, the first event of the next block
 * will be seeked.
 *
 * IMPORTANT NOTE : // FIXME everywhere...
 *
 * You MUST NOT do a ltt_tracefile_read right after a ltt_tracefile_seek_time :
 * you will jump over an event if you do.
 *
 * Return value : 0 : no error, the tf->event can be used
 *                otherwise : this is an error.
 *
 * */

int ltt_tracefile_seek_time(LttTracefile *tf, LttTime time)
{
  int ret = 0;
  int err;
  unsigned int block_num, high, low;

  /* seek at the beginning of trace */
  err = map_block(tf, 0);  /* First block */
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

 /* If the time is lower or equal the beginning of the trace,
  * go to the first event. */
  if(ltt_time_compare(time, tf->buffer.start.timestamp) <= 0) {
    ret = ltt_tracefile_read(tf)
    goto found; /* There is either no event in the trace or the event points
                   to the first event in the trace */
  }

  err = map_block(tf, tf->num_blocks - 1);  /* Last block */
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

 /* If the time is after the end of the trace, get the last event. */
  if(ltt_time_compare(time, tf->buffer.end.timestamp) >= 0) {
    /* While the ltt_tracefile_read doesn't return ERANGE or EPERM,
     * continue reading.
     */
    while(1) {
      ret = ltt_tracefile_read(tf);
      if(ret == ERANGE) goto found; /* ERANGE or EPERM */
      else if(ret) goto fail;
    }
  }

  /* Binary search the block */
  high = tf->num_blocks - 1;
  low = 0;
  
  while(1) {
    block_num = ((high-low) / 2) + low;

    err = map_block(tf, block_num);
    if(unlikely(err)) {
      g_error("Can not map block");
      goto fail;
    }
    if(high == low) {
      /* We cannot divide anymore : this is what would happen if the time
       * requested was exactly between two consecutive buffers'end and start 
       * timestamps. This is also what would happend if we didn't deal with out
       * of span cases prior in this function. */
      /* The event is right in the buffer!
       * (or in the next buffer first event) */
      while(1) {
        ret = ltt_tracefile_read(tf);
        if(ret == ERANGE) goto found; /* ERANGE or EPERM */
        else if(ret) goto fail;

        if(ltt_time_compare(time, tf->event.event_time) >= 0)
          break;
      }

    } if(ltt_time_compare(time, tf->buffer.start.timestamp) < 0) {
      /* go to lower part */
      high = block_num;
    } else if(ltt_time_compare(time, tf->buffer.end.timestamp) > 0) {
      /* go to higher part */
      low = block_num;
    } else {/* The event is right in the buffer!
               (or in the next buffer first event) */
      while(1) {
        ltt_tracefile_read(tf);
        if(ret == ERANGE) goto found; /* ERANGE or EPERM */
        else if(ret) goto fail;

        if(ltt_time_compare(time, tf->event.event_time) >= 0)
          break;
      }
      goto found;
    }
  }

found:
  return 0;

  /* Error handling */
fail:
  g_error("ltt_tracefile_seek_time failed on tracefile %s", 
      g_quark_to_string(tf->name));
  return EPERM;
}


int ltt_tracefile_seek_position(LttTracefile *tf, const LttEventPosition *ep) {
  
  int err;
  
  if(ep->tracefile != tf) {
    goto fail;
  }

  err = map_block(tf, ep->block);
  if(unlikely(err)) {
    g_error("Can not map block");
    goto fail;
  }

  tf->event.offset = ep->offset;

  err = ltt_tracefile_read_update_event(tf);
  if(err) goto fail;
  err = ltt_tracefile_read_op(tf);
  if(err) goto fail;

  return;

fail:
  g_error("ltt_tracefile_seek_time failed on tracefile %s", 
      g_quark_to_string(tf->name));
}

/* Calculate the real event time based on the buffer boundaries */
LttTime ltt_interpolate_time(LttTracefile *tf, LttEvent *event)
{
  LttTime time;

  g_assert(t->trace->has_tsc);

  time = ltt_time_from_uint64(
      (guint64)tf->buffer.tsc*tf->buffer.nsecs_per_cycle);
  time = ltt_time_add(tf->buffer.begin.timestamp, time);

  return time;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_read : Read the next event in the tracefile
 *Input params
 *    t                  : tracefile
 *Return value
 *
 *    Returns 0 if an event can be used in tf->event.
 *    Returns ERANGE on end of trace. The event in tf->event still can be used.
 *    Returns EPERM on error.
 *
 *    This function does make the tracefile event structure point to the event
 *    currently pointed to by the tf->event.
 *
 *    Note : you must call a ltt_tracefile_seek to the beginning of the trace to
 *    reinitialize it after an error if you want results to be coherent.
 *    It would be the case if a end of trace last buffer has no event : the end
 *    of trace wouldn't be returned, but an error.
 *    We make the assumption there is at least one event per buffer.
 ****************************************************************************/

int ltt_tracefile_read(LttTracefile *tf)
{
  int err;

  err = ltt_tracefile_read_seek(tf);
  if(err) return err;
  err = ltt_tracefile_read_update_event(tf);
  if(err) return err;
  err = ltt_tracefile_read_op(tf);
  if(err) return err;

  return 0;
}

int ltt_tracefile_read_seek(LttTracefile *tf)
{
  int err;

  /* Get next buffer until we finally have an event, or end of trace */
  while(1) {
    err = ltt_seek_next_event(tf);
    if(unlikely(err == ENOPROTOOPT)) {
      return EPERM;
    }

    /* Are we at the end of the buffer ? */
    if(err == ERANGE) {
      if(unlikely(tf->buffer.index == tf->num_blocks-1)){ /* end of trace ? */
        return ERANGE;
      } else {
        /* get next block */
        err = map_block(tf, tf->buffer.index + 1);
        if(unlikely(err)) {
          g_error("Can not map block");
          return EPERM;
        }
      }
    } else break; /* We found an event ! */
  }
  
  return 0;
}


/* do specific operation on events */
int ltt_tracefile_read_op(LttTracefile *tf)
{
  int err;
  LttFacility *f;
  void * pos;
  LttEvent *event;

  event = &tf->event;

   /* do event specific operation */

  /* do something if its an heartbeat event : increment the heartbeat count */
  if(event->facility_id != 0) { /* except core */
    f = (LttFacility*)g_ptr_array_index(tf->trace->facilities,
                                          event->facility_id);
    g_assert(f != NULL);

    if(unlikely(ltt_facility_name(f)
          != LTT_FACILITY_NAME_HEARTBEAT)) {
      LttEventType *et = ltt_facility_eventtype_get_by_name(f, 
                    LTT_EVENT_NAME_HEARTBEAT);
      if(et->id == event->event_id)
        t->cur_heart_beat_number++;
    }
  }
  
  return 0;
}


/* same as ltt_tracefile_read, but does not seek to the next event nor call
 * event specific operation. */
int ltt_tracefile_read_update_event(LttTracefile *tf)
{
  int err;
  LttFacility *f;
  void * pos;
  LttEvent *event;
 
  event = &tf->event;
  pos = event->offset;

  /* Read event header */
  
  //TODO align
  
  if(tf->trace->has_tsc) {
    event->time.timestamp = ltt_get_uint32(LTT_GET_BO(t),
                                          pos);
    /* 32 bits -> 64 bits tsc */
    /* note : still works for seek and non seek cases. */
    if(event->time.timestamp < (0xFFFFFFFFULL&tf->buffer.tsc)) {
      tf->buffer.tsc = ((tf->buffer.tsc&0xFFFFFFFF00000000ULL)
                          + 0x100000000ULL)
                              | (guint64)event->time.timestamp;
    } else {
      /* no overflow */
      tf->buffer.tsc = (tf->buffer.tsc&0xFFFFFFFF00000000ULL) 
                              | (guint64)event->time.timestamp;
    }

    event->event_time = ltt_interpolate_time(tf, event);

    pos += sizeof(uint32);
  } else {
    event->time.delta = ltt_get_uint32(LTT_GET_BO(tf),
                                          pos);
    tf->buffer.tsc = 0;

    event->event_time = ltt_time_add(tf->buffer.begin.timestamp,
                                     event->time_delta);
    pos += sizeof(uint32);
  }

  event->facility_id = ltt_get_uint8(LTT_GET_BO(tf),
													tf->cur_event_pos);
  pos += sizeof(uint8);

  event->event_id = ltt_get_uint8(LTT_GET_BO(tf),
													tf->cur_event_pos);
  pos += sizeof(uint8);

  event->data = tf->cur_event_pos + EVENT_HEADER_SIZE;

  event->data = pos;

  return 0;
}


/****************************************************************************
 *Function name
 *    map_block       : map a block from the file
 *Input Params
 *    lttdes          : ltt trace file 
 *    whichBlock      : the block which will be read
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 ****************************************************************************/

static int map_block(LttTracefile * tf, int block_num)
{
  struct ltt_block_start_header *header;

  g_assert(block_num < tf->num_blocks);

  if(tf->buffer.head != NULL)
    munmap(tf->buffer.head, tf->buf_size);
  
  /* Multiple of pages aligned head */
  tf->buffer.head = mmap(0, tf->block_size, PROT_READ, tf->fd,
                            (off_t)tf->block_size * (off_t)block_num);

  if(tf->buffer.head == NULL) {
    perror("Error in allocating memory for buffer of tracefile %s\n", fileName);
    goto map_error;
  }
  g_assert(tf->buffer.head & (8-1) == 0); // make sure it's aligned.
  

  tf->buffer.index = block_num;

  header = (struct ltt_block_start_header*)tf->buffer.head;

  tf->buffer.begin.timestamp = ltt_get_uint64(LTT_GET_BO(tf),
                                              header->begin.timestamp)
                                * NSEC_PER_USEC;
  tf->buffer.begin.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              header->begin.cycle_count);
  tf->buffer.end.timestamp = ltt_get_uint64(LTT_GET_BO(tf),
                                              header->end.timestamp)
                                * NSEC_PER_USEC;
  tf->buffer.end.cycle_count = ltt_get_uint64(LTT_GET_BO(tf),
                                              header->end.cycle_count);
  tf->buffer.lost_size = ltt_get_uint32(LTT_GET_BO(tf),
                                              header->lost_size);
  
  tf->buffer.tsc =  tf->buffer.begin.cycle_count;

  /* FIXME
   * eventually support variable buffer size : will need a partial pre-read of
   * the headers to create an index when we open the trace... eventually. */
  g_assert(tf->block_size  == ltt_get_uint32(header->buf_size));
  
  /* Now that the buffer is mapped, calculate the time interpolation for the
   * block. */
  
  tf->buffer.nsecs_per_cycle = calc_nsecs_per_cycle(&tf->buffer);
 
  /* Make the current event point to the beginning of the buffer :
   * it means that the event read must get the first event. */
  tf->event.tracefile = tf;
  tf->event.block = block_num;
  tf->event.offset = tf->buffer.head;
  
  return 0;

map_error:
  return -errno;

}

/* Take the tf current event offset and use the event facility id and event id
 * to figure out where is the next event offset.
 *
 * This is an internal function not aiming at being used elsewhere : it will
 * not jump over the current block limits. Please consider using
 * ltt_tracefile_read to do this.
 *
 * Returns 0 on success
 *         ERANGE if we are at the end of the buffer.
 *         ENOPROTOOPT if an error occured when getting the current event size.
 */
static int ltt_seek_next_event(LttTracefile *tf)
{
  int ret = 0;
  void *pos;
  
  /* seek over the buffer header if we are at the buffer start */
  if(tf->event.offset == tf->buffer.head) {
    tf->event.offset += sizeof(struct ltt_block_start_header);
    goto found;
  }

  
  if(tf->event.offset == tf->buffer.head + tf->buffer.lost_size) {
    ret = ERANGE;
    goto found;
  }

  pos = tf->event.data;

  /* FIXME : do this function. Remember to hardcode the sizes of heartbeat and
   * core */
  pos += ltt_facility_get_event_size(tf->event.facility_id, tf->event.event_id);
  on error : goto error;

  tf->event.offset = pos;

found:
  return ret;

error:
  g_error("Error in ltt_seek_next_event for tracefile %s",
      g_quark_to_string(tf->name));
  return ENOPROTOOPT;
}


/*****************************************************************************
 *Function name
 *    calc_nsecs_per_cycle : calculate nsecs per cycle for current block
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

static double calc_nsecs_per_cycle(LttTracefile * t)
{
  LttTime           lBufTotalTime; /* Total time for this buffer */
  double            lBufTotalNSec; /* Total time for this buffer in nsecs */
  LttCycleCount     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  lBufTotalTime = ltt_time_sub(
       ltt_get_time(t->buffer.end.timestamp),
       ltt_get_time(t->buffer.begin.timestamp));

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle  = t->buffer.end.cycle_count;
  lBufTotalCycle -= t->buffer.start.cycle_count;

  /* Convert the total time to double */
  lBufTotalNSec  = ltt_time_to_double(lBufTotalTime);
  
  return lBufTotalNSec / (double)lBufTotalCycle;

}

/*****************************************************************************
 *Function name
 *    setFieldsOffset : set offset of the fields
 *Input params 
 *    tracefile       : opened trace file  
 *    evT             : the event type
 *    evD             : event data, it may be NULL
 ****************************************************************************/

void setFieldsOffset(LttTracefile *tf,LttEventType *evT,void *evD,LttTrace* t)
{
  LttField * rootFld = evT->root_field;
  //  rootFld->base_address = evD;

  if(likely(rootFld))
    rootFld->field_size = getFieldtypeSize(tf, evT, 0,0,rootFld, evD,t);  
}

/*****************************************************************************
 *Function name
 *    getFieldtypeSize: get the size of the field type (primitive type)
 *Input params 
 *    tracefile       : opened trace file 
 *    evT             : event type
 *    offsetRoot      : offset from the root
 *    offsetParent    : offset from the parrent
 *    fld             : field
 *    evD             : event data, it may be NULL
 *Return value
 *    int             : size of the field
 ****************************************************************************/

static inline gint getFieldtypeSize(LttTracefile * t,
       LttEventType * evT, gint offsetRoot,
	     gint offsetParent, LttField * fld, void *evD, LttTrace *trace)
{
  gint size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  if(unlikely(t && evT->latest_block==t->which_block &&
                   evT->latest_event==t->which_event)){
    size = fld->field_size;
    goto end_getFieldtypeSize;
  } else {
    /* This likely has been tested with gcov : half of them.. */
    if(unlikely(fld->field_fixed == 1)){
      /* tested : none */
      if(unlikely(fld == evT->root_field)) {
        size = fld->field_size;
        goto end_getFieldtypeSize;
      }
    }

    /* From gcov profiling : half string, half struct, can we gain something
     * from that ? (Mathieu) */
    switch(type->type_class) {
      case LTT_ARRAY:
        element_number = (int) type->element_number;
        if(fld->field_fixed == -1){
          size = getFieldtypeSize(t, evT, offsetRoot,
                                  0,fld->child[0], NULL, trace);
          if(size == 0){ //has string or sequence
            fld->field_fixed = 0;
          }else{
            fld->field_fixed = 1;
            size *= element_number; 
          }
        }else if(fld->field_fixed == 0){// has string or sequence
          size = 0;
          for(i=0;i<element_number;i++){
            size += getFieldtypeSize(t, evT, offsetRoot+size,size, 
            fld->child[0], evD+size, trace);
          }
        }else size = fld->field_size;
        if(unlikely(!evD)){
          fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
          fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        }

        break;

      case LTT_SEQUENCE:
        size1 = (int) ltt_type_size(trace, type);
        if(fld->field_fixed == -1){
          fld->sequ_number_size = size1;
          fld->field_fixed = 0;
          size = getFieldtypeSize(t, evT, offsetRoot,
                                  0,fld->child[0], NULL, trace);      
          fld->element_size = size;
        }else{//0: sequence
          element_number = getIntNumber(t->trace->reverse_byte_order,size1,evD);
          type->element_number = element_number;
          if(fld->element_size > 0){
            size = element_number * fld->element_size;
          }else{//sequence has string or sequence
            size = 0;
            for(i=0;i<element_number;i++){
              size += getFieldtypeSize(t, evT, offsetRoot+size+size1,size+size1, 
                                       fld->child[0], evD+size+size1, trace);
            }
          }
          size += size1;
        }
        if(unlikely(!evD)){
          fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
          fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        }

        break;
        
      case LTT_STRING:
        size = 0;
        if(fld->field_fixed == -1){
          fld->field_fixed = 0;
        }else{//0: string
          /* Hope my implementation is faster than strlen (Mathieu) */
          char *ptr=(char*)evD;
          size = 1;
          /* from gcov : many many strings are empty, make it the common case.*/
          while(unlikely(*ptr != '\0')) { size++; ptr++; }
          //size = ptr - (char*)evD + 1; //include end : '\0'
        }
        fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
        fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;

        break;
        
      case LTT_STRUCT:
        element_number = (int) type->element_number;
        size = 0;
        /* tested with gcov */
        if(unlikely(fld->field_fixed == -1)){
          offset1 = offsetRoot;
          offset2 = 0;
          for(i=0;i<element_number;i++){
            size1=getFieldtypeSize(t, evT,offset1,offset2,
                                   fld->child[i], NULL, trace);
            if(likely(size1 > 0 && size >= 0)){
              size += size1;
              if(likely(offset1 >= 0)) offset1 += size1;
                offset2 += size1;
            }else{
              size = -1;
              offset1 = -1;
              offset2 = -1;
            }
          }
          if(unlikely(size == -1)){
             fld->field_fixed = 0;
             size = 0;
          }else fld->field_fixed = 1;
        }else if(likely(fld->field_fixed == 0)){
          offset1 = offsetRoot;
          offset2 = 0;
          for(i=0;unlikely(i<element_number);i++){
            size=getFieldtypeSize(t,evT,offset1,offset2,
                                  fld->child[i],evD+offset2, trace);
            offset1 += size;
            offset2 += size;
          }      
          size = offset2;
        }else size = fld->field_size;
        fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
        fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        break;

      default:
        if(unlikely(fld->field_fixed == -1)){
          size = (int) ltt_type_size(trace, type);
          fld->field_fixed = 1;
        }else size = fld->field_size;
        if(unlikely(!evD)){
          fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
          fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
        }
        break;
    }
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  fld->field_size      = size;

end_getFieldtypeSize:

  return size;
}


/*****************************************************************************
 *Function name
 *    getIntNumber    : get an integer number
 *Input params 
 *    size            : the size of the integer
 *    evD             : the event data
 *Return value
 *    gint64          : a 64 bits integer
 ****************************************************************************/

gint64 getIntNumber(gboolean reverse_byte_order, int size, void *evD)
{
  gint64 i;

  switch(size) {
    case 1: i = *((gint8*)evD); break;
    case 2: i = ltt_get_int16(reverse_byte_order, evD); break;
    case 4: i = ltt_get_int32(reverse_byte_order, evD); break;
    case 8: i = ltt_get_int64(reverse_byte_order, evD); break;
    default: i = ltt_get_int64(reverse_byte_order, evD);
             g_critical("getIntNumber : integer size %d unknown", size);
             break;
  }

  return i;
}

/* get the node name of the system */

char * ltt_trace_system_description_node_name (LttSystemDescription * s)
{
  return s->node_name;
}


/* get the domain name of the system */

char * ltt_trace_system_description_domain_name (LttSystemDescription * s)
{
  return s->domain_name;
}


/* get the description of the system */

char * ltt_trace_system_description_description (LttSystemDescription * s)
{
  return s->description;
}


/* get the start time of the trace */

LttTime ltt_trace_system_description_trace_start_time(LttSystemDescription *s)
{
  return s->trace_start;
}


LttTracefile *ltt_tracefile_new()
{
  return g_new(LttTracefile, 1);
}

void ltt_tracefile_destroy(LttTracefile *tf)
{
  g_free(tf);
}

void ltt_tracefile_copy(LttTracefile *dest, const LttTracefile *src)
{
  *dest = *src;
}

/* Before library loading... */

static void __attribute__((constructor)) init(void)
{
  LTT_FACILITY_NAME_HEARTBEAT = g_quark_from_string("heartbeat");
  LTT_EVENT_NAME_HEARTBEAT = g_quark_from_string("heartbeat");
  
  LTT_TRACEFILE_NAME_FACILITIES = g_quark_from_string("control/facilities");
}

