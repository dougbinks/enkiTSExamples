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


#include <imgui.h>
#include "imgui_impl_glfw.h"
#include <stdio.h>
#include <GLFW/glfw3.h>

#define MICROPROFILE_IMPL
#define MICROPROFILEUI_IMPL
#define MICROPROFILE_WEBSERVER 0
#define MICROPROFILE_TEXT_HEIGHT 12
#define MICROPROFILE_TEXT_WIDTH 7
#include "microprofile.h"
#include "microprofileui.h"

// define GPU queries as NULL
uint32_t MicroProfileGpuInsertTimeStamp() { return 0; }
uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey) { return 0; }
uint64_t MicroProfileTicksPerSecondGpu() { return 1; }
int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu) { return 0; }

// UI functions
static ImDrawList*  g_pImDraw = 0;
static ImVec2       g_DrawPos;
void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters)
{
    g_pImDraw->AddText( ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ), nColor, pText, pText + nNumCharacters );
}

void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType boxType )
{
    switch( boxType )
    {
    case MicroProfileBoxTypeBar:
    {
        uint32_t cul = nColor; 
        uint32_t cur = ( nColor & 0x00FFFFFF ) + 0xFF000000; 
        uint32_t clr = ( nColor & 0x00FFFFFF ) + 0x50000000; 
        uint32_t cll = ( nColor & 0x00FFFFFF ) + 0x50000000; 
        g_pImDraw->AddRectFilledMultiColor(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), cul, cur, clr, cll );
        if( nX1 - nX > 5 )
        {
            g_pImDraw->AddRect(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), 0x50000000 );
        }
        break;
    }
    case MicroProfileBoxTypeFlat:
        g_pImDraw->AddRectFilled(ImVec2(nX + g_DrawPos.x,nY + g_DrawPos.y ),
                                 ImVec2(nX1 + g_DrawPos.x,nY1 + g_DrawPos.y ), nColor );
        break;
    default:
        assert(false);
    }
}

void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
    for( uint32_t vert = 0; vert + 1 < nVertices; ++vert )
    {
        uint32_t i = 2*vert;
        ImVec2 posA( pVertices[i] + g_DrawPos.x, pVertices[i+1] + g_DrawPos.y );
        ImVec2 posB( pVertices[i+2] + g_DrawPos.x, pVertices[i+3] + g_DrawPos.y );
        g_pImDraw->AddLine( posA, posB, nColor );
    }
}


static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}


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
        MICROPROFILE_SCOPEI("Parallel", "SumInit", 0xFF008800 );
        delete[] m_pPartialSums;
		m_NumPartialSums = g_TS.GetNumTaskThreads();
		m_pPartialSums = new Count[ m_NumPartialSums ];
		memset( m_pPartialSums, 0, sizeof(Count)*m_NumPartialSums );
	}

	virtual void    ExecuteRange( TaskSetPartition range, uint32_t threadnum )
	{
        MICROPROFILE_SCOPEI("Parallel", "SumTask", 0xFF00D000 );
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
        MICROPROFILE_SCOPEI("Parallel", "ReductionTask", 0xFF20C000 );
        g_TS.AddTaskSetToPipe( &m_ParallelSumTaskSet );
		g_TS.WaitforTaskSet( &m_ParallelSumTaskSet );

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
    MicroProfileOnThreadCreate( out.str().c_str()  );
}

#if 0 ==  MICROPROFILE_ENABLED
void waitStartCallback( uint32_t threadnum_ ){}
void waitStopCallback( uint32_t threadnum_ ) {}
void profilerInit() {}
#else
// following is somewhat difficult way to get wait callbacks working
MicroProfileToken g_ProfileWait = MicroProfileGetToken( "enkiTS", "Wait", 0xFF505000, MicroProfileTokenTypeCpu);

struct TickStore
{
    TickStore() : pTicks(NULL) {}
    ~TickStore() { delete[] pTicks; }
    uint64_t* pTicks;
} g_Ticks;
void profilerInit()
{
    g_Ticks.pTicks = new uint64_t[ g_TS.GetNumTaskThreads() ];
}
void waitStartCallback( uint32_t threadnum_ )
{
    g_Ticks.pTicks[ threadnum_ ] = MicroProfileEnter( g_ProfileWait );
}

void waitStopCallback( uint32_t threadnum_ )
{
    MicroProfileLeave( g_ProfileWait, g_Ticks.pTicks[ threadnum_ ] );
}
#endif

static const int SUMS = 10*1024*1024;

int main(int argc, const char * argv[])
{
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(1);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL2 example", NULL, NULL);
    glfwMakeContextCurrent(window);

    // Setup ImGui binding
    ImGui_ImplGlfw_Init(window, true);

    // Set up Microprofile
    MicroProfileToggleDisplayMode();
    MicroProfileInitUI();
    MicroProfileOnThreadCreate("Main");


    // Set the callbacks BEFORE initialize or we will get no threadstart nor first waitStart calls
    g_TS.GetProfilerCallbacks()->threadStart    = threadStartCallback;
    g_TS.GetProfilerCallbacks()->waitStart      = waitStartCallback;
    g_TS.GetProfilerCallbacks()->waitStop       = waitStopCallback;

	g_TS.Initialize();
    profilerInit();

	double avSpeedUp = 0.0;
    ImVec4 clear_color = ImColor(114, 144, 154);
    bool showWindow = true;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfw_NewFrame();


		ParallelReductionSumTaskSet m_ParallelReductionSumTaskSet( SUMS );
		{

			m_ParallelReductionSumTaskSet.Init();

			g_TS.AddTaskSetToPipe(&m_ParallelReductionSumTaskSet);

			g_TS.WaitforTaskSet(&m_ParallelReductionSumTaskSet);
		}



		volatile uint64_t sum = 0;
		{
            MICROPROFILE_SCOPEI("Serial", "Sum", 0xFF0000D0 );
            for (uint64_t i = 0; i < (uint64_t)m_ParallelReductionSumTaskSet.m_ParallelSumTaskSet.m_SetSize; ++i)
			{
				sum += i + 1;
			}
		}

        if (showWindow)
        {
            MicroProfileFlip();
            ImGui::SetNextWindowSize(ImVec2(1200,700), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(10,10), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Microprofile", &showWindow );
                
            g_pImDraw = ImGui::GetWindowDrawList();
            g_DrawPos = ImGui::GetCursorScreenPos();


            ImVec2 sizeForMicroDraw = ImGui::GetContentRegionAvail();
            ImGui::InvisibleButton("canvas", sizeForMicroDraw);
            if (ImGui::IsItemHovered())
            {
                MicroProfileMouseButton( ImGui::GetIO().MouseDown[0], ImGui::GetIO().MouseDown[1] );       
            }
            else
            {
                MicroProfileMouseButton( 0, 0 );       
            }
            MicroProfileMousePosition(ImGui::GetIO().MousePos.x- g_DrawPos.x, ImGui::GetIO().MousePos.y - g_DrawPos.y, ImGui::GetIO().MouseWheel );
            MicroProfileDraw(sizeForMicroDraw.x, sizeForMicroDraw.y);

            ImGui::End();
        }

        // Rendering
        glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
	}

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();

	return 0;
}
