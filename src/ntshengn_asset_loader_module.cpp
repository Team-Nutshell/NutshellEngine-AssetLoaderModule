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
#include <algorithm>
#include <unordered_map>

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
		NTSHENGN_MODULE_WARNING("Sound file extension \"." + extension + "\" not supported.");
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
	std::vector<nml::vec3> tan1(mesh.vertices.size());
	std::vector<nml::vec3> tan2(mesh.vertices.size());
	for (size_t i = 0; i < mesh.indices.size(); i += 3) {
		NtshEngn::Vertex& vertex0 = mesh.vertices[mesh.indices[i]];
		NtshEngn::Vertex& vertex1 = mesh.vertices[mesh.indices[i + 1]];
		NtshEngn::Vertex& vertex2 = mesh.vertices[mesh.indices[i + 2]];

		const nml::vec3 dPos1 = nml::vec3(vertex1.position.data()) - nml::vec3(vertex0.position.data());
		const nml::vec3 dPos2 = nml::vec3(vertex2.position.data()) - nml::vec3(vertex0.position.data());

		const nml::vec2 dUV1 = nml::vec2(vertex1.uv.data()) - nml::vec2(vertex0.uv.data());
		const nml::vec2 dUV2 = nml::vec2(vertex2.uv.data()) - nml::vec2(vertex0.uv.data());

		const float r = 1.0f / (dUV1.x * dUV2.y - dUV1.y * dUV2.x);

		const nml::vec3 uDir = (dPos1 * dUV2.y - dPos2 * dUV1.y) * r;
		const nml::vec3 vDir = (dPos2 * dUV1.x - dPos1 * dUV2.x) * r;

		tan1[mesh.indices[i]] += uDir;
		tan1[mesh.indices[i + 1]] += uDir;
		tan1[mesh.indices[i + 2]] += uDir;

		tan2[mesh.indices[i]] += vDir;
		tan2[mesh.indices[i + 1]] += vDir;
		tan2[mesh.indices[i + 2]] += vDir;
	}

	for (size_t i = 0; i < mesh.vertices.size(); i++) {
		const nml::vec3 n = mesh.vertices[i].normal.data();
		const nml::vec3 t = tan1[i].data();

		const nml::vec4 tangent = nml::vec4(nml::normalize(t - n * nml::dot(n, t)),
			(nml::dot(nml::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f);

		mesh.vertices[i].tangent = { tangent.x, tangent.y, tangent.z, tangent.w };
	}
}

std::array<std::array<float, 3>, 2> NtshEngn::AssetLoaderModule::calculateAABB(const Mesh& mesh) {
	nml::vec3 min = nml::vec3(std::numeric_limits<float>::max());
	nml::vec3 max = nml::vec3(std::numeric_limits<float>::lowest());
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

			nml::mat4 modelMatrix = nml::mat4();

			for (size_t i = 0; i < scene->nodes_count; i++) {
				loadGltfNode(filePath, model, scene->nodes[i], modelMatrix);
			}
		}
	}
	else {
		NTSHENGN_MODULE_WARNING("Could not load model file \"" + filePath + "\".");
	}
}

void NtshEngn::AssetLoaderModule::loadGltfNode(const std::string& filePath, Model& model, cgltf_node* node, nml::mat4& modelMatrix) {
	if (node->has_matrix) {
		modelMatrix *= nml::mat4(node->matrix);
	}
	else {
		if (node->has_translation) {
			modelMatrix *= nml::translate(nml::vec3(node->translation));
		}
		if (node->has_rotation) {
			modelMatrix *= nml::to_mat4(nml::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]));
		}
		if (node->has_scale) {
			modelMatrix *= nml::scale(nml::vec3(node->scale));
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

				nml::vec3 vertexPosition = modelMatrix * nml::vec4(nml::vec3(position + positionCursor), 1.0f);
				vertex.position = { vertexPosition.x, vertexPosition.y, vertexPosition.z };
				positionCursor += (positionStride / sizeof(float));

				nml::vec3 vertexNormal = (normalCount != 0) ? nml::vec3(modelMatrix * nml::vec4(nml::vec3(normal + normalCursor), 1.0f)) : nml::vec3(0.0f, 0.0f, 0.0f);
				vertex.normal = { vertexNormal.x, vertexNormal.y, vertexNormal.z };
				normalCursor += (normalStride / sizeof(float));

				nml::vec2 vertexUV = (uvCount != 0) ? nml::vec2(uv + uvCursor) : nml::vec2(0.5f, 0.5f);
				vertex.uv = { vertexUV.x, vertexUV.y };
				uvCursor += (uvStride / sizeof(float));

				nml::vec3 vertexColor = (colorCount != 0) ? nml::vec3(color + colorCursor) : nml::vec3(0.0f, 0.0f, 0.0f);
				vertex.color = { vertexColor.x, vertexColor.y, vertexColor.z };
				colorCursor += (colorStride / sizeof(float));

				nml::vec4 vertexTangent = (tangentCount != 0) ? nml::vec4(tangent + tangentCursor) : nml::vec4(0.5f, 0.5f, 0.5f, 1.0f);
				vertex.tangent = { vertexTangent.x, vertexTangent.y, vertexTangent.z, vertexTangent.w };
				tangentCursor += (tangentStride / sizeof(float));

				nml::vec4 vertexJoints = (jointsCount != 0) ? nml::vec4(static_cast<float>(joints[jointsCursor]), static_cast<float>(joints[jointsCursor + 1]), static_cast<float>(joints[jointsCursor] + 2), static_cast<float>(joints[jointsCursor + 3])) : nml::vec4(0.0f, 0.0f, 0.0f, 0.0f);
				vertex.joints = { vertexJoints.x, vertexJoints.y, vertexJoints.z, vertexJoints.w };
				jointsCursor += (jointsStride / sizeof(unsigned short));

				nml::vec4 vertexWeights = (weightsCount != 0) ? nml::vec4(weights + weightsCursor) : nml::vec4(0.0f, 0.0f, 0.0f, 0.0f);
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
					std::copy(index, index + accessor->count, primitive.mesh.indices.begin());
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
			if ((tangentCount == 0) && (uvCount != 0) && (primitive.mesh.indices.size() != 0)) {
				calculateTangents(primitive.mesh);
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