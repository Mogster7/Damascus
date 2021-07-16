#pragma once

namespace dm
{

//BK_TYPE(Sampler)
class Sampler : public IVulkanType<vk::Sampler>, public IOwned<Device>
{
	BK_TYPE_VULKAN_OWNED_BODY(Sampler, IOwned<Device>)
	BK_TYPE_VULKAN_OWNED_GENERIC(Sampler, Sampler)
};


}
