#pragma once

#include "godot_cpp/classes/node3d.hpp"
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>

#include "chunk_generator.h"
#include "mesh_generator_pool.h"

using namespace godot;

class ChunkLoader : public Node
{
	GDCLASS(ChunkLoader, Node)

public:
	bool init();
	void update();
	void stop();
	
	void queue_chunk_update(Vector3i chunk_pos);

	Ref<ChunkGenerator> chunk_generator;
	Ref<MeshGeneratorPool> mesh_generator_pool;

protected:
	static void _bind_methods();

	void _queue_generate_mesh_data(Vector3i chunk_pos);

	Ref<ChunkGenerator> get_chunk_generator() const { return chunk_generator; }
	void set_chunk_generator(Ref<ChunkGenerator> p_chunk_generator) { chunk_generator = p_chunk_generator; }

private:
	MeshInstance3D* get_chunk(Vector3i chunk_pos);

	HashMap<Vector3i, MeshInstance3D*> chunk_node_map{};
	std::vector<MeshData> mesh_datas{};

};
