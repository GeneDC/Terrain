#include "chunk_generator.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/os.hpp>

using namespace godot;

#include "terrain_constants.hpp"
using namespace terrain_constants;

void ChunkGenerator::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("generate_points", "chunk_pos"), &ChunkGenerator::generate_points);

	ClassDB::bind_method(D_METHOD("get_base_height_offset"), &ChunkGenerator::get_base_height_offset);
	ClassDB::bind_method(D_METHOD("set_base_height_offset", "base_height_offset"), &ChunkGenerator::set_base_height_offset);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_height_offset"), "set_base_height_offset", "get_base_height_offset");

	ClassDB::bind_method(D_METHOD("get_height_base_noise"), &ChunkGenerator::get_height_base_noise);
	ClassDB::bind_method(D_METHOD("set_height_base_noise", "height_base_noise"), &ChunkGenerator::set_height_base_noise);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_base_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_height_base_noise", "get_height_base_noise");

	ClassDB::bind_method(D_METHOD("get_base_height_multiplier"), &ChunkGenerator::get_base_height_multiplier);
	ClassDB::bind_method(D_METHOD("set_base_height_multiplier", "base_height_multiplier"), &ChunkGenerator::set_base_height_multiplier);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_height_multiplier"), "set_base_height_multiplier", "get_base_height_multiplier");

	ClassDB::bind_method(D_METHOD("get_height_multiplier_noise"), &ChunkGenerator::get_height_multiplier_noise);
	ClassDB::bind_method(D_METHOD("set_height_multiplier_noise", "height_multiplier_noise"), &ChunkGenerator::set_height_multiplier_noise);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "height_multiplier_noise", PROPERTY_HINT_RESOURCE_TYPE, "FastNoiseLite"), "set_height_multiplier_noise", "get_height_multiplier_noise");
}

PackedFloat32Array ChunkGenerator::generate_points(Vector3i chunk_pos)
{
	// NOTE: this currently generates a chunk size + 1 array, but a chunk only needs the the chunk size data and the extra data can be added before it's sent to the shader

	PackedFloat32Array points{};
	points.resize(POINTS_VOLUME);
	points.fill(0.0f);

	Vector3 chunk_world_pos = chunk_pos * CHUNK_SIZE;

	Vector<float> height_map = _generate_height_map(POINTS_SIZE, chunk_world_pos);

	for (int x = 0; x < POINTS_SIZE; x++) {
		for (int z = 0; z < POINTS_SIZE; z++) {
			float height = height_map[x + z * POINTS_SIZE];

			for (int y = 0; y < POINTS_SIZE; y++) {
				Vector3 world_pos = chunk_world_pos + Vector3(x, y, z);

				//float density = generate_density(world_pos, height);
				float value = CLAMP(height - world_pos.y, 0.0f, 1.0f);
				//value = CLAMP(value - density, 0.0f, 1.0f);

				points[x + y * POINTS_SIZE + z * POINTS_SIZE * POINTS_SIZE] = value;
			}
		}
	}
	
	return points;
}

Vector<float> ChunkGenerator::_generate_height_map(int p_size, const Vector3 &p_chunk_world_pos) const
{
	Vector<float> height_map = Vector<float>();
	height_map.resize(p_size * p_size);

	if (!height_base_noise.is_valid())
	{
		print_error("height_base_noise is invalid");
		return height_map;
	}

	if (!height_multiplier_noise.is_valid())
	{
		print_error("height_multiplier_noise is invalid");
		return height_map;
	}

	for (int x = 0; x < p_size; x++)
	{
		for (int z = 0; z < p_size; z++)
		{
			Vector2 pos = Vector2(x + p_chunk_world_pos.x, z + p_chunk_world_pos.z);
			float height_offset = base_height_offset + 100 * height_multiplier_noise->get_noise_2dv(pos);
			float height_base = height_base_noise->get_noise_2dv(pos);
			float height = height_offset + height_base * base_height_multiplier;
			height_map.set(x + z * p_size, height);
		}
	}

	return height_map;
}
