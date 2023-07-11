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

void NtshEngn::AssetLoaderModule::calculateTangents(Mesh& mesh) {
	NTSHENGN_UNUSED(mesh);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();
}

std::array<std::array<float, 3>, 2> NtshEngn::AssetLoaderModule::calculateAABB(const Mesh& mesh) {
	NTSHENGN_UNUSED(mesh);
	NTSHENGN_MODULE_FUNCTION_NOT_IMPLEMENTED();

	return { std::array<float, 3>{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
}

extern "C" NTSHENGN_MODULE_API NtshEngn::AssetLoaderModuleInterface* createModule() {
	return new NtshEngn::AssetLoaderModule;
}

extern "C" NTSHENGN_MODULE_API void destroyModule(NtshEngn::AssetLoaderModuleInterface* m) {
	delete m;
}