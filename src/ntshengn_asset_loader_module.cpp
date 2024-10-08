#include "ntshengn_asset_loader_module.h"
#include "../Module/utils/ntshengn_module_defines.h"
#include "../Module/utils/ntshengn_dynamic_library.h"
#include "../Common/utils/ntshengn_defines.h"
#include "../Common/utils/ntshengn_enums.h"

NtshEngn::Sound NtshEngn::AssetLoaderModule::loadSound(const std::string& filePath) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return Sound();
}

NtshEngn::Image NtshEngn::AssetLoaderModule::loadImage(const std::string& filePath) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return Image();
}

NtshEngn::Model NtshEngn::AssetLoaderModule::loadModel(const std::string& filePath) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return Model();
}

NtshEngn::Material NtshEngn::AssetLoaderModule::loadMaterial(const std::string& filePath) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return Material();
}

NtshEngn::Font NtshEngn::AssetLoaderModule::loadFont(const std::string& filePath, float fontHeight) {
	NTSHENGN_UNUSED(filePath);
	NTSHENGN_UNUSED(fontHeight);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return Font();
}

extern "C" NTSHENGN_MODULE_API NtshEngn::AssetLoaderModuleInterface* createModule() {
	return new NtshEngn::AssetLoaderModule;
}

extern "C" NTSHENGN_MODULE_API void destroyModule(NtshEngn::AssetLoaderModuleInterface* m) {
	delete m;
}