#pragma once

#include "godot_cpp/classes/ref_counted.hpp"
#include <godot_cpp/classes/thread.hpp>

#include "safe_queue.hpp"
#include "mesh_generator.h"

using namespace godot;

class MeshGeneratorPool : public RefCounted
{
	GDCLASS(MeshGeneratorPool, RefCounted)

public:
	void init(uint32_t p_thread_count);
	void stop();
	void queue_generate_mesh_data(Vector3i chunk_pos, PackedFloat32Array points);

	[[nodiscard]] std::vector<MeshData> take_done_mesh_data();

protected:
	static void _bind_methods() {};

private:
	struct Task
	{
		Vector3i chunk_pos;
		PackedFloat32Array points;
	};
	SafeQueue<Task*> task_queue{};

	struct MeshWorker
	{
		Ref<MeshGenerator> mesh_generator;
	};
	std::vector<Ref<Thread>> threads{};
	std::vector<MeshWorker*> mesh_workers{};

	std::atomic<bool> stopping{false};

	Ref<Mutex> done_mesh_data_mutex;
	std::vector<MeshData> done_mesh_data{};

	void _thread_loop(size_t mesh_worker_index);
};