#pragma once

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/semaphore.hpp>
#include <deque>

using namespace godot;

template <typename T>
class SafeQueue
{
public:
	SafeQueue()
	{
		mutex.instantiate();
		semaphore.instantiate();
	}

	void push(T value)
	{
		mutex->lock();
		queue.push_back(value);
		mutex->unlock();

		semaphore->post();
	}

	T pop_blocking()
	{
		semaphore->wait();
		
		mutex->lock();

		if (queue.empty())
		{
			mutex->unlock();
			return nullptr; 
		}

		T value = queue.front();
		queue.pop_front();

		mutex->unlock();
		return value;
	}

	bool is_empty()
	{
		mutex->lock();
		bool empty = queue.empty();
		mutex->unlock();
		return empty;
	}

private:
	std::deque<T> queue;
	Ref<Mutex> mutex;
	Ref<Semaphore> semaphore;
};
