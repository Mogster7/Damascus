
#include "CommandBuffer.h"

namespace dm
{

void CommandBufferVector::Create(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo, CommandPool* inOwner)
{
    Destroy();

    IOwned<CommandPool>::CreateOwned(inOwner);
	commandBuffers.resize(OwnerGet<Device>().ImageCount());
	DM_ASSERT_VK(OwnerGet<Device>().allocateCommandBuffers(&commandBufferAllocateInfo, commandBuffers.data()));
}

CommandBufferVector::~CommandBufferVector() noexcept
{
    Destroy();
}

void CommandBufferVector::Destroy()
{
    if (created)
    {
        OwnerGet<Device>().freeCommandBuffers(
            owner->VkType(),
            static_cast<uint32_t>(commandBuffers.size()),
            commandBuffers.data()
        );
        created = false;
    }
}

}
