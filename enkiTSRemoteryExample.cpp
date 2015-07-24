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

#include "Remotery.h"

using namespace enki;


TaskScheduler g_TS;


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

	ParallelSumTaskSet( uint32_t size_ ) : m_pPartialSums(NULL), m_NumPartialSums(0) { m_SetSize = size_; }
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
		rmt_ScopedCPUSample(Sum);
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
		rmt_ScopedCPUSample(Reduce);

		g_TS.AddTaskSetToPipe( &m_ParallelSumTaskSet );
		g_TS.WaitforTaskSet( &m_ParallelSumTaskSet );

		for( uint32_t i = 0; i < m_ParallelSumTaskSet.m_NumPartialSums; ++i )
		{
			m_FinalSum += m_ParallelSumTaskSet.m_pPartialSums[i].count;
		}
	}
};

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

void waitStartCallback( uint32_t threadnum_ )
{
    rmt_BeginCPUSample(WAIT);
}

void waitStopCallback( uint32_t threadnum_ )
{
    rmt_EndCPUSample();
}

static const int RUNS = 1024*1024;
static const int SUMS = 10*1024*1024;

int main(int argc, const char * argv[])
{
	Remotery* rmt;
	rmt_CreateGlobalInstance(&rmt);

    // Set the callbacks BEFORE initialize or we will get no threadstart nor first waitStart calls
    g_TS.GetProfilerCallbacks()->threadStart    = threadStartCallback;
    g_TS.GetProfilerCallbacks()->waitStart      = waitStartCallback;
    g_TS.GetProfilerCallbacks()->waitStop       = waitStopCallback;

	g_TS.Initialize();


	rmt_SetCurrentThreadName("Main");

	double avSpeedUp = 0.0;
	for( int run = 0; run< RUNS; ++run )
	{
		rmt_ScopedCPUSample(Run);

		printf("Run %d.....\n", run);
		ParallelReductionSumTaskSet m_ParallelReductionSumTaskSet( SUMS );
		{
			rmt_ScopedCPUSample(Parallel);

			m_ParallelReductionSumTaskSet.Init();

			g_TS.AddTaskSetToPipe(&m_ParallelReductionSumTaskSet);

			g_TS.WaitforTaskSet(&m_ParallelReductionSumTaskSet);
		}



		volatile uint64_t sum = 0;
		{
			rmt_ScopedCPUSample(Serial);
			for (uint64_t i = 0; i < (uint64_t)m_ParallelReductionSumTaskSet.m_ParallelSumTaskSet.m_SetSize; ++i)
			{
				sum += i + 1;
			}
		}

	}
	rmt_DestroyGlobalInstance(rmt);
	return 0;
}
