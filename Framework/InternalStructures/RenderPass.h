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

	template <class VertexType>
	void RenderObjects(vk::CommandBuffer cmdBuf,
					   vk::DescriptorSet descriptorSet,
					   vk::PipelineLayout pipelineLayout,
					   Mesh<VertexType>* objects,
					   uint32_t numObjects = 1)
	{
		for (size_t j = 0; j < numObjects; ++j)
		{
			auto& mesh = objects[j];
			// Buffers to bind
			vk::Buffer vertexBuffers[] = { mesh.GetVertexBuffer() };
			// Offsets
			vk::DeviceSize offsets[] = { 0 };

			cmdBuf.bindVertexBuffers(0, 1, vertexBuffers, offsets);
			bool hasIndex = mesh.GetIndexCount() > 0;
			if (hasIndex)
				cmdBuf.bindIndexBuffer(mesh.GetIndexBuffer(), 0, vk::IndexType::eUint32);

			cmdBuf.pushConstants(
				pipelineLayout,
				// Stage
				vk::ShaderStageFlagBits::eVertex,
				// Offset
				0,
				// Size of data being pushed
				sizeof(glm::mat4),
				// Actual data being pushed
				&objects[j].model
			);

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

			// Execute pipeline
			hasIndex ?
				cmdBuf.drawIndexed(mesh.GetIndexCount(), 1, 0, 0, 0)
				:
				cmdBuf.draw(mesh.GetVertexCount(), 1, 0, 0);
		}
	}
	
	void End(vk::CommandBuffer cmdBuf)
	{
		cmdBuf.endRenderPass();
	}


	vk::Extent2D extent;
	std::vector<vk::ClearValue> clearValues;
};



