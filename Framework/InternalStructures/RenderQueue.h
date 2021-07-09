#pragma once

namespace bk {

//BK_TYPE(RenderQueue)
class RenderQueue
{
public:
	static bool Begin(Device& device, uint32_t currentFrame);

	static void SetSemaphores(
		vk::Semaphore const* const& waitSemaphores, vk::Semaphore const* const& signalSemaphores,
		uint32_t waitSemaphoresCount = 1, uint32_t signalSemaphoresCount = 1
	);

	static void SetSyncChain(
		vk::Semaphore const* const& signalSemaphores,
		uint32_t signalSemaphoresCount = 1
	);

	static void SetCommandBuffers(vk::CommandBuffer* commandBuffers, uint32_t commandBuffersCount = 1);

	static void SetStageMask(vk::PipelineStageFlags* stageMask);

	static void Submit(Device& device, vk::Fence fence = nullptr);

	static bool End(
		Device& device, uint32_t currentFrame,
		const vk::Semaphore* deviceWaitSemaphores = nullptr,
		uint32_t deviceWaitSemaphoresCount = 0
	);


private:
	inline static vk::SubmitInfo _currentSubmitInfo = {};

	inline static vk::Semaphore const* const* _previousSignalSemaphores = {};
	inline static uint32_t _previousSignalSemaphoresCount = 0;
};

}