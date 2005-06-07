/*

genevent.c: Generate helper declarations and functions to trace events
  from an event description file.

Copyright (C) 2002, Xianxiu Yang
Copyright (C) 2002, Michel Dagenais
 
This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

/* This program reads the ".event" event definitions input files 
   specified as command line arguments and generates corresponding
   ".c" and ".h" files required to trace such events in the kernel.
 
   The program uses a very simple tokenizer, called from a hand written
   recursive descent parser to fill a data structure describing the events.
   The result is a sequence of events definitions which refer to type
   definitions.

   A table of named types is maintained to allow refering to types by name
   when the same type is used at several places. Finally a sequence of
   all types is maintained to facilitate the freeing of all type 
   information when the processing of an ".event" file is finished. */

#include <stdlib.h> 
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <linux/errno.h>  
#include <assert.h>

#include "parser.h"
#include "genevent.h"

/* Named types may be referenced from anywhere */

facility * fac;

int main(int argc, char** argv)
{
  char *token;
  parse_file in;
  char buffer[BUFFER_SIZE];
  int i;

  if(argc < 2){
    printf("At least one event definition file is needed\n");
    exit(1);
  }

  in.buffer = buffer;
  in.error = error_callback;

  for(i = 1 ; i < argc ; i++) {
    in.lineno = 0;
    in.name = allocAndCopy(argv[i]);

    in.fp = fopen(in.name, "r");
    if(!in.fp ){
      in.error(&in,"cannot open facility input file");
    }

    while(1){
      token = getToken(&in);
      if(in.type == ENDFILE) break;

      if(strcmp(token, "<")) in.error(&in,"not a facility file");
      token = getName(&in);
    
      if(strcmp("facility",token) == 0) {
				fac = memAlloc(sizeof(facility));
				fac->name = NULL;
				fac->description = NULL;
				sequence_init(&(fac->events));
				table_init(&(fac->named_types));
				sequence_init(&(fac->unnamed_types));
				
				parseFacility(&in, fac);
				
				//check if any namedType is not defined
				checkNamedTypesImplemented(&fac->named_types);
      }
			else in.error(&in,"facility token was expected");

      generateFile(argv[i]);

      free(fac->name);
      free(fac->description);
      freeEvents(&fac->events);
      sequence_dispose(&fac->events);
      freeNamedType(&fac->named_types);
      table_dispose(&fac->named_types);
      freeTypes(&fac->unnamed_types);
      sequence_dispose(&fac->unnamed_types);      
      free(fac);      
    }

    free(in.name);
    fclose(in.fp);

  }
  return 0;
}


/*****************************************************************************
 *Function name
 *    generateFile : generate .c and .h file 
 *Input Params
 *    name         : name of event definition file
 ****************************************************************************/
void generateFile(char *name){
  char *loadName, *hName, *tmp, *tmp2;
  FILE * lFp, *hFp;
  int nbEvent;
  unsigned long checksum=0;

  //remove .xml if it exists
  tmp = &name[strlen(name)-4];
  if(strcmp(tmp, ".xml") == 0){
    *tmp = '\0';
  }

  tmp = strrchr(name,'/');
  if(tmp){
    tmp++;
  }else{
    tmp = name;
  }

  loadName = appendString("ltt-facility-loader-", tmp);
  tmp2 = appendString(loadName,".h");
  free(loadName);
  loadName = tmp2;
  hName = appendString("ltt-facility-", tmp);
  tmp2 = appendString(hName,".h");
  free(hName);
  hName = tmp2;
  lFp = fopen(loadName,"w");
  if(!lFp){
    printf("Cannot open the file : %s\n",loadName);
    exit(1);
  }

  hFp = fopen(hName,"w");
  if(!hFp){
     printf("Cannot open the file : %s\n",hName);
     exit(1);
  }
  
  free(loadName);
  free(hName);

  generateChecksum(fac->name, &checksum, &(fac->events));

  /* generate .h file, event enumeration then structures and functions */
	fprintf(hFp, "#ifndef _LTT_FACILITY_%s_H_\n",fac->capname);
	fprintf(hFp, "#define _LTT_FACILITY_%s_H_\n\n",fac->capname);
  generateEnumEvent(hFp, fac->name, &nbEvent, checksum);
  generateTypeDefs(hFp);
  generateStructFunc(hFp, fac->name,checksum);
	fprintf(hFp, "#endif //_LTT_FACILITY_%s_H_\n",fac->capname);

  /* generate .h file, calls to register the facility at init time */
  generateLoaderfile(lFp,fac->name,nbEvent,checksum,fac->capname);

  fclose(hFp);
  fclose(lFp);
}


/*****************************************************************************
 *Function name
 *    generateEnumEvent : output event enum to .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 *Output Params
 *    nbEvent           : number of events in the facility
 ****************************************************************************/
void generateEnumEvent(FILE *fp, char *facName, int * nbEvent, unsigned long checksum) {
  int pos = 0;

  fprintf(fp,"#include <linux/ltt-log.h>\n\n");

  fprintf(fp,"/****  facility handle  ****/\n\n");
  fprintf(fp,"extern trace_facility_t ltt_facility_%s_%X;\n",facName, checksum);
  fprintf(fp,"extern trace_facility_t ltt_facility_%s;\n\n\n",facName, checksum);

  fprintf(fp,"/****  event type  ****/\n\n");
  fprintf(fp,"enum %s_event {\n",facName);

  for(pos = 0; pos < fac->events.position;pos++) {
    fprintf(fp,"\t%s", ((event *)(fac->events.array[pos]))->name);
    if(pos != fac->events.position-1) fprintf(fp,",\n");
  }
  fprintf(fp,"\n};\n\n\n");

  //  fprintf(fp,"/****  number of events in the facility  ****/\n\n");
  //  fprintf(fp,"int nbEvents_%s = %d;\n\n\n",facName, fac->events.position);
  *nbEvent = fac->events.position;
}


/*****************************************************************************
 *Function name
 *    printStruct       : Generic struct printing function
 *Input Params
 *    fp                : file to be written to
 *    len               : number of fields
 *    array             : array of field info
 *    name              : basic struct name
 *    facName           : name of facility
 *    whichTypeFirst    : struct or array/sequence first
 *    hasStrSeq         : string or sequence present?
 *    structCount       : struct postfix
 ****************************************************************************/

static void
printStruct(FILE * fp, int len, void ** array, char * name, char * facName,
	     int * whichTypeFirst, int * hasStrSeq, int * structCount)
{
  int flag = 0;
  int pos;
  field * fld;
  type_descriptor * td;

  for (pos = 0; pos < len; pos++) {
    fld  = (field *)array[pos];
    td = fld->type;
    if( td->type != STRING && td->type != SEQUENCE &&
	      td->type != ARRAY) {
      if (*whichTypeFirst == 0) {
        *whichTypeFirst = 1; //struct first
      }
      if (flag == 0) {
        flag = 1;

        fprintf(fp,"struct %s_%s",name, facName);
        if (structCount) {
	        fprintf(fp, "_%d {\n",++*structCount);
        } else {
          fprintf(fp, " {\n");
        }
      }
      fprintf(fp, "\t%s %s; /* %s */\n", 
          getTypeStr(td),fld->name,fld->description );
    } else {
        if (*whichTypeFirst == 0) {
        //string or sequence or array first
          *whichTypeFirst = 2;
        }
        (*hasStrSeq)++;
        if(flag) {
          fprintf(fp,"} __attribute__ ((packed));\n\n");
        }
        flag = 0;
    }
  }

  if(flag) {
    fprintf(fp,"} __attribute__ ((packed));\n\n");
  }
}


/*****************************************************************************
 *Function name
 *    generateHfile     : Create the typedefs
 *Input Params
 *    fp                : file to be written to
 ****************************************************************************/
void
generateTypeDefs(FILE * fp)
{
  int pos, tmp = 1;

  fprintf(fp, "/****  Basic Type Definitions  ****/\n\n");

  for (pos = 0; pos < fac->named_types.values.position; pos++) {
    type_descriptor * type =
      (type_descriptor*)fac->named_types.values.array[pos];
    printStruct(fp, type->fields.position, type->fields.array,
                "", type->type_name, &tmp, &tmp, NULL);
    fprintf(fp, "typedef struct _%s %s;\n\n",
            type->type_name, type->type_name);
  }
}


/*****************************************************************************
 *Function name
 *    generateEnumDefinition: generate enum definition if it exists 
 *Input Params
 *    fp                    : file to be written to
 *    fHead                 : enum type
 ****************************************************************************/
void generateEnumDefinition(FILE * fp, type_descriptor * type){
  int pos;

	if(type->already_printed) return;
	
  fprintf(fp,"enum {\n");
  for(pos = 0; pos < type->labels.position; pos++){
    fprintf(fp,"\t%s", type->labels.array[pos]);
    if (pos != type->labels.position - 1) fprintf(fp,",");
		if(type->labels_description.array[pos] != NULL)
			fprintf(fp,"\t/* %s */\n",type->labels_description.array[pos]);
		else
			fprintf(fp,"\n");
  }
  fprintf(fp,"};\n\n\n");

	type->already_printed = 1;
}

/*****************************************************************************
 *Function name
 *    generateStrucTFunc: output structure and function to .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 ****************************************************************************/
void generateStructFunc(FILE * fp, char * facName, unsigned long checksum){
  event * ev;
  field * fld;
  type_descriptor * td;
  int pos, pos1;
  int hasStrSeq, flag, structCount, seqCount,strCount, whichTypeFirst=0;

  for(pos = 0; pos < fac->events.position; pos++){
    ev = (event *) fac->events.array[pos];
    //yxx    if(ev->nested)continue;
    fprintf(fp,"/****  structure and trace function for event: %s  ****/\n\n",
        ev->name);
    if(ev->type == 0){ // event without type
      fprintf(fp,"static inline void trace_%s_%s(void){\n",facName,ev->name);
      fprintf(fp,"\tltt_log_event(ltt_facility_%s_%X, %s, 0, NULL);\n",
          facName,checksum,ev->name);
      fprintf(fp,"};\n\n\n");
      continue;
    }

    //if fields contain enum, print out enum definition
    //MD : fixed in generateEnumDefinition to do not print the same enum
    //twice.
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld = (field *)ev->type->fields.array[pos1];
      if(fld->type->type == ENUM) generateEnumDefinition(fp, fld->type);      
    }
      
    //default: no string, array or sequence in the event
    hasStrSeq = 0;
    whichTypeFirst = 0;
    structCount = 0;

    //structure for kernel
    printStruct(fp, ev->type->fields.position, ev->type->fields.array,
		  ev->name, facName, &whichTypeFirst, &hasStrSeq, &structCount);

    //trace function : function name and parameters
    seqCount = 0;
    strCount = 0;
    fprintf(fp,"static inline void trace_%s_%s(",facName,ev->name);
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type == ARRAY ){
	      fprintf(fp,"%s * %s",getTypeStr(td), fld->name);
      }else if(td->type == STRING){
       	fprintf(fp,"short int strlength_%d, %s * %s",
            ++strCount, getTypeStr(td), fld->name);
     }else if(td->type == SEQUENCE){
        fprintf(fp,"%s seqlength_%d, %s * %s",
            uintOutputTypes[td->size], ++seqCount,getTypeStr(td), fld->name);
     }else fprintf(fp,"%s %s",getTypeStr(td), fld->name);     
     if(pos1 != ev->type->fields.position - 1) fprintf(fp,", ");
    }
    fprintf(fp,")\n{\n");

    //length of buffer : length of all structures
    fprintf(fp,"\tint length = ");
    for(pos1=0;pos1<structCount;pos1++){
      fprintf(fp,"sizeof(struct %s_%s_%d)",ev->name, facName,pos1+1);
      if(pos1 != structCount-1) fprintf(fp," + ");
    }

    //length of buffer : length of all arrays, sequences and strings
    seqCount = 0;
    strCount = 0;
    flag = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type == SEQUENCE || td->type==STRING || td->type==ARRAY){
      	if(structCount || flag > 0) fprintf(fp," + ");	  
      	if(td->type == SEQUENCE) 
          fprintf(fp,"sizeof(%s) + sizeof(%s) * seqlength_%d",
              uintOutputTypes[td->size], getTypeStr(td), ++seqCount);
      	else if(td->type==STRING) fprintf(fp,"strlength_%d + 1", ++strCount);
      	else if(td->type==ARRAY) 
          fprintf(fp,"sizeof(%s) * %d", getTypeStr(td),td->size);
      	if(structCount == 0) flag = 1;
      }
    }
    fprintf(fp,";\n");

    //allocate buffer
    // MD no more need. fprintf(fp,"\tchar buff[buflength];\n");
    // write directly to the channel
    fprintf(fp, "\tunsigned int index;\n");
    fprintf(fp, "\tstruct ltt_channel_struct *channel;\n");
    fprintf(fp, "\tstruct ltt_trace_struct *trace;\n");
    fprintf(fp, "\tunsigned long flags;\n");
    fprintf(fp, "\tstruct %s_%s_1* __1;\n\n", ev->name, facName);

		fprintf(fp, "\tread_lock(&ltt_traces.traces_rwlock);\n\n");
		fprintf(fp,
				"\tif(ltt_traces.num_active_traces == 0) goto unlock_traces;\n\n");

    fprintf(fp, 
        "\tindex = ltt_get_index_from_facility(ltt_facility_%s_%X,\n"\
						"\t\t\t\t%s);\n",
        facName, checksum, ev->name);
    fprintf(fp,"\n");

    fprintf(fp, "\t/* Disable interrupts. */\n");
    fprintf(fp, "\tlocal_irq_save(flags);\n\n");

		/* For each trace */
    fprintf(fp, "\tlist_for_each_entry(trace, &ltt_traces.head, list) {\n");
    fprintf(fp, "\t\tif(!trace->active) goto skip_trace;\n\n");
		
		fprintf(fp, "\t\tunsigned int header_length = "
								"ltt_get_event_header_size(trace);\n");
		fprintf(fp, "\t\tunsigned int event_length = header_length + length;\n");
   
    /* Reserve the channel */
    fprintf(fp, "\t\tchannel = ltt_get_channel_from_index(trace, index);\n");
    fprintf(fp,
				"\t\tvoid *buff = relay_reserve(channel->rchan, event_length);\n");
		fprintf(fp, "\t\tif(buff == NULL) {\n");
		fprintf(fp, "\t\t\t/* Buffer is full*/\n");
		fprintf(fp, "\t\t\tchannel->events_lost++;\n");
		fprintf(fp, "\t\t\tgoto commit_work;\n");
		fprintf(fp, "\t\t}\n");

		/* Write the header */
		fprintf(fp, "\n");
    fprintf(fp, "\t\tltt_write_event_header(channel, buff, \n"
								"\t\t\t\tltt_facility_%s_%X, %s, length);\n",
								facName, checksum, ev->name);
		fprintf(fp, "\n");
		
    //declare a char pointer if needed : starts at the end of the structs.
    if(structCount + hasStrSeq > 1) {
      fprintf(fp,"\t\tchar * ptr = (char*)buff + header_length");
      for(pos1=0;pos1<structCount;pos1++){
        fprintf(fp," + sizeof(struct %s_%s_%d)",ev->name, facName,pos1+1);
      }
      if(structCount + hasStrSeq > 1) fprintf(fp,";\n");
    }

    // Declare an alias pointer of the struct type to the beginning
    // of the reserved area, just after the event header.
    fprintf(fp, "\t\t__1 = (struct %s_%s_1 *)(buff + header_length);\n",
				ev->name, facName);
    //allocate memory for new struct and initialize it
    //if(whichTypeFirst == 1){ //struct first
      //for(pos1=0;pos1<structCount;pos1++){	
      //	if(pos1==0) fprintf(fp,
      //      "\tstruct %s_%s_1 * __1 = (struct %s_%s_1 *)buff;\n",
      //      ev->name, facName,ev->name, facName);
  //MD disabled      else fprintf(fp,
  //          "\tstruct %s_%s_%d  __%d;\n",
  //          ev->name, facName,pos1+1,pos1+1);
      //}      
    //}else if(whichTypeFirst == 2){
     // for(pos1=0;pos1<structCount;pos1++)
     // 	fprintf(fp,"\tstruct %s_%s_%d  __%d;\n",
     //       ev->name, facName,pos1+1,pos1+1);
    //}
    fprintf(fp,"\n");

    if(structCount) fprintf(fp,"\t\t//initialize structs\n");
    //flag = 0;
    //structCount = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
      if(td->type != ARRAY && td->type != SEQUENCE && td->type != STRING){
      	//if(flag == 0){
      	//  flag = 1;	
      	//  structCount++;
      	//  if(structCount > 1) fprintf(fp,"\n");
      	//}
        fprintf(fp, "\t\t__1->%s = %s;\n", fld->name, fld->name );

	    //if(structCount == 1 && whichTypeFirst == 1)
      //  fprintf(fp, "\t__1->%s =  %s;\n",fld->name,fld->name );
    	//else 
      //  fprintf(fp, "\t__%d.%s =  %s;\n",structCount ,fld->name,fld->name);
      }
      //else flag = 0;
    }
   // if(structCount) fprintf(fp,"\n");
    //set ptr to the end of first struct if needed;
    //if(whichTypeFirst == 1 && structCount + hasStrSeq > 1){
    //  fprintf(fp,"\n\t//set ptr to the end of the first struct\n");
    //  fprintf(fp,"\tptr +=  sizeof(struct %s_%s_1);\n\n",ev->name, facName);
   // }

    //copy struct, sequence and string to buffer
    seqCount = 0;
    strCount = 0;
    flag = 0;
    structCount = 0;
    for(pos1 = 0; pos1 < ev->type->fields.position; pos1++){
      fld  = (field *)ev->type->fields.array[pos1];
      td = fld->type;
//      if(td->type != STRING && td->type != SEQUENCE && td->type != ARRAY){
//				if(flag == 0) structCount++;	
//				flag++;	
//				if((structCount > 1 || whichTypeFirst == 2) && flag == 1){
//				  assert(0); // MD : disabled !
//				  fprintf(fp,"\t//copy struct to buffer\n");
//				  fprintf(fp,"\tmemcpy(ptr, &__%d, sizeof(struct %s_%s_%d));\n",
//							structCount, ev->name, facName,structCount);
//				  fprintf(fp,"\tptr +=  sizeof(struct %s_%s_%d);\n\n",
//							ev->name, facName,structCount);
//				}
 //     }
			//else if(td->type == SEQUENCE){
			if(td->type == SEQUENCE){
      	flag = 0;
      	fprintf(fp,"\t\t//copy sequence length and sequence to buffer\n");
	      fprintf(fp,"\t\t*ptr = seqlength_%d;\n",++seqCount);
  	    fprintf(fp,"\t\tptr += sizeof(%s);\n",uintOutputTypes[td->size]);
    	  fprintf(fp,"\t\tmemcpy(ptr, %s, sizeof(%s) * seqlength_%d);\n",
          fld->name, getTypeStr(td), seqCount);
     	  fprintf(fp,"\t\tptr += sizeof(%s) * seqlength_%d;\n\n",
          getTypeStr(td), seqCount);
      }
      else if(td->type==STRING){
        flag = 0;
        fprintf(fp,"\t\t//copy string to buffer\n");
        fprintf(fp,"\t\tif(strlength_%d > 0){\n",++strCount);
        fprintf(fp,"\t\t\tmemcpy(ptr, %s, strlength_%d + 1);\n",
            fld->name, strCount);
        fprintf(fp,"\t\t\tptr += strlength_%d + 1;\n",strCount);
        fprintf(fp,"\t\t}else{\n");
        fprintf(fp,"\t\t\t*ptr = '\\0';\n");
        fprintf(fp,"\t\t\tptr += 1;\n");
        fprintf(fp,"\t\t}\n\n");
      }else if(td->type==ARRAY){
        flag = 0;
        fprintf(fp,"\t//copy array to buffer\n");
        fprintf(fp,"\tmemcpy(ptr, %s, sizeof(%s) * %d);\n",
            fld->name, getTypeStr(td), td->size);
        fprintf(fp,"\tptr += sizeof(%s) * %d;\n\n", getTypeStr(td), td->size);
      }      
    }    
    if(structCount + seqCount > 1) fprintf(fp,"\n");

    fprintf(fp,"\n");
    fprintf(fp,"commit_work:\n");
    fprintf(fp,"\n");
    fprintf(fp, "\t\t/* Commit the work */\n");
    fprintf(fp, "\t\trelay_commit(channel->rchan, buff, event_length);\n");

   /* End of traces iteration */
    fprintf(fp, "skip_trace:\n\n");
    fprintf(fp, "\t}\n\n");

    fprintf(fp, "\t/* Re-enable interrupts */\n");
    fprintf(fp, "\tlocal_irq_restore(flags);\n");
    fprintf(fp, "\tpreempt_check_resched();\n");
		
		fprintf(fp, "\n");
    fprintf(fp, "unlock_traces:\n");
    fprintf(fp, "\tread_unlock(&ltt_traces.traces_rwlock);\n");
    //call trace function
    //fprintf(fp,"\n\t//call trace function\n");
    //fprintf(fp,"\tltt_log_event(ltt_facility_%s_%X, %s, bufLength, buff);\n",facName,checksum,ev->name);
    fprintf(fp,"};\n\n\n");
  }

}

/*****************************************************************************
 *Function name
 *    getTypeStr        : generate type string 
 *Input Params
 *    td                : a type descriptor
 *Return Values
 *    char *            : type string
 ****************************************************************************/
char * getTypeStr(type_descriptor * td){
  type_descriptor * t ;

  switch(td->type){
    case INT:
      return intOutputTypes[td->size];
    case UINT:
      return uintOutputTypes[td->size];
    case POINTER:
      return "void *";
    case LONG:
      return "long";
    case ULONG:
      return "unsigned long";
    case SIZE_T:
      return "size_t";
    case SSIZE_T:
      return "ssize_t";
    case OFF_T:
      return "off_t";
    case FLOAT:
      return floatOutputTypes[td->size];
    case STRING:
      return "char";
    case ENUM:
      return uintOutputTypes[td->size];
    case ARRAY:
    case SEQUENCE:
      t = td->nested_type;
      switch(t->type){
        case INT:
      	  return intOutputTypes[t->size];
        case UINT:
      	  return uintOutputTypes[t->size];
        case POINTER:
          return "void *";
        case LONG:
          return "long";
        case ULONG:
          return "unsigned long";
        case SIZE_T:
          return "size_t";
        case SSIZE_T:
          return "ssize_t";
        case OFF_T:
          return "off_t";
        case FLOAT:
	        return floatOutputTypes[t->size];
        case STRING:
      	  return "char";
        case ENUM:
      	  return uintOutputTypes[t->size];
        default :
      	  error_callback(NULL,"Nested struct is not supportted");
      	  break;	
      }
      break;
    case STRUCT: //for now we do not support nested struct
      error_callback(NULL,"Nested struct is not supportted");
      break;
    default:
      error_callback(NULL,"No type information");
      break;
  }
  return NULL;
}

/*****************************************************************************
 *Function name
 *    generateLoaderfile: generate a facility loaded .h file 
 *Input Params
 *    fp                : file to be written to
 *    facName           : name of facility
 *    nbEvent           : number of events in the facility
 *    checksum          : checksum for the facility
 ****************************************************************************/
void generateLoaderfile(FILE * fp, char * facName, int nbEvent, unsigned long checksum, char *capname){
	fprintf(fp, "#ifndef _LTT_FACILITY_LOADER_%s_H_\n",capname);
	fprintf(fp, "#define _LTT_FACILITY_LOADER_%s_H_\n\n",capname);
  fprintf(fp,"#include <linux/ltt-facilities.h>\n", facName, checksum);
  fprintf(fp,"#include <linux/module.h>\n\n", facName, checksum);
  fprintf(fp,"ltt_facility_t\tltt_facility_%s;\n", facName, checksum);
  fprintf(fp,"ltt_facility_t\tltt_facility_%s_%X;\n\n", facName, checksum);

  fprintf(fp,"EXPORT_SYMBOL(ltt_facility_%s);\n\n",facName, checksum);
  fprintf(fp,"EXPORT_SYMBOL(ltt_facility_%s_%X);\n\n",facName, checksum);
  fprintf(fp,"#define LTT_FACILITY_SYMBOL\t\t\t\tltt_facility_%s\n",
      facName);
  fprintf(fp,"#define LTT_FACILITY_CHECKSUM_SYMBOL\t\t\t\tltt_facility_%s_%X\n",
      facName, checksum);
  fprintf(fp,"#define LTT_FACILITY_CHECKSUM\t\t\t0x%X\n", checksum);
  fprintf(fp,"#define LTT_FACILITY_NAME\t\t\t\t\t\"%s\"\n", facName);
  fprintf(fp,"#define LTT_FACILITY_NUM_EVENTS\t\t%d\n\n", nbEvent);
	fprintf(fp, "#endif //_LTT_FACILITY_LOADER_%s_H_\n",capname);
}


