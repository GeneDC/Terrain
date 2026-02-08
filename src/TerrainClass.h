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

using namespace godot;

class TerrainClass : public Resource
{
	GDCLASS(TerrainClass, Resource)

public:
	TerrainClass() = default;
	~TerrainClass() override;

	bool init();
	void update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos);

	// Modifies where the terrain height should start int base_height_offset;
	float base_height_offset = 0.0f;
	// Base height. Creates the initial variations in terrain
	Ref<FastNoiseLite> height_base_noise;
	// Exagerates the values from the Base Height
	float base_height_multiplier = 1.0f;
	// Continentalness. Changes terrain height over greater distances
	Ref<FastNoiseLite> height_multiplier_noise;

protected:
	static void _bind_methods();

	float get_base_height_offset() const { return base_height_offset; }
	void set_base_height_offset(float p_base_height_offset) { base_height_offset = p_base_height_offset; }

	Ref<FastNoiseLite> get_height_base_noise() { return height_base_noise; }
	void set_height_base_noise(Ref<FastNoiseLite> p_height_base_noise) { height_base_noise = p_height_base_noise; }

	float get_base_height_multiplier() const { return base_height_multiplier; }
	void set_base_height_multiplier(float p_base_height_multiplier) { base_height_multiplier = p_base_height_multiplier; }

	Ref<FastNoiseLite> get_height_multiplier_noise() { return height_multiplier_noise; }
	void set_height_multiplier_noise(Ref<FastNoiseLite> p_height_multiplier_noise) { height_multiplier_noise = p_height_multiplier_noise; }

private:
	Vector<float> _generate_height_map(int p_size, const Vector3 &p_chunk_world_pos) const;

	void _update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos);
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
