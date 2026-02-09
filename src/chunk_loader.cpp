#include "chunk_loader.h"

#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/classes/time.hpp>

#include <cstdio>

using namespace godot;

#include "godot_utility.h"
#include "terrain_constants.hpp"
using namespace terrain_constants;

void ChunkLoader::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("init"), &ChunkLoader::init);
	ClassDB::bind_method(D_METHOD("update"), &ChunkLoader::update);
	ClassDB::bind_method(D_METHOD("stop"), &ChunkLoader::stop);
	ClassDB::bind_method(D_METHOD("queue_chunk_update", "chunk_pos"), &ChunkLoader::queue_chunk_update);

	ClassDB::bind_method(D_METHOD("get_chunk_generator"), &ChunkLoader::get_chunk_generator);
	ClassDB::bind_method(D_METHOD("set_chunk_generator", "chunk_generator"), &ChunkLoader::set_chunk_generator);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "chunk_generator", PROPERTY_HINT_RESOURCE_TYPE, "ChunkGenerator"), "set_chunk_generator", "get_chunk_generator");
}

bool ChunkLoader::init()
{
	if (!chunk_generator.is_valid())
	{
		PRINT_ERROR("chunk_generator not set!");
		return false;
	}

	if (!mesh_generator_pool.is_valid())
	{
		mesh_generator_pool.instantiate();
		mesh_generator_pool->init(4); // 4 Threads
	}

	return true;
}

void ChunkLoader::update()
{
	{ // move the done meshes to our array so we can take time applying them
		std::vector<MeshData> done_mesh_datas = mesh_generator_pool->take_done_mesh_data();
		if (!done_mesh_datas.empty())
		{
			mesh_datas.insert(

				mesh_datas.end(),
				std::make_move_iterator(done_mesh_datas.begin()),
				std::make_move_iterator(done_mesh_datas.end())
			);
		}
	}

	uint64_t start_time = Time::get_singleton()->get_ticks_usec();
	// budget in microseconds: 2000us = 2ms
	constexpr uint64_t time_budget = 4000;

	while (!mesh_datas.empty())
	{
		if (Time::get_singleton()->get_ticks_usec() - start_time > time_budget)
		{
			break;
		}

		MeshData mesh_data = mesh_datas.back();
		mesh_datas.pop_back();

		MeshInstance3D* chunk = get_chunk(mesh_data.chunk_pos);
	
		Ref<ArrayMesh> array_mesh = chunk->get_mesh();
		if (array_mesh.is_null())
		{
			PRINT_ERROR("array_mesh is invalid");
			continue;
		}

		// remove all old surface meshes
		while (array_mesh->get_surface_count() > 0)
		{
			array_mesh->surface_remove(0);
		}
		
		if (mesh_data.vertex_count > 0 && !mesh_data.mesh_arrays.is_empty())
		{
			array_mesh->add_surface_from_arrays(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES, mesh_data.mesh_arrays);
		}
	}
}

void ChunkLoader::stop()
{
	mesh_generator_pool->stop(); // Blocks execution until all threads are stopped
}

void ChunkLoader::queue_chunk_update(Vector3i chunk_pos)
{
	if (!mesh_generator_pool.is_valid())
	{
		PRINT_ERROR("not initialised");
		return;
	}

	if (!chunk_generator.is_valid())
	{
		PRINT_ERROR("chunk_generator not set!");
		return;
	}

	Callable callable = callable_mp(this, &ChunkLoader::_queue_generate_mesh_data);
	Callable callable_bound = callable.bind(chunk_pos);
	WorkerThreadPool::get_singleton()->add_task(callable_bound, true);
}

void ChunkLoader::_queue_generate_mesh_data(Vector3i chunk_pos)
{
	PackedFloat32Array points = chunk_generator->generate_points(chunk_pos);
	mesh_generator_pool->queue_generate_mesh_data(chunk_pos, points);
}

MeshInstance3D* ChunkLoader::get_chunk(Vector3i chunk_pos)
{
	auto it = chunk_node_map.find(chunk_pos);
	if (it != chunk_node_map.end())
	{
		return it->value;
	}

	MeshInstance3D* mesh_instance = memnew(MeshInstance3D);

	Ref<ArrayMesh> array_mesh;
	array_mesh.instantiate();
	mesh_instance->set_mesh(array_mesh);

	mesh_instance->set_position(chunk_pos * CHUNK_SIZE);

#ifdef DEBUG_ENABLED
	// The node name shouldn't be needed in release so we can skip it for a negligible speed increase
	char buffer[32]; 
	std::snprintf(buffer, sizeof(buffer), "Chunk_%d_%d_%d", chunk_pos.x, chunk_pos.y, chunk_pos.z);
	mesh_instance->set_name(buffer);
#endif

	add_child(mesh_instance);
	chunk_node_map[chunk_pos] = mesh_instance;

	return mesh_instance;
}