#include "ntshengn_asset_loader_module.h"
#include "../Module/utils/ntshengn_module_defines.h"
#include "../Module/utils/ntshengn_dynamic_library.h"
#include "../Common/utils/ntshengn_defines.h"
#include "../Common/utils/ntshengn_enums.h"

void NtshEngn::AssetLoaderModule::init() {
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();
}

void NtshEngn::AssetLoaderModule::update(float dt) {
	NTSHENGN_UNUSED(dt);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();
}

void NtshEngn::AssetLoaderModule::destroy() {
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();
}

bool NtshEngn::AssetLoaderModule::loadSound(const std::string& filePath, Sound& sound) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(sound);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

bool NtshEngn::AssetLoaderModule::loadImage(const std::string& filePath, Image& image) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(image);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

bool NtshEngn::AssetLoaderModule::loadModel(const std::string& filePath, Model& model) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(model);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

bool NtshEngn::AssetLoaderModule::loadMaterial(const std::string& filePath, Material& material) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(material);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

bool NtshEngn::AssetLoaderModule::loadFontBitmap(const std::string& filePath, float fontHeight, Font& font) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(fontHeight);
	NTSHENGN_UNUSED(font);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

bool NtshEngn::AssetLoaderModule::loadFontSDF(const std::string& filePath, Font& font) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(font);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return false;
}

extern "C" NTSHENGN_MODULE_API NtshEngn::AssetLoaderModuleInterface* createModule() {
	return new NtshEngn::AssetLoaderModule;
}

extern "C" NTSHENGN_MODULE_API void destroyModule(NtshEngn::AssetLoaderModuleInterface* m) {
	delete m;
}