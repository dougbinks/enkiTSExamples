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
	rmt_BeginCPUSample(Sum);

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
	rmt_BeginCPUSample(Reduce);

	ParallelSumTaskSetArgs args;
	uint64_t sum;
	uint64_t inMax_outSum, i;

	inMax_outSum = *(uint64_t*)pArgs_;

	ParallelSumTaskSetArgsInit( &args );

	enkiAddTaskSetToPipe( pETS, pPSumTask, &args, (uint32_t)inMax_outSum);
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

static const int RUNS = 1024 * 1024;
static const int SUMS = 10 * 1024 * 1024;

int main(int argc, const char * argv[])
{
	uint64_t inMax_outSum, i, serialSum;
	Remotery* rmt;

	rmt_CreateGlobalInstance(&rmt);

	pETS = enkiCreateTaskScheduler();

	rmt_SetCurrentThreadName("Main");

	pPSumTask			= enkiCreateTaskSet( pETS, ParallelSumTaskSetFunc );
	pPSumReductionTask	= enkiCreateTaskSet( pETS, ParallelReductionSumTaskSet );

	for (int run = 0; run < RUNS; ++run)
	{
		rmt_BeginCPUSample(Run);

		printf("Run %d.....\n", run);

		rmt_BeginCPUSample(Parallel);
		inMax_outSum = SUMS;
		enkiAddTaskSetToPipe(pETS, pPSumReductionTask, &inMax_outSum, 1);
		enkiWaitForTaskSet(pETS, pPSumReductionTask);
		rmt_EndCPUSample();


		rmt_BeginCPUSample(Serial);
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