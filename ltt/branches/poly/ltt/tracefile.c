#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/errno.h>  

#include <ltt/LTTTypes.h>  
#include "parser.h"
#include <ltt/trace.h>

#define DIR_NAME_SIZE 256

/* set the offset of the fields belonging to the event,
   need the information of the archecture */
void setFieldsOffset(LttTracefile * t, LttEventType *evT, void *evD);

/* get the size of the field type according to the archtecture's
   size and endian type(info of the archecture) */
int getFieldtypeSize(LttTracefile * t, LttEventType * evT, int offsetRoot,
		     int offsetParent, LttField * fld, void * evD );

/* read a fixed size or a block information from the file (fd) */
int readFile(int fd, void * buf, size_t size, char * mesg);
int readBlock(LttTracefile * tf, int whichBlock);

/* calculate cycles per nsec for current block */
void getCyclePerNsec(LttTracefile * t);

/* reinitialize the info of the block which is already in the buffer */
void updateTracefile(LttTracefile * tf);

/* go to the next event */
int skipEvent(LttTracefile * t);

/* compare two time (LttTime), 0:t1=t2, -1:t1<t2, 1:t1>t2 */
int timecmp(LttTime * t1, LttTime * t2);

/* get an integer number */
int getIntNumber(int size1, void *evD);


/* Time operation macros for LttTime (struct timespec) */
/*  (T3 = T2 - T1) */
#define TimeSub(T3, T2, T1) \
do \
{\
  (T3).tv_sec  = (T2).tv_sec  - (T1).tv_sec;  \
  (T3).tv_nsec = (T2).tv_nsec - (T1).tv_nsec; \
  if((T3).tv_nsec < 0)\
    {\
    (T3).tv_sec--;\
    (T3).tv_nsec += 1000000000;\
    }\
} while(0)

/*  (T3 = T2 + T1) */
#define TimeAdd(T3, T2, T1) \
do \
{\
  (T3).tv_sec  = (T2).tv_sec  + (T1).tv_sec;  \
  (T3).tv_nsec = (T2).tv_nsec + (T1).tv_nsec; \
  if((T3).tv_nsec >= 1000000000)\
    {\
    (T3).tv_sec += (T3).tv_nsec / 1000000000;\
    (T3).tv_nsec = (T3).tv_nsec % 1000000000;\
    }\
} while(0)



/*****************************************************************************
 *Function name
 *    ltt_tracefile_open : open a trace file, construct a LttTracefile
 *Input params
 *    t                  : the trace containing the tracefile
 *    fileName           : path name of the trace file
 *Return value
 *                       : a pointer to a tracefile
 ****************************************************************************/ 

LttTracefile* ltt_tracefile_open(LttTrace * t, char * fileName)
{
  LttTracefile * tf;
  struct stat    lTDFStat;    /* Trace data file status */
  BlockStart     a_block_start;

  tf = g_new(LttTracefile, 1);  

  //open the file
  tf->name = g_strdup(fileName);
  tf->trace = t;
  tf->fd = open(fileName, O_RDONLY, 0);
  if(tf->fd < 0){
    g_error("Unable to open input data file %s\n", fileName);
  }
 
  // Get the file's status 
  if(fstat(tf->fd, &lTDFStat) < 0){
    g_error("Unable to get the status of the input data file %s\n", fileName);
  }

  // Is the file large enough to contain a trace 
  if(lTDFStat.st_size < sizeof(BlockStart) + EVENT_HEADER_SIZE){
    g_error("The input data file %s does not contain a trace\n", fileName);
  }
  
  //store the size of the file
  tf->file_size = lTDFStat.st_size;
  tf->block_size = t->system_description->ltt_block_size;
  tf->block_number = tf->file_size / tf->block_size;
  tf->which_block = 0;

  //allocate memory to contain the info of a block
  tf->buffer = (void *) g_new(char, t->system_description->ltt_block_size);

  //read the first block
  if(readBlock(tf,1)) exit(1);

  return tf;
}


/*****************************************************************************
 *Open control and per cpu tracefiles
 ****************************************************************************/

void ltt_tracefile_open_cpu(LttTrace *t, char * tracefile_name)
{
  LttTracefile * tf;
  tf = ltt_tracefile_open(t,tracefile_name);
  t->per_cpu_tracefile_number++;
  g_ptr_array_add(t->per_cpu_tracefiles, tf);
}

void ltt_tracefile_open_control(LttTrace *t, char * control_name)
{
  LttTracefile * tf;
  LttEvent * ev;
  LttFacility * f;
  uint16_t evId;
  void * pos;
  FacilityLoad fLoad;
  int i;

  tf = ltt_tracefile_open(t,control_name);
  t->control_tracefile_number++;
  g_ptr_array_add(t->control_tracefiles,tf);

  //parse facilities tracefile to get base_id
  if(strcmp(control_name,"facilities") ==0){
    while(1){
      evId = *(uint16_t*)tf->cur_event_pos;    
      if(evId == TRACE_FACILITY_LOAD){
	pos = tf->cur_event_pos + EVENT_HEADER_SIZE;
	fLoad.name = (char*)pos;
	fLoad.checksum = *(LttChecksum*)(pos + sizeof(pos));
	fLoad.base_code = *(uint32_t*)(pos + sizeof(pos) + sizeof(LttChecksum));

	for(i=0;i<t->facility_number;i++){
	  f = (LttFacility*)g_ptr_array_index(t->facilities,i);
	  if(strcmp(f->name,fLoad.name)==0 && fLoad.checksum==f->checksum){
	    f->base_id = fLoad.base_code;
	    break;
	  }
	}
	if(i==t->facility_number)
	  g_error("Facility: %s, checksum: %d is not founded\n",
		  fLoad.name,fLoad.checksum);

	ev = ltt_tracefile_read(tf); //get next event
	if(!ev) break;           //end of tracefile
      }else if(evId == TRACE_BLOCK_END){
	//can only reach here if it is the first event
	g_error("Facilities does not contain any facility_load event\n");
      }else g_error("Not valid facilities trace file\n");
    }
  }
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_close: close a trace file, 
 *Input params
 *    t                  : tracefile which will be closed
 ****************************************************************************/

void ltt_tracefile_close(LttTracefile *t)
{
  g_free(t->name);
  g_free(t->buffer);
  g_free(t);
}


/*****************************************************************************
 *Get system information
 ****************************************************************************/
void getSystemInfo(LttSystemDescription* des, char * pathname)
{
  FILE * fp;
  int i;
  int entry_number = 15;
  char buf[DIR_NAME_SIZE];
  char description[4*DIR_NAME_SIZE];
  char * ptr;

  fp = fopen(pathname,"r");
  if(!fp){
    g_error("Can not open file : %s\n", pathname);
  }
  
  while(fgets(buf,DIR_NAME_SIZE, fp)!= NULL){
    ptr = buf;
    while(isspace(*ptr)) ptr++;
    if(strlen(ptr) == 0) continue;     
    break;
  }
  
  if(strlen(ptr) == 0) g_error("Not a valid file: %s\n", pathname);
  if(strncmp("<system",ptr,7) !=0)g_error("Not a valid file: %s\n", pathname);

  for(i=0;i<entry_number;i++){
    if(fgets(buf,DIR_NAME_SIZE, fp)== NULL)
      g_error("Not a valid file: %s\n", pathname);
    ptr = buf;
    while(isspace(*ptr)) ptr++;
    switch(i){
      case 0:
	if(strncmp("node_name=",ptr,10)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	des->node_name = g_strdup(ptr+10);
        break;
      case 1:
	if(strncmp("domainname=",ptr,11)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	des->domain_name = g_strdup(ptr+11);
        break;
      case 2:
	if(strncmp("cpu=",ptr,4)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	des->nb_cpu = (unsigned)atoi(ptr+4);
        break;
      case 3:
	if(strncmp("arch_size=",ptr,10)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	if(strcmp(ptr+10,"\"LP32\"") == 0) des->size = LTT_LP32;
	else if(strcmp(ptr+10,"\"ILP32\"") == 0) des->size = LTT_ILP32;
	else if(strcmp(ptr+10,"\"LP64\"") == 0) des->size = LTT_LP64;
	else if(strcmp(ptr+10,"\"ILP64\"") == 0) des->size = LTT_ILP64;
	else if(strcmp(ptr+10,"\"UNKNOWN\"") == 0) des->size = LTT_UNKNOWN;
        break;
      case 4:
	if(strncmp("endian=",ptr,7)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	if(strcmp(ptr+7,"\"LITTLE_ENDIAN\"") == 0)
	  des->endian = LTT_LITTLE_ENDIAN;
	else if(strcmp(ptr+7,"\"BIG_ENDIAN\"") == 0) 
	  des->endian = LTT_BIG_ENDIAN;
        break;
      case 5:
	if(strncmp("kernel_name=",ptr,12)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	des->kernel_name = g_strdup(ptr+12);
        break;
      case 6:
	if(strncmp("kernel_release=",ptr,15)!=0)
	  g_error("Not a valid file: %s\n", pathname);
	des->kernel_release = g_strdup(ptr+15);
        break;
       case 7:
	 if(strncmp("kernel_version=",ptr,15)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 des->kernel_version = g_strdup(ptr+15);
	 break;
       case 8:
	 if(strncmp("machine=",ptr,8)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 des->machine = g_strdup(ptr+8);
	 break;
       case 9:
	 if(strncmp("processor=",ptr,10)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 des->processor = g_strdup(ptr+10);
	 break;
       case 10:
	 if(strncmp("hardware_platform=",ptr,18)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 des->hardware_platform = g_strdup(ptr+18);
	 break;
       case 11:
	 if(strncmp("operating_system=",ptr,17)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 des->operating_system = g_strdup(ptr+17);
	 break;
       case 12:
	 if(strncmp("ltt_major_version=",ptr,18)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 ptr += 18; 
	 ptr++;//skip begining "
	 ptr[strlen(ptr)-1] = '\0'; //get rid of the ending "
	 des->ltt_major_version = (unsigned)atoi(ptr);
	 break;
       case 13:
	 if(strncmp("ltt_minor_version=",ptr,18)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 ptr += 18; 
	 ptr++;//skip begining "
	 ptr[strlen(ptr)-1] = '\0'; //get rid of the ending "
	 des->ltt_minor_version = (unsigned)atoi(ptr);
	 break;
       case 14:
	 if(strncmp("ltt_block_size=",ptr,15)!=0)
	   g_error("Not a valid file: %s\n", pathname);
	 ptr += 15; 
	 ptr++;//skip begining "
	 ptr[strlen(ptr)-1] = '\0'; //get rid of the ending "
	 des->ltt_block_size = (unsigned)atoi(ptr);
	 break;
       default:
	 g_error("Not a valid file: %s\n", pathname);      
   }
  }

  //get description
  description[0] = '\0';
  if(fgets(buf,DIR_NAME_SIZE, fp)== NULL)
    g_error("Not a valid file: %s\n", pathname);
  ptr = buf;
  while(isspace(*ptr)) ptr++;
  if(*ptr != '>') g_error("Not a valid file: %s\n", pathname);
  while((ptr=fgets(buf,DIR_NAME_SIZE, fp))!= NULL){
    ptr = buf;
    while(isspace(*ptr)) ptr++;
    if(strncmp("</system>",ptr,9) == 0 )break;
    strcat(description, buf);
  }
  if(!ptr)g_error("Not a valid file: %s\n", pathname);
  if(description[0] = '\0')des->description = NULL;
  des->description = g_strdup(description);  

  fclose(fp);
}

/*****************************************************************************
 *The following functions get facility/tracefile information
 ****************************************************************************/

void getFacilityInfo(LttTrace *t, char* eventdefs)
{
  DIR * dir;
  struct dirent *entry;
  char * ptr;
  int i,j;
  LttFacility * f;
  LttEventType * et;
  LttTracefile * tracefile;

  dir = opendir(eventdefs);
  if(!dir) g_error("Can not open directory: %s\n", eventdefs);

  while((entry = readdir(dir)) != NULL){
    ptr = &entry->d_name[strlen(entry->d_name)-4];
    if(strcmp(ptr,".xml") != 0) continue;
    ltt_facility_open(t,entry->d_name);
  }  
  closedir(dir);
  
  tracefile = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles,0);
  for(j=0;j<t->facility_number;j++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    for(i=0; i<f->event_number; i++){
      et = f->events[i];
      setFieldsOffset(tracefile, et, NULL);
    }    
  }
}

void getControlFileInfo(LttTrace *t, char* control)
{
  DIR * dir;
  struct dirent *entry;

  dir = opendir(control);
  if(!dir) g_error("Can not open directory: %s\n", control);

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,"facilities") != 0 ||
       strcmp(entry->d_name,"interrupts") != 0 ||
       strcmp(entry->d_name,"processes") != 0) continue;
    
    ltt_tracefile_open_control(t,entry->d_name);
  }  
  closedir(dir);
}

void getCpuFileInfo(LttTrace *t, char* cpu)
{
  DIR * dir;
  struct dirent *entry;

  dir = opendir(cpu);
  if(!dir) g_error("Can not open directory: %s\n", cpu);

  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name,".") != 0 ||
       strcmp(entry->d_name,"..") != 0 ){
      ltt_tracefile_open_cpu(t,entry->d_name);
    }else continue;
  }  
  closedir(dir);
}

/*****************************************************************************
 *A trace is specified as a pathname to the directory containing all the
 *associated data (control tracefiles, per cpu tracefiles, event 
 *descriptions...).
 *
 *When a trace is closed, all the associated facilities, types and fields
 *are released as well.
 ****************************************************************************/

LttTrace *ltt_trace_open(char *pathname)
{
  LttTrace  * t;
  LttSystemDescription * sys_description;
  char eventdefs[DIR_NAME_SIZE];
  char info[DIR_NAME_SIZE];
  char control[DIR_NAME_SIZE];
  char cpu[DIR_NAME_SIZE];
  char tmp[DIR_NAME_SIZE];
  gboolean has_slash = FALSE;

  //establish the pathname to different directories
  if(pathname[strlen(pathname)-1] == '/')has_slash = TRUE;
  strcpy(eventdefs,pathname);
  if(!has_slash)strcat(eventdefs,"/");
  strcat(eventdefs,"eventdefs/");

  strcpy(info,pathname);
  if(!has_slash)strcat(info,"/");
  strcat(info,"info/");

  strcpy(control,pathname);
  if(!has_slash)strcat(control,"/");
  strcat(control,"control/");

  strcpy(cpu,pathname);
  if(!has_slash)strcat(cpu,"/");
  strcat(cpu,"cpu/");

  //new trace
  t               = g_new(LttTrace, 1);
  sys_description = g_new(LttSystemDescription, 1);  
  t->pathname     = g_strdup(pathname);
  t->facility_number          = 0;
  t->control_tracefile_number = 0;
  t->per_cpu_tracefile_number = 0;
  t->system_description = sys_description;
  t->control_tracefiles = g_ptr_array_new();
  t->per_cpu_tracefiles = g_ptr_array_new();
  t->facilities         = g_ptr_array_new();
  getDataEndianType(&(t->my_arch_size), &(t->my_arch_endian));

  //get system description  
  strcpy(tmp,info);
  strcat(tmp,"system.xml");
  getSystemInfo(sys_description, tmp);

  //get control tracefile info
  getControlFileInfo(t,control);

  //get cpu tracefile info
  getCpuFileInfo(t,cpu);

  //get facilities info
  getFacilityInfo(t,eventdefs);
  
  return t;
}

void ltt_trace_close(LttTrace *t)
{
  int i;
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
  g_ptr_array_free(t->control_tracefiles, FALSE);

  //free per_cpu_tracefiles
  for(i=0;i<t->per_cpu_tracefile_number;i++){
    tf = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles,i);
    ltt_tracefile_close(tf);
  }
  g_ptr_array_free(t->per_cpu_tracefiles, FALSE);

  //free facilities
  for(i=0;i<t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities,i);
    ltt_facility_close(f);
  }
  g_ptr_array_free(t->facilities, FALSE);

  g_free(t);
}


/*****************************************************************************
 *Get the system description of the trace
 ****************************************************************************/

LttSystemDescription *ltt_trace_system_description(LttTrace *t)
{
  return t->system_description;
}

/*****************************************************************************
 * The following functions discover the facilities of the trace
 ****************************************************************************/

unsigned ltt_trace_facility_number(LttTrace *t)
{
  return (unsigned)(t->facility_number);
}

LttFacility *ltt_trace_facility_get(LttTrace *t, unsigned i)
{
  return (LttFacility*)g_ptr_array_index(t->facilities, i);
}

/*****************************************************************************
 *Function name
 *    ltt_trace_facility_find : find facilities in the trace
 *Input params
 *    t                       : the trace 
 *    name                    : facility name
 *Output params
 *    position                : position of the facility in the trace
 *Return value
 *                            : the number of facilities
 ****************************************************************************/

unsigned ltt_trace_facility_find(LttTrace *t, char *name, unsigned *position)
{
  int i, count=0;
  LttFacility * f;
  for(i=0;i=t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    if(strcmp(f->name,name)==0){
      count++;
      if(count==1) *position = i;      
    }else{
      if(count) break;
    }
  }
  return count;
}

/*****************************************************************************
 * Functions to discover all the event types in the trace 
 ****************************************************************************/

unsigned ltt_trace_eventtype_number(LttTrace *t)
{
  int i;
  unsigned count = 0;
  LttFacility * f;
  for(i=0;i=t->facility_number;i++){
    f = (LttFacility*)g_ptr_array_index(t->facilities, i);
    count += f->event_number;
  }
  return count;
}

LttFacility * ltt_trace_facility_by_id(LttTrace * trace, unsigned id)
{
  LttFacility * facility;
  int i;
  for(i=0;i<trace->facility_number;i++){
    facility = (LttFacility*) g_ptr_array_index(trace->facilities,i);
    if(id >= facility->base_id && 
       id < facility->base_id + facility->event_number)
      break;
  }
  if(i==trace->facility_number) return NULL;
  else return facility;
}

LttEventType *ltt_trace_eventtype_get(LttTrace *t, unsigned evId)
{
  LttFacility * f;
  f = ltt_trace_facility_by_id(t,evId);
  if(!f) return NULL;
  return f->events[evId - f->base_id];
}

/*****************************************************************************
 *There is one "per cpu" tracefile for each CPU, numbered from 0 to
 *the maximum number of CPU in the system. When the number of CPU installed
 *is less than the maximum, some positions are unused. There are also a
 *number of "control" tracefiles (facilities, interrupts...). 
 ****************************************************************************/
unsigned ltt_trace_control_tracefile_number(LttTrace *t)
{
  return t->control_tracefile_number;
}

unsigned ltt_trace_per_cpu_tracefile_number(LttTrace *t)
{
  return t->per_cpu_tracefile_number;
}

/*****************************************************************************
 *It is possible to search for the tracefiles by name or by CPU position.
 *The index within the tracefiles of the same type is returned if found
 *and a negative value otherwise. 
 ****************************************************************************/

int ltt_trace_control_tracefile_find(LttTrace *t, char *name)
{
  LttTracefile * tracefile;
  int i;
  for(i=0;i<t->control_tracefile_number;i++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->control_tracefiles, i);
    if(strcmp(tracefile->name, name)==0)break;
  }
  if(i == t->control_tracefile_number) return -1;
  return i;
}

int ltt_trace_per_cpu_tracefile_find(LttTrace *t, unsigned i)
{
  LttTracefile * tracefile;
  int j, name;
  for(j=0;j<t->per_cpu_tracefile_number;j++){
    tracefile = (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, j);
    name = atoi(tracefile->name);
    if(name == (int)i)break;
  }
  if(j == t->per_cpu_tracefile_number) return -1;
  return j;
}

/*****************************************************************************
 *Get a specific tracefile 
 ****************************************************************************/

LttTracefile *ltt_trace_control_tracefile_get(LttTrace *t, unsigned i)
{
  return (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, i);  
}

LttTracefile *ltt_trace_per_cpu_tracefile_get(LttTrace *t, unsigned i)
{
  return (LttTracefile*)g_ptr_array_index(t->per_cpu_tracefiles, i);
}

/*****************************************************************************
 *Get the name of a tracefile
 ****************************************************************************/

char *ltt_tracefile_name(LttTracefile *tf)
{
  return tf->name;
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_seek_time: seek to the first event of the trace with time 
 *                             larger or equal to time
 *Input params
 *    t                      : tracefile
 *    time                   : criteria of the time
 ****************************************************************************/

void ltt_tracefile_seek_time(LttTracefile *t, LttTime time)
{
  int err;
  LttTime lttTime;
  int headTime = timecmp(&(t->a_block_start->time), &time);
  int tailTime = timecmp(&(t->a_block_end->time), &time);
  
  if(headTime < 0 && tailTime > 0){
    lttTime = getEventTime(t);
    err = timecmp(&lttTime, &time);
    if(err > 0){
      if(t->which_event==1 || timecmp(&t->prev_event_time,&time)<0){
	return;
      }else{
	updateTracefile(t);
	return ltt_tracefile_seek_time(t, time);
      }
    }else if(err < 0){
      err = t->which_block;
      if(ltt_tracefile_read(t) == NULL){
	g_printf("End of file\n");      
	return;
      }
      if(t->which_block == err)
	return ltt_tracefile_seek_time(t,time);
    }else return;    
  }else if(headTime > 0){
    if(t->which_block == 1){
      updateTracefile(t);      
    }else{
      if( (t->prev_block_end_time.tv_sec == 0 && 
	   t->prev_block_end_time.tv_nsec == 0  ) ||
	   timecmp(&(t->prev_block_end_time),&time) > 0 ){
	err=readBlock(t,t->which_block-1);
	if(err) g_error("Can not read tracefile: %s\n", t->name); 
	return ltt_tracefile_seek_time(t, time) ;
      }else{
	updateTracefile(t);
      }
    }
  }else if(tailTime <= 0){
    if(t->which_block != t->block_number){
      err=readBlock(t,t->which_block+1);
      if(err) g_error("Can not read tracefile: %s\n", t->name); 
    }else {
      g_printf("End of file\n");      
      return;      
    }    
    if(tailTime < 0) return ltt_tracefile_seek_time(t, time);
  }else if(headTime == 0){
    updateTracefile(t);
  }
}

/*****************************************************************************
 *Function name
 *    ltt_tracefile_read : read the next event 
 *Input params
 *    t                  : tracefile
 *Return value
 *    LttEvent *        : an event to be processed
 ****************************************************************************/

LttEvent *ltt_tracefile_read(LttTracefile *t)
{
  LttEvent * lttEvent = (LttEvent *)g_new(LttEvent, 1);
  int err;

  //update the fields of the current event and go to the next event
  err = skipEvent(t);
  if(err == ENOENT) return NULL;
  if(err == ERANGE) g_error("event id is out of range\n");
  if(err)g_error("Can not read tracefile\n");

  lttEvent->event_id = (int)(*(uint16_t *)(t->cur_event_pos));
  if(lttEvent->event_id == TRACE_TIME_HEARTBEAT)
    t->cur_heart_beat_number++;

  t->current_event_time = getEventTime(t);

  lttEvent->time_delta = *(uint32_t*)(t->cur_event_pos + EVENT_ID_SIZE);
  lttEvent->event_time = t->current_event_time;

  lttEvent->event_cycle_count = ((uint64_t)1)<<32  * t->cur_heart_beat_number 
                                + lttEvent->time_delta;

  lttEvent->tracefile = t;
  lttEvent->data = t->cur_event_pos + EVENT_HEADER_SIZE;  

  return lttEvent;
}

/****************************************************************************
 *Function name
 *    readFile    : wrap function to read from a file
 *Input Params
 *    fd          : file descriptor
 *    buf         : buf to contain the content
 *    size        : number of bytes to be read
 *    mesg        : message to be printed if some thing goes wrong
 *return value 
 *    0           : success
 *    EIO         : can not read from the file
 ****************************************************************************/

int readFile(int fd, void * buf, size_t size, char * mesg)
{
   ssize_t nbBytes;
   nbBytes = read(fd, buf, size);
   if(nbBytes != size){
     printf("%s\n",mesg);
     return EIO;
   }
   return 0;
}

/****************************************************************************
 *Function name
 *    readBlock       : read a block from the file
 *Input Params
 *    lttdes          : ltt trace file 
 *    whichBlock      : the block which will be read
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 ****************************************************************************/

int readBlock(LttTracefile * tf, int whichBlock)
{
  off_t nbBytes;
  uint32_t lostSize;

  if(whichBlock - tf->which_block == 1 && tf->which_block != 0){
    tf->prev_block_end_time = tf->a_block_end->time;
  }else{
    tf->prev_block_end_time.tv_sec = 0;
    tf->prev_block_end_time.tv_nsec = 0;
  }
  tf->prev_event_time.tv_sec = 0;
  tf->prev_event_time.tv_nsec = 0;

  nbBytes=lseek(tf->fd,(off_t)((whichBlock-1)*tf->block_size), SEEK_SET);
  if(nbBytes == -1) return EINVAL;
  
  if(readFile(tf->fd,tf->buffer,tf->block_size,"Unable to read a block")) 
    return EIO;

  tf->a_block_start=(BlockStart *) (tf->buffer + EVENT_HEADER_SIZE);
  lostSize = *(uint32_t*)(tf->buffer + tf->block_size - sizeof(uint32_t));
  tf->a_block_end=(BlockEnd *)(tf->buffer + tf->block_size - 
				lostSize + EVENT_HEADER_SIZE); 

  tf->which_block = whichBlock;
  tf->which_event = 1;
  tf->cur_event_pos = tf->a_block_start + sizeof(BlockStart); //first event
  tf->cur_heart_beat_number = 0;

  tf->current_event_time = getEventTime(tf);
  
  getCyclePerNsec(tf);

  return 0;  
}

/*****************************************************************************
 *Function name
 *    updateTracefile : reinitialize the info of the block which is already 
 *                      in the buffer
 *Input params 
 *    tf              : tracefile
 ****************************************************************************/

void updateTracefile(LttTracefile * tf)
{
  tf->which_event = 1;
  tf->cur_event_pos = tf->a_block_start + sizeof(BlockStart);
  tf->current_event_time = getEventTime(tf);  
  tf->cur_heart_beat_number = 0;

  tf->prev_event_time.tv_sec = 0;
  tf->prev_event_time.tv_nsec = 0;
}

/*****************************************************************************
 *Function name
 *    skipEvent : go to the next event, update the fields of the current event
 *Input params 
 *    t         : tracefile
 *return value 
 *    0               : success
 *    EINVAL          : lseek fail
 *    EIO             : can not read from the file
 *    ENOENT          : end of file
 *    ERANGE          : event id is out of range
 ****************************************************************************/

int skipEvent(LttTracefile * t)
{
  int evId, err;
  void * evData;
  LttEventType * evT;
  LttField * rootFld;

  evId   = (int)(*(uint16_t *)(t->cur_event_pos));
  evData = t->cur_event_pos + EVENT_HEADER_SIZE;
  evT    = ltt_trace_eventtype_get(t->trace,(unsigned)evId);

  if(evT) rootFld = evT->root_field;
  else return ERANGE;

  t->prev_event_time = getEventTime(t);
  
  //event has string/sequence or the last event is not the same event
  if((evT->latest_block!=t->which_block || evT->latest_event!=t->which_event) 
     && rootFld->field_fixed == 0){
    setFieldsOffset(t, evT, evData);
  }
  t->cur_event_pos += EVENT_HEADER_SIZE + rootFld->field_size;

  evT->latest_block = t->which_block;
  evT->latest_event = t->which_event;
  
  //the next event is in the next block
  if(evId == TRACE_BLOCK_END){
    if(t->which_block == t->block_number) return ENOENT;
    err = readBlock(t, t->which_block + 1);
    if(err) return err;
  }else{
    t->which_event++;
  }

  return 0;
}

/*****************************************************************************
 *Function name
 *    getCyclePerNsec : calculate cycles per nsec for current block
 *Input Params
 *    t               : tracefile
 ****************************************************************************/

void getCyclePerNsec(LttTracefile * t)
{
  LttTime           lBufTotalTime; /* Total time for this buffer */
  LttCycleCount     lBufTotalNSec; /* Total time for this buffer in nsecs */
  LttCycleCount     lBufTotalCycle;/* Total cycles for this buffer */

  /* Calculate the total time for this buffer */
  TimeSub(lBufTotalTime,t->a_block_end->time, t->a_block_start->time);

  /* Calculate the total cycles for this bufffer */
  lBufTotalCycle = t->a_block_end->cycle_count 
                   - t->a_block_start->cycle_count;

  /* Convert the total time to nsecs */
  lBufTotalNSec = lBufTotalTime.tv_sec * 1000000000 + lBufTotalTime.tv_nsec;
  
  t->cycle_per_nsec = (double)lBufTotalCycle / (double)lBufTotalNSec;
}

/****************************************************************************
 *Function name
 *    getEventTime    : obtain the time of an event 
 *Input params 
 *    tf              : tracefile
 *Return value
 *    LttTime        : the time of the event
 ****************************************************************************/

LttTime getEventTime(LttTracefile * tf)
{
  LttTime       time;
  LttCycleCount cycle_count;      // cycle count for the current event
  LttCycleCount lEventTotalCycle; // Total cycles from start for event
  double        lEventNSec;       // Total usecs from start for event
  LttTime       lTimeOffset;      // Time offset in struct LttTime
  
  // Calculate total time in cycles from start of buffer for this event 
  cycle_count = (LttCycleCount)*(uint32_t*)(tf->cur_event_pos + EVENT_ID_SIZE);
  if(tf->cur_heart_beat_number)
    cycle_count += ((uint64_t)1)<<32  * tf->cur_heart_beat_number;
  lEventTotalCycle = cycle_count - tf->a_block_start->cycle_count;

  // Convert it to nsecs
  lEventNSec = lEventTotalCycle / tf->cycle_per_nsec;
  
  // Determine offset in struct LttTime 
  lTimeOffset.tv_nsec = (long)lEventNSec % 1000000000;
  lTimeOffset.tv_sec  = (long)lEventNSec / 1000000000;

  TimeAdd(time, tf->a_block_start->time, lTimeOffset);  

  return time;
}

/*****************************************************************************
 *Function name
 *    setFieldsOffset : set offset of the fields
 *Input params 
 *    tracefile       : opened trace file  
 *    evT             : the event type
 *    evD             : event data, it may be NULL
 ****************************************************************************/

void setFieldsOffset(LttTracefile * t, LttEventType * evT, void * evD)
{
  LttField * rootFld = evT->root_field;
  //  rootFld->base_address = evD;

  rootFld->field_size = getFieldtypeSize(t, evT, 0,0,rootFld, evD);  
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

int getFieldtypeSize(LttTracefile * t, LttEventType * evT, int offsetRoot,
		     int offsetParent, LttField * fld, void * evD)
{
  int size, size1, element_number, i, offset1, offset2;
  LttType * type = fld->field_type;

  if(!t){
    if(evT->latest_block==t->which_block && evT->latest_event==t->which_event){
      return fld->field_size;
    } 
  }

  if(fld->field_fixed == 1){
    if(fld == evT->root_field) return fld->field_size;
  }     

  if(type->type_class != LTT_STRUCT && type->type_class != LTT_ARRAY &&
     type->type_class != LTT_SEQUENCE && type->type_class != LTT_STRING){
    if(fld->field_fixed == -1){
      size = (int) ltt_type_size(t->trace, type);
      fld->field_fixed = 1;
    }else size = fld->field_size;

  }else if(type->type_class == LTT_ARRAY){
    element_number = (int) type->element_number;
    if(fld->field_fixed == -1){
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL);
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
				fld->child[0], evD+size);
      }      
    }else size = fld->field_size;

  }else if(type->type_class == LTT_SEQUENCE){
    size1 = (int) ltt_type_size(t->trace, type);
    if(fld->field_fixed == -1){
      fld->field_fixed = 0;
      size = getFieldtypeSize(t, evT, offsetRoot,0,fld->child[0], NULL);      
      fld->element_size = size;
    }else{//0: sequence
      element_number = getIntNumber(size1,evD);
      type->element_number = element_number;
      if(fld->element_size > 0){
	size = element_number * fld->element_size;
      }else{//sequence has string or sequence
	size = 0;
	for(i=0;i<element_number;i++){
	  size += getFieldtypeSize(t, evT, offsetRoot+size+size1,size+size1, 
				   fld->child[0], evD+size+size1);
	}	
      }
      size += size1;
    }

  }else if(type->type_class == LTT_STRING){
    size = 0;
    if(fld->field_fixed == -1){
      fld->field_fixed = 0;
    }else{//0: string
      size = sizeof((char*)evD) + 1; //include end : '\0'
    }

  }else if(type->type_class == LTT_STRUCT){
    element_number = (int) type->element_number;
    size = 0;
    if(fld->field_fixed == -1){      
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size1=getFieldtypeSize(t, evT,offset1,offset2, fld->child[i], NULL);
	if(size1 > 0 && size >= 0){
	  size += size1;
	  if(offset1 >= 0) offset1 += size1;
	  offset2 += size1;
	}else{
	  size = -1;
	  offset1 = -1;
	  offset2 = -1;
	}
      }
      if(size == -1){
	fld->field_fixed = 0;
	size = 0;
      }else fld->field_fixed = 1;
    }else if(fld->field_fixed == 0){
      offset1 = offsetRoot;
      offset2 = 0;
      for(i=0;i<element_number;i++){
	size=getFieldtypeSize(t,evT,offset1,offset2,fld->child[i],evD+offset2);
	offset1 += size;
	offset2 += size;
      }      
      size = offset2;
    }else size = fld->field_size;
  }

  fld->offset_root     = offsetRoot;
  fld->offset_parent   = offsetParent;
  if(!evD){
    fld->fixed_root    = (offsetRoot==-1)   ? 0 : 1;
    fld->fixed_parent  = (offsetParent==-1) ? 0 : 1;
  }
  fld->field_size      = size;

  return size;
}

/*****************************************************************************
 *Function name
 *    timecmp   : compare two time
 *Input params 
 *    t1        : first time
 *    t2        : second time
 *Return value
 *    int       : 0: t1 == t2; -1: t1 < t2; 1: t1 > t2
 ****************************************************************************/

int timecmp(LttTime * t1, LttTime * t2)
{
  LttTime T;
  TimeSub(T, *t1, *t2);
  if(T.tv_sec == 0 && T.tv_nsec == 0) return 0;
  else if(T.tv_sec > 0 || (T.tv_sec==0 && T.tv_nsec > 0)) return 1;
  else return -1;
}

/*****************************************************************************
 *Function name
 *    getIntNumber    : get an integer number
 *Input params 
 *    size            : the size of the integer
 *    evD             : the event data
 *Return value
 *    int             : an integer
 ****************************************************************************/

int getIntNumber(int size, void *evD)
{
  int64_t i;
  if(size == 1)      i = *(int8_t *)evD;
  else if(size == 2) i = *(int16_t *)evD;
  else if(size == 4) i = *(int32_t *)evD;
  else if(size == 8) i = *(int64_t *)evD;
 
  return (int) i;
}

/*****************************************************************************
 *Function name
 *    getDataEndianType : get the data type size and endian type of the local
 *                        machine
 *Input params 
 *    size              : size of data type
 *    endian            : endian type, little or big
 ****************************************************************************/

void getDataEndianType(LttArchSize * size, LttArchEndian * endian)
{
  int i = 1;
  char c = (char) i;
  int sizeInt=sizeof(int), sizeLong=sizeof(long), sizePointer=sizeof(void *);

  if(c == 1) *endian = LTT_LITTLE_ENDIAN;
  else *endian = LTT_BIG_ENDIAN;

  if(sizeInt == 2 && sizeLong == 4 && sizePointer == 4) 
    *size = LTT_LP32;
  else if(sizeInt == 4 && sizeLong == 4 && sizePointer == 4) 
    *size = LTT_ILP32;
  else if(sizeInt == 4 && sizeLong == 8 && sizePointer == 8) 
    *size = LTT_LP64;
  else if(sizeInt == 8 && sizeLong == 8 && sizePointer == 8) 
    *size = LTT_ILP64;
  else *size = LTT_UNKNOWN;
}

