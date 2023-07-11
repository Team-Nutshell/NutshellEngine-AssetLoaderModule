#pragma once
#include "../Common/module_interfaces/ntshengn_asset_loader_module_interface.h"
#include "../external/cgltf/cgltf.h"
#include "../external/nml/include/nml.h"
#include <vector>

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

		// Calculate tangents for mesh
		void calculateTangents(Mesh& mesh);
		// Calculate and return the mesh's AABB
		std::array<std::array<float, 3>, 2> calculateAABB(const Mesh& mesh);

	private:
		void loadSoundWav(const std::string& filePath, Sound& sound);

		void loadImageStb(const std::string& filePath, Image& image);

		void loadModelObj(const std::string& filePath, Model& model);

		void loadModelGltf(const std::string& filePath, Model& model);
		void loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, nml::mat4& modelMatrix);

	private:
		std::vector<Image> m_internalImages;
	};

}