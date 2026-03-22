#pragma once
#include "../Common/modules/ntshengn_asset_loader_module_interface.h"

namespace NtshEngn {

	class AssetLoaderModule : public AssetLoaderModuleInterface {
	public:
		AssetLoaderModule() : AssetLoaderModuleInterface("NutshellEngine Default Asset Loader Module") {}

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
	};

}