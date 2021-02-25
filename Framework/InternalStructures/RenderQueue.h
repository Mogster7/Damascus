#pragma once

class RenderQueue
{
public:
	static bool Begin(Device& device, uint32_t currentFrame)
	{
		_currentSubmitInfo = vk::SubmitInfo();
		_previousSignalSemaphores = nullptr;
		_previousSignalSemaphoresCount = 0;
		return device.PrepareFrame(currentFrame);
	}

	static void SetSemaphores(vk::Semaphore const * const & waitSemaphores, vk::Semaphore const * const & signalSemaphores, 
							  const uint32_t waitSemaphoresCount = 1, const uint32_t signalSemaphoresCount = 1)
	{
		_currentSubmitInfo.waitSemaphoreCount = waitSemaphoresCount;
		_currentSubmitInfo.pWaitSemaphores = waitSemaphores;
		_currentSubmitInfo.signalSemaphoreCount = signalSemaphoresCount;
		_currentSubmitInfo.pSignalSemaphores = signalSemaphores;

		_previousSignalSemaphores = &signalSemaphores;
		_previousSignalSemaphoresCount = signalSemaphoresCount;
	}

	static void SetSyncChain(vk::Semaphore const * const & signalSemaphores, 
							 uint32_t signalSemaphoresCount = 1)
	{
		_currentSubmitInfo.waitSemaphoreCount = _previousSignalSemaphoresCount;
		_currentSubmitInfo.pWaitSemaphores = *_previousSignalSemaphores;
		_currentSubmitInfo.pSignalSemaphores = signalSemaphores;
		_currentSubmitInfo.signalSemaphoreCount = signalSemaphoresCount;

		_previousSignalSemaphores = &signalSemaphores;
		_previousSignalSemaphoresCount = signalSemaphoresCount;
	}

	static void SetCommandBuffers(vk::CommandBuffer* commandBuffers, uint32_t commandBuffersCount = 1)
	{
		_currentSubmitInfo.commandBufferCount = commandBuffersCount;
		_currentSubmitInfo.pCommandBuffers = commandBuffers;
	}

	static void SetStageMask(vk::PipelineStageFlags* stageMask)
	{
		_currentSubmitInfo.pWaitDstStageMask = stageMask;
	}

	static void Submit(Device& device, vk::Fence fence = nullptr)
	{
		utils::CheckVkResult(
			device.graphicsQueue.submit(1,
										&_currentSubmitInfo,
										fence),
			"Failed to submit draw queue"
		);
	}

	static bool End(Device& device, uint32_t currentFrame,
					const vk::Semaphore* deviceWaitSemaphores = nullptr,
					uint32_t deviceWaitSemaphoresCount = 0)
	{
		bool previousSemaphore = _previousSignalSemaphores != nullptr;

		ASSERT(
			previousSemaphore ||
			deviceWaitSemaphoresCount != 0,
			"No wait semaphore provided for device frame submission"
		);
		
		return device.SubmitFrame(
			currentFrame, 
			(previousSemaphore) ? *_previousSignalSemaphores : deviceWaitSemaphores,
			 (previousSemaphore) ? _previousSignalSemaphoresCount: deviceWaitSemaphoresCount);
	}

	
private:
	inline static vk::SubmitInfo _currentSubmitInfo = {};

	inline static vk::Semaphore const * const * _previousSignalSemaphores = {};
	inline static uint32_t _previousSignalSemaphoresCount = 0;
};