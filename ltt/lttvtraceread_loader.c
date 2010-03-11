
/* Important to get consistent size_t type */
#define _FILE_OFFSET_BITS 64

#include <jni.h>
#include <ltt/trace.h>
#include <ltt/time.h>
#include <ltt/marker.h>
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>

/* Static handle need to initialized before use */ 
void *static_handle = NULL;

struct function_tables {
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC)(JNIEnv *env, jobject jobj, jstring new_string) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace)(JNIEnv *env, jobject jobj, jstring pathname, jboolean show_debug) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace)(JNIEnv *env, jobject jobj, jlong trace_ptr);
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange)(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject jstart_time, jobject jend_time) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace)(JNIEnv *env, jobject jobj, jlong trace_ptr) ;
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jboolean JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent)(JNIEnv *env, jobject jobj, jlong tracefile_ptr, jobject time_jobj) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime)(JNIEnv *env, jobject jobj, jlong event_ptr, jobject time_jobj) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent)(JNIEnv *env, jobject jobj, jlong event_ptr, jlong data_size, jbyteArray dataArray) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent)(JNIEnv *env, jobject jobj, jlong event_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jshort JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker)(JNIEnv *env, jobject jobj, jlong marker_info_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jlong JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jint JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT jstring JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField)(JNIEnv *env, jobject jobj, jlong marker_field_ptr) ;
        JNIEXPORT void JNICALL (*Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData)(JNIEnv *env, jclass accessClass, jobject javaObj, jlong event_ptr, jlong marker_field_ptr) ;
};

struct function_tables allFunctions;

void ignore_and_drop_message(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data) {
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_factory_JniTraceVersion_ltt_1getTraceVersion(JNIEnv *env, jobject jobj, jstring tracepath) {
        
        void *handle = dlopen("liblttvtraceread.so", RTLD_LAZY );
        
        if (!handle ) {
                printf ("WARNING : Failed to initialize library handle from %s!\n", "liblttvtraceread.so");
        }
        JNIEXPORT void JNICALL (*functor)(JNIEnv *env, jobject jobj, jstring tracepath);
        functor=dlsym(handle, "Java_org_eclipse_linuxtools_lttng_jni_factory_JniTraceVersion_ltt_1getTraceVersion");
        
        char *error = dlerror();
        if ( error != NULL)  {
                printf ("Call failed with : %s\n", error);
        }
        else {
                (*functor)(env, jobj, tracepath);
        }
}

void freeHandle() {
        if (static_handle != NULL) {
                /* Memory will be freed by dlclose as well */
                dlclose(static_handle);
                static_handle = NULL;
        }
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1freeHandle(JNIEnv *env, jobject jobj) {
        // Call function to free the memory
        freeHandle();
}

JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1initializeHandle(JNIEnv *env, jobject jobj, jstring libname) {
        
        jboolean result = TRUE;
        const char* c_path = (*env)->GetStringUTFChars(env, libname, 0);
        
        /* ***FIXME *** */
        /* If we free the memory here, we might crash if java is multithreading its call */
        /* We will trust java to call the free by itself (hope!)*/
        //freeHandle();
       
        static_handle = dlopen(c_path, RTLD_LAZY );
        
        if (!static_handle ) {
                printf ("WARNING : Failed to initialize library handle from %s!\n", c_path);
                result = FALSE;
        }
        else {
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField");
                allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData = dlsym(static_handle, "Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData");
        }
        
        return result;
}

JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC(JNIEnv *env, jobject jobj, jstring new_string)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_Jni_1C_1Common_ltt_1printC)(env, jobj, new_string) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace(JNIEnv *env, jobject jobj, jstring pathname, jboolean show_debug)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1openTrace)(env, jobj, pathname, show_debug) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace(JNIEnv *env, jobject jobj, jlong trace_ptr) {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1closeTrace)(env, jobj, trace_ptr);
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getTracepath)(env, jobj, trace_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getCpuNumber)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchType)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchVariant)(env, jobj, trace_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getArchSize)(env, jobj, trace_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMajorVersion)(env, jobj, trace_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getLttMinorVersion)(env, jobj, trace_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFlightRecorder)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getFreqScale)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartFreq)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartTimestampCurrentCounter)(env, jobj, trace_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1getStartMonotonic)(env, jobj, trace_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTime)(env, jobj, trace_ptr, time_jobj) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject time_jobj)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedStartTimeFromTimestampCurrentCounter)(env, jobj, trace_ptr, time_jobj) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedAllTracefiles)(env, jobj, trace_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange(JNIEnv *env, jobject jobj, jlong trace_ptr, jobject jstart_time, jobject jend_time)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1feedTracefileTimeRange)(env, jobj, trace_ptr, jstart_time, jend_time) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace(JNIEnv *env, jobject jobj, jlong trace_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTrace_ltt_1printTrace)(env, jobj, trace_ptr) ;
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsCpuOnline)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilepath)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracefilename)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCpuNumber)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTid)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getPgid)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCreation)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getTracePtr)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getMarkerDataPtr)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCFileDescriptor)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getFileSize)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBlockNumber)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsBytesOrderReversed)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jboolean JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getIsFloatWordOrdered)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getAlignement)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferHeaderSize)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfCurrentTimestampCounter)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBitsOfEvent)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMask)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getCurrentTimestampCounterMaskNextBit)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventsLost)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getSubBufferCorrupt)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getEventPtr)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferPtr)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1getBufferSize)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1feedAllMarkers)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniTracefile_ltt_1printTracefile)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1positionToFirstEvent)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1readNextEvent)(env, jobj, tracefile_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent(JNIEnv *env, jobject jobj, jlong tracefile_ptr, jobject time_jobj)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1seekEvent)(env, jobj, tracefile_ptr, time_jobj) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTracefilePtr)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getBlock)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOffset)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCurrentTimestampCounter)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getTimestamp)(env, jobj, event_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventMarkerId)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getNanosencondsTime)(env, jobj, event_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime(JNIEnv *env, jobject jobj, jlong event_ptr, jobject time_jobj)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1feedEventTime)(env, jobj, event_ptr, time_jobj) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent(JNIEnv *env, jobject jobj, jlong event_ptr, jlong data_size, jbyteArray dataArray)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getDataContent)(env, jobj, event_ptr, data_size, dataArray) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventDataSize)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getEventSize)(env, jobj, event_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getCount)(env, jobj, event_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1getOverflowNanoSeconds)(env, jobj, event_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent(JNIEnv *env, jobject jobj, jlong event_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniEvent_ltt_1printEvent)(env, jobj, event_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getName)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getFormatOverview)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAllMarkerFields)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLargestAlign)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getIntSize)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getLongSize)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getPointerSize)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getSize_1tSize)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jshort JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getAlignement)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1getNextMarkerPtr)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker(JNIEnv *env, jobject jobj, jlong marker_info_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarker_ltt_1printMarker)(env, jobj, marker_info_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getField)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getType)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getOffset)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getSize)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAlignment)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jlong JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getAttributes)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jint JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getStatic_1offset)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT jstring JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        return (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1getFormat)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField(JNIEnv *env, jobject jobj, jlong marker_field_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniMarkerField_ltt_1printMarkerField)(env, jobj, marker_field_ptr) ;
}


JNIEXPORT void JNICALL Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData(JNIEnv *env, jclass accessClass, jobject javaObj, jlong event_ptr, jlong marker_field_ptr)  {
        (*allFunctions.Java_org_eclipse_linuxtools_lttng_jni_JniParser_ltt_1getParsedData)(env, accessClass, javaObj, event_ptr, marker_field_ptr) ;
}


