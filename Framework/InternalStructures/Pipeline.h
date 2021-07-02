//------------------------------------------------------------------------------
//
// File Name:	Pipeline.h
// Author(s):	Jonathan Bourim (j.bourim)
// Date:        6/23/2020 
//
//------------------------------------------------------------------------------
#pragma once

CUSTOM_VK_DECLARE_DERIVE(PipelineLayout, PipelineLayout, Device)

};

namespace vk
{
    using GraphicsPipeline = Pipeline;
}

using VkGraphicsPipeline = VkPipeline;

CUSTOM_VK_DECLARE_DERIVE_NC(GraphicsPipeline, GraphicsPipeline, Device)

public:
    CUSTOM_VK_DERIVED_CREATE_FULL(GraphicsPipeline, Pipeline, Device, , GraphicsPipeline, GraphicsPipelines)
};



