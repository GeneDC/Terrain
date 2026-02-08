#[compute]
#version 450

#include "MarchTables.glsl"

// Invocations in the (x, y, z) dimension. Max 1024! (32x32x1 or 10x10x10)
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

// A binding to the buffer we create in our script
layout(set = 0, binding = 0, std430) restrict readonly buffer PointsBuffer {
	float points[];
}
points_buffer;

layout(set = 0, binding = 1, std430) restrict writeonly buffer VertexBuffer {
	float verts[];
}
vertex_buffer;

layout(set = 0, binding = 2, std430) restrict writeonly buffer NormalBuffer {
	float normals[];
}
normal_buffer;

layout(set = 0, binding = 3, std430) restrict writeonly buffer ColourBuffer {
	vec4 colours[];
}
colour_buffer;

layout(set = 0, binding = 4, std430) restrict buffer AtomicBuffer {
	uint count;
}
count_buffer;

const float isoLevel = 0.5;
const uint numCubesPerAxis = 64;
const uint numPointsPerAxis = numCubesPerAxis + 1;

vec4 InterpolateVerts(vec4 v1, vec4 v2)
{
	float t = (isoLevel - v1.w) / (v2.w - v1.w);
	return vec4(v1.xyz + t * (v2.xyz - v1.xyz), 0);
}

void AddTri(vec4 vert_a, vec4 vert_b, vec4 vert_c)
{
	uint index = atomicAdd(count_buffer.count, 3);
	uint vert_index = index * 3;

	// Vertex A
	vertex_buffer.verts[vert_index + 0] = vert_a.x;
	vertex_buffer.verts[vert_index + 1] = vert_a.y;
	vertex_buffer.verts[vert_index + 2] = vert_a.z;
	// Vertex B
	vertex_buffer.verts[vert_index + 3] = vert_b.x;
	vertex_buffer.verts[vert_index + 4] = vert_b.y;
	vertex_buffer.verts[vert_index + 5] = vert_b.z;
	// Vertex C
	vertex_buffer.verts[vert_index + 6] = vert_c.x;
	vertex_buffer.verts[vert_index + 7] = vert_c.y;
	vertex_buffer.verts[vert_index + 8] = vert_c.z;

	vec3 vert_a_v3 = vert_a.xyz;
	vec3 vert_b_v3 = vert_b.xyz;
	vec3 vert_c_v3 = vert_c.xyz;
	vec4 cross_norm = vec4(normalize(cross(vert_b_v3 - vert_a_v3, vert_c_v3 - vert_a_v3)), 0);
	// Normal A
	normal_buffer.normals[vert_index + 0] = cross_norm.x;
	normal_buffer.normals[vert_index + 1] = cross_norm.y;
	normal_buffer.normals[vert_index + 2] = cross_norm.z;
	// Normal B
	normal_buffer.normals[vert_index + 3] = cross_norm.x;
	normal_buffer.normals[vert_index + 4] = cross_norm.y;
	normal_buffer.normals[vert_index + 5] = cross_norm.z;
	// Normal C
	normal_buffer.normals[vert_index + 6] = cross_norm.x;
	normal_buffer.normals[vert_index + 7] = cross_norm.y;
	normal_buffer.normals[vert_index + 8] = cross_norm.z;

	vec4 terrain_colour = vec4(0.2, 0.8, 0.2, 1.0); // Green
	if (vert_a.y > 10.0) terrain_colour = vec4(0.9, 0.9, 0.9, 1.0); // Snow

	colour_buffer.colours[index] = terrain_colour;
	colour_buffer.colours[index + 1] = terrain_colour;
	colour_buffer.colours[index + 2] = terrain_colour;
}


shared float points_cache[9 * 9 * 9];

int get_cache_idx(uvec3 p) {
	return int(p.x + (p.y * 9u) + (p.z * 81u));
}

float Point(uint x, uint y, uint z)
{
	uint index = get_cache_idx(uvec3(x, y, z));
	return points_cache[index];
}

uint get_density_index(uvec3 pos)
{
	return pos.x + pos.y * numPointsPerAxis + pos.z * numPointsPerAxis * numPointsPerAxis;
}

float get_density(uvec3 pos)
{
	uint index = get_density_index(pos);
	return points_buffer.points[index];
}

void main()
{
	uvec3 local_id = gl_LocalInvocationID;
	uvec3 global_id = gl_GlobalInvocationID;
	if (global_id.x >= numCubesPerAxis || global_id.y >= numCubesPerAxis || global_id.z >= numCubesPerAxis) return;

	points_cache[get_cache_idx(local_id)] = get_density(global_id);

	bool edge_x = (local_id.x == 7u);
	bool edge_y = (local_id.y == 7u);
	bool edge_z = (local_id.z == 7u);

	if (edge_x) points_cache[get_cache_idx(local_id + uvec3(1,0,0))] = get_density(global_id + uvec3(1,0,0));
	if (edge_y) points_cache[get_cache_idx(local_id + uvec3(0,1,0))] = get_density(global_id + uvec3(0,1,0));
	if (edge_z) points_cache[get_cache_idx(local_id + uvec3(0,0,1))] = get_density(global_id + uvec3(0,0,1));
	if (edge_x && edge_y) points_cache[get_cache_idx(local_id + uvec3(1,1,0))] = get_density(global_id + uvec3(1,1,0));
	if (edge_y && edge_z) points_cache[get_cache_idx(local_id + uvec3(0,1,1))] = get_density(global_id + uvec3(0,1,1));
	if (edge_x && edge_z) points_cache[get_cache_idx(local_id + uvec3(1,0,1))] = get_density(global_id + uvec3(1,0,1));
	if (edge_x && edge_y && edge_z) points_cache[get_cache_idx(uvec3(8,8,8))] = get_density(global_id + uvec3(1,1,1));

	// Wait for all 512 threads to finish the loading dance
	memoryBarrierShared();
	barrier();

	uvec3 id = local_id;
	vec4 cubeCorners[8] = 
	{
		vec4(0, 0, 0, Point(id.x,		id.y,		id.z	)),
		vec4(1, 0, 0, Point(id.x + 1,	id.y,		id.z	)),
		vec4(1, 0, 1, Point(id.x + 1,	id.y,		id.z + 1)),
		vec4(0, 0, 1, Point(id.x,		id.y,		id.z + 1)),
		vec4(0, 1, 0, Point(id.x,		id.y + 1,	id.z	)),
		vec4(1, 1, 0, Point(id.x + 1,	id.y + 1,	id.z	)),
		vec4(1, 1, 1, Point(id.x + 1,	id.y + 1,	id.z + 1)),
		vec4(0, 1, 1, Point(id.x,		id.y + 1,	id.z + 1)),
	};

	uint cubeIndex = 0;
	if (cubeCorners[0].w < isoLevel) cubeIndex |= 1;
	if (cubeCorners[1].w < isoLevel) cubeIndex |= 2;
	if (cubeCorners[2].w < isoLevel) cubeIndex |= 4;
	if (cubeCorners[3].w < isoLevel) cubeIndex |= 8;
	if (cubeCorners[4].w < isoLevel) cubeIndex |= 16;
	if (cubeCorners[5].w < isoLevel) cubeIndex |= 32;
	if (cubeCorners[6].w < isoLevel) cubeIndex |= 64;
	if (cubeCorners[7].w < isoLevel) cubeIndex |= 128;

	int edge_flags = edges[cubeIndex];
	if (edge_flags == 0)
		return;
	
	vec4 edgePoints[12] = 
	{
		vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0),
		vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0),
		vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0), vec4(0, 0, 0, 0)
	};

	// Find the vertices where the surface intersects the cube
	if ((edge_flags & 1) == 1)
		edgePoints[0] = InterpolateVerts(cubeCorners[0], cubeCorners[1]);
	if ((edge_flags & 2) == 2)
		edgePoints[1] = InterpolateVerts(cubeCorners[1], cubeCorners[2]);
	if ((edge_flags & 4) == 4)
		edgePoints[2] = InterpolateVerts(cubeCorners[2], cubeCorners[3]);
	if ((edge_flags & 8) == 8)
		edgePoints[3] = InterpolateVerts(cubeCorners[3], cubeCorners[0]);
	if ((edge_flags & 16) == 16)
		edgePoints[4] = InterpolateVerts(cubeCorners[4], cubeCorners[5]);
	if ((edge_flags & 32) == 32)
		edgePoints[5] = InterpolateVerts(cubeCorners[5], cubeCorners[6]);
	if ((edge_flags & 64) == 64)
		edgePoints[6] = InterpolateVerts(cubeCorners[6], cubeCorners[7]);
	if ((edge_flags & 128) == 128)
		edgePoints[7] = InterpolateVerts(cubeCorners[7], cubeCorners[4]);
	if ((edge_flags & 256) == 256)
		edgePoints[8] = InterpolateVerts(cubeCorners[0], cubeCorners[4]);
	if ((edge_flags & 512) == 512)
		edgePoints[9] = InterpolateVerts(cubeCorners[1], cubeCorners[5]);
	if ((edge_flags & 1024) == 1024)
		edgePoints[10] = InterpolateVerts(cubeCorners[2], cubeCorners[6]);
	if ((edge_flags & 2048) == 2048)
		edgePoints[11] = InterpolateVerts(cubeCorners[3], cubeCorners[7]);

	vec4 pos = vec4(gl_GlobalInvocationID.xyz, 0);
	const int tris[16] = triangulation[cubeIndex];
	for (uint i = 0; tris[i] != -1; i += 3)
	{
		vec4 tri_vert_a = pos + edgePoints[tris[i]];
		vec4 tri_vert_b = pos + edgePoints[tris[i + 1]];
		vec4 tri_vert_c = pos + edgePoints[tris[i + 2]];
		
		AddTri(tri_vert_a, tri_vert_b, tri_vert_c);
	}
}
