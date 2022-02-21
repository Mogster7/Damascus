#pragma once

namespace dm
{

class Sampler : public IVulkanType<vk::Sampler>, public IOwned<Device>
{
	DM_TYPE_VULKAN_OWNED_BODY(Sampler, IOwned<Device>)
	DM_TYPE_VULKAN_OWNED_GENERIC(Sampler, Sampler)
};


}
