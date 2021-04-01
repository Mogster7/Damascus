//------------------------------------------------------------------------------
//
// File Name:	RenderPass.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

class Device;

CUSTOM_VK_DECLARE_NO_CREATE(RenderPass, RenderPass, Device)
public:
	void Create(const vk::RenderPassCreateInfo& info,
				Device& owner,
				vk::Extent2D extent,
				const std::vector<vk::ClearValue>& clearValues);

	void Begin(vk::CommandBuffer cmdBuf,
			   vk::Framebuffer framebuffer,
			   vk::Pipeline pipeline)
	{
		vk::RenderPassBeginInfo renderPassInfo;
		renderPassInfo.renderPass = Get();
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();
		renderPassInfo.framebuffer = framebuffer;

		cmdBuf.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
		cmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	}
	
	void End(vk::CommandBuffer cmdBuf)
	{
		cmdBuf.endRenderPass();
	}


	vk::Extent2D extent;
	std::vector<vk::ClearValue> clearValues;
};



