#pragma once

#include <Volk/volk.h>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace Dlight
{
	class VulkanDevice;

	// 하나의 기록 스레드와 그래픽·컴퓨트 공용 큐에서 사용한다.
	// 디바이스는 이 객체보다 오래 살아 있어야 하며, 쿼리 풀 해제 전에 제출한 GPU 작업이 완료되어야 한다.
	class GPUProfiler
	{
	public:
		struct Result
		{
			std::string name;
			double milliseconds = 0.0;
			uint32_t depth = 0;
		};

		// C++ 스코프의 CPU 실행 시간 대신 GPU 타임스탬프 명령을 기록한다.
		// EndFrame 전에 소멸해야 하며, 중첩 스코프의 측정 시간은 자식 구간의 시간을 포함한다.
		class Scope
		{
		public:
			Scope(GPUProfiler& profiler, const char* name);
			~Scope() noexcept;

			Scope(const Scope&) = delete;
			Scope& operator=(const Scope&) = delete;
			Scope(Scope&&) = delete;
			Scope& operator=(Scope&&) = delete;

		private:
			GPUProfiler& profiler;
			uint32_t scopeIndex;
		};

		GPUProfiler(VulkanDevice& device, uint32_t frameCount);
		~GPUProfiler();

		GPUProfiler(const GPUProfiler&) = delete;
		GPUProfiler& operator=(const GPUProfiler&) = delete;
		GPUProfiler(GPUProfiler&&) = delete;
		GPUProfiler& operator=(GPUProfiler&&) = delete;

		// 이 프레임 슬롯의 이전 GPU 작업 완료를 기다리고 vkBeginCommandBuffer를 호출한 뒤,
		// 렌더링 구간 밖에서 호출한다. 쿼리를 초기화하기 전에 이전 측정 결과를 읽는다.
		void BeginFrame(uint32_t frameSlot, VkCommandBuffer commandBuffer);
		void EndFrame();

		bool IsSupported() const { return supported; }
		// 최근 완료된 프레임의 결과로, 일반적으로 CPU보다 frameCount 프레임 늦게 얻는다.
		const std::vector<Result>& GetResults() const { return results; }

	private:
		static constexpr uint32_t MaxScopes = 32;
		static constexpr uint32_t InvalidScope = UINT32_MAX;

		struct ScopeRecord
		{
			std::string name;
			uint32_t depth;
		};

		struct Timestamp
		{
			uint64_t value;
			uint64_t available;
		};

		struct Frame
		{
			VkQueryPool queryPool = VK_NULL_HANDLE;
			std::vector<ScopeRecord> scopes;
			std::vector<Timestamp> timestamps;
		};

		uint32_t BeginScope(const char* name);
		void EndScope(uint32_t scopeIndex) noexcept;
		void ReadResults(Frame& frame);
		void DestroyQueryPools() noexcept;

		VkDevice device = VK_NULL_HANDLE;
		std::vector<Frame> frames;
		std::vector<Result> results;
		double timestampPeriod = 0.0;
		uint64_t timestampMask = UINT64_MAX;
		bool supported = false;
		bool overflowWarningShown = false;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
		uint32_t activeFrame = 0;
		uint32_t activeScopes = 0;
		std::chrono::steady_clock::time_point lastLogTime = std::chrono::steady_clock::now();
	};
}
