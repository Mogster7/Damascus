//------------------------------------------------------------------------------
//
// File Name:	CommandPool.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/23/2020
//
//------------------------------------------------------------------------------

CUSTOM_VK_DEFINE(CommandPool, CommandPool, Device)

vk::UniqueCommandBuffer CommandPool::BeginCommandBuffer()
{
    // Describe how to create our command buffer - we only create 1.
    vk::CommandBufferAllocateInfo allocateInfo{
        Get(),                      // Command pool
        vk::CommandBufferLevel::ePrimary, // Level
        1 };                               // Command buffer count

    // Create and move the first (and only) command buffer that gets created by the device.
    vk::UniqueCommandBuffer commandBuffer{
        std::move(m_owner->allocateCommandBuffersUnique(allocateInfo)[0])
    };

    // Define how this command buffer should begin.
    vk::CommandBufferBeginInfo beginInfo{
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // Flags
        nullptr                                         // Inheritance info
    };

    // Request the command buffer to begin itself.
    commandBuffer->begin(beginInfo);

    // The caller will now take ownership of the command buffer and is
    // responsible for invoking the 'endCommandBuffer' upon it.
    return commandBuffer;
}

void CommandPool::EndCommandBuffer(vk::CommandBuffer buffer) const
{
    buffer.end();

    // Configure a submission object to send to the graphics queue
    vk::SubmitInfo submitInfo{
        0,              // Wait semaphore count
        nullptr,        // Wait semaphores
        nullptr,        // Wait destination stage mask
        1,              // Command buffer count
        &buffer,        // Command buffers,
        0,              // Signal semaphore count
        nullptr         // Signal semaphores
    };

    // Submit it with a fence, making it wait until the command buffer is done submitting
    utils::CheckVkResult(m_owner->graphicsQueue.submit(1, &submitInfo, vk::Fence()),
        "Failed to submit command buffer to the queue");
    m_owner->graphicsQueue.waitIdle();
}


