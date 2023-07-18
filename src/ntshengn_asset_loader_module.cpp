#include "ntshengn_asset_loader_module.h"
#include "../Module/utils/ntshengn_module_defines.h"
#include "../Module/utils/ntshengn_dynamic_library.h"
#include "../Common/utils/ntshengn_defines.h"
#include "../Common/utils/ntshengn_enums.h"
#include "../Common/utils/ntshengn_utils_file.h"
#define CGLTF_IMPLEMENTATION
#include "../external/cgltf/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb/stb_image.h"
#include <cstddef>
#include <cmath>
#include <iterator>
#include <algorithm>

NtshEngn::Sound NtshEngn::AssetLoaderModule::loadSound(const std::string& filePath) {
	Sound newSound;

	std::string extension = File::extension(filePath);
	if (extension == "wav") {
		loadSoundWav(filePath, newSound);
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

void NtshEngn::AssetLoaderModule::calculateTangents(Mesh& mesh) {
	std::vector<Math::vec3> tan1(mesh.vertices.size());
	std::vector<Math::vec3> tan2(mesh.vertices.size());
	for (size_t i = 0; i < mesh.indices.size(); i += 3) {
		const NtshEngn::Vertex& vertex0 = mesh.vertices[mesh.indices[i]];
		const NtshEngn::Vertex& vertex1 = mesh.vertices[mesh.indices[i + 1]];
		const NtshEngn::Vertex& vertex2 = mesh.vertices[mesh.indices[i + 2]];

		const Math::vec3 dPos1 = Math::vec3(vertex1.position.data()) - Math::vec3(vertex0.position.data());
		const Math::vec3 dPos2 = Math::vec3(vertex2.position.data()) - Math::vec3(vertex0.position.data());

		const Math::vec2 dUV1 = Math::vec2(vertex1.uv.data()) - Math::vec2(vertex0.uv.data());
		const Math::vec2 dUV2 = Math::vec2(vertex2.uv.data()) - Math::vec2(vertex0.uv.data());

		const float r = 1.0f / (dUV1.x * dUV2.y - dUV1.y * dUV2.x);

		const Math::vec3 uDir = (dPos1 * dUV2.y - dPos2 * dUV1.y) * r;
		const Math::vec3 vDir = (dPos2 * dUV1.x - dPos1 * dUV2.x) * r;

		tan1[mesh.indices[i]] += uDir;
		tan1[mesh.indices[i + 1]] += uDir;
		tan1[mesh.indices[i + 2]] += uDir;

		tan2[mesh.indices[i]] += vDir;
		tan2[mesh.indices[i + 1]] += vDir;
		tan2[mesh.indices[i + 2]] += vDir;
	}

	for (size_t i = 0; i < mesh.vertices.size(); i++) {
		const Math::vec3 n = mesh.vertices[i].normal.data();
		const Math::vec3 t = tan1[i];

		const Math::vec3 tangent = Math::normalize(t - n * Math::dot(n, t));
		const float tangentHandedness = (Math::dot(Math::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;

		mesh.vertices[i].tangent = { tangent.x, tangent.y, tangent.z, tangentHandedness };
	}
}

std::array<std::array<float, 3>, 2> NtshEngn::AssetLoaderModule::calculateAABB(const Mesh& mesh) {
	Math::vec3 min = Math::vec3(std::numeric_limits<float>::max());
	Math::vec3 max = Math::vec3(std::numeric_limits<float>::lowest());
	for (const NtshEngn::Vertex& vertex : mesh.vertices) {
		if (vertex.position[0] < min.x) {
			min.x = vertex.position[0];
		}
		if (vertex.position[0] > max.x) {
			max.x = vertex.position[0];
		}

		if (vertex.position[1] < min.y) {
			min.y = vertex.position[1];
		}
		if (vertex.position[1] > max.y) {
			max.y = vertex.position[1];
		}

		if (vertex.position[2] < min.z) {
			min.z = vertex.position[2];
		}
		if (vertex.position[2] > max.z) {
			max.z = vertex.position[2];
		}
	}

	const float epsilon = 0.0001f;

	if (min.x == max.x) {
		min.x -= epsilon;
		max.x += epsilon;
	}

	if (min.y == max.y) {
		min.y -= epsilon;
		max.y += epsilon;
	}

	if (min.z == max.z) {
		min.z -= epsilon;
		max.z += epsilon;
	}

	return { std::array<float, 3>{ min.x, min.y, min.z }, { max.x, max.y, max.z } };
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
		NTSHENGN_MODULE_WARNING("Unknown error while loading WAVE sound file.");
		return;
	}

	// Data
	data.resize(sound.size);
	file.read(&data[0], sound.size);
	sound.data.insert(sound.data.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
	data.erase(data.begin(), data.end());

	file.close();
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

	std::vector<std::array<float, 3>> positions;
	std::vector<std::array<float, 3>> normals;
	std::vector<std::array<float, 2>> uvs;

	std::unordered_map<std::string, uint32_t> uniqueVertices;
	Mesh mesh = {};
	mesh.topology = MeshTopology::TriangleList;

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
							vertex.position[0] = positions[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][0];
							vertex.position[1] = positions[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][1];
							vertex.position[2] = positions[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][2];
						}
						// UV index
						else if (j == 1) {
							vertex.uv[0] = uvs[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][0];
							vertex.uv[1] = uvs[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][1];
						}
						// Normal index
						else if (j == 2) {
							vertex.normal[0] = normals[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][0];
							vertex.normal[1] = normals[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][1];
							vertex.normal[2] = normals[static_cast<size_t>(std::atoi(valueIndices[j].c_str())) - 1][2];
						}
					}
				}

				if (uniqueVertices.count(tokens[i]) == 0) {
					uniqueVertices[tokens[i]] = static_cast<uint32_t>(mesh.vertices.size());
					mesh.vertices.push_back(vertex);
				}
				tmpIndices.push_back(uniqueVertices[tokens[i]]);
			}

			// Face can be a triangle or a rectangle
			// Triangle
			if (tmpIndices.size() == 3) {
				mesh.indices.insert(mesh.indices.end(), std::make_move_iterator(tmpIndices.begin()), std::make_move_iterator(tmpIndices.end()));
			}
			// Rectangle
			else if (tmpIndices.size() == 4) {
				// Triangle 1
				mesh.indices.push_back(tmpIndices[0]);
				mesh.indices.push_back(tmpIndices[1]);
				mesh.indices.push_back(tmpIndices[2]);

				// Triangle 2
				mesh.indices.push_back(tmpIndices[0]);
				mesh.indices.push_back(tmpIndices[2]);
				mesh.indices.push_back(tmpIndices[3]);
			}
		}
	}

	calculateTangents(mesh);

	model.primitives.push_back({ mesh, {} });

	file.close();
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

			Math::mat4 modelMatrix = Math::mat4();

			for (size_t i = 0; i < scene->nodes_count; i++) {
				loadGltfNode(filePath, model, scene->nodes[i], modelMatrix);
			}
		}

		cgltf_free(data);
	}
	else {
		NTSHENGN_MODULE_WARNING("Could not load model file \"" + filePath + "\".");
	}
}

void NtshEngn::AssetLoaderModule::loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, Math::mat4 modelMatrix) {
	if (node->has_matrix) {
		modelMatrix *= Math::mat4(node->matrix);
	}
	else {
		if (node->has_translation) {
			modelMatrix *= Math::translate(Math::vec3(node->translation));
		}
		if (node->has_rotation) {
			modelMatrix *= Math::to_mat4(Math::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
		}
		if (node->has_scale) {
			modelMatrix *= Math::scale(Math::vec3(node->scale));
		}
	}

	if (node->mesh != NULL) {
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
			unsigned short* joints = nullptr;
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
					joints = reinterpret_cast<unsigned short*>(bufferOffset);
					jointsCount = attribute.data->count;
					jointsStride = std::max(bufferView->stride, 4 * sizeof(unsigned short));
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

				Math::vec3 vertexPosition = modelMatrix * Math::vec4(Math::vec3(position + positionCursor), 1.0f);
				vertex.position = { vertexPosition.x, vertexPosition.y, vertexPosition.z };
				positionCursor += (positionStride / sizeof(float));

				Math::vec3 vertexNormal = (normalCount != 0) ? Math::normalize(Math::vec3(Math::transpose(Math::inverse(modelMatrix)) * Math::vec4(Math::vec3(normal + normalCursor), 1.0f))) : Math::vec3(0.0f, 0.0f, 0.0f);
				vertex.normal = { vertexNormal.x, vertexNormal.y, vertexNormal.z };
				normalCursor += (normalStride / sizeof(float));

				Math::vec2 vertexUV = (uvCount != 0) ? Math::vec2(uv + uvCursor) : Math::vec2(0.5f, 0.5f);
				vertex.uv = { vertexUV.x, vertexUV.y };
				uvCursor += (uvStride / sizeof(float));

				Math::vec3 vertexColor = (colorCount != 0) ? Math::vec3(color + colorCursor) : Math::vec3(0.0f, 0.0f, 0.0f);
				vertex.color = { vertexColor.x, vertexColor.y, vertexColor.z };
				colorCursor += (colorStride / sizeof(float));

				Math::vec4 vertexTangent = (tangentCount != 0) ? Math::vec4(tangent + tangentCursor) : Math::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				vertex.tangent = { vertexTangent.x, vertexTangent.y, vertexTangent.z, vertexTangent.w };
				tangentCursor += (tangentStride / sizeof(float));

				Math::vec4 vertexJoints = (jointsCount != 0) ? Math::vec4(static_cast<float>(joints[jointsCursor]), static_cast<float>(joints[jointsCursor + 1]), static_cast<float>(joints[jointsCursor] + 2), static_cast<float>(joints[jointsCursor + 3])) : Math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				vertex.joints = { vertexJoints.x, vertexJoints.y, vertexJoints.z, vertexJoints.w };
				jointsCursor += (jointsStride / sizeof(unsigned short));

				Math::vec4 vertexWeights = (weightsCount != 0) ? Math::vec4(weights + weightsCursor) : Math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				vertex.weights = { vertexWeights.x, vertexWeights.y, vertexWeights.z, vertexWeights.w };
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

			// Tangents
			if ((tangentCount == 0) && (uvCount != 0) && (normalCount != 0) && (primitive.mesh.indices.size() != 0)) {
				calculateTangents(primitive.mesh);

				// Invert tangent handedness
				for (Vertex& vertex : primitive.mesh.vertices) {
					vertex.tangent[3] *= -1.0f;
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
						std::string imageURI = baseColorImage->uri;

						if (m_internalImages.find(imageURI) == m_internalImages.end()) {
							Image image;

							size_t base64Pos = imageURI.find(";base64,");
							if (base64Pos != std::string::npos) {
								cgltf_options options = {};

								const std::string uriBase64 = imageURI.substr(base64Pos + 8);
								const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
								void* decodedData = new uint8_t[decodedDataSize];
								cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), &decodedData);
								if (result == cgltf_result_success) {
									loadImageFromMemory(decodedData, decodedDataSize, image);
									image.colorSpace = ImageColorSpace::SRGB;
								}
								else {
									NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (base color texture).");
								}
								delete[] decodedData;
							}
							else {
								image = loadImage(File::directory(filePath) + imageURI);
								image.colorSpace = ImageColorSpace::SRGB;
							}

							m_internalImages[imageURI] = image;
						}
						primitive.material.diffuseTexture.image = &m_internalImages[imageURI];
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
					else if (baseColorFactor != NULL) {
						std::string mapKey = "srgb" + std::to_string(baseColorFactor[0]) + std::to_string(baseColorFactor[1]) + std::to_string(baseColorFactor[2]) + std::to_string(baseColorFactor[3]);

						if (m_internalImages.find(mapKey) == m_internalImages.end()) {
							Image image;
							image.width = 1;
							image.height = 1;
							image.format = ImageFormat::R8G8B8A8;
							image.colorSpace = ImageColorSpace::SRGB;
							image.data = { static_cast<uint8_t>(round(255.0f * baseColorFactor[0])),
								static_cast<uint8_t>(round(255.0f * baseColorFactor[1])),
								static_cast<uint8_t>(round(255.0f * baseColorFactor[2])),
								static_cast<uint8_t>(round(255.0f * baseColorFactor[3]))
							};

							m_internalImages[mapKey] = image;
						}
						primitive.material.diffuseTexture.image = &m_internalImages[mapKey];
						primitive.material.diffuseTexture.imageSampler = nearestSampler;
					}

					// Metallic Roughness texture
					cgltf_texture_view metallicRoughnessTextureView = pbrMetallicRoughness.metallic_roughness_texture;
					cgltf_texture* metallicRoughnessTexture = metallicRoughnessTextureView.texture;
					cgltf_float metallicFactor = pbrMetallicRoughness.metallic_factor;
					cgltf_float roughnessFactor = pbrMetallicRoughness.roughness_factor;
					if (metallicRoughnessTexture != NULL) {
						cgltf_image* metallicRoughnessImage = metallicRoughnessTexture->image;
						std::string imageURI = metallicRoughnessImage->uri;

						if (m_internalImages.find(imageURI) == m_internalImages.end()) {
							Image image;

							size_t base64Pos = imageURI.find(";base64,");
							if (base64Pos != std::string::npos) {
								cgltf_options options = {};

								const std::string uriBase64 = imageURI.substr(base64Pos + 8);
								const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
								void* decodedData = new uint8_t[decodedDataSize];
								cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), &decodedData);
								if (result == cgltf_result_success) {
									loadImageFromMemory(decodedData, decodedDataSize, image);
									image.colorSpace = ImageColorSpace::Linear;
								}
								else {
									NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (metallic roughness texture).");
								}
								delete[] decodedData;
							}
							else {
								image = loadImage(File::directory(filePath) + imageURI);
								image.colorSpace = ImageColorSpace::Linear;
							}

							m_internalImages[imageURI] = image;
						}
						primitive.material.metalnessTexture.image = &m_internalImages[imageURI];
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
					else {
						std::string mapKey = "linear" + std::to_string(metallicFactor) + std::to_string(roughnessFactor);

						if (m_internalImages.find(mapKey) == m_internalImages.end()) {
							Image image;
							image.width = 1;
							image.height = 1;
							image.format = ImageFormat::R8G8B8A8;
							image.colorSpace = ImageColorSpace::Linear;
							image.data = { 0,
								static_cast<uint8_t>(round(255.0f * roughnessFactor)),
								static_cast<uint8_t>(round(255.0f * metallicFactor)),
								0
							};

							m_internalImages[mapKey] = image;
						}
						primitive.material.metalnessTexture.image = &m_internalImages[mapKey];
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
					std::string imageURI = normalImage->uri;

					if (m_internalImages.find(imageURI) == m_internalImages.end()) {
						Image image;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							void* decodedData = new uint8_t[decodedDataSize];
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), &decodedData);
							if (result == cgltf_result_success) {
								loadImageFromMemory(decodedData, decodedDataSize, image);
								image.colorSpace = ImageColorSpace::Linear;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (normal texture).");
							}
							delete[] decodedData;
						}
						else {
							image = loadImage(File::directory(filePath) + imageURI);
							image.colorSpace = ImageColorSpace::Linear;
						}

						m_internalImages[imageURI] = image;
					}
					primitive.material.normalTexture.image = &m_internalImages[imageURI];
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

				// Emissive texture
				cgltf_texture_view emissiveTextureView = primitiveMaterial->emissive_texture;
				cgltf_texture* emissiveTexture = emissiveTextureView.texture;
				cgltf_float* emissiveFactor = primitiveMaterial->emissive_factor;
				if (emissiveTexture != NULL) {
					cgltf_image* emissiveImage = emissiveTexture->image;
					std::string imageURI = emissiveImage->uri;

					if (m_internalImages.find(imageURI) == m_internalImages.end()) {
						Image image;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							void* decodedData = new uint8_t[decodedDataSize];
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), &decodedData);
							if (result == cgltf_result_success) {
								loadImageFromMemory(decodedData, decodedDataSize, image);
								image.colorSpace = ImageColorSpace::SRGB;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (emissive texture).");
							}
							delete[] decodedData;
						}
						else {
							image = loadImage(File::directory(filePath) + imageURI);
							image.colorSpace = ImageColorSpace::SRGB;
						}

						m_internalImages[imageURI] = image;
					}
					primitive.material.emissiveTexture.image = &m_internalImages[imageURI];
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
				else if (emissiveFactor != NULL) {
					std::string mapKey = "srgb" + std::to_string(emissiveFactor[0]) + std::to_string(emissiveFactor[1]) + std::to_string(emissiveFactor[2]);

					if (m_internalImages.find(mapKey) == m_internalImages.end()) {
						Image image;
						image.width = 1;
						image.height = 1;
						image.format = ImageFormat::R8G8B8A8;
						image.colorSpace = ImageColorSpace::SRGB;
						image.data = { static_cast<uint8_t>(round(255.0f * emissiveFactor[0])),
							static_cast<uint8_t>(round(255.0f * emissiveFactor[1])),
							static_cast<uint8_t>(round(255.0f * emissiveFactor[2])),
							255
						};

						m_internalImages[mapKey] = image;
					}
					primitive.material.emissiveTexture.image = &m_internalImages[mapKey];
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
					std::string imageURI = occlusionImage->uri;

					if (m_internalImages.find(imageURI) == m_internalImages.end()) {
						Image image;

						size_t base64Pos = imageURI.find(";base64,");
						if (base64Pos != std::string::npos) {
							cgltf_options options = {};

							const std::string uriBase64 = imageURI.substr(base64Pos + 8);
							const size_t decodedDataSize = ((3 * uriBase64.size()) / 4) - std::count(uriBase64.begin(), uriBase64.end(), '=');
							void* decodedData = new uint8_t[decodedDataSize];
							cgltf_result result = cgltf_load_buffer_base64(&options, decodedDataSize, uriBase64.c_str(), &decodedData);
							if (result == cgltf_result_success) {
								loadImageFromMemory(decodedData, decodedDataSize, image);
								image.colorSpace = ImageColorSpace::Linear;
							}
							else {
								NTSHENGN_MODULE_WARNING("Invalid Base64 data when loading glTF embedded texture for model file \"" + filePath + "\" (occlusion texture).");
							}
							delete[] decodedData;
						}
						else {
							image = loadImage(File::directory(filePath) + imageURI);
							image.colorSpace = ImageColorSpace::Linear;
						}

						m_internalImages[imageURI] = image;
					}
					primitive.material.occlusionTexture.image = &m_internalImages[imageURI];
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

	for (size_t i = 0; i < node->children_count; i++) {
		loadGltfNode(filePath, model, node->children[i], modelMatrix);
	}
}

extern "C" NTSHENGN_MODULE_API NtshEngn::AssetLoaderModuleInterface* createModule() {
	return new NtshEngn::AssetLoaderModule;
}

extern "C" NTSHENGN_MODULE_API void destroyModule(NtshEngn::AssetLoaderModuleInterface* m) {
	delete m;
}