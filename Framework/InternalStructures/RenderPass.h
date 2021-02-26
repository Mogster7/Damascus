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


	void RenderObjects(vk::CommandBuffer cmdBuf,
					   vk::DescriptorSet descriptorSet,
					   vk::PipelineLayout pipelineLayout,
					   Object* objects,
					   uint32_t numObjects = 1)
	{
		//// Bind descriptor sets
		cmdBuf.bindDescriptorSets(
			// Point of pipeline and layout
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			1,
			&descriptorSet, // 1 to 1 with command buffers
			0,
			nullptr
		);

		for (size_t j = 0; j < numObjects; ++j)
		{
			auto& object = objects[j];
			auto& mesh = objects[j].mesh;
			mesh.Bind(cmdBuf);

			cmdBuf.pushConstants(
				pipelineLayout,
				// Stage
				vk::ShaderStageFlagBits::eVertex,
				// Offset
				0,
				// Size of data being pushed
				sizeof(glm::mat4),
				// Actual data being pushed
				&object.GetModel()
			);

			mesh.Draw(cmdBuf);
		}
	}	
	
	template <class VertexType>
	void RenderObjects(vk::CommandBuffer cmdBuf,
					   vk::DescriptorSet descriptorSet,
					   vk::PipelineLayout pipelineLayout,
					   Mesh<VertexType>* objects,
					   uint32_t numObjects = 1)
	{
		//// Bind descriptor sets
		cmdBuf.bindDescriptorSets(
			// Point of pipeline and layout
			vk::PipelineBindPoint::eGraphics,
			pipelineLayout,
			0,
			1,
			&descriptorSet, // 1 to 1 with command buffers
			0,
			nullptr
		);

		for (size_t j = 0; j < numObjects; ++j)
		{
			auto& mesh = objects[j];
			mesh.Bind(cmdBuf);
			Object::PushIdentityModel(cmdBuf, pipelineLayout);
			mesh.Draw(cmdBuf);
		}
	}

	
	void End(vk::CommandBuffer cmdBuf)
	{
		cmdBuf.endRenderPass();
	}


	vk::Extent2D extent;
	std::vector<vk::ClearValue> clearValues;
};



