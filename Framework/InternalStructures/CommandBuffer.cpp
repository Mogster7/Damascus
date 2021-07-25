
namespace dm
{

void CommandBufferVector::Create(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo, Device* inOwner)
{
	IOwned<Device>::Create(inOwner);
	ASSERT_VK(owner->allocateCommandBuffers(&commandBufferAllocateInfo, data()));
}

CommandBufferVector::~CommandBufferVector() noexcept
{
	OwnerGet<RenderingContext>().commandPool.FreeCommandBuffers(
		*this
	);
}

}
