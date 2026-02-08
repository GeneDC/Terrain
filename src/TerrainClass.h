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
#include "mesh_generator.h"

using namespace godot;

class TerrainClass : public Resource
{
	GDCLASS(TerrainClass, Resource)

public:
	TerrainClass() = default;
	~TerrainClass() override = default;

	bool init();
	void update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos);

	Ref<ChunkGenerator> chunk_generator;
	Ref<MeshGenerator> mesh_generator;

protected:
	static void _bind_methods();

	Ref<ChunkGenerator> get_chunk_generator() const { return chunk_generator; }
	void set_chunk_generator(Ref<ChunkGenerator> p_chunk_generator) { chunk_generator = p_chunk_generator; }

private:
};
