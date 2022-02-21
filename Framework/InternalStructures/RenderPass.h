//------------------------------------------------------------------------------
//
// File Name:	RenderPass.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once
namespace dm
{

//DM_TYPE(RenderPass)
class RenderPass : public IVulkanType<vk::RenderPass>, public IOwned<Device>
{
public:
DM_TYPE_VULKAN_OWNED_BODY(RenderPass, IOwned<Device>)

	void Create(
			const vk::RenderPassCreateInfo& info,
			vk::Extent2D inExtent,
			const std::vector<vk::ClearValue>& inClearValues,
			Device* inOwner
	)
	{
        IOwned::CreateOwned(inOwner);

		extent = inExtent;
		clearValues = inClearValues;

		DM_ASSERT_VK(owner->createRenderPass(&info, nullptr, &VkType()));
	}

    void Destroy()
    {
        if (created)
        {
            owner->destroyRenderPass(VkType());
            created = false;
        }
    }

	~RenderPass() noexcept override
	{
		Destroy();
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