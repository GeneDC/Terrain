#pragma once

#include "godot_cpp/classes/resource.hpp"
#include <godot_cpp/classes/fast_noise_lite.hpp>

using namespace godot;

class ChunkGenerator : public Resource
{
	GDCLASS(ChunkGenerator, Resource)

public:
	ChunkGenerator() = default;
	~ChunkGenerator() override = default;

	PackedFloat32Array generate_points(Vector3i chunk_pos) const;

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
};
