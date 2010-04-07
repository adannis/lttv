/* This file is part of the Linux Trace Toolkit viewer
 * Copyright (C) 2009, 2010 Benjamin Poirier <benjamin.poirier@polymtl.ca>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sync_chain.h"


GQueue processingModules= G_QUEUE_INIT;
GQueue matchingModules= G_QUEUE_INIT;
GQueue analysisModules= G_QUEUE_INIT;
GQueue reductionModules= G_QUEUE_INIT;
GQueue moduleOptions= G_QUEUE_INIT;


/*
 * Call the statistics function of each module of a sync chain
 *
 * Args:
 *   syncState:    Container for synchronization data
 */
void printStats(SyncState* const syncState)
{
	if (syncState->processingModule->printProcessingStats != NULL)
	{
		syncState->processingModule->printProcessingStats(syncState);
	}
	if (syncState->matchingModule != NULL &&
		syncState->matchingModule->printMatchingStats != NULL)
	{
		syncState->matchingModule->printMatchingStats(syncState);
	}
	if (syncState->analysisModule != NULL &&
		syncState->analysisModule->printAnalysisStats != NULL)
	{
		syncState->analysisModule->printAnalysisStats(syncState);
	}
	if (syncState->reductionModule != NULL &&
		syncState->reductionModule->printReductionStats != NULL)
	{
		syncState->reductionModule->printReductionStats(syncState);
	}
}


/*
 * Calculate the elapsed time between two timeval values
 *
 * Args:
 *   end:          end time, result is also stored in this structure
 *   start:        start time
 */
void timeDiff(struct timeval* const end, const struct timeval* const start)
{
		if (end->tv_usec >= start->tv_usec)
		{
			end->tv_sec-= start->tv_sec;
			end->tv_usec-= start->tv_usec;
		}
		else
		{
			end->tv_sec= end->tv_sec - start->tv_sec - 1;
			end->tv_usec= end->tv_usec - start->tv_usec + 1e6;
		}
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            ProcessingModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the processing module a's name is b
 */
gint gcfCompareProcessing(gconstpointer a, gconstpointer b)
{
	const ProcessingModule* processingModule;
	const char* name;

	processingModule= (const ProcessingModule*) a;
	name= (const char*) b;

	return strncmp(processingModule->name, name,
		strlen(processingModule->name) + 1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            MatchingModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the matching module a's name is b
 */
gint gcfCompareMatching(gconstpointer a, gconstpointer b)
{
	const MatchingModule* matchingModule;
	const char* name;

	matchingModule= (const MatchingModule*) a;
	name= (const char*) b;

	return strncmp(matchingModule->name, name, strlen(matchingModule->name) +
		1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            AnalysisModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the analysis module a's name is b
 */
gint gcfCompareAnalysis(gconstpointer a, gconstpointer b)
{
	const AnalysisModule* analysisModule;
	const char* name;

	analysisModule= (const AnalysisModule*) a;
	name= (const char*) b;

	return strncmp(analysisModule->name, name, strlen(analysisModule->name) +
		1);
}


/*
 * A GCompareFunc for g_slist_find_custom()
 *
 * Args:
 *   a:            ReductionModule*, element's data
 *   b:            char*, user data to compare against
 *
 * Returns:
 *   0 if the reduction module a's name is b
 */
gint gcfCompareReduction(gconstpointer a, gconstpointer b)
{
	const ReductionModule* reductionModule;
	const char* name;

	reductionModule= (const ReductionModule*) a;
	name= (const char*) b;

	return strncmp(reductionModule->name, name, strlen(reductionModule->name) +
		1);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Concatenate analysis module names.
 *
 * Args:
 *   data:         AnalysisModule*
 *   user_data:    GString*, concatenated names
 */
void gfAppendAnalysisName(gpointer data, gpointer user_data)
{
	g_string_append((GString*) user_data, ((AnalysisModule*) data)->name);
	g_string_append((GString*) user_data, ", ");
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Concatenate reduction module names.
 *
 * Args:
 *   data:         ReductionModule*
 *   user_data:    GString*, concatenated names
 */
void gfAppendReductionName(gpointer data, gpointer user_data)
{
	g_string_append((GString*) user_data, ((ReductionModule*) data)->name);
	g_string_append((GString*) user_data, ", ");
}
