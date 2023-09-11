#pragma once
#include "../Common/module_interfaces/ntshengn_asset_loader_module_interface.h"
#include "../Common/utils/ntshengn_utils_math.h"
#include "../Common/utils/ntshengn_utils_bimap.h"
#include "../external/cgltf/cgltf.h"
#include <string>
#include <forward_list>
#include <unordered_map>

namespace NtshEngn {

	class AssetLoaderModule : public AssetLoaderModuleInterface {
	public:
		AssetLoaderModule() : AssetLoaderModuleInterface("NutshellEngine Multi Asset Loader Module") {}

		// Loads the sound in file at path filePath
		Sound loadSound(const std::string& filePath);
		// Loads the image in file at path filePath
		Image loadImage(const std::string& filePath);
		// Loads the model in file at path filePath
		Model loadModel(const std::string& filePath);
		// Loads the font in file at path filePath
		Font loadFont(const std::string& filePath, float fontHeight);

		// Calculate tangents for mesh
		void calculateTangents(Mesh& mesh);

	private:
		void loadSoundWav(const std::string& filePath, Sound& sound);

		void loadImageStb(const std::string& filePath, Image& image);
		void loadImageFromMemory(void* data, size_t size, Image& image);

		void loadModelObj(const std::string& filePath, Model& model);

		void loadFontTtf(const std::string& filePath, float fontHeight, Font& font);

		void loadModelGltf(const std::string& filePath, Model& model);
		void loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, Math::mat4 modelMatrix, Bimap<uint32_t, cgltf_node*>& jointNodes);
		void loadGltfJoint(uint32_t jointIndex, Mesh& mesh, Math::mat4 jointMatrix, Bimap<uint32_t, cgltf_node*>& jointNodes, const std::unordered_map<uint32_t, size_t>& meshJoints);
		void loadGltfAnimation(Model& model, cgltf_animation* node, Bimap<uint32_t, cgltf_node*>& jointNodes);

	private:
		std::unordered_map<std::string, Image> m_internalImages;
		std::forward_list<Image> m_fontImages;

		ImageSampler trilinearSampler = { ImageSamplerFilter::Linear,
			ImageSamplerFilter::Linear,
			ImageSamplerFilter::Linear,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerBorderColor::IntOpaqueBlack,
			16.0f
		};
		ImageSampler nearestSampler = { ImageSamplerFilter::Nearest,
			ImageSamplerFilter::Nearest,
			ImageSamplerFilter::Nearest,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerAddressMode::ClampToEdge,
			ImageSamplerBorderColor::IntOpaqueBlack,
			0.0f
		};

		std::unordered_map<int, ImageSamplerFilter> m_gltfFilterToImageSamplerFilter = {
			{ 9728, ImageSamplerFilter::Nearest },
			{ 9729, ImageSamplerFilter::Linear },
			{ 9984, ImageSamplerFilter::Nearest },
			{ 9985, ImageSamplerFilter::Linear },
			{ 9986, ImageSamplerFilter::Nearest },
			{ 9987, ImageSamplerFilter::Linear }
		};
		std::unordered_map<int, ImageSamplerFilter> m_gltfFilterToImageSamplerFilterMipMap = {
			{ 9984, ImageSamplerFilter::Nearest },
			{ 9985, ImageSamplerFilter::Nearest },
			{ 9986, ImageSamplerFilter::Linear },
			{ 9987, ImageSamplerFilter::Linear },

			{ 9728, ImageSamplerFilter::Nearest }, // Cover case
			{ 9729, ImageSamplerFilter::Nearest } // Cover case
		};
		std::unordered_map<int, ImageSamplerAddressMode> m_gltfFilterToImageSamplerAddressMode = {
			{ 33071, ImageSamplerAddressMode::ClampToEdge },
			{ 33648, ImageSamplerAddressMode::MirroredRepeat },
			{ 10497, ImageSamplerAddressMode::Repeat }
		};
	};

}