#include "wiGPUSortLib.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"
#include "shaders/ShaderInterop_GPUSortLib.h"
#include "wiEvent.h"
#include "wiTimer.h"
#include "wiBackLog.h"

using namespace wiGraphics;

namespace wiGPUSortLib
{
	static GPUBuffer indirectBuffer;
	static Shader kickoffSortCS;
	static Shader sortCS;
	static Shader sortInnerCS;
	static Shader sortStepCS;


	void LoadShaders()
	{
		std::string path = wiRenderer::GetShaderPath();

		wiRenderer::LoadShader(CS, kickoffSortCS, "gpusortlib_kickoffSortCS.cso");
		wiRenderer::LoadShader(CS, sortCS, "gpusortlib_sortCS.cso");
		wiRenderer::LoadShader(CS, sortInnerCS, "gpusortlib_sortInnerCS.cso");
		wiRenderer::LoadShader(CS, sortStepCS, "gpusortlib_sortStepCS.cso");

	}

	void Initialize()
	{
		wiTimer timer;

		GPUBufferDesc bd;
		bd.Usage = USAGE_DEFAULT;
		bd.BindFlags = BIND_UNORDERED_ACCESS;
		bd.MiscFlags = RESOURCE_MISC_INDIRECT_ARGS | RESOURCE_MISC_BUFFER_RAW;
		bd.Size = sizeof(IndirectDispatchArgs);
		wiRenderer::GetDevice()->CreateBuffer(&bd, nullptr, &indirectBuffer);

		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();

		wiBackLog::post("wiGPUSortLib Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
	}


	void Sort(
		uint32_t maxCount, 
		const GPUBuffer& comparisonBuffer_read, 
		const GPUBuffer& counterBuffer_read, 
		uint32_t counterReadOffset, 
		const GPUBuffer& indexBuffer_write,
		CommandList cmd)
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		device->EventBegin("GPUSortLib", cmd);


		SortConstants sc;
		sc.counterReadOffset = counterReadOffset;
		device->BindDynamicConstantBuffer(sc, CB_GETBINDSLOT(SortConstants), cmd);

		// initialize sorting arguments:
		{
			device->BindComputeShader(&kickoffSortCS, cmd);

			const GPUResource* res[] = {
				&counterBuffer_read,
			};
			device->BindResources(res, 0, arraysize(res), cmd);

			const GPUResource* uavs[] = {
				&indirectBuffer,
			};
			device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Buffer(&indirectBuffer, RESOURCE_STATE_INDIRECT_ARGUMENT, RESOURCE_STATE_UNORDERED_ACCESS)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->Dispatch(1, 1, 1, cmd);

			{
				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
					GPUBarrier::Buffer(&indirectBuffer, RESOURCE_STATE_UNORDERED_ACCESS, RESOURCE_STATE_INDIRECT_ARGUMENT)
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

		}


		const GPUResource* uavs[] = {
			&indexBuffer_write,
		};
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		const GPUResource* resources[] = {
			&counterBuffer_read,
			&comparisonBuffer_read,
		};
		device->BindResources(resources, 0, arraysize(resources), cmd);

		// initial sorting:
		bool bDone = true;
		{
			// calculate how many threads we'll require:
			//   we'll sort 512 elements per CU (threadgroupsize 256)
			//     maybe need to optimize this or make it changeable during init
			//     TGS=256 is a good intermediate value

			unsigned int numThreadGroups = ((maxCount - 1) >> 9) + 1;

			//assert(numThreadGroups <= 1024);

			if (numThreadGroups > 1)
			{
				bDone = false;
			}

			// sort all buffers of size 512 (and presort bigger ones)
			device->BindComputeShader(&sortCS, cmd);
			device->DispatchIndirect(&indirectBuffer, 0, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		int presorted = 512;
		while (!bDone)
		{
			// Incremental sorting:

			bDone = true;
			device->BindComputeShader(&sortStepCS, cmd);

			// prepare thread group description data
			uint32_t numThreadGroups = 0;

			if (maxCount > (uint32_t)presorted)
			{
				if (maxCount > (uint32_t)presorted * 2)
					bDone = false;

				uint32_t pow2 = presorted;
				while (pow2 < maxCount)
					pow2 *= 2;
				numThreadGroups = pow2 >> 9;
			}

			uint32_t nMergeSize = presorted * 2;
			for (uint32_t nMergeSubSize = nMergeSize >> 1; nMergeSubSize > 256; nMergeSubSize = nMergeSubSize >> 1)
			{
				SortConstants sc;
				sc.job_params.x = nMergeSubSize;
				if (nMergeSubSize == nMergeSize >> 1)
				{
					sc.job_params.y = (2 * nMergeSubSize - 1);
					sc.job_params.z = -1;
				}
				else
				{
					sc.job_params.y = nMergeSubSize;
					sc.job_params.z = 1;
				}
				sc.counterReadOffset = counterReadOffset;

				device->BindDynamicConstantBuffer(sc, CB_GETBINDSLOT(SortConstants), cmd);

				device->Dispatch(numThreadGroups, 1, 1, cmd);

				GPUBarrier barriers[] = {
					GPUBarrier::Memory(),
				};
				device->Barrier(barriers, arraysize(barriers), cmd);
			}

			device->BindComputeShader(&sortInnerCS, cmd);
			device->Dispatch(numThreadGroups, 1, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			presorted *= 2;
		}



		device->EventEnd(cmd);
	}

}
