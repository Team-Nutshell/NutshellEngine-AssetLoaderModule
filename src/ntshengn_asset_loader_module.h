#pragma once
#include "../Common/modules/ntshengn_asset_loader_module_interface.h"

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
		// Loads the material in file at path filePath
		Material loadMaterial(const std::string& filePath);
		// Loads the bitmap font in file at path filePath
		Font loadFontBitmap(const std::string& filePath, float fontHeight);
		// Loads the SDF font in file at path filePath
		Font loadFontSDF(const std::string& filePath);
	};

}