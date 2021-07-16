#pragma once

namespace dm {

//BK_TYPE(Texture)
class Texture : public IOwned<Device>
{
public:
	BK_TYPE_OWNED_BODY(Texture, IOwned<Device>)

	void Create(const std::string& filepath, Device* owner);

	vk::DescriptorImageInfo GetDescriptor(
		vk::ImageLayout imageLayout
	);


	int32_t width = 0;
	int32_t height = 0;
	int32_t channels = 0;

	Image image;
	ImageView imageView;
	Sampler sampler;
};

}