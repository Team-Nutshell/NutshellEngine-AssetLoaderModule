#pragma once
#include "../Common/module_interfaces/ntshengn_asset_loader_module_interface.h"

namespace NtshEngn {

	class AssetLoaderModule : public AssetLoaderModuleInterface {
	public:
		AssetLoaderModule() : AssetLoaderModuleInterface("NutshellEngine Default Asset Loader Module") {}

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
	};

}