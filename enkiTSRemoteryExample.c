// Copyright (c) 2013 Doug Binks
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "TaskScheduler_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Remotery.h"

enkiTaskScheduler*	pETS;
enkiTaskSet*		pPSumTask;
enkiTaskSet*		pPSumReductionTask;

static const int RUNS = 1024 * 1024;
static const int MINRANGE = 10 * 1024;
static const int SUMS = 10 * 1024 * 1024;


typedef struct ParallelSumTaskSetArgs
{
	uint64_t* pPartialSums;
	uint32_t  numPartialSums;
} ParallelSumTaskSetArgs;

void ParallelSumTaskSetArgsInit( ParallelSumTaskSetArgs* pArgs_ )
{
	pArgs_->numPartialSums = enkiGetNumTaskThreads( pETS );
	pArgs_->pPartialSums = (uint64_t*)malloc( sizeof(uint64_t) * pArgs_->numPartialSums );
	memset( pArgs_->pPartialSums, 0, sizeof(uint64_t) * pArgs_->numPartialSums );
}

void ParallelSumTaskSetFunc( uint32_t start_, uint32_t end, uint32_t threadnum_, void* pArgs_ )
{
	rmt_BeginCPUSample(Sum, 0);

	ParallelSumTaskSetArgs args;
	uint64_t sum, i;
	
	args = *(ParallelSumTaskSetArgs*)pArgs_;

	sum = args.pPartialSums[threadnum_];
	for( i = start_; i < end; ++i )
	{
		sum += i + 1;
	}
	args.pPartialSums[threadnum_] = sum;

	rmt_EndCPUSample();
}

void ParallelReductionSumTaskSet(  uint32_t start_, uint32_t end, uint32_t threadnum_, void* pArgs_ )
{
	rmt_BeginCPUSample(Reduce, 0);

	ParallelSumTaskSetArgs args;
	uint64_t sum;
	uint64_t inMax_outSum, i;

	inMax_outSum = *(uint64_t*)pArgs_;

	ParallelSumTaskSetArgsInit( &args );

	enkiAddTaskSetToPipeMinRange( pETS, pPSumTask, &args, (uint32_t)inMax_outSum, MINRANGE );
	enkiWaitForTaskSet( pETS, pPSumTask );

	sum = 0;
	for( i = 0; i < args.numPartialSums; ++i )
	{
		sum += args.pPartialSums[i];
	}

	free( args.pPartialSums );

	*(uint64_t*)pArgs_ = sum;

	rmt_EndCPUSample();
}


static char* nameTable[] = {
    "enkiTS_00", "enkiTS_01", "enkiTS_02", "enkiTS_03", "enkiTS_04", "enkiTS_05", "enkiTS_06", "enkiTS_07", "enkiTS_08", "enkiTS_09",
    "enkiTS_10", "enkiTS_11", "enkiTS_12", "enkiTS_13", "enkiTS_14", "enkiTS_15", "enkiTS_16", "enkiTS_17", "enkiTS_18", "enkiTS_19",
    "enkiTS_20", "enkiTS_21", "enkiTS_22", "enkiTS_23", "enkiTS_24", "enkiTS_25", "enkiTS_26", "enkiTS_27", "enkiTS_28", "enkiTS_29",
    "enkiTS_30", "enkiTS_31", "enkiTS_32", "enkiTS_33", "enkiTS_34", "enkiTS_35", "enkiTS_36", "enkiTS_37", "enkiTS_38", "enkiTS_39",
    "enkiTS_40", "enkiTS_41", "enkiTS_42", "enkiTS_43", "enkiTS_44", "enkiTS_45", "enkiTS_46", "enkiTS_47", "enkiTS_48", "enkiTS_49",
    "enkiTS_50", "enkiTS_51", "enkiTS_52", "enkiTS_53", "enkiTS_54", "enkiTS_55", "enkiTS_56", "enkiTS_57", "enkiTS_58", "enkiTS_59",
    "enkiTS_60", "enkiTS_61", "enkiTS_62", "enkiTS_63", "enkiTS_64", "enkiTS_XX",
};
const size_t nameTableSize = sizeof( nameTable ) / sizeof( const char* );

void threadStartCallback( uint32_t threadnum_ )
{
    uint32_t nameNum = threadnum_;
    if( nameNum >= nameTableSize ) { nameNum = nameTableSize-1; }
    rmt_SetCurrentThreadName( nameTable[ nameNum ] );
}

void waitForNewTaskSuspendStartCallback( uint32_t threadnum_ )
{
    rmt_BeginCPUSample(WaitForNewTaskSuspend, 0);
}

void waitForTaskCompleteStartCallback( uint32_t threadnum_ )
{
    rmt_BeginCPUSample(WaitForTaskComplete, 0);
}

void waitForTaskCompleteSuspendStartCallback( uint32_t threadnum_ )
{
    rmt_BeginCPUSample(WaitForTaskCompleteSuspend, 0);
}

void stopCallback( uint32_t threadnum_ )
{
    rmt_EndCPUSample();
}

int main(int argc, const char * argv[])
{
    int run;
	uint64_t inMax_outSum, i;
    volatile uint64_t serialSum;
	Remotery* rmt;

    // the example generates a lot of samples, so increase update rate for profiler
    rmt_Settings()->maxNbMessagesPerUpdate  = 128 * rmt_Settings()->maxNbMessagesPerUpdate;
	rmt_CreateGlobalInstance(&rmt);

	pETS = enkiNewTaskScheduler();

    enkiGetProfilerCallbacks( pETS )->threadStart                     = threadStartCallback;
    enkiGetProfilerCallbacks( pETS )->waitForNewTaskSuspendStart      = waitForNewTaskSuspendStartCallback;
    enkiGetProfilerCallbacks( pETS )->waitForNewTaskSuspendStop       = stopCallback;
    enkiGetProfilerCallbacks( pETS )->waitForTaskCompleteStart        = waitForTaskCompleteStartCallback;
    enkiGetProfilerCallbacks( pETS )->waitForTaskCompleteStop         = stopCallback;
    enkiGetProfilerCallbacks( pETS )->waitForTaskCompleteSuspendStart = waitForTaskCompleteSuspendStartCallback;
    enkiGetProfilerCallbacks( pETS )->waitForTaskCompleteSuspendStop  = stopCallback;

    enkiInitTaskScheduler( pETS );

	rmt_SetCurrentThreadName("Main");

	pPSumTask			= enkiCreateTaskSet( pETS, ParallelSumTaskSetFunc );
	pPSumReductionTask	= enkiCreateTaskSet( pETS, ParallelReductionSumTaskSet );

	for (run = 0; run < RUNS; ++run)
	{
		rmt_BeginCPUSample(Run, 0);

		printf("Run %d.....\n", run);

		rmt_BeginCPUSample(Parallel, 0);
		inMax_outSum = SUMS;
		enkiAddTaskSetToPipe(pETS, pPSumReductionTask, &inMax_outSum, 1);
		enkiWaitForTaskSet(pETS, pPSumReductionTask);
		rmt_EndCPUSample();


		rmt_BeginCPUSample(Serial, 0);
		serialSum = 0;
		for (i = 0; i < SUMS; ++i)
		{
			serialSum += i + 1;
		}
		rmt_EndCPUSample();

		rmt_EndCPUSample();
	}

	enkiDeleteTaskScheduler( pETS );
}