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

#include "terrain_constants.hpp"
using namespace terrain_constants;

void TerrainClass::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("init"), &TerrainClass::init);
	ClassDB::bind_method(D_METHOD("update_chunk_mesh", "array_mesh", "chunk_pos"), &TerrainClass::update_chunk_mesh);

	// Required to use call_deferred
	ClassDB::bind_method(D_METHOD("_apply_mesh_data", "mesh", "arrays"), &TerrainClass::_apply_mesh_data);

	ClassDB::bind_method(D_METHOD("get_chunk_generator"), &TerrainClass::get_chunk_generator);
	ClassDB::bind_method(D_METHOD("set_chunk_generator", "chunk_generator"), &TerrainClass::set_chunk_generator);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "chunk_generator", PROPERTY_HINT_RESOURCE_TYPE, "ChunkGenerator"), "set_chunk_generator", "get_chunk_generator");
}

TerrainClass::~TerrainClass()
{
	if (local_rendering_device)
	{
		//local_rendering_device->free_rid(shader);
		//local_rendering_device->free_rid(uniform_set);
		//local_rendering_device->free_rid(pipeline);
		local_rendering_device->free_rid(points_buffer);
		local_rendering_device->free_rid(vertex_buffer);
		local_rendering_device->free_rid(normal_buffer);
		local_rendering_device->free_rid(count_buffer);

		memdelete(local_rendering_device);
	}
}

void TerrainClass::update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos)
{
	_update_chunk_mesh(array_mesh, chunk_pos);
}

bool TerrainClass::init()
{
	rendering_thread_id = OS::get_singleton()->get_thread_caller_id();

	RenderingServer* rendering_server = RenderingServer::get_singleton();
	if (!rendering_server) {
		print_error("Failed to get rendering server");
		return false;
	}

	local_rendering_device = rendering_server->create_local_rendering_device();
	if (!local_rendering_device) {
		print_error("Failed to create local rendering device");
		return false;
	}

	ResourceLoader* resource_loader = ResourceLoader::get_singleton();
	if (!resource_loader) {
		print_error("Missing Resource Loader Singleton");
		return false;
	}

	shader_file = resource_loader->load("res://scripts/ComputeCubes.glsl");
	if (shader_file.is_null()) {
		print_error("Failed to load shader file");
		return false;
	}

	shader_spirv = shader_file->get_spirv();
	shader = local_rendering_device->shader_create_from_spirv(shader_spirv);
	if (!shader.is_valid()) {
		print_error("Failed to create shader from spirv:");
		print_error(shader_spirv->get_stage_compile_error(RenderingDevice::ShaderStage::SHADER_STAGE_COMPUTE));
		return false;
	}

	TypedArray<RDUniform> uniforms{};

	// create a storage buffer that can hold our points.
	points_buffer = local_rendering_device->storage_buffer_create(POINTS_VOLUME * sizeof(float));

	Ref<RDUniform> uniform_points{};
	uniform_points.instantiate();
	uniform_points->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	uniform_points->set_binding(0); // Match layout(binding = 0) in the shader
	uniform_points->add_id(points_buffer);
	uniforms.push_back(uniform_points);

	constexpr int max_triangle_count = CHUNK_VOLUME * 5; // max tris per voxel is 5
	constexpr int max_vertex_count = max_triangle_count * 3;

	vertex_buffer_byte_count = max_vertex_count * sizeof(float) * 3;
	vertex_buffer = local_rendering_device->storage_buffer_create(vertex_buffer_byte_count);

	Ref<RDUniform> uniform_vertex{}; 
	uniform_vertex.instantiate();
	uniform_vertex->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	uniform_vertex->set_binding(1);
	uniform_vertex->add_id(vertex_buffer);
	uniforms.push_back(uniform_vertex);

	normal_buffer = local_rendering_device->storage_buffer_create(vertex_buffer_byte_count);

	Ref<RDUniform> uniform_normal{}; 
	uniform_normal.instantiate();
	uniform_normal->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	uniform_normal->set_binding(2);
	uniform_normal->add_id(normal_buffer);
	uniforms.push_back(uniform_normal);

	colour_buffer_byte_count = max_vertex_count * sizeof(float) * 4;
	colour_buffer = local_rendering_device->storage_buffer_create(colour_buffer_byte_count);

	Ref<RDUniform> uniform_colour{}; 
	uniform_colour.instantiate();
	uniform_colour->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	uniform_colour->set_binding(3);
	uniform_colour->add_id(colour_buffer);
	uniforms.push_back(uniform_colour);

	count_buffer = local_rendering_device->storage_buffer_create(sizeof(uint32_t));

	Ref<RDUniform> uniform_count{}; 
	uniform_count.instantiate();
	uniform_count->set_uniform_type(RenderingDevice::UNIFORM_TYPE_STORAGE_BUFFER);
	uniform_count->set_binding(4);
	uniform_count->add_id(count_buffer);
	uniforms.push_back(uniform_count);

	int shader_set = 0;
	uniform_set = local_rendering_device->uniform_set_create(uniforms, shader, shader_set);

	pipeline = local_rendering_device->compute_pipeline_create(shader);

	return true;
}

void TerrainClass::_update_chunk_mesh(Ref<ArrayMesh> array_mesh, Vector3i chunk_pos)
{
	if (rendering_thread_id == -1)
	{
		print_error("make_mesh() called before init()!");
		return;
	}

	if (rendering_thread_id != OS::get_singleton()->get_thread_caller_id())
	{
		print_error("Thread id missmatch");
		return;
	}

	if (!shader.is_valid())
	{
		print_error("Shader not initialized!");
		return;
	}

	if (!local_rendering_device)
	{
		print_error("Not initalised");
		return;
	}

	PackedFloat32Array points = chunk_generator->generate_points(chunk_pos);

	PackedByteArray points_byte_array = points.to_byte_array();

	// update the points buffer
	local_rendering_device->buffer_update(points_buffer, 0, points_byte_array.size(), points_byte_array);

	// clear the previous buffers
	local_rendering_device->buffer_clear(vertex_buffer, 0, vertex_buffer_byte_count);
	local_rendering_device->buffer_clear(normal_buffer, 0, vertex_buffer_byte_count);
	local_rendering_device->buffer_clear(colour_buffer, 0, colour_buffer_byte_count);
	local_rendering_device->buffer_clear(count_buffer, 0, sizeof(uint32_t));

	// begin a list of instructions for our GPU to execute
	int64_t compute_list_id = local_rendering_device->compute_list_begin();

	// bind our compute shader to the list
	local_rendering_device->compute_list_bind_compute_pipeline(compute_list_id, pipeline);
	// bind our buffer uniform to our pipeline
	local_rendering_device->compute_list_bind_uniform_set(compute_list_id, uniform_set, 0);
	// specify how many workgroups to use
	constexpr uint32_t group_size = CHUNK_SIZE / 8;
	local_rendering_device->compute_list_dispatch(compute_list_id, group_size, group_size, group_size);
	// end the list of instructions
	local_rendering_device->compute_list_end();

	// Submit to GPU and wait for sync
	local_rendering_device->submit();

	// calling sync makes the execution pause, ideally this is done on a thread
	local_rendering_device->sync();

	PackedByteArray count_bytes = local_rendering_device->buffer_get_data(count_buffer);
	uint32_t vertex_count = *reinterpret_cast<const uint32_t*>(count_bytes.ptr());

	if (vertex_count > 0)
	{
		Array mesh_arrays{};
		mesh_arrays.resize(Mesh::ARRAY_MAX);

		PackedByteArray vertex_data = local_rendering_device->buffer_get_data(vertex_buffer, 0, vertex_count * sizeof(float) * 3);
		PackedVector3Array vertices{};
		vertices.resize(vertex_count);
		memcpy(vertices.ptrw(), vertex_data.ptr(), vertex_data.size());
		mesh_arrays[Mesh::ARRAY_VERTEX] = vertices;

		PackedByteArray normal_data = local_rendering_device->buffer_get_data(normal_buffer, 0, vertex_count * sizeof(float) * 3);
		PackedVector3Array normals{};
		normals.resize(vertex_count);
		memcpy(normals.ptrw(), normal_data.ptr(), normal_data.size());
		mesh_arrays[Mesh::ARRAY_NORMAL] = normals;

		//PackedColorArray colour_data = local_rendering_device->buffer_get_data(colour_buffer, 0, vertex_count * sizeof(float) * 4);
		//PackedVector3Array colours{};
		//colours.resize(vertex_count);
		//memcpy(colours.ptrw(), colour_data.ptr(), colour_data.size());
		//mesh_arrays[Mesh::ARRAY_COLOR] = colours;

		call_deferred("_apply_mesh_data", array_mesh, mesh_arrays);
	}
}

void TerrainClass::_apply_mesh_data(Ref<ArrayMesh> array_mesh, Array mesh_arrays)
{
	if (!array_mesh.is_valid())
	{
		return;
	}

	if (array_mesh->get_surface_count() > 0)
	{
		array_mesh->clear_surfaces();
	}

	array_mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, mesh_arrays);
}