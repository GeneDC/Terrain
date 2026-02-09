extends Node

var chunk_loader : ChunkLoader
var chunk_positions: Array[Vector3i] = []

func _ready() -> void:
	call_deferred("init")

func _exit_tree():
	chunk_loader.stop()
	
func init() -> void:
	chunk_loader = get_node("ChunkLoader")
	chunk_loader.init()
	generate_chunks(20)
	#chunk_positions = [Vector3.ZERO]

func generate_chunks(radius: int) -> void:
	for x in range(-radius, radius + 1):
		for y in range(-5, 5 + 1):
			for z in range(-radius, radius + 1):
				spawn_chunk(Vector3i(x, y, z))
	
func _process(_delta_time: float) -> void:
	chunk_loader.update()

func spawn_chunk(chunk_pos: Vector3i) -> void:
	chunk_loader.queue_chunk_update(chunk_pos)
