
namespace dm
{

void CommandBufferVector::Create(const vk::CommandBufferAllocateInfo& commandBufferAllocateInfo, CommandPool* inOwner)
{
	IOwned<CommandPool>::Create(inOwner);
	resize(commandBufferAllocateInfo.commandBufferCount);
	DM_ASSERT_VK(OwnerGet<Device>().allocateCommandBuffers(&commandBufferAllocateInfo, data()));
}

CommandBufferVector::~CommandBufferVector() noexcept
{
	OwnerGet<Device>().freeCommandBuffers(
		owner->VkType(),
		static_cast<uint32_t>(size()),
		data()
	);
}

}
