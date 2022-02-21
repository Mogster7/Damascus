//------------------------------------------------------------------------------
//
// File Name:	Pipeline.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------

#include "Pipeline.h"

namespace dm
{

void IGraphicsPipeline::Destroy()
{
    if (created)
    {
        renderPass.Destroy();
        pipelineLayout.Destroy();
        frameBuffers.clear();
        for(auto& sem : semaphores) sem.Destroy();
        owner->destroyPipeline(VkType());
        created = false;
    }
}

Renderer& IGraphicsPipeline::Renderer()
{
    return OwnerGet<class Renderer>();
}

vk::CommandBuffer IGraphicsPipeline::Begin()
{
    auto commandBuffer = drawBuffers[owner->ImageIndex()];
    vk::CommandBufferBeginInfo beginInfo = {};
    DM_ASSERT_VK(commandBuffer.begin(&beginInfo));

    return commandBuffer;
}

void IGraphicsPipeline::End()
{
    GetCommandBufferPtr()->end();
}

vk::CommandBuffer* IGraphicsPipeline::GetCommandBufferPtr()
{
    return &drawBuffers.commandBuffers[owner->ImageIndex()];
}


}