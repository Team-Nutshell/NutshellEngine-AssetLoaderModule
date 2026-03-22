#pragma once
#include "../Common/modules/ntshengn_asset_loader_module_interface.h"
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

		void init();
		void update(float dt);
		void destroy();

		// Loads the sound in file at path filePath
		bool loadSound(const std::string& filePath, Sound& sound);
		// Loads the image in file at path filePath
		bool loadImage(const std::string& filePath, Image& image);
		// Loads the model in file at path filePath
		bool loadModel(const std::string& filePath, Model& model);
		// Loads the material in file at path filePath
		bool loadMaterial(const std::string& filePath, Material& material);
		// Loads the bitmap font in file at path filePath
		bool loadFontBitmap(const std::string& filePath, float fontHeight, Font& font);
		// Loads the SDF font in file at path filePath
		bool loadFontSDF(const std::string& filePath, Font& font);

	private:
		bool loadSoundWav(const std::string& filePath, Sound& sound);
		bool loadSoundOgg(const std::string& filePath, Sound& sound);

		bool loadImageStb(const std::string& filePath, Image& image);
		bool loadImageFromMemory(void* data, size_t size, Image& image);

		bool loadModelObj(const std::string& filePath, Model& model);

		std::unordered_map<std::string, Material> loadMaterialMtl(const std::string& filePath);

		bool loadFontBitmapTtf(const std::string& filePath, float fontHeight, Font& font);
		bool loadFontSDFTtf(const std::string& filePath, Font& font);

		bool loadModelGltf(const std::string& filePath, Model& model);
		void loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, Bimap<uint32_t, cgltf_node*>& jointNodes);
		void loadGltfAnimation(Model& model, cgltf_animation* node, Bimap<uint32_t, cgltf_node*>& jointNodes);

	private:
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