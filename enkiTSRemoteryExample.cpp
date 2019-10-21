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

#include "TaskScheduler.h"

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <sstream>

#include "Remotery.h"

using namespace enki;

TaskScheduler g_TS;

static const int RUNS = 1024*1024;
static const int MINRANGE = 10 * 1024;
static const int SUMS = 10*1024*1024;


struct ParallelSumTaskSet : ITaskSet
{
	struct Count
	{
		// prevent false sharing.
		uint64_t	count;
		char		cacheline[64];
	};
	Count*    m_pPartialSums;
	uint32_t  m_NumPartialSums;

	ParallelSumTaskSet( uint32_t size_ ) : m_pPartialSums(NULL), m_NumPartialSums(0) { m_SetSize = size_; m_MinRange = MINRANGE; }
	virtual ~ParallelSumTaskSet()
	{
		delete[] m_pPartialSums;
	}

	void Init()
	{
		delete[] m_pPartialSums;
		m_NumPartialSums = g_TS.GetNumTaskThreads();
		m_pPartialSums = new Count[ m_NumPartialSums ];
		memset( m_pPartialSums, 0, sizeof(Count)*m_NumPartialSums );
	}

	virtual void    ExecuteRange( TaskSetPartition range, uint32_t threadnum )
	{
		rmt_ScopedCPUSample(Sum, 0);
		assert( m_pPartialSums && m_NumPartialSums );
		uint64_t sum = m_pPartialSums[threadnum].count;
		for( uint64_t i = range.start; i < range.end; ++i )
		{
			sum += i + 1;
		}
		m_pPartialSums[threadnum].count = sum;
	}
  
};

struct ParallelReductionSumTaskSet : ITaskSet
{
	ParallelSumTaskSet m_ParallelSumTaskSet;
	uint64_t m_FinalSum;

	ParallelReductionSumTaskSet( uint32_t size_ ) : m_ParallelSumTaskSet( size_ ), m_FinalSum(0) {}

	void Init()
	{
		m_ParallelSumTaskSet.Init();
	}

	virtual void    ExecuteRange( TaskSetPartition range, uint32_t threadnum )
	{
		rmt_ScopedCPUSample(Reduce, 0);

		g_TS.AddTaskSetToPipe( &m_ParallelSumTaskSet );
		g_TS.WaitforTask( &m_ParallelSumTaskSet );

		for( uint32_t i = 0; i < m_ParallelSumTaskSet.m_NumPartialSums; ++i )
		{
			m_FinalSum += m_ParallelSumTaskSet.m_pPartialSums[i].count;
		}
	}
};

void threadStartCallback( uint32_t threadnum_ )
{
    std::ostringstream out;  
    out << "enkiTS_" << threadnum_;
    rmt_SetCurrentThreadName( out.str().c_str()  );
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
    // the example generates a lot of samples, so increase update rate for profiler
    rmt_Settings()->maxNbMessagesPerUpdate  = 128 * rmt_Settings()->maxNbMessagesPerUpdate;

    Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

    // Set the callbacks BEFORE initialize or we will get no threadstart nor first waitStart calls
    g_TS.GetProfilerCallbacks()->threadStart                     = threadStartCallback;
    g_TS.GetProfilerCallbacks()->waitForNewTaskSuspendStart      = waitForNewTaskSuspendStartCallback;
    g_TS.GetProfilerCallbacks()->waitForNewTaskSuspendStop       = stopCallback;
    g_TS.GetProfilerCallbacks()->waitForTaskCompleteStart        = waitForTaskCompleteStartCallback;
    g_TS.GetProfilerCallbacks()->waitForTaskCompleteStop         = stopCallback;
    g_TS.GetProfilerCallbacks()->waitForTaskCompleteSuspendStart = waitForTaskCompleteSuspendStartCallback;
    g_TS.GetProfilerCallbacks()->waitForTaskCompleteSuspendStop  = stopCallback;

	g_TS.Initialize();


	rmt_SetCurrentThreadName("Main");

	double avSpeedUp = 0.0;
	for( int run = 0; run< RUNS; ++run )
	{
		rmt_ScopedCPUSample(Run, 0);

		printf("Run %d.....\n", run);
		ParallelReductionSumTaskSet m_ParallelReductionSumTaskSet( SUMS );
		{
			rmt_ScopedCPUSample(Parallel, 0);

			m_ParallelReductionSumTaskSet.Init();

			g_TS.AddTaskSetToPipe(&m_ParallelReductionSumTaskSet);

			g_TS.WaitforTask(&m_ParallelReductionSumTaskSet);
		}



		volatile uint64_t sum = 0;
		{
			rmt_ScopedCPUSample(Serial, 0);
			for (uint64_t i = 0; i < (uint64_t)m_ParallelReductionSumTaskSet.m_ParallelSumTaskSet.m_SetSize; ++i)
			{
				sum += i + 1;
			}
		}

	}
    g_TS.WaitforAllAndShutdown();
	rmt_DestroyGlobalInstance(rmt);
	return 0;
}
