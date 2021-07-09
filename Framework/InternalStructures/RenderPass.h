//------------------------------------------------------------------------------
//
// File Name:	RenderPass.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace bk {

//BK_TYPE(RenderPass)
class RenderPass : public IVulkanType<vk::RenderPass>, public IOwned<Device>
{
public:
BK_TYPE_VULKAN_OWNED_BODY(RenderPass, IOwned<Device>)

	void Create(
			const vk::RenderPassCreateInfo& info,
			vk::Extent2D inExtent,
			const std::vector<vk::ClearValue>& inClearValues,
			Device* inOwner
	)
	{
		IOwned::Create(inOwner, [this]()
		{
			owner->destroyRenderPass(VkType());
		});

		extent = inExtent;
		clearValues = inClearValues;

		ASSERT_VK(owner->createRenderPass(&info, nullptr, &VkType()));
	}

	void Begin(
			vk::CommandBuffer cmdBuf,
			vk::Framebuffer framebuffer,
			vk::Pipeline pipeline,
			vk::SubpassContents subpassContents = vk::SubpassContents::eInline
	)
	{
		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = VkType();
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		renderPassInfo.framebuffer = framebuffer;

		cmdBuf.beginRenderPass(&renderPassInfo, subpassContents);

		if (subpassContents == vk::SubpassContents::eInline)
		{
			cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		}
	}

	void End(vk::CommandBuffer cmdBuf)
	{
		cmdBuf.endRenderPass();
	}


	vk::Extent2D extent;
	std::vector<vk::ClearValue> clearValues;
};


}