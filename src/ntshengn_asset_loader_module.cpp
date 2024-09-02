#include "ntshengn_asset_loader_module.h"
#include "../Common/asset_manager/ntshengn_asset_manager_interface.h"
#include "../Module/utils/ntshengn_module_defines.h"
#include "../Module/utils/ntshengn_dynamic_library.h"
#include "../Common/utils/ntshengn_defines.h"
#include "../Common/utils/ntshengn_enums.h"
#include "../Common/utils/ntshengn_utils_file.h"
#define CGLTF_IMPLEMENTATION
#include "../external/cgltf/cgltf.h"
#if defined(NTSHENGN_COMPILER_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#elif defined(NTSHENGN_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstringop-overflow"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#if defined(NTSHENGN_COMPILER_GCC)
#pragma GCC diagnostic pop
#elif defined(NTSHENGN_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "../external/stb/stb_truetype.h"
#if defined(NTSHENGN_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4245)
#pragma warning(disable : 4456)
#pragma warning(disable : 4457)
#pragma warning(disable : 4701)
#endif
#include "../external/stb/stb_vorbis.c"
#if defined(NTSHENGN_COMPILER_MSVC)
#pragma warning(pop)
#endif
#include <cstddef>
#include <cmath>
#include <iterator>
#include <algorithm>
#include <numeric>

NtshEngn::Sound NtshEngn::AssetLoaderModule::loadSound(const std::string& filePath) {
	Sound newSound;

	std::string extension = File::extension(filePath);
	if (extension == "wav") {
		loadSoundWav(filePath, newSound);
	}
	else if (extension == "ogg") {
		loadSoundOgg(filePath, newSound);
	}
	else {
		NTSHENGN_MODULE_WARNING("Sound file extension \"." + extension + "\" not supported.");
	}

	return newSound;
}

NtshEngn::Image NtshEngn::AssetLoaderModule::loadImage(const std::string& filePath) {
	Image newImage;

	std::string extension = File::extension(filePath);
	if ((extension == "jpg") ||
		(extension == "jpeg") ||
		(extension == "png") ||
		(extension == "tga") ||
		(extension == "bmp") ||
		(extension == "gif")) {
		loadImageStb(filePath, newImage);
	}
	else {
		NTSHENGN_MODULE_WARNING("Image file extension \"." + extension + "\" not supported.");
	}

	return newImage;
}

NtshEngn::Model NtshEngn::AssetLoaderModule::loadModel(const std::string& filePath) {
	Model newModel;

	std::string extension = File::extension(filePath);
	if (extension == "obj") {
		loadModelObj(filePath, newModel);
	}
	else if ((extension == "gltf") ||
		(extension == "glb")) {
		loadModelGltf(filePath, newModel);
	}
	else {
		NTSHENGN_MODULE_WARNING("Model file extension \"." + extension + "\" not supported.");
	}

	return newModel;
}

NtshEngn::Material NtshEngn::AssetLoaderModule::loadMaterial(const std::string& filePath) {
	Material newMaterial;

	std::string extension = File::extension(filePath);
	NTSHENGN_MODULE_WARNING("Material file extension \"." + extension + "\" not supported.");

	return newMaterial;
}

NtshEngn::Font NtshEngn::AssetLoaderModule::loadFont(const std::string& filePath, float fontHeight) {
	Font newFont;

	std::string extension = File::extension(filePath);
	if (extension == "ttf") {
		loadFontTtf(filePath, fontHeight, newFont);
	}
	else {
		NTSHENGN_MODULE_WARNING("Font file extension \"." + extension + "\" not supported.");
	}

	return newFont;
}

void NtshEngn::AssetLoaderModule::loadSoundWav(const std::string& filePath, Sound& sound) {
	char buffer[4];
	int64_t tmp = 0;
	std::vector<char> data;

	std::ifstream file(filePath, std::ios::binary);

	// Open file
	if (!file.is_open()) {
		NTSHENGN_MODULE_WARNING("Could not open sound file \"" + filePath + "\".");
		return;
	}

	// RIFF header
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read \"RIFF\" for sound file \"" + filePath + "\".");
		return;
	}
	if (strncmp(buffer, "RIFF", 4) != 0) {
		NTSHENGN_MODULE_WARNING("File \"" + filePath + "\" is not a valid WAVE sound file (RIFF header missing).");
		return;
	}

	// Size
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read size for sound file \"" + filePath + "\".");
		return;
	}

	// WAVE header
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read \"WAVE\" for sound file \"" + filePath + "\".");
		return;
	}
	if (strncmp(buffer, "WAVE", 4) != 0) {
		NTSHENGN_MODULE_WARNING("File \"" + filePath + "\" is not a valid WAVE sound file (WAVE header missing).");
		return;
	}

	// fmt/0
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read fmt/0 for sound file \"" + filePath + "\".");
		return;
	}

	// 16
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read 16 for sound file \"" + filePath + "\".");
		return;
	}

	// PCM
	if (!file.read(buffer, 2)) {
		NTSHENGN_MODULE_WARNING("Could not read PCM for sound file \"" + filePath + "\".");
		return;
	}

	// Channels
	if (!file.read(buffer, 2)) {
		NTSHENGN_MODULE_WARNING("Could not read the number of channels for sound file \"" + filePath + "\".");
		return;
	}
	memcpy(&tmp, buffer, 2);
	sound.channels = static_cast<uint8_t>(tmp);

	// Sample rate
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read sample rate for sound file \"" + filePath + "\".");
		return;
	}
	memcpy(&tmp, buffer, 4);
	sound.sampleRate = static_cast<int32_t>(tmp);

	// Byte rate ((sampleRate * bitsPerSample * channels) / 8)
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read byte rate for sound file \"" + filePath + "\".");
		return;
	}

	// Block align ((bitsPerSample * channels) / 8)
	if (!file.read(buffer, 2)) {
		NTSHENGN_MODULE_WARNING("Could not read block align for sound file \"" + filePath + "\".");
		return;
	}

	// Bits per sample
	if (!file.read(buffer, 2)) {
		NTSHENGN_MODULE_WARNING("Could not read bits per sample for sound file \"" + filePath + "\".");
		return;
	}
	memcpy(&tmp, buffer, 2);
	sound.bitsPerSample = static_cast<uint8_t>(tmp);

	// data header
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read \"data\" for sound file \"" + filePath + "\".");
		return;
	}
	if (strncmp(buffer, "data", 4) != 0) {
		NTSHENGN_MODULE_WARNING("File \"" + filePath + "\" is not a valid WAVE sound file (data header missing).");
		return;
	}

	// Data size
	if (!file.read(buffer, 4)) {
		NTSHENGN_MODULE_WARNING("Could not read data size for sound file \"" + filePath + "\".");
		return;
	}
	memcpy(&tmp, buffer, 4);
	sound.size = static_cast<size_t>(tmp);

	if (file.eof()) {
		NTSHENGN_MODULE_WARNING("File \"" + filePath + "\" is not a valid WAVE sound file (data missing).");
		return;
	}
	if (file.fail()) {
		NTSHENGN_MODULE_WARNING("Unknown error while loading \"" + filePath + "\" WAVE sound file.");
		return;
	}

	// Data
	data.resize(sound.size);
	file.read(&data[0], sound.size);
	sound.data.insert(sound.data.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
	data.erase(data.begin(), data.end());

	file.close();
}

void NtshEngn::AssetLoaderModule::loadSoundOgg(const std::string& filePath, Sound& sound) {
	int channels;
	int size;
	short* data;
	size = stb_vorbis_decode_filename(filePath.c_str(), &channels, &sound.sampleRate, &data);

	if (size < 0) {
		NTSHENGN_MODULE_WARNING("Unknown error when loading \"" + filePath + "\" Ogg Vorbis sound file.");
	}

	sound.channels = static_cast<uint8_t>(channels);
	sound.size = static_cast<size_t>(size) * sound.channels * (sizeof(int16_t) / sizeof(uint8_t));
	sound.bitsPerSample = 16;
	sound.data.resize(sound.size);
	memcpy(sound.data.data(), reinterpret_cast<uint8_t*>(data), sound.size);
}

void NtshEngn::AssetLoaderModule::loadImageStb(const std::string& filePath, Image& image) {
	int width;
	int height;
	int texChannels;

	stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		NTSHENGN_MODULE_WARNING("Could not load image file \"" + filePath + "\".");
		return;
	}

	image.width = static_cast<uint32_t>(width);
	image.height = static_cast<uint32_t>(height);
	image.format = ImageFormat::R8G8B8A8;
	image.colorSpace = ImageColorSpace::Linear;
	image.data.resize(width * height * 4);
	std::copy(pixels, pixels + (width * height * 4), image.data.begin());

	stbi_image_free(pixels);
}

void NtshEngn::AssetLoaderModule::loadImageFromMemory(void* data, size_t size, Image& image) {
	int width;
	int height;
	int texChannels;

	stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(data), static_cast<int>(size), &width, &height, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		NTSHENGN_MODULE_WARNING("Could not load image from memory.");
		return;
	}

	image.width = static_cast<uint32_t>(width);
	image.height = static_cast<uint32_t>(height);
	image.format = ImageFormat::R8G8B8A8;
	image.colorSpace = ImageColorSpace::Linear;
	image.data.resize(width * height * 4);
	std::copy(pixels, pixels + (width * height * 4), image.data.begin());

	stbi_image_free(pixels);
}

void NtshEngn::AssetLoaderModule::loadModelObj(const std::string& filePath, Model& model) {
	std::ifstream file(filePath);

	// Open file
	if (!file.is_open()) {
		NTSHENGN_MODULE_WARNING("Could not open model file \"" + filePath + "\".");
		return;
	}

	std::vector<Math::vec3> positions;
	std::vector<Math::vec3> normals;
	std::vector<Math::vec2> uvs;

	std::unordered_map<std::string, uint32_t> uniqueVertices;
	model.primitives.push_back(ModelPrimitive());
	ModelPrimitive* currentPrimitive = &model.primitives.back();
	currentPrimitive->mesh.topology = MeshTopology::TriangleList;

	std::string modelDirectory = filePath.substr(0, filePath.rfind('/'));
	std::unordered_map<std::string, Material> mtlMaterials;

	bool hasNormals = false;
	bool hasUvs = false;

	std::string line;
	while (std::getline(file, line)) {
		// Ignore comment
		if (line[0] == '#') {
			continue;
		}

		// Parse line
		std::vector<std::string> tokens;
		size_t spacePosition = 0;
		while ((spacePosition = line.find(' ')) != std::string::npos) {
			tokens.push_back(line.substr(0, spacePosition));
			line.erase(0, spacePosition + 1);
		}
		tokens.push_back(line);

		// Parse tokens
		// Position
		if (tokens[0] == "v") {
			positions.push_back({
				static_cast<float>(std::atof(tokens[1].c_str())),
				static_cast<float>(std::atof(tokens[2].c_str())),
				static_cast<float>(std::atof(tokens[3].c_str()))
				});
		}
		// Normal
		else if (tokens[0] == "vn") {
			normals.push_back({
				static_cast<float>(std::atof(tokens[1].c_str())),
				static_cast<float>(std::atof(tokens[2].c_str())),
				static_cast<float>(std::atof(tokens[3].c_str()))
				});
		}
		// UV
		else if (tokens[0] == "vt") {
			uvs.push_back({
				static_cast<float>(std::atof(tokens[1].c_str())),
				static_cast<float>(std::atof(tokens[2].c_str()))
				});
		}
		// Object
		else if (tokens[0] == "o") {
			if (!currentPrimitive->mesh.indices.empty()) {
				if (hasNormals && hasUvs) {
					assetManager->calculateTangents(currentPrimitive->mesh);
				}
				model.primitives.push_back(ModelPrimitive());
				currentPrimitive = &model.primitives.back();
				uniqueVertices.clear();
				hasNormals = false;
				hasUvs = false;
			}
		}
		// Material
		else if (tokens[0] == "mtllib") {
			std::string materialPath = modelDirectory + "/" + tokens[1];
			mtlMaterials = loadMaterialMtl(materialPath);
		}
		else if (tokens[0] == "usemtl") {
			if (!currentPrimitive->mesh.indices.empty()) {
				if (hasNormals && hasUvs) {
					assetManager->calculateTangents(currentPrimitive->mesh);
				}
				model.primitives.push_back(ModelPrimitive());
				currentPrimitive = &model.primitives.back();
				uniqueVertices.clear();
				hasNormals = false;
				hasUvs = false;
			}
			if (mtlMaterials.find(tokens[1]) != mtlMaterials.end()) {
				currentPrimitive->material = mtlMaterials[tokens[1]];
			}
		}
		// Face
		else if (tokens[0] == "f") {
			std::vector<uint32_t> tmpIndices;
			for (size_t i = 1; i < tokens.size(); i++) {
				Vertex vertex = {};

				std::string tmp = tokens[i];
				std::vector<std::string> valueIndices;
				size_t slashPosition = 0;
				while ((slashPosition = tmp.find('/')) != std::string::npos) {
					valueIndices.push_back(tmp.substr(0, slashPosition));
					tmp.erase(0, slashPosition + 1);
				}
				valueIndices.push_back(tmp);

				for (size_t j = 0; j < valueIndices.size(); j++) {
					if (valueIndices[j] != "") {
						// v/vt/vn
						// Position index
						if (j == 0) {
							vertex.position = positions[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1];
						}
						// UV index
						else if (j == 1) {
							hasUvs = true;
							vertex.uv = uvs[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1];
						}
						// Normal index
						else if (j == 2) {
							hasNormals = true;
							vertex.normal = normals[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1];
						}
					}
				}

				if (!hasUvs) {
					vertex.uv = Math::vec2(0.5f, 0.5f);
				}
				vertex.tangent = Math::vec4(0.5f, 0.5f, 0.5f, 1.0f);

				if (uniqueVertices.count(tokens[i]) == 0) {
					uniqueVertices[tokens[i]] = static_cast<uint32_t>(currentPrimitive->mesh.vertices.size());
					currentPrimitive->mesh.vertices.push_back(vertex);
				}
				tmpIndices.push_back(uniqueVertices[tokens[i]]);
			}

			// Face can be a triangle, a rectangle or a N-gons
			// Triangle
			if (tmpIndices.size() == 3) {
				currentPrimitive->mesh.indices.insert(currentPrimitive->mesh.indices.end(), std::make_move_iterator(tmpIndices.begin()), std::make_move_iterator(tmpIndices.end()));
			}
			// Rectangle
			else if (tmpIndices.size() == 4) {
				// Triangle 1
				currentPrimitive->mesh.indices.push_back(tmpIndices[0]);
				currentPrimitive->mesh.indices.push_back(tmpIndices[1]);
				currentPrimitive->mesh.indices.push_back(tmpIndices[2]);

				// Triangle 2
				currentPrimitive->mesh.indices.push_back(tmpIndices[0]);
				currentPrimitive->mesh.indices.push_back(tmpIndices[2]);
				currentPrimitive->mesh.indices.push_back(tmpIndices[3]);
			}
			// N-gons
			else if (tmpIndices.size() > 4) {
				for (size_t i = 2; i < tmpIndices.size(); i++) {
					currentPrimitive->mesh.indices.push_back(tmpIndices[0]);
					currentPrimitive->mesh.indices.push_back(tmpIndices[i - 1]);
					currentPrimitive->mesh.indices.push_back(tmpIndices[i]);
				}
			}
		}
	}

	file.close();
}

std::unordered_map<std::string, NtshEngn::Material> NtshEngn::AssetLoaderModule::loadMaterialMtl(const std::string& filePath) {
	std::unordered_map<std::string, Material> mtlMaterials;

	std::ifstream file(filePath);

	// Open file
	if (!file.is_open()) {
		NTSHENGN_MODULE_WARNING("Could not open material file \"" + filePath + "\".");
		return mtlMaterials;
	}

	Material* currentMaterial = nullptr;

	std::string materialDirectory = filePath.substr(0, filePath.rfind('/'));

	std::string line;
	while (std::getline(file, line)) {
		// Ignore comment
		if (line[0] == '#') {
			continue;
		}

		// Parse line
		std::vector<std::string> tokens;
		size_t spacePosition = 0;
		while ((spacePosition = line.find(' ')) != std::string::npos) {
			tokens.push_back(line.substr(0, spacePosition));
			line.erase(0, spacePosition + 1);
		}
		tokens.push_back(line);

		// Parse tokens
		if (tokens[0] == "newmtl") {
			mtlMaterials[tokens[1]] = Material();
			currentMaterial = &mtlMaterials[tokens[1]];
		}
		else if (tokens[0] == "Kd") {
			std::string mapKey = "srgb " + tokens[1] + " " + tokens[2] + " " + tokens[3];

			Image* image = assetManager->createImage();
			image->width = 1;
			image->height = 1;
			image->format = ImageFormat::R8G8B8A8;
			image->colorSpace = ImageColorSpace::SRGB;
			image->data = { static_cast<uint8_t>(round(255.0f * std::atof(tokens[1].c_str()))),
				static_cast<uint8_t>(round(255.0f * std::atof(tokens[2].c_str()))),
				static_cast<uint8_t>(round(255.0f * std::atof(tokens[3].c_str()))),
				255
			};

			if (currentMaterial) {
				currentMaterial->diffuseTexture.image = image;
			}
		}
		else if (tokens[0] == "map_Kd") {
			Image* image = assetManager->loadImage(materialDirectory + "/" + tokens[1]);
			if (currentMaterial) {
				currentMaterial->diffuseTexture.image = image;
			}
		}
		else if (tokens[0] == "Ke") {
			std::string mapKey = "srgb " + tokens[1] + " " + tokens[2] + " " + tokens[3];

			Image* image = assetManager->createImage();
			image->width = 1;
			image->height = 1;
			image->format = ImageFormat::R8G8B8A8;
			image->colorSpace = ImageColorSpace::SRGB;
			image->data = { static_cast<uint8_t>(round(255.0f * std::atof(tokens[1].c_str()))),
				static_cast<uint8_t>(round(255.0f * std::atof(tokens[2].c_str()))),
				static_cast<uint8_t>(round(255.0f * std::atof(tokens[3].c_str()))),
				255
			};

			if (currentMaterial) {
				currentMaterial->emissiveTexture.image = image;
			}
		}
		else if (tokens[0] == "map_Ke") {
			Image* image = assetManager->loadImage(materialDirectory + "/" + tokens[1]);
			if (currentMaterial) {
				currentMaterial->emissiveTexture.image = image;
			}
		}
	}

	file.close();

	return mtlMaterials;
}

void NtshEngn::AssetLoaderModule::loadFontTtf(const std::string& filePath, float fontHeight, Font& font) {
	const int width = 512;
	const int height = 512;
	stbtt_bakedchar backedChars[96];

	unsigned char pixels[512 * 512];
	std::string fileContent = File::readBinary(filePath);
	stbtt_BakeFontBitmap(reinterpret_cast<const unsigned char*>(fileContent.c_str()), 0, fontHeight, pixels, width, height, 32, 96, backedChars);

	Image* fontImage = assetManager->createImage();
	fontImage->width = 512;
	fontImage->height = 512;
	fontImage->format = ImageFormat::R8;
	fontImage->colorSpace = ImageColorSpace::Linear;
	fontImage->data.resize(512 * 512);
	std::copy(pixels, pixels + (width * height), fontImage->data.begin());

	font.image = fontImage;
	font.imageSamplerFilter = ImageSamplerFilter::Linear;

	for (unsigned char c = 32; c < 128; c++) {
		float x = 0.0f;
		float y = 0.0f;
		stbtt_aligned_quad alignedQuad;
		stbtt_GetBakedQuad(backedChars, width, height, static_cast<int>(c) - 32, &x, &y, &alignedQuad, 1);

		FontGlyph glyph;
		glyph.positionTopLeft = { alignedQuad.x0, alignedQuad.y0 };
		glyph.positionBottomRight = { alignedQuad.x1, alignedQuad.y1 };
		glyph.positionAdvance = backedChars[static_cast<int>(c) - 32].xadvance;
		glyph.uvTopLeft = { alignedQuad.s0, alignedQuad.t0 };
		glyph.uvBottomRight = { alignedQuad.s1, alignedQuad.t1 };

		font.glyphs[c] = glyph;
	}
}

void NtshEngn::AssetLoaderModule::loadModelGltf(const std::string& filePath, Model& model) {
	cgltf_options options = {};
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, filePath.c_str(), &data);
	if (result == cgltf_result_success) {
		result = cgltf_load_buffers(&options, data, filePath.c_str());

		if (result != cgltf_result_success) {
			NTSHENGN_MODULE_WARNING("Could not load buffers for model file \"" + filePath + "\".");
			return;
		}
		else {
			cgltf_scene* scene = data->scene;

			Bimap<uint32_t, cgltf_node*> jointNodes;

			for (size_t i = 0; i < scene->nodes_count; i++) {
				loadGltfNode(filePath, model, scene->nodes[i], jointNodes);
			}

			for (size_t i = 0; i < data->animations_count; i++) {
				loadGltfAnimation(model, &data->animations[i], jointNodes);
			}
		}

		cgltf_free(data);
	}
	else {
		NTSHENGN_MODULE_WARNING("Could not load model file \"" + filePath + "\".");
	}
}

void NtshEngn::AssetLoaderModule::loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, Bimap<uint32_t, cgltf_node*>& jointNodes) {
	Math::mat4 modelMatrix = Math::mat4::identity();
	cgltf_node* matrixNode = node;
	while (matrixNode) {
		Math::mat4 nodeMatrix = Math::mat4::identity();
		if (matrixNode->has_matrix) {
			nodeMatrix = Math::mat4(matrixNode->matrix);
		}
		else {
			if (matrixNode->has_translation) {
				nodeMatrix *= Math::translate(Math::vec3(matrixNode->translation));
			}
			if (matrixNode->has_rotation) {
				nodeMatrix *= Math::quatToRotationMatrix(Math::quat(matrixNode->rotation[3], matrixNode->rotation[0], matrixNode->rotation[1], matrixNode->rotation[2]));
			}
			if (matrixNode->has_scale) {
				nodeMatrix *= Math::scale(Math::vec3(matrixNode->scale));
			}
		}

		modelMatrix = nodeMatrix * modelMatrix;

		matrixNode = matrixNode->parent;
	}

	if (node->mesh) {
		cgltf_mesh* nodeMesh = node->mesh;
		for (size_t i = 0; i < nodeMesh->primitives_count; i++) {
			cgltf_primitive nodeMeshPrimitive = nodeMesh->primitives[i];

			ModelPrimitive primitive;

			// Vertices
			float* position = nullptr;
			float* normal = nullptr;
			float* uv = nullptr;
			float* color = nullptr;
			float* tangent = nullptr;
			uint8_t* jointsu8 = nullptr;
			uint16_t* jointsu16 = nullptr;
			float* weights = nullptr;

			size_t positionCount = 0;
			size_t normalCount = 0;
			size_t uvCount = 0;
			size_t colorCount = 0;
			size_t tangentCount = 0;
			size_t jointsCount = 0;
			size_t weightsCount = 0;

			size_t positionStride = 0;
			size_t normalStride = 0;
			size_t uvStride = 0;
			size_t colorStride = 0;
			size_t tangentStride = 0;
			size_t jointsStride = 0;
			size_t weightsStride = 0;

			for (size_t j = 0; j < nodeMeshPrimitive.attributes_count; j++) {
				cgltf_attribute attribute = nodeMeshPrimitive.attributes[j];
				std::string attributeName = std::string(attribute.name);

				cgltf_accessor* accessor = attribute.data;
				cgltf_buffer_view* bufferView = accessor->buffer_view;
				std::byte* buffer = static_cast<std::byte*>(bufferView->buffer->data);
				std::byte* bufferOffset = buffer + accessor->offset + bufferView->offset;
				if (attributeName == "POSITION") {
					position = reinterpret_cast<float*>(bufferOffset);
					positionCount = attribute.data->count;
					positionStride = std::max(bufferView->stride, 3 * sizeof(float));
				}
				else if (attributeName == "NORMAL") {
					normal = reinterpret_cast<float*>(bufferOffset);
					normalCount = attribute.data->count;
					normalStride = std::max(bufferView->stride, 3 * sizeof(float));
				}
				else if (attributeName == "TEXCOORD_0") {
					uv = reinterpret_cast<float*>(bufferOffset);
					uvCount = attribute.data->count;
					uvStride = std::max(bufferView->stride, 2 * sizeof(float));
				}
				else if (attributeName == "COLOR_0") {
					color = reinterpret_cast<float*>(bufferOffset);
					colorCount = attribute.data->count;
					colorStride = std::max(bufferView->stride, 3 * sizeof(float));
				}
				else if (attributeName == "TANGENT") {
					tangent = reinterpret_cast<float*>(bufferOffset);
					tangentCount = attribute.data->count;
					tangentStride = std::max(bufferView->stride, 4 * sizeof(float));
				}
				else if (attributeName == "JOINTS_0") {
					if (accessor->component_type == cgltf_component_type_r_8u) {
						jointsu8 = reinterpret_cast<uint8_t*>(bufferOffset);
						jointsCount = attribute.data->count;
						jointsStride = std::max(bufferView->stride, 4 * sizeof(uint8_t));
					}
					else {
						jointsu16 = reinterpret_cast<uint16_t*>(bufferOffset);
						jointsCount = attribute.data->count;
						jointsStride = std::max(bufferView->stride, 4 * sizeof(uint16_t));
					}
				}
				else if (attributeName == "WEIGHTS_0") {
					weights = reinterpret_cast<float*>(bufferOffset);
					weightsCount = attribute.data->count;
					weightsStride = std::max(bufferView->stride, 4 * sizeof(float));
				}
			}
			size_t vertexCount = positionCount;

			size_t positionCursor = 0;
			size_t normalCursor = 0;
			size_t uvCursor = 0;
			size_t colorCursor = 0;
			size_t tangentCursor = 0;
			size_t jointsCursor = 0;
			size_t weightsCursor = 0;
			for (size_t j = 0; j < vertexCount; j++) {
				Vertex vertex;

				if (!node->skin) {
					vertex.position = Math::vec3(position + positionCursor);
					positionCursor += (positionStride / sizeof(float));

					vertex.normal = (normalCount != 0) ? Math::vec3(normal + normalCursor) : Math::vec3(0.0f, 0.0f, 0.0f);
					normalCursor += (normalStride / sizeof(float));
				}
				else {
					vertex.position = Math::vec3(position + positionCursor);
					positionCursor += (positionStride / sizeof(float));

					vertex.normal = (normalCount != 0) ? Math::vec3(normal + normalCursor) : Math::vec3(0.0f, 0.0f, 0.0f);
					normalCursor += (normalStride / sizeof(float));
				}

				vertex.uv = (uvCount != 0) ? Math::vec2(uv + uvCursor) : Math::vec2(0.5f, 0.5f);
				uvCursor += (uvStride / sizeof(float));

				vertex.color = (colorCount != 0) ? Math::vec3(color + colorCursor) : Math::vec3(0.0f, 0.0f, 0.0f);
				colorCursor += (colorStride / sizeof(float));

				vertex.tangent = (tangentCount != 0) ? Math::vec4(tangent + tangentCursor) : Math::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				tangentCursor += (tangentStride / sizeof(float));

				if (jointsu8) {
					vertex.joints = (jointsCount != 0) ? std::array<uint32_t, 4>({ static_cast<uint32_t>(jointsu8[jointsCursor]), static_cast<uint32_t>(jointsu8[jointsCursor + 1]), static_cast<uint32_t>(jointsu8[jointsCursor] + 2), static_cast<uint32_t>(jointsu8[jointsCursor + 3]) }) : std::array<uint32_t, 4>({ 0, 0, 0, 0 });
					jointsCursor += (jointsStride / sizeof(uint8_t));
				}
				else {
					vertex.joints = (jointsCount != 0) ? std::array<uint32_t, 4>({ static_cast<uint32_t>(jointsu16[jointsCursor]), static_cast<uint32_t>(jointsu16[jointsCursor + 1]), static_cast<uint32_t>(jointsu16[jointsCursor] + 2), static_cast<uint32_t>(jointsu16[jointsCursor + 3]) }) : std::array<uint32_t, 4>({ 0, 0, 0, 0 });
					jointsCursor += (jointsStride / sizeof(uint16_t));
				}

				vertex.weights = (weightsCount != 0) ? Math::vec4(weights + weightsCursor) : Math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				weightsCursor += (weightsStride / sizeof(float));

				primitive.mesh.vertices.push_back(vertex);
			}

			// Indices
			cgltf_accessor* accessor = nodeMeshPrimitive.indices;
			if (accessor != NULL) {
				primitive.mesh.indices.reserve(accessor->count);
				cgltf_buffer_view* bufferView = accessor->buffer_view;
				cgltf_component_type componentType = accessor->component_type;
				std::byte* buffer = static_cast<std::byte*>(bufferView->buffer->data);
				switch (componentType) {
				case cgltf_component_type_r_8:
				{
					int8_t* index = reinterpret_cast<int8_t*>(buffer + accessor->offset + bufferView->offset);
					for (size_t j = 0; j < accessor->count; j++) {
						primitive.mesh.indices.push_back(static_cast<uint32_t>(index[j]));
					}
					break;
				}

				case cgltf_component_type_r_8u:
				{
					uint8_t* index = reinterpret_cast<uint8_t*>(buffer + accessor->offset + bufferView->offset);
					for (size_t j = 0; j < accessor->count; j++) {
						primitive.mesh.indices.push_back(static_cast<uint32_t>(index[j]));
					}
					break;
				}

				case cgltf_component_type_r_16:
				{
					int16_t* index = reinterpret_cast<int16_t*>(buffer + accessor->offset + bufferView->offset);
					for (size_t j = 0; j < accessor->count; j++) {
						primitive.mesh.indices.push_back(static_cast<uint32_t>(index[j]));
					}
					break;
				}

				case cgltf_component_type_r_16u:
				{
					uint16_t* index = reinterpret_cast<uint16_t*>(buffer + accessor->offset + bufferView->offset);
					for (size_t j = 0; j < accessor->count; j++) {
						primitive.mesh.indices.push_back(static_cast<uint32_t>(index[j]));
					}
					break;
				}

				case cgltf_component_type_r_32u:
				{
					uint32_t* index = reinterpret_cast<uint32_t*>(buffer + accessor->offset + bufferView->offset);
					std::copy(index, index + accessor->count, std::back_inserter(primitive.mesh.indices));
					break;
				}

				case cgltf_component_type_r_32f:
				{
					float* index = reinterpret_cast<float*>(buffer + accessor->offset + bufferView->offset);
					for (size_t j = 0; j < accessor->count; j++) {
						primitive.mesh.indices.push_back(static_cast<uint32_t>(index[j]));
					}
					break;
				}

				default:
					NTSHENGN_MODULE_WARNING("Index component type invalid for model file \"" + filePath + "\".");
				}
			}
			else {
				// Calculate indices
				primitive.mesh.indices.resize(primitive.mesh.vertices.size());
				std::iota(primitive.mesh.indices.begin(), primitive.mesh.indices.end(), 0);
			}

			// Tangents
			if ((tangentCount == 0) && (uvCount != 0) && (normalCount != 0) && (primitive.mesh.indices.size() != 0)) {
				assetManager->calculateTangents(primitive.mesh);

				// Invert tangent handedness
				for (Vertex& vertex : primitive.mesh.vertices) {
					vertex.tangent.w *= -1.0f;
				}
			}

			// Material
			cgltf_material* primitiveMaterial = nodeMeshPrimitive.material;
			if (primitiveMaterial != NULL) {
				if (primitiveMaterial->has_pbr_metallic_roughness) {
					cgltf_pbr_metallic_roughness pbrMetallicRoughness = primitiveMaterial->pbr_metallic_roughness;

					// Base Color / Diffuse texture
					cgltf_texture_view baseColorTextureView = pbrMetallicRoughness.base_color_texture;
					cgltf_texture* baseColorTexture = baseColorTextureView.texture;
					cgltf_float* baseColorFactor = pbrMetallicRoughness.base_color_factor;
					if (baseColorTexture != NULL) {
						cgltf_image* baseColorImage = baseColorTexture->image;
						if (baseColorImage->uri) {
							std::string imageURI = baseColorImage->uri;

							Image* image = nullptr;

							size_t base64Pos = imageURI.find(";base64,");
							if (base64Pos != std::string::npos) {
								cgltf_options options = {};

								const std::string uriBase64 = imageURI.substr(base64Pos + 8);
								const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
								std::vector<uint8_t> decodedData(decodedDataSize);
								cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), reinterpret_cast<void**>(decodedData.data()));
								if (result == cgltf_result_success) {
									image = assetManager->createImage();
									loadImageFromMemory(decodedData.data(), decodedDataSize, *image);
									image->colorSpace = ImageColorSpace::SRGB;
								}
								else {
									NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (base color texture).");
								}
							}
							else {
								image = assetManager->loadImage(File::directory(filePath) + imageURI);
								if (image) {
									image->colorSpace = ImageColorSpace::SRGB;
								}
							}

							primitive.material.diffuseTexture.image = image;
							if (baseColorTexture->sampler != NULL) {
								primitive.material.diffuseTexture.imageSampler.magFilter = m_gltfFilterToImageSamplerFilter[baseColorTexture->sampler->mag_filter];
								primitive.material.diffuseTexture.imageSampler.minFilter = m_gltfFilterToImageSamplerFilter[baseColorTexture->sampler->min_filter];
								primitive.material.diffuseTexture.imageSampler.mipmapFilter = m_gltfFilterToImageSamplerFilterMipMap[baseColorTexture->sampler->min_filter];
								primitive.material.diffuseTexture.imageSampler.addressModeU = m_gltfFilterToImageSamplerAddressMode[baseColorTexture->sampler->wrap_s];
								primitive.material.diffuseTexture.imageSampler.addressModeV = m_gltfFilterToImageSamplerAddressMode[baseColorTexture->sampler->wrap_t];
								primitive.material.diffuseTexture.imageSampler.addressModeW = ImageSamplerAddressMode::ClampToEdge;
								primitive.material.diffuseTexture.imageSampler.borderColor = ImageSamplerBorderColor::IntOpaqueBlack;
								primitive.material.diffuseTexture.imageSampler.anisotropyLevel = 16.0f;
							}
							else {
								primitive.material.diffuseTexture.imageSampler = trilinearSampler;
							}
						}
					}
					else if (baseColorFactor != NULL) {
						Image* image = assetManager->createImage();
						image->width = 1;
						image->height = 1;
						image->format = ImageFormat::R8G8B8A8;
						image->colorSpace = ImageColorSpace::SRGB;
						image->data = { static_cast<uint8_t>(round(255.0f * baseColorFactor[0])),
							static_cast<uint8_t>(round(255.0f * baseColorFactor[1])),
							static_cast<uint8_t>(round(255.0f * baseColorFactor[2])),
							static_cast<uint8_t>(round(255.0f * baseColorFactor[3]))
						};

						primitive.material.diffuseTexture.image = image;
						primitive.material.diffuseTexture.imageSampler = nearestSampler;
					}

					// Metallic Roughness texture
					cgltf_texture_view metallicRoughnessTextureView = pbrMetallicRoughness.metallic_roughness_texture;
					cgltf_texture* metallicRoughnessTexture = metallicRoughnessTextureView.texture;
					cgltf_float metallicFactor = pbrMetallicRoughness.metallic_factor;
					cgltf_float roughnessFactor = pbrMetallicRoughness.roughness_factor;
					if (metallicRoughnessTexture != NULL) {
						cgltf_image* metallicRoughnessImage = metallicRoughnessTexture->image;
						if (metallicRoughnessImage->uri) {
							std::string imageURI = metallicRoughnessImage->uri;

							Image* image = nullptr;

							size_t base64Pos = imageURI.find(";base64,");
							if (base64Pos != std::string::npos) {
								cgltf_options options = {};

								const std::string uriBase64 = imageURI.substr(base64Pos + 8);
								const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
								std::vector<uint8_t> decodedData(decodedDataSize);
								cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), reinterpret_cast<void**>(decodedData.data()));
								if (result == cgltf_result_success) {
									image = assetManager->createImage();
									loadImageFromMemory(decodedData.data(), decodedDataSize, *image);
									image->colorSpace = ImageColorSpace::Linear;
								}
								else {
									NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (metallic roughness texture).");
								}
							}
							else {
								image = assetManager->loadImage(File::directory(filePath) + imageURI);
								if (image) {
									image->colorSpace = ImageColorSpace::Linear;
								}
							}

							primitive.material.metalnessTexture.image = image;
							primitive.material.roughnessTexture.image = primitive.material.metalnessTexture.image;
							if (metallicRoughnessTexture->sampler != NULL) {
								primitive.material.metalnessTexture.imageSampler.magFilter = m_gltfFilterToImageSamplerFilter[metallicRoughnessTexture->sampler->mag_filter];
								primitive.material.metalnessTexture.imageSampler.minFilter = m_gltfFilterToImageSamplerFilter[metallicRoughnessTexture->sampler->min_filter];
								primitive.material.metalnessTexture.imageSampler.mipmapFilter = m_gltfFilterToImageSamplerFilterMipMap[metallicRoughnessTexture->sampler->min_filter];
								primitive.material.metalnessTexture.imageSampler.addressModeU = m_gltfFilterToImageSamplerAddressMode[metallicRoughnessTexture->sampler->wrap_s];
								primitive.material.metalnessTexture.imageSampler.addressModeV = m_gltfFilterToImageSamplerAddressMode[metallicRoughnessTexture->sampler->wrap_t];
								primitive.material.metalnessTexture.imageSampler.addressModeW = ImageSamplerAddressMode::ClampToEdge;
								primitive.material.metalnessTexture.imageSampler.borderColor = ImageSamplerBorderColor::IntOpaqueBlack;
								primitive.material.metalnessTexture.imageSampler.anisotropyLevel = 16.0f;
							}
							else {
								primitive.material.metalnessTexture.imageSampler = trilinearSampler;
							}
							primitive.material.roughnessTexture.imageSampler = primitive.material.metalnessTexture.imageSampler;
						}
					}
					else {
						Image* image = assetManager->createImage();
						image->width = 1;
						image->height = 1;
						image->format = ImageFormat::R8G8B8A8;
						image->colorSpace = ImageColorSpace::Linear;
						image->data = { 0,
							static_cast<uint8_t>(round(255.0f * roughnessFactor)),
							static_cast<uint8_t>(round(255.0f * metallicFactor)),
							0
						};

						primitive.material.metalnessTexture.image = image;
						primitive.material.roughnessTexture.image = primitive.material.metalnessTexture.image;
						primitive.material.metalnessTexture.imageSampler = nearestSampler;
						primitive.material.roughnessTexture.imageSampler = primitive.material.metalnessTexture.imageSampler;
					}
				}

				// Normal texture
				cgltf_texture_view normalTextureView = primitiveMaterial->normal_texture;
				cgltf_texture* normalTexture = normalTextureView.texture;
				if (normalTexture != NULL) {
					cgltf_image* normalImage = normalTexture->image;
					if (normalImage->uri) {
						std::string imageURI = normalImage->uri;

						Image* image = nullptr;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							std::vector<uint8_t> decodedData(decodedDataSize);
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), reinterpret_cast<void**>(decodedData.data()));
							if (result == cgltf_result_success) {
								image = assetManager->createImage();
								loadImageFromMemory(decodedData.data(), decodedDataSize, *image);
								image->colorSpace = ImageColorSpace::Linear;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (normal texture).");
							}
						}
						else {
							image = assetManager->loadImage(File::directory(filePath) + imageURI);
							if (image) {
								image->colorSpace = ImageColorSpace::Linear;
							}
						}

						primitive.material.normalTexture.image = image;
						if (normalTexture->sampler != NULL) {
							primitive.material.normalTexture.imageSampler.magFilter = m_gltfFilterToImageSamplerFilter[normalTexture->sampler->mag_filter];
							primitive.material.normalTexture.imageSampler.minFilter = m_gltfFilterToImageSamplerFilter[normalTexture->sampler->min_filter];
							primitive.material.normalTexture.imageSampler.mipmapFilter = m_gltfFilterToImageSamplerFilterMipMap[normalTexture->sampler->min_filter];
							primitive.material.normalTexture.imageSampler.addressModeU = m_gltfFilterToImageSamplerAddressMode[normalTexture->sampler->wrap_s];
							primitive.material.normalTexture.imageSampler.addressModeV = m_gltfFilterToImageSamplerAddressMode[normalTexture->sampler->wrap_t];
							primitive.material.normalTexture.imageSampler.addressModeW = ImageSamplerAddressMode::ClampToEdge;
							primitive.material.normalTexture.imageSampler.borderColor = ImageSamplerBorderColor::IntOpaqueBlack;
							primitive.material.normalTexture.imageSampler.anisotropyLevel = 16.0f;
						}
						else {
							primitive.material.normalTexture.imageSampler = trilinearSampler;
						}
					}
				}

				// Emissive texture
				cgltf_texture_view emissiveTextureView = primitiveMaterial->emissive_texture;
				cgltf_texture* emissiveTexture = emissiveTextureView.texture;
				cgltf_float* emissiveFactor = primitiveMaterial->emissive_factor;
				if (emissiveTexture != NULL) {
					cgltf_image* emissiveImage = emissiveTexture->image;
					if (emissiveImage->uri) {
						std::string imageURI = emissiveImage->uri;

						Image* image = nullptr;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							std::vector<uint8_t> decodedData(decodedDataSize);
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), reinterpret_cast<void**>(decodedData.data()));
							if (result == cgltf_result_success) {
								image = assetManager->createImage();
								loadImageFromMemory(decodedData.data(), decodedDataSize, *image);
								image->colorSpace = ImageColorSpace::Linear;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (emissive texture).");
							}
						}
						else {
							image = assetManager->loadImage(File::directory(filePath) + imageURI);
							if (image) {
								image->colorSpace = ImageColorSpace::SRGB;
							}
						}

						primitive.material.emissiveTexture.image = image;
						if (emissiveTexture->sampler != NULL) {
							primitive.material.emissiveTexture.imageSampler.magFilter = m_gltfFilterToImageSamplerFilter[emissiveTexture->sampler->mag_filter];
							primitive.material.emissiveTexture.imageSampler.minFilter = m_gltfFilterToImageSamplerFilter[emissiveTexture->sampler->min_filter];
							primitive.material.emissiveTexture.imageSampler.mipmapFilter = m_gltfFilterToImageSamplerFilterMipMap[emissiveTexture->sampler->min_filter];
							primitive.material.emissiveTexture.imageSampler.addressModeU = m_gltfFilterToImageSamplerAddressMode[emissiveTexture->sampler->wrap_s];
							primitive.material.emissiveTexture.imageSampler.addressModeV = m_gltfFilterToImageSamplerAddressMode[emissiveTexture->sampler->wrap_t];
							primitive.material.emissiveTexture.imageSampler.addressModeW = ImageSamplerAddressMode::ClampToEdge;
							primitive.material.emissiveTexture.imageSampler.borderColor = ImageSamplerBorderColor::IntOpaqueBlack;
							primitive.material.emissiveTexture.imageSampler.anisotropyLevel = 16.0f;
						}
						else {
							primitive.material.emissiveTexture.imageSampler = trilinearSampler;
						}
					}
				}
				else if (emissiveFactor != NULL) {
					Image* image = assetManager->createImage();
					image->width = 1;
					image->height = 1;
					image->format = ImageFormat::R8G8B8A8;
					image->colorSpace = ImageColorSpace::SRGB;
					image->data = { static_cast<uint8_t>(round(255.0f * emissiveFactor[0])),
						static_cast<uint8_t>(round(255.0f * emissiveFactor[1])),
						static_cast<uint8_t>(round(255.0f * emissiveFactor[2])),
						255
					};

					primitive.material.emissiveTexture.image = image;
					primitive.material.emissiveTexture.imageSampler = nearestSampler;
				}
				if (primitiveMaterial->has_emissive_strength) {
					primitive.material.emissiveFactor = primitiveMaterial->emissive_strength.emissive_strength;
				}

				// Occlusion texture
				cgltf_texture_view occlusionTextureView = primitiveMaterial->occlusion_texture;
				cgltf_texture* occlusionTexture = occlusionTextureView.texture;
				if (occlusionTexture != NULL) {
					cgltf_image* occlusionImage = occlusionTexture->image;
					if (occlusionImage->uri) {
						std::string imageURI = occlusionImage->uri;

						Image* image = nullptr;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							std::vector<uint8_t> decodedData(decodedDataSize);
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), reinterpret_cast<void**>(decodedData.data()));
							if (result == cgltf_result_success) {
								image = assetManager->createImage();
								loadImageFromMemory(decodedData.data(), decodedDataSize, *image);
								image->colorSpace = ImageColorSpace::Linear;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (occlusion texture).");
							}
						}
						else {
							image = assetManager->loadImage(File::directory(filePath) + imageURI);
							if (image) {
								image->colorSpace = ImageColorSpace::Linear;
							}
						}

						primitive.material.occlusionTexture.image = image;
						if (occlusionTexture->sampler != NULL) {
							primitive.material.occlusionTexture.imageSampler.magFilter = m_gltfFilterToImageSamplerFilter[occlusionTexture->sampler->mag_filter];
							primitive.material.occlusionTexture.imageSampler.minFilter = m_gltfFilterToImageSamplerFilter[occlusionTexture->sampler->min_filter];
							primitive.material.occlusionTexture.imageSampler.mipmapFilter = m_gltfFilterToImageSamplerFilterMipMap[occlusionTexture->sampler->min_filter];
							primitive.material.occlusionTexture.imageSampler.addressModeU = m_gltfFilterToImageSamplerAddressMode[occlusionTexture->sampler->wrap_s];
							primitive.material.occlusionTexture.imageSampler.addressModeV = m_gltfFilterToImageSamplerAddressMode[occlusionTexture->sampler->wrap_t];
							primitive.material.occlusionTexture.imageSampler.addressModeW = ImageSamplerAddressMode::ClampToEdge;
							primitive.material.occlusionTexture.imageSampler.borderColor = ImageSamplerBorderColor::IntOpaqueBlack;
							primitive.material.occlusionTexture.imageSampler.anisotropyLevel = 16.0f;
						}
						else {
							primitive.material.occlusionTexture.imageSampler = trilinearSampler;
						}
					}
				}

				// Alpha cutoff
				if (primitiveMaterial->alpha_mode == cgltf_alpha_mode_mask) {
					primitive.material.alphaCutoff = primitiveMaterial->alpha_cutoff;
				}

				// Index of refraction
				if (primitiveMaterial->has_ior) {
					primitive.material.indexOfRefraction = primitiveMaterial->ior.ior;
				}
			}

			model.primitives.push_back(primitive);
		}
	}

	if (node->skin) {
		cgltf_skin* nodeSkin = node->skin;

		Skin skin;
		skin.inverseGlobalTransform = Math::inverse(modelMatrix);

		cgltf_accessor* accessor = nodeSkin->inverse_bind_matrices;
		cgltf_buffer_view* bufferView = accessor->buffer_view;
		std::byte* buffer = static_cast<std::byte*>(bufferView->buffer->data);
		
		std::unordered_map<uint32_t, size_t> skinJoints;
		for (size_t i = 0; i < nodeSkin->joints_count; i++) {
			cgltf_node* nodeJoint = nodeSkin->joints[i];
			if (!jointNodes.exist(nodeJoint)) {
				jointNodes.insert_or_assign(static_cast<uint32_t>(jointNodes.size()), nodeJoint);
			}
			std::byte* bufferOffset = buffer + accessor->offset + bufferView->offset + (16 * sizeof(float) * i);

			Joint joint;
			joint.inverseBindMatrix = Math::mat4(reinterpret_cast<float*>(bufferOffset));

			if (nodeJoint->has_matrix) {
				joint.localTransform = Math::mat4(nodeJoint->matrix);
			}
			else {
				if (nodeJoint->has_translation) {
					joint.localTransform *= Math::translate(Math::vec3(nodeJoint->translation));
				}
				if (nodeJoint->has_rotation) {
					joint.localTransform *= Math::quatToRotationMatrix(Math::quat(nodeJoint->rotation[3], nodeJoint->rotation[0], nodeJoint->rotation[1], nodeJoint->rotation[2]));
				}
				if (nodeJoint->has_scale) {
					joint.localTransform *= Math::scale(Math::vec3(nodeJoint->scale));
				}
			}

			skin.joints.push_back(joint);

			skinJoints[jointNodes[nodeJoint]] = skin.joints.size() - 1;

			if (i == 0) {
				skin.rootJoint = jointNodes[nodeJoint];
			}
		}
		if (nodeSkin->skeleton) {
			skin.rootJoint = jointNodes[nodeSkin->skeleton];
		}

		for (size_t i = 0; i < nodeSkin->joints_count; i++) {
			for (size_t j = 0; j < jointNodes[static_cast<uint32_t>(i)]->children_count; j++) {
				skin.joints[i].children.push_back(jointNodes[jointNodes[static_cast<uint32_t>(i)]->children[j]]);
			}
		}

		cgltf_node* baseMatrixNode = jointNodes[skin.rootJoint]->parent;
		while (baseMatrixNode) {
			Math::mat4 nodeMatrix = Math::mat4::identity();
			if (baseMatrixNode->has_matrix) {
				nodeMatrix = Math::mat4(baseMatrixNode->matrix);
			}
			else {
				if (baseMatrixNode->has_translation) {
					nodeMatrix *= Math::translate(Math::vec3(baseMatrixNode->translation));
				}
				if (baseMatrixNode->has_rotation) {
					nodeMatrix *= Math::quatToRotationMatrix(Math::quat(baseMatrixNode->rotation[3], baseMatrixNode->rotation[0], baseMatrixNode->rotation[1], baseMatrixNode->rotation[2]));
				}
				if (baseMatrixNode->has_scale) {
					nodeMatrix *= Math::scale(Math::vec3(baseMatrixNode->scale));
				}
			}

			skin.baseMatrix = nodeMatrix * skin.baseMatrix;

			baseMatrixNode = baseMatrixNode->parent;
		}

		for (size_t i = 0; i < model.primitives.size(); i++) {
			model.primitives[i].mesh.skin = skin;
		}
	}

	for (size_t i = 0; i < node->children_count; i++) {
		loadGltfNode(filePath, model, node->children[i], jointNodes);
	}
}

void NtshEngn::AssetLoaderModule::loadGltfAnimation(Model& model, cgltf_animation* node, Bimap<uint32_t, cgltf_node*>& jointNodes) {
	Animation animation;

	for (size_t i = 0; i < node->channels_count; i++) {
		AnimationChannel channel;

		cgltf_animation_channel animationChannel = node->channels[i];

		cgltf_node* animationTargetNode = animationChannel.target_node;

		uint32_t jointIndex = 0;
		if (jointNodes.exist(animationTargetNode)) {
			jointIndex = jointNodes[animationTargetNode];
		}
		else {
			continue;
		}

		cgltf_animation_sampler* animationSampler = animationChannel.sampler;
		switch (animationSampler->interpolation) {
		case cgltf_interpolation_type_linear:
			channel.interpolationType = AnimationChannelInterpolationType::Linear;
			break;

		case cgltf_interpolation_type_step:
			channel.interpolationType = AnimationChannelInterpolationType::Step;
			break;

		case cgltf_interpolation_type_cubic_spline:
			channel.interpolationType = AnimationChannelInterpolationType::CubicSpline;
			break;

		default:
			channel.interpolationType = AnimationChannelInterpolationType::Unknown;
		}

		switch (animationChannel.target_path) {
		case cgltf_animation_path_type_translation:
			channel.transformType = AnimationChannelTransformType::Translation;
			break;

		case cgltf_animation_path_type_rotation:
			channel.transformType = AnimationChannelTransformType::Rotation;
			break;

		case cgltf_animation_path_type_scale:
			channel.transformType = AnimationChannelTransformType::Scale;
			break;

		default:
			channel.transformType = AnimationChannelTransformType::Unknown;
		}
		
		cgltf_accessor* animationSamplerInputAccessor = animationSampler->input;
		cgltf_buffer_view* animationSamplerInputBufferView = animationSamplerInputAccessor->buffer_view;
		std::byte* animationSamplerInputBuffer = static_cast<std::byte*>(animationSamplerInputBufferView->buffer->data);
		std::byte* animationSamplerInputBufferOffset = animationSamplerInputBuffer + animationSamplerInputAccessor->offset + animationSamplerInputBufferView->offset;

		cgltf_accessor* animationSamplerOutputAccessor = animationSampler->output;
		cgltf_buffer_view* animationSamplerOutputBufferView = animationSamplerOutputAccessor->buffer_view;
		std::byte* animationSamplerOutputBuffer = static_cast<std::byte*>(animationSamplerOutputBufferView->buffer->data);
		std::byte* animationSamplerOutputBufferOffset = animationSamplerOutputBuffer + animationSamplerOutputAccessor->offset + animationSamplerOutputBufferView->offset;

		std::vector<AnimationChannelKeyframe> keyframes;
		for (size_t j = 0; j < animationSamplerInputAccessor->count; j++) {
			AnimationChannelKeyframe keyframe;
			keyframe.timestamp = *(reinterpret_cast<float*>(animationSamplerInputBufferOffset) + j);
			
			switch (channel.transformType) {
			case AnimationChannelTransformType::Translation:
			case AnimationChannelTransformType::Scale:
				keyframe.value = Math::vec4(Math::vec3(reinterpret_cast<float*>(animationSamplerOutputBufferOffset)), 0.0f);
				animationSamplerOutputBufferOffset += sizeof(float) * 3;
				break;

			case AnimationChannelTransformType::Rotation:
				keyframe.value = { *(reinterpret_cast<float*>(animationSamplerOutputBufferOffset) + 3), *(reinterpret_cast<float*>(animationSamplerOutputBufferOffset) + 0), *(reinterpret_cast<float*>(animationSamplerOutputBufferOffset) + 1), *(reinterpret_cast<float*>(animationSamplerOutputBufferOffset) + 2) };
				animationSamplerOutputBufferOffset += sizeof(float) * 4;
				break;

			default:
				break;
			}

			channel.keyframes.push_back(keyframe);
		}

		if (animationChannel.sampler->input->has_max) {
			if (animationChannel.sampler->input->max[0] > animation.duration) {
				animation.duration = animationChannel.sampler->input->max[0];
			}
		}
		else {
			if (channel.keyframes.back().timestamp) {
				animation.duration = channel.keyframes.back().timestamp;
			}
		}

		animation.jointChannels[jointIndex].push_back(channel);
	}

	for (size_t i = 0; i < model.primitives.size(); i++) {
		model.primitives[i].mesh.animations.push_back(animation);
	}
}

extern "C" NTSHENGN_MODULE_API NtshEngn::AssetLoaderModuleInterface* createModule() {
	return new NtshEngn::AssetLoaderModule;
}

extern "C" NTSHENGN_MODULE_API void destroyModule(NtshEngn::AssetLoaderModuleInterface* m) {
	delete m;
}
