//------------------------------------------------------------------------------
//
// File Name:	Pipeline.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

namespace bk {

//BK_TYPE(PipelineLayout)
class PipelineLayout : public IVulkanType<vk::PipelineLayout>, public IOwned<Device>
{
BK_TYPE_VULKAN_OWNED_BODY(PipelineLayout, IOwned<Device>)
BK_TYPE_VULKAN_OWNED_GENERIC(PipelineLayout, PipelineLayout)
};

//BK_TYPE(GraphicsPipeline)
class GraphicsPipeline : public IVulkanType<vk::Pipeline>, public IOwned<Device>
{
BK_TYPE_VULKAN_OWNED_BODY(GraphicsPipeline, IOwned<Device>)
BK_TYPE_VULKAN_OWNED_CREATE_FULL(GraphicsPipeline, Pipeline, ,GraphicsPipeline, GraphicsPipelines)

	void Create(const vk::GraphicsPipelineCreateInfo& createInfo, Device* owner)
	{
		Create(createInfo, owner, vk::PipelineCache(), 1);
	}
};


}