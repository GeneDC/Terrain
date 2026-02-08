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

#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/semaphore.hpp>

#include "chunk_generator.h"

using namespace godot;

class MeshGenerator : public RefCounted
{
	GDCLASS(MeshGenerator, RefCounted)

public:
	MeshGenerator() = default;
	~MeshGenerator() override;

	bool init();
	void update_chunk_mesh(Ref<ArrayMesh> array_mesh, PackedFloat32Array points);

protected:
	static void _bind_methods();

private:
	void _update_chunk_mesh(Ref<ArrayMesh> array_mesh, PackedFloat32Array points);
	void _apply_mesh_data(Ref<ArrayMesh> array_mesh, Array mesh_arrays);

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

	bool has_requested_chunk = false;
	Ref<Thread> worker_thread;
	bool exit_thread = false;
};
