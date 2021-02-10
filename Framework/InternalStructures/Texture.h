#pragma once

class Texture
{
public:
	void Create(const std::string& filepath, Device& owner);
	void Destroy();

	vk::DescriptorImageInfo GetDescriptor(
		vk::ImageLayout imageLayout
	);


	int32_t width = 0;
	int32_t height = 0;
	int32_t channels = 0;

	Image image;
	ImageView imageView;
	vk::UniqueSampler sampler;


	Device* m_owner;
};
