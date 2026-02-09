#pragma once

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/resource.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/variant.hpp"
#include "godot_cpp/core/binder_common.hpp"
#include <godot_cpp/classes/rd_shader_file.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/rendering_device.hpp>
#include <godot_cpp/classes/rd_shader_spirv.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>

#include "chunk_generator.h"

using namespace godot;

struct MeshData
{
	Vector3i chunk_pos{};
	Array mesh_arrays{};
	uint32_t vertex_count = 0;
};

class MeshGenerator : public RefCounted
{
	GDCLASS(MeshGenerator, RefCounted)

public:
	MeshGenerator() = default;
	~MeshGenerator() override;

	// Call once to setup. Creates local rendering device, loads shader, and setups the buffers and uniforms
	bool init();
	// Returns MeshData for a ArrayMesh using the compute shader results.
	[[nodiscard]] MeshData generate_mesh_data(Vector3i chunk_pos, PackedFloat32Array points);

protected:
	static void _bind_methods() {};

private:
	RenderingDevice* local_rendering_device = nullptr;

	uint64_t rendering_thread_id = -1;

	Ref<RDShaderFile> shader_file;
	Ref<RDShaderSPIRV> shader_spirv;
	RID shader;
	RID uniform_set;
	RID pipeline;

	// Shader Input buffers
	RID points_buffer;
	
	// Shader Output buffers
	int vertex_buffer_byte_count = -1;
	RID vertex_buffer;
	RID normal_buffer;
	int colour_buffer_byte_count = -1;
	RID colour_buffer;
	RID count_buffer;
};
