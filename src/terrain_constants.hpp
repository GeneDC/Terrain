#pragma once

#ifndef TERRAIN_CONSTANTS_HPP
#define TERRAIN_CONSTANTS_HPP

namespace terrain_constants
{
	inline constexpr int CHUNK_SIZE = 64;
	static_assert((CHUNK_SIZE & 7) == 0, "chunk size must be a multiple of 8");
	inline constexpr int CHUNK_AREA = CHUNK_SIZE * CHUNK_SIZE;
	inline constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

	inline constexpr int POINTS_SIZE = CHUNK_SIZE + 1;
	inline constexpr int POINTS_AREA = POINTS_SIZE * POINTS_SIZE;
	inline constexpr int POINTS_VOLUME = POINTS_SIZE * POINTS_SIZE * POINTS_SIZE;
}

#endif