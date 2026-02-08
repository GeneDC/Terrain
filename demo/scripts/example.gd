extends Node

@export var terrain := TerrainClass.new()

var chunk_positions: Array[Vector3i] = []

func _ready() -> void:
	call_deferred("init")

func init() -> void:
	terrain.init()
	chunk_positions = generate_chunk_positions(10)

func generate_chunk_positions(radius: int) -> Array[Vector3i]:
	var positions: Array[Vector3i] = []
	for x in range(-radius, radius + 1):
		for y in range(-5, 5 + 1):
			for z in range(-radius, radius + 1):
				positions.append(Vector3i(x, y, z))
	
	return positions
	
func _process(_delta_time: float) -> void:
	for i in range(1):
		if not chunk_positions.is_empty():
			spawn_chunk(chunk_positions.pop_back())

func spawn_chunk(chunk_pos: Vector3i) -> void:
	var mesh_data := ArrayMesh.new()
	
	var mesh_instance := MeshInstance3D.new()
	mesh_instance.mesh = mesh_data
	mesh_instance.position = chunk_pos * 64
	mesh_instance.name = "Chunk_%d_%d_%d" % [chunk_pos.x, chunk_pos.y, chunk_pos.z]
	add_child(mesh_instance)
	
	terrain.update_chunk_mesh(mesh_data, chunk_pos)
