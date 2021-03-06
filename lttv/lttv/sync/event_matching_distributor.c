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

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "event_analysis.h"
#include "sync_chain.h"

#include "event_matching_distributor.h"


struct InitAggregate
{
	SyncState* syncState;
	GQueue* matchingModules;
};


struct GraphAggregate
{
	/* Offset whithin Matching module of the field* containing the function
	 * pointer */
	size_t offset;
	unsigned int i, j;
};


// Functions common to all matching modules
static void initMatchingDistributor(SyncState* const syncState);
static void destroyMatchingDistributor(SyncState* const syncState);

static void matchEventDistributor(SyncState* const syncState, Event* const
	event);
static AllFactors* finalizeMatchingDistributor(SyncState* const syncState);
static void printMatchingStatsDistributor(SyncState* const syncState);
static void writeMatchingTraceTraceForePlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeMatchingTraceTraceBackPlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeMatchingTraceTraceOptionsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeMatchingTraceTimeForePlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeMatchingTraceTimeBackPlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);
static void writeMatchingTraceTimeOptionsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j);

// Functions specific to this module
void gfInitModule(gpointer data, gpointer user_data);
void gfDestroyModule(gpointer data, gpointer user_data);
void gfMatchEvent(gpointer data, gpointer user_data);
void gfFinalize(gpointer data, gpointer user_data);
void gfPrintStats(gpointer data, gpointer user_data);
void gfGraphFunctionCall(gpointer data, gpointer user_data);


static MatchingModule matchingModuleDistributor = {
	.name= "distributor",
	.canMatch[TCP]= true,
	.canMatch[UDP]= true,
	.initMatching= &initMatchingDistributor,
	.destroyMatching= &destroyMatchingDistributor,
	.matchEvent= &matchEventDistributor,
	.finalizeMatching= &finalizeMatchingDistributor,
	.printMatchingStats= &printMatchingStatsDistributor,
	.graphFunctions= {
		.writeTraceTraceForePlots=
			&writeMatchingTraceTraceForePlotsDistributor,
		.writeTraceTraceBackPlots=
			&writeMatchingTraceTraceBackPlotsDistributor,
		.writeTraceTraceOptions= &writeMatchingTraceTraceOptionsDistributor,
		.writeTraceTimeForePlots= &writeMatchingTraceTimeForePlotsDistributor,
		.writeTraceTimeBackPlots= &writeMatchingTraceTimeBackPlotsDistributor,
		.writeTraceTimeOptions= &writeMatchingTraceTimeOptionsDistributor,
	},
};


/*
 * Matching module registering function
 */
void registerMatchingDistributor()
{
	g_queue_push_tail(&matchingModules, &matchingModuleDistributor);
}


/*
 * Matching init function
 *
 * This function is called at the beginning of a synchronization run for a set
 * of traces.
 *
 * Build the list and initialize other matching Modules
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void initMatchingDistributor(SyncState* const syncState)
{
	MatchingDataDistributor* matchingData;

	matchingData= malloc(sizeof(MatchingDataDistributor));
	syncState->matchingData= matchingData;

	matchingData->distributedModules= g_queue_new();
	g_queue_foreach(&matchingModules, &gfInitModule, &(struct InitAggregate)
		{syncState, matchingData->distributedModules});
}


/*
 * Matching destroy function
 *
 * Destroy other modules and free the matching specific data structures
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void destroyMatchingDistributor(SyncState* const syncState)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfDestroyModule, NULL);

	g_queue_free(matchingData->distributedModules);
	free(syncState->matchingData);
	syncState->matchingData= NULL;
}



/*
 * Copy event and distribute to matching modules
 *
 * Args:
 *   syncState     container for synchronization data.
 *   event         new event to match
 */
static void matchEventDistributor(SyncState* const syncState, Event* const event)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfMatchEvent, event);
	event->destroy(event);
}


/*
 * Call the distributed finalization functions and return absent factors
 *
 * Args:
 *   syncState     container for synchronization data.
 *
 * Returns:
 *   AllFactors*   synchronization factors for each trace pair
 */
static AllFactors* finalizeMatchingDistributor(SyncState* const syncState)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfFinalize, NULL);

	return createAllFactors(syncState->traceNb);
}


/*
 * Call the distributed statistics functions (when they exist). Must be called
 * after finalizeMatching.
 *
 * Args:
 *   syncState     container for synchronization data.
 */
static void printMatchingStatsDistributor(SyncState* const syncState)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfPrintStats, NULL);
}


/*
 * Call the distributed graph lines functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTraceForePlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTraceForePlots), i, j});
}


/*
 * Call the distributed graph lines functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTraceBackPlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTraceBackPlots), i, j});
}


/*
 * Call the distributed graph lines functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTimeForePlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTimeForePlots), i, j});
}


/*
 * Call the distributed graph lines functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTimeBackPlotsDistributor(SyncState* const
	syncState, const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTimeBackPlots), i, j});
}


/*
 * Call the distributed graph options functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTraceOptionsDistributor(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTraceOptions), i, j});
}


/*
 * Call the distributed graph options functions (when they exist).
 *
 * Args:
 *   syncState:    container for synchronization data
 *   i:            first trace number
 *   j:            second trace number, garanteed to be larger than i
 */
static void writeMatchingTraceTimeOptionsDistributor(SyncState* const syncState,
	const unsigned int i, const unsigned int j)
{
	MatchingDataDistributor* matchingData= syncState->matchingData;

	g_queue_foreach(matchingData->distributedModules, &gfGraphFunctionCall,
		&(struct GraphAggregate) {offsetof(MatchingModule,
			graphFunctions.writeTraceTimeOptions), i, j});
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Add and initialize matching module
 *
 * Args:
 *   data          MatchingModule*, module to add
 *   user_data     InitAggregate*
 */
void gfInitModule(gpointer data, gpointer user_data)
{
	SyncState* parallelSS;
	struct InitAggregate* aggregate= user_data;
	MatchingModule* matchingModule= data;

	if (strcmp(matchingModule->name, matchingModuleDistributor.name) == 0)
	{
		return;
	}

	parallelSS= malloc(sizeof(SyncState));
	memcpy(parallelSS, aggregate->syncState, sizeof(SyncState));
	g_queue_push_tail(aggregate->matchingModules, parallelSS);

	parallelSS->matchingModule= matchingModule;
	parallelSS->matchingModule->initMatching(parallelSS);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Destroy and remove matching module
 *
 * Args:
 *   data          SyncState* containing the module to destroy
 *   user_data     NULL
 */
void gfDestroyModule(gpointer data, gpointer user_data)
{
	SyncState* parallelSS= data;

	parallelSS->matchingModule->destroyMatching(parallelSS);
	free(parallelSS);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          SyncState* containing the distributed matching module
 *   user_data     Event* original event
 */
void gfMatchEvent(gpointer data, gpointer user_data)
{
	SyncState* parallelSS= data;
	const Event* event= user_data;
	Event* newEvent;

	if (parallelSS->matchingModule->canMatch[event->type])
	{
		event->copy(event, &newEvent);
		parallelSS->matchingModule->matchEvent(parallelSS, newEvent);
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          SyncState* containing the distributed matching module
 *   user_data     NULL
 */
void gfFinalize(gpointer data, gpointer user_data)
{
	SyncState* parallelSS= data;

	freeAllFactors(parallelSS->matchingModule->finalizeMatching(parallelSS),
		parallelSS->traceNb);
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Args:
 *   data          SyncState* containing the distributed matching module
 *   user_data     NULL
 */
void gfPrintStats(gpointer data, gpointer user_data)
{
	SyncState* parallelSS= data;

	if (parallelSS->matchingModule->printMatchingStats != NULL)
	{
		parallelSS->matchingModule->printMatchingStats(parallelSS);
	}
}


/*
 * A GFunc for g_queue_foreach()
 *
 * Call a certain matching function
 *
 * Args:
 *   data          SyncState* containing the distributed matching module
 *   user_data     size_t,
 */
void gfGraphFunctionCall(gpointer data, gpointer user_data)
{
	SyncState* parallelSS= data;
	struct GraphAggregate* aggregate= user_data;
	typedef void (*GraphFunction)(struct _SyncState*, const unsigned int,
		const unsigned int);
	GraphFunction graphFunction= *(GraphFunction*)((void*)
		parallelSS->matchingModule + (size_t) aggregate->offset);

	if (graphFunction != NULL)
	{
		graphFunction(parallelSS, aggregate->i, aggregate->j);
	}
}
