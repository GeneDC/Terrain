#include "mesh_generator_pool.h"

#include "godot_utility.h"

using namespace godot;

void MeshGeneratorPool::init(uint32_t p_thread_count)
{
	if (stopping)
	{
		PRINT_ERROR("can't init while stopping");
		return;
	}

	done_mesh_data_mutex.instantiate();
	
	for (int i = 0; i < p_thread_count; i++)
	{
		Ref<Thread> thread;
		thread.instantiate();
		
		mesh_workers.emplace_back(memnew(MeshWorker));
		size_t index = mesh_workers.size() - 1;

		Callable callable = callable_mp(this, &MeshGeneratorPool::_thread_loop);
		Callable bound_callable = callable.bind(index);
		thread->start(bound_callable);
		
		threads.emplace_back(thread);
	}
}

void MeshGeneratorPool::stop()
{
	stopping = true;

	for (MeshWorker* mesh_worker : mesh_workers)
	{
		// Add null tasks to wake up the threads
		task_queue.push(nullptr);
	}
	
	for (Ref<Thread> thread : threads)
	{
		if (thread.is_valid() && thread->is_started())
		{
			thread->wait_to_finish();
		}
	}
	threads.clear();
	
	for (MeshWorker* mesh_worker : mesh_workers)
	{
		memdelete(mesh_worker);
	}
	mesh_workers.clear();
}

void MeshGeneratorPool::queue_generate_mesh_data(Vector3i chunk_pos, PackedFloat32Array points)
{
	if (stopping)
	{
		PRINT_ERROR("can't update chunk while stopping");
		return;
	}

	Task* task = memnew(Task);
	task->chunk_pos = chunk_pos;
	task->points = points;
	task_queue.push(task);
}

std::vector<MeshData> MeshGeneratorPool::take_done_mesh_data()
{
	done_mesh_data_mutex->lock();
	std::vector<MeshData> result = std::move(done_mesh_data);
	done_mesh_data.clear(); 
	done_mesh_data_mutex->unlock();
	return result;
}

void MeshGeneratorPool::_thread_loop(size_t mesh_worker_index)
{
	auto mesh_worker = mesh_workers[mesh_worker_index];
	mesh_worker->mesh_generator.instantiate();
	mesh_worker->mesh_generator->init();

	while (!stopping)
	{
		Task* task = task_queue.pop_blocking();

		if (!task || stopping)
		{
			break;
		}

		MeshData mesh_data = mesh_worker->mesh_generator->generate_mesh_data(task->chunk_pos, task->points);
		done_mesh_data_mutex->lock();
		done_mesh_data.push_back(std::move(mesh_data));
		done_mesh_data_mutex->unlock();

		memdelete(task);
	}
	mesh_worker->mesh_generator.unref();
}
