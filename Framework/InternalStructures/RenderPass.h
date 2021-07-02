//------------------------------------------------------------------------------
//
// File Name:	RenderPass.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

class Device;

BK_TYPE(RenderPass)
class RenderPass : public IVulkanType<vk::RenderPass>, public IOwned<Device>
{
public:
	RenderPass(const vk::RenderPassCreateInfo& info,
			   const Device& owner,
			   vk::Extent2D extent,
			   const std::vector<vk::ClearValue>& clearValues)
		: IOwned<Device>(owner)
		, extent(extent)
		, clearValues(clearValues)
	{
		OwnerCreate([&info, this](const Device& owner)
			{
				return owner.createRenderPass(&info, nullptr, &this->GetBase());
			}
		);
	}

	void Begin(vk::CommandBuffer cmdBuf,
			   vk::Framebuffer framebuffer,
			   vk::Pipeline pipeline,
			   vk::SubpassContents subpassContents = vk::SubpassContents::eInline)
	{
		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = GetBase();
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



