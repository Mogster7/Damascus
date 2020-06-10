//------------------------------------------------------------------------------
//
// File Name:	Buffer.cpp
// Author(s):	Jonathan Bourim (j.bourim)
// Date:		6/9/2020
//
//------------------------------------------------------------------------------
#include "RenderingStructures.hpp"
#include "Renderer.h"


Buffer::Buffer(vk::PhysicalDevice physDevice, vk::Device device, vk::DeviceSize bufferSize, 
    vk::MemoryPropertyFlags bufferProps, vk::BufferUsageFlags bufferUsage, 
    vk::SharingMode sharingMode
    ) :

    m_PhysicalDevice(physDevice), m_Device(device),
    m_Properties(bufferProps), m_Usage(bufferUsage),
    m_Size(bufferSize)
{
    // Create buffer
    vk::BufferCreateInfo createInfo;
    createInfo.size = bufferSize;
    createInfo.usage = bufferUsage;
    createInfo.sharingMode = sharingMode;

    utils::CheckVkResult(m_Device.createBuffer(&createInfo, nullptr, &m_Buffer), 
        "Failed to create vertex buffer");

    // Get buffer memory requirements
    vk::MemoryRequirements memReq;
    m_Device.getBufferMemoryRequirements(m_Buffer, &memReq);

    // Allocate memory
    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    // Provide index of memory type on physical device that has the required bits
    // Memory is visible to the host, and allows placement of data straight into buffer after mapping
    // (No flushing required)
    allocInfo.memoryTypeIndex = FindMemoryTypeIndex(memReq.memoryTypeBits);

    // Allocate memory to device memory
    utils::CheckVkResult(m_Device.allocateMemory(&allocInfo, nullptr, &m_Memory),
        "Failed to allocate vertex buffer memory");

    // Allocate memory to the vertex buffer
    m_Device.bindBufferMemory(m_Buffer, m_Memory, 0);
    
}

void Buffer::StageTransfer(vk::Buffer src, vk::Buffer dst, vk::DeviceSize size)
{
    vk::Device device = Renderer::GetDevice();
    vk::Queue transferQueue = Renderer::GetGraphicsQueue();
    vk::CommandPool transferCmdPool = Renderer::GetGraphicsPool();

    // Command buffer to hold transfer commands
    vk::CommandBuffer transferCmdBuffer;

    // Command Buffer details
    vk::CommandBufferAllocateInfo allocInfo = {};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = transferCmdPool;
    allocInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    device.allocateCommandBuffers(&allocInfo, &transferCmdBuffer);

    // Create begin info for command buffer recording
    vk::CommandBufferBeginInfo beginInfo;
    // We're only using the command buffer once
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    // Begin recording transfer commands
    transferCmdBuffer.begin(&beginInfo);

    // Create copy region for command buffer
    vk::BufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    // Command to copy src to dst
    transferCmdBuffer.copyBuffer(src, dst, 1, &copyRegion);
    
    transferCmdBuffer.end();

    // Queue submission info
    vk::SubmitInfo submitInfo;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCmdBuffer;
    
    // Submit transfer command to transfer queue and wait until done (not optimal)
    transferQueue.submit(1, &submitInfo, {});
    transferQueue.waitIdle();

    // Free temporary command buffer back to pool
    device.freeCommandBuffers(transferCmdPool, transferCmdBuffer);
}

void Buffer::Destroy()
{
    m_Device.freeMemory(m_Memory);
    m_Device.destroyBuffer(m_Buffer);
}

uint32_t Buffer::FindMemoryTypeIndex(uint32_t allowedTypes) const
{
    // Get properties of physical device memory
    vk::PhysicalDeviceMemoryProperties memProps;
    m_PhysicalDevice.getMemoryProperties(&memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
    {
        // Index of memory type must match corresponding bit in allowed types
        // And that the property types are valid by checking if the result
        // of propertyFlags and properties give us the original property field back
        // In other words, the propertyFlags allowed either match or exceed the number
        // of properties described by the local variable 'properties'
        if ((allowedTypes & (1 << i))
            && (memProps.memoryTypes[i].propertyFlags & m_Properties) == m_Properties)
        {
            // Memory is valid, return index
            return i;
        }

    }

    throw std::runtime_error("Failed to find a correct physical device memory index for our flags");
}

VertexBuffer::VertexBuffer(vk::PhysicalDevice physDevice, vk::Device device, 
    const std::vector<Vertex>& vertices) :

    // Our buffer, destination
    Buffer(
        physDevice, device, 
        sizeof(Vertex) * vertices.size(), // Buffer size
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst
    ), 
    m_VertexCount(vertices.size())
{
    // Staging buffer, source
    Buffer stagingBuffer(
        physDevice, device,
        m_Size,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        vk::BufferUsageFlagBits::eTransferSrc
    );

    // Create pointer to memory
    void* data; 

    // Map and copy vertices to the memory, then unmap
    m_Device.mapMemory(stagingBuffer.GetMemory(), 0, m_Size, {}, &data);
    std::memcpy(data, vertices.data(), (size_t)m_Size);
    m_Device.unmapMemory(stagingBuffer.GetMemory());

    // Copy staging buffer to GPU-side vertex buffer
    StageTransfer(stagingBuffer.GetBuffer(), m_Buffer, m_Size);

    // Cleanup
    stagingBuffer.Destroy();
}
