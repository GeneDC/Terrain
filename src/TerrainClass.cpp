#include "TerrainClass.h"

#include "godot_cpp/classes/surface_tool.hpp"
#include "godot_cpp/classes/mesh_instance3d.hpp"
#include "godot_cpp/classes/array_mesh.hpp"
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/rd_uniform.hpp>

using namespace godot;

#include "godot_utility.h"
#include "terrain_constants.hpp"
using namespace terrain_constants;

void TerrainClass::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("init"), &TerrainClass::init);
	ClassDB::bind_method(D_METHOD("update_chunk_mesh", "array_mesh", "chunk_pos"), &TerrainClass::update_chunk_mesh);

	ClassDB::bind_method(D_METHOD("get_chunk_generator"), &TerrainClass::get_chunk_generator);
	ClassDB::bind_method(D_METHOD("set_chunk_generator", "chunk_generator"), &TerrainClass::set_chunk_generator);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "chunk_generator", PROPERTY_HINT_RESOURCE_TYPE, "ChunkGenerator"), "set_chunk_generator", "get_chunk_generator");
}

void TerrainClass::update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos)
{
	if (!mesh_generator.is_valid())
	{
		PRINT_ERROR("not initialised");
		return;
	}

	if (!chunk_generator.is_valid())
	{
		PRINT_ERROR("chunk_generator not set!");
		return;
	}

	PackedFloat32Array points = chunk_generator->generate_points(chunk_pos);
	mesh_generator->update_chunk_mesh(array_mesh, points);
}

bool TerrainClass::init()
{
	if (!chunk_generator.is_valid())
	{
		PRINT_ERROR("chunk_generator not set!");
		return false;
	}

	if (!mesh_generator.is_valid())
	{
		mesh_generator.instantiate();
	}

	return mesh_generator->init();
}
