//------------------------------------------------------------------------------
//
// File Name:	Pipeline.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace dm
{

//DM_TYPE(PipelineLayout)
class PipelineLayout : public IVulkanType<vk::PipelineLayout>, public IOwned<Device>
{
DM_TYPE_VULKAN_OWNED_BODY(PipelineLayout, IOwned < Device >)

DM_TYPE_VULKAN_OWNED_GENERIC(PipelineLayout, PipelineLayout)
};

//DM_TYPE(GraphicsPipeline)
class GraphicsPipeline : public IVulkanType<vk::Pipeline>, public IOwned<Device>
{
DM_TYPE_VULKAN_OWNED_BODY(GraphicsPipeline, IOwned < Device >)

DM_TYPE_VULKAN_OWNED_GENERIC_FULL(GraphicsPipeline, Pipeline, , GraphicsPipeline, GraphicsPipelines)

	void Create(const vk::GraphicsPipelineCreateInfo& createInfo, Device* owner)
	{
		Create(createInfo, owner, vk::PipelineCache(), 1);
	}
};

class PipelinePass : public IOwned<Device>
{
DM_TYPE_OWNED_BODY(PipelinePass, IOwned < Device >)

	template <size_t AttachmentCount>
	void Create(
		const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo,
		const vk::RenderPassCreateInfo& renderPassCreateInfo,
		const vk::PipelineLayoutCreateInfo& pipelineLayoutCreateInfo,
		vk::GraphicsPipelineCreateInfo& graphicsPipelineCreateInfo,
		const vk::SemaphoreCreateInfo& semaphoreCreateInfo,
		vk::FramebufferCreateInfo& frameBufferCreateInfo,
		const vk::Extent2D& extent,
		const std::vector<vk::ClearValue>& clearValues,
		const std::vector<std::array<vk::ImageView, AttachmentCount>>& frameBufferAttachments,
		size_t imageViewCount,
		CommandPool* commandPool,
		Device* inOwner
		)
	{
		IOwned<Device>::Create(inOwner);
		semaphores.resize(MAX_FRAME_DRAWS);
		drawCommandBuffers.resize(MAX_FRAME_DRAWS);
		frameBuffers.resize(imageViewCount);

		for(size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
		{
			semaphores[i].Create(semaphoreCreateInfo, inOwner);
		}

		renderPass.Create(renderPassCreateInfo, extent, clearValues, inOwner);
		pipelineLayout.Create(pipelineLayoutCreateInfo, inOwner);
		graphicsPipelineCreateInfo.renderPass = renderPass.VkType();                            // Render pass description the pipeline is compatible with
		graphicsPipelineCreateInfo.layout = pipelineLayout.VkType();

		drawCommandBuffers.Create(commandBufferAllocateInfo, commandPool);

		// Create FrameBuffers
		frameBufferCreateInfo.renderPass = renderPass.VkType();

		for(size_t i = 0; i < imageViewCount; ++i)
		{
			// List of attachments 1 to 1 with render pass
			frameBufferCreateInfo.pAttachments = frameBufferAttachments[i].data();

			frameBuffers[i].Create(frameBufferCreateInfo, inOwner);
		}

		pipeline.Create(graphicsPipelineCreateInfo, inOwner);
	}

	RenderPass renderPass = {};
	GraphicsPipeline pipeline = {};
	PipelineLayout pipelineLayout = {};

	std::vector<FrameBuffer> frameBuffers = {};
	CommandBufferVector drawCommandBuffers = {};
	std::vector<Semaphore> semaphores = {};
};


}