#include "pch.h"
#include "GPUProfiler.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include <iomanip>

namespace Dlight
{
	GPUProfiler::Scope::Scope(GPUProfiler& _profiler, const char* name)
		: profiler(_profiler), scopeIndex(profiler.BeginScope(name))
	{
	}

	GPUProfiler::Scope::~Scope() noexcept
	{
		profiler.EndScope(scopeIndex);
	}

	GPUProfiler::GPUProfiler(VulkanDevice& vulkanDevice, uint32_t frameCount)
		: device(vulkanDevice.GetDevice())
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(vulkanDevice.GetPhysicalDevice(), &properties);

		uint32_t queueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(vulkanDevice.GetPhysicalDevice(), &queueCount, nullptr);
		std::vector<VkQueueFamilyProperties> queues(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(vulkanDevice.GetPhysicalDevice(), &queueCount, queues.data());
		const uint32_t queueIndex = vulkanDevice.GetGraphicsAndComputeQueueFamilyIndex();
		if (frameCount == 0 || queueIndex >= queueCount ||
			queues[queueIndex].timestampValidBits == 0 || properties.limits.timestampPeriod <= 0.0f)
		{
			DL_LOG_WARN("GPU timestamp profiling is unavailable for this queue");
			return;
		}

		timestampPeriod = properties.limits.timestampPeriod;
		const uint32_t validBits = queues[queueIndex].timestampValidBits;
		// 64비트 시프트를 피한다. 부호 없는 정수의 뺄셈과 마스킹으로 한 번의 순환을 처리한다.
		timestampMask = validBits >= 64 ? UINT64_MAX : (uint64_t{ 1 } << validBits) - 1;
		frames.resize(frameCount);
		results.reserve(MaxScopes);
		for (Frame& frame : frames)
		{
			frame.scopes.reserve(MaxScopes);
			frame.timestamps.resize(MaxScopes * 2);
		}

		VkQueryPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		poolInfo.queryCount = MaxScopes * 2;
		for (Frame& frame : frames)
		{
			const VkResult result = vkCreateQueryPool(device, &poolInfo, nullptr, &frame.queryPool);
			if (result != VK_SUCCESS)
			{
				DL_LOG_WARN("GPU profiling disabled: query pool creation failed: ", result);
				DestroyQueryPools();
				return;
			}
		}
		supported = true;
	}

	GPUProfiler::~GPUProfiler()
	{
		DestroyQueryPools();
	}

	void GPUProfiler::DestroyQueryPools() noexcept
	{
		for (Frame& frame : frames)
		{
			if (frame.queryPool != VK_NULL_HANDLE)
			{
				vkDestroyQueryPool(device, frame.queryPool, nullptr);
				frame.queryPool = VK_NULL_HANDLE;
			}
		}
	}

	void GPUProfiler::BeginFrame(uint32_t frameSlot, VkCommandBuffer _commandBuffer)
	{
		if (!supported)
		{
			return;
		}
		if (frameSlot >= frames.size() || _commandBuffer == VK_NULL_HANDLE || commandBuffer != VK_NULL_HANDLE)
		{
			DL_LOG_ERROR("Invalid GPUProfiler::BeginFrame lifecycle");
			std::abort();
		}

		Frame& frame = frames[frameSlot];
		ReadResults(frame);
		frame.scopes.clear();
		vkCmdResetQueryPool(_commandBuffer, frame.queryPool, 0, MaxScopes * 2);
		activeFrame = frameSlot;
		commandBuffer = _commandBuffer;
	}

	void GPUProfiler::EndFrame()
	{
		if (activeScopes != 0)
		{
			DL_LOG_ERROR("GPU profiler scopes must end before EndFrame");
			std::abort();
		}
		commandBuffer = VK_NULL_HANDLE;
	}

	uint32_t GPUProfiler::BeginScope(const char* name)
	{
		if (!supported || commandBuffer == VK_NULL_HANDLE)
		{
			return InvalidScope;
		}
		Frame& frame = frames[activeFrame];
		if (frame.scopes.size() == MaxScopes)
		{
			if (!overflowWarningShown)
			{
				DL_LOG_WARN("GPU profiler scope limit reached (", MaxScopes, "); extra scopes are skipped");
				overflowWarningShown = true;
			}
			return InvalidScope;
		}

		const uint32_t index = static_cast<uint32_t>(frame.scopes.size());
		frame.scopes.push_back({ name ? name : "Unnamed", activeScopes });
		++activeScopes;
		// TOP/BOTTOM으로 구간의 대략적인 시간을 측정한다. GPU 단계가 겹쳐 실행되므로
		// 같은 깊이의 구간 시간을 합해도 부모 구간 시간과 일치하지 않을 수 있다.
		vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, frame.queryPool, index * 2);
		return index;
	}

	void GPUProfiler::EndScope(uint32_t scopeIndex) noexcept
	{
		if (scopeIndex == InvalidScope)
		{
			return;
		}
		vkCmdWriteTimestamp2(commandBuffer, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			frames[activeFrame].queryPool, scopeIndex * 2 + 1);
		--activeScopes;
	}

	void GPUProfiler::ReadResults(Frame& frame)
	{
		if (frame.scopes.empty())
		{
			return; // 첫 사용 시에는 GPU에서 쿼리를 초기화하거나 기록한 적이 없다.
		}
		const uint32_t queryCount = static_cast<uint32_t>(frame.scopes.size()) * 2;
		const VkResult result = vkGetQueryPoolResults(device, frame.queryPool, 0, queryCount,
			queryCount * sizeof(Timestamp), frame.timestamps.data(), sizeof(Timestamp),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
		// 기존 프레임 슬롯의 타임라인 대기가 GPU 완료를 보장하므로 WAIT_BIT를 사용하지 않는다.
		// 측정 결과를 수집하기 위해 vkQueueWaitIdle을 추가하지 않는다.
		results.clear();
		if (result != VK_SUCCESS && result != VK_NOT_READY)
		{
			DL_LOG_WARN("Failed to read GPU timestamps: ", result);
			return;
		}
		for (uint32_t i = 0; i < static_cast<uint32_t>(frame.scopes.size()); ++i)
		{
			const Timestamp& begin = frame.timestamps[i * 2];
			const Timestamp& end = frame.timestamps[i * 2 + 1];
			if (begin.available == 0 || end.available == 0)
			{
				continue;
			}
			const uint64_t ticks = (end.value - begin.value) & timestampMask;
			const double milliseconds = static_cast<double>(ticks) * timestampPeriod / 1000000.0;
			const ScopeRecord& scope = frame.scopes[i];
			auto existing = std::find_if(
				results.begin(),
				results.end(),
				[&scope](const Result& timing)
				{
					return timing.name == scope.name && timing.depth == scope.depth;
				});

			if (existing != results.end())
			{
				existing->milliseconds += milliseconds;
			}
			else
			{
				results.push_back({ scope.name, milliseconds, scope.depth });
			}
		}

		const auto now = std::chrono::steady_clock::now();
		if (!results.empty() && now - lastLogTime >= std::chrono::seconds(1))
		{
			std::ostringstream text;
			text << std::fixed << std::setprecision(3) << "GPU (last completed frame):";
			for (const Result& timing : results)
			{
				text << ' ' << timing.name << '=' << timing.milliseconds << " ms;";
			}
			DL_LOG_INFO(text.str());
			lastLogTime = now;
		}
	}
}
