const vec4 normalized_cube_coordinates[8] = {
  {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f},
  { 1.0f,  1.0f, 1.0f, 1.0f}, {0.0f,  1.0f, 1.0f, 1.0f},
  {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f,  0.0f, 1.0f},
  { 1.0f,  1.0f, 0.0f, 1.0f}, {0.0f,  1.0f,  0.0f, 1.0f}
};

const uint8_t normalized_cube_indices_triangles[36] = {
	// front
	uint8_t(0), uint8_t(1), uint8_t(2),
	uint8_t(2), uint8_t(3), uint8_t(0),
	// right
	uint8_t(1), uint8_t(5), uint8_t(6),
	uint8_t(6), uint8_t(2), uint8_t(1),
	// back
	uint8_t(7), uint8_t(6), uint8_t(5),
	uint8_t(5), uint8_t(4), uint8_t(7),
	// left
	uint8_t(4), uint8_t(0), uint8_t(3),
	uint8_t(3), uint8_t(7), uint8_t(4),
	// bottom
	uint8_t(4), uint8_t(5), uint8_t(1),
	uint8_t(1), uint8_t(0), uint8_t(4),
	// top
	uint8_t(3), uint8_t(2), uint8_t(6),
	uint8_t(6), uint8_t(7), uint8_t(3)
};

uint8_t normalized_cube_indices_lines[24] = {
	uint8_t(0), uint8_t(1),
	uint8_t(0), uint8_t(3),
	uint8_t(0), uint8_t(4),
	uint8_t(1), uint8_t(2),
	uint8_t(1), uint8_t(5),
	uint8_t(2), uint8_t(3),
	uint8_t(2), uint8_t(6),
	uint8_t(3), uint8_t(7),
	uint8_t(4), uint8_t(5),
	uint8_t(4), uint8_t(7),
	uint8_t(5), uint8_t(6),
	uint8_t(6), uint8_t(7)
};