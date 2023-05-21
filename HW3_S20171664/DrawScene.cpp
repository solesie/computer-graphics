//
//  DrawScene.cpp
//
//  Written for CSE4170
//  Department of Computer Science and Engineering
//  Copyright © 2023 Sogang University. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include "LoadScene.h"

// Begin of shader setup
#include "Shaders/LoadShaders.h"
#include "ShadingInfo.h"

extern SCENE scene;

// for simple shaders
GLuint h_ShaderProgram_simple; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// for PBR
GLint loc_texture;
GLuint h_ShaderProgram_TXPBR;
#define NUMBER_OF_LIGHT_SUPPORTED 13
GLint loc_global_ambient_color;
GLint loc_lightCount;
loc_light_Parameters loc_light[NUMBER_OF_LIGHT_SUPPORTED];
loc_Material_Parameters loc_material;
GLint loc_ModelViewProjectionMatrix_TXPBR, loc_ModelViewMatrix_TXPBR, loc_ModelViewMatrixInvTrans_TXPBR;
GLint loc_cameraPos;

#define TEXTURE_INDEX_DIFFUSE	(0)
#define TEXTURE_INDEX_NORMAL	(1)
#define TEXTURE_INDEX_SPECULAR	(2)
#define TEXTURE_INDEX_EMISSIVE	(3)
#define TEXTURE_INDEX_SKYMAP	(4)

// for skybox shaders
GLuint h_ShaderProgram_skybox;
GLint loc_cubemap_skybox;
GLint loc_ModelViewProjectionMatrix_SKY;

// include glm/*.hpp only if necessary
// #include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, lookAt, perspective, etc.
// ViewProjectionMatrix = ProjectionMatrix * ViewMatrix
glm::mat4 ViewProjectionMatrix, ViewMatrix, ProjectionMatrix;
// ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix
glm::mat4 ModelViewProjectionMatrix; // This one is sent to vertex shader when ready.
glm::mat4 ModelViewMatrix;
glm::mat3 ModelViewMatrixInvTrans;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define PI 3.141592654f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0
#define LOC_NORMAL 1
#define LOC_TEXCOORD 2

// texture stuffs
#define N_TEXTURES_USED 2
#define TEXTURE_ID_FLOOR 0
#define TEXTURE_ID_TIGER 1
GLuint texture_names[N_TEXTURES_USED];

// tiger object
#define N_TIGER_FRAMES 12
int cur_frame_tiger = 0, cur_frame_ben = 0, cur_frame_spider = 0;
GLuint tiger_VBO, tiger_VAO;
int tiger_n_triangles[N_TIGER_FRAMES];
int tiger_vertex_offset[N_TIGER_FRAMES];
GLfloat* tiger_vertices[N_TIGER_FRAMES];

Material_Parameters material_tiger;

// ben object
#define N_BEN_FRAMES 30
GLuint ben_VBO, ben_VAO;
int ben_n_triangles[N_BEN_FRAMES];
int ben_vertex_offset[N_BEN_FRAMES];
GLfloat* ben_vertices[N_BEN_FRAMES];

Material_Parameters material_ben;

// spider object
#define N_SPIDER_FRAMES 16
GLuint spider_VBO, spider_VAO;
int spider_n_triangles[N_SPIDER_FRAMES];
int spider_vertex_offset[N_SPIDER_FRAMES];
GLfloat* spider_vertices[N_SPIDER_FRAMES];

Material_Parameters material_spider;

// ironman object
GLuint ironman_VBO, ironman_VAO;
int ironman_n_triangles;
GLfloat* ironman_vertices;

Material_Parameters material_ironman;

// optimus object
GLuint optimus_VBO, optimus_VAO;
int optimus_n_triangles;
GLfloat* optimus_vertices;

Material_Parameters material_optimus;

// dragon object
GLuint dragon_VBO, dragon_VAO;
int dragon_n_triangles;
GLfloat* dragon_vertices;

Material_Parameters material_dragon;

//세상 이동 카메라
bool is_world_moving_camera = false;
#define CAM_MOVING_SPEED 70.0
#define CAM_ROTATING_SPEED 3.0

//호랑이눈카메라
bool is_tiger_related_camera = false;

//호랑이관찰카메라
bool is_tiger_observe_camera = false;

unsigned int _timestamp_scene = 0;
unsigned int tiger_timestamp_scene = 0;
int animation_mode = 1;
void timer_scene(int value) {
	_timestamp_scene %= UINT_MAX;
	tiger_timestamp_scene %= UINT_MAX;
	cur_frame_tiger = tiger_timestamp_scene % N_TIGER_FRAMES;
	cur_frame_ben = _timestamp_scene % N_BEN_FRAMES;
	cur_frame_spider = _timestamp_scene % N_SPIDER_FRAMES;
	_timestamp_scene++;
	glutPostRedisplay();
	if (animation_mode) {
		tiger_timestamp_scene++;
	}
	glutTimerFunc(100, timer_scene, 0);
}

void My_glTexImage2D_from_file(char* filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap, * tx_pixmap_32;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	if (tx_bits_per_pixel == 32)
		tx_pixmap_32 = tx_pixmap;
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap_32 = FreeImage_ConvertTo32Bits(tx_pixmap);
	}

	width = FreeImage_GetWidth(tx_pixmap_32);
	height = FreeImage_GetHeight(tx_pixmap_32);
	data = FreeImage_GetBits(tx_pixmap_32);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap_32);
	if (tx_bits_per_pixel != 32)
		FreeImage_Unload(tx_pixmap);
}

int read_geometry(GLfloat** object, int bytes_per_primitive, char* filename) {
	int n_triangles;
	FILE* fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open the object file %s ...", filename);
		return -1;
	}
	fread(&n_triangles, sizeof(int), 1, fp);

	*object = (float*)malloc(n_triangles * bytes_per_primitive);
	if (*object == NULL) {
		fprintf(stderr, "Cannot allocate memory for the geometry file %s ...", filename);
		return -1;
	}

	fread(*object, bytes_per_primitive, n_triangles, fp);
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);

	return n_triangles;
}

/*********************************  START: camera *********************************/
typedef enum {
	CAMERA_1,
	CAMERA_2,
	CAMERA_3,
	CAMERA_4,
	WORLD_MOVING_CAMERA,
	TIGER_EYE_CAMERA,
	TIGER_OBSERVE_CAMERA,
	NUM_CAMERAS
} CAMERA_INDEX;

typedef struct _Camera {
	float pos[3];
	float uaxis[3], vaxis[3], naxis[3];
	float fovy, aspect_ratio, near_c, far_c;
	int move, rotation_axis;
} Camera;

Camera camera_info[NUM_CAMERAS];
Camera* current_camera;

using glm::mat4;
void set_ViewMatrix_from_camera_frame(void) {
	ViewMatrix = glm::mat4(current_camera->uaxis[0], current_camera->vaxis[0], current_camera->naxis[0], 0.0f,
		current_camera->uaxis[1], current_camera->vaxis[1], current_camera->naxis[1], 0.0f,
		current_camera->uaxis[2], current_camera->vaxis[2], current_camera->naxis[2], 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	ViewMatrix = glm::translate(ViewMatrix, glm::vec3(-current_camera->pos[0], -current_camera->pos[1], -current_camera->pos[2]));
}

void update_vp_matrix() {
	set_ViewMatrix_from_camera_frame();
	ProjectionMatrix = glm::perspective(current_camera->fovy, current_camera->aspect_ratio, current_camera->near_c, current_camera->far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
}

void set_current_camera(int camera_num) {
	current_camera = &camera_info[camera_num];
	update_vp_matrix();
}

typedef enum {
	U_MOVING,
	V_MOVING,
	N_MOVING
} CAMERA_MOVING;
void move_world_moving_camera(int dist, CAMERA_MOVING moving_direction) {
	if (!is_world_moving_camera) {
		return;
	}
	switch (moving_direction) {
	case U_MOVING:
		current_camera->pos[0] += dist * (current_camera->uaxis[0]);
		current_camera->pos[1] += dist * (current_camera->uaxis[1]);
		current_camera->pos[2] += dist * (current_camera->uaxis[2]);
		break;
	case V_MOVING:
		current_camera->pos[0] += dist * (current_camera->vaxis[0]);
		current_camera->pos[1] += dist * (current_camera->vaxis[1]);
		current_camera->pos[2] += dist * (current_camera->vaxis[2]);
		break;
	case N_MOVING:
		current_camera->pos[0] += dist * (current_camera->naxis[0]);
		current_camera->pos[1] += dist * (current_camera->naxis[1]);
		current_camera->pos[2] += dist * (current_camera->naxis[2]);
		break;
	}
	update_vp_matrix();
}

typedef enum {
	U_AXIS,
	V_AXIS,
	N_AXIS
} CAMERA_AXIS;
void rotate_world_moving_camera(int angle, CAMERA_AXIS rotating_axis) {
	if (!is_world_moving_camera) {
		return;
	}
	glm::mat3 RotationMatrix;
	glm::vec3 direction;

	switch (rotating_axis) {
	case U_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), TO_RADIAN * angle,
			glm::vec3(current_camera->uaxis[0], current_camera->uaxis[1], current_camera->uaxis[2])));

		direction = RotationMatrix * glm::vec3(current_camera->vaxis[0], current_camera->vaxis[1], current_camera->vaxis[2]);
		current_camera->vaxis[0] = direction.x; 
		current_camera->vaxis[1] = direction.y; 
		current_camera->vaxis[2] = direction.z;
		direction = RotationMatrix * glm::vec3(current_camera->naxis[0], current_camera->naxis[1], current_camera->naxis[2]);
		current_camera->naxis[0] = direction.x; 
		current_camera->naxis[1] = direction.y; 
		current_camera->naxis[2] = direction.z;
		break;
	case V_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), TO_RADIAN * angle,
			glm::vec3(current_camera->vaxis[0], current_camera->vaxis[1], current_camera->vaxis[2])));

		direction = RotationMatrix * glm::vec3(current_camera->uaxis[0], current_camera->uaxis[1], current_camera->uaxis[2]);
		current_camera->uaxis[0] = direction.x;
		current_camera->uaxis[1] = direction.y; 
		current_camera->uaxis[2] = direction.z;
		direction = RotationMatrix * glm::vec3(current_camera->naxis[0], current_camera->naxis[1], current_camera->naxis[2]);
		current_camera->naxis[0] = direction.x; 
		current_camera->naxis[1] = direction.y;
		current_camera->naxis[2] = direction.z;
		break;
	case N_AXIS:
		RotationMatrix = glm::mat3(glm::rotate(glm::mat4(1.0), TO_RADIAN * angle,
			glm::vec3(current_camera->naxis[0], current_camera->naxis[1], current_camera->naxis[2])));

		direction = RotationMatrix * glm::vec3(current_camera->vaxis[0], current_camera->vaxis[1], current_camera->vaxis[2]);
		current_camera->vaxis[0] = direction.x; current_camera->vaxis[1] = direction.y; current_camera->vaxis[2] = direction.z;
		direction = RotationMatrix * glm::vec3(current_camera->uaxis[0], current_camera->uaxis[1], current_camera->uaxis[2]);
		current_camera->uaxis[0] = direction.x; current_camera->uaxis[1] = direction.y; current_camera->uaxis[2] = direction.z;
		break;
	}
	update_vp_matrix();
}

void initialize_camera(void) {
	//CAMERA_1
	Camera* pCamera = &camera_info[CAMERA_1];
	pCamera = &camera_info[CAMERA_1];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = -1187.0f; pCamera->pos[2] = 622.0f;
	pCamera->uaxis[0] = 1.0f; pCamera->uaxis[1] = 0.0f; pCamera->uaxis[2] = 0.0f;
	pCamera->vaxis[0] = -0.011190f; pCamera->vaxis[1] = -0.034529f; pCamera->vaxis[2] = 0.999328f;
	pCamera->naxis[0] = -0.032200f; pCamera->naxis[1] = -0.998855f; pCamera->naxis[2] = -0.034872f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	//CAMERA_2
	pCamera = &camera_info[CAMERA_2];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = -3657.0f; pCamera->pos[2] = 1591.0f;
	pCamera->uaxis[0] = 0.999733f; pCamera->uaxis[1] = -0.009260f; pCamera->uaxis[2] = -0.020767f;
	pCamera->vaxis[0] = 0.022322f; pCamera->vaxis[1] = 0.788382f; pCamera->vaxis[2] = 0.614751f;
	pCamera->naxis[0] = -0.009271f; pCamera->naxis[1] = -0.667971f; pCamera->naxis[2] = 0.744097f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	//CAMERA_3
	pCamera = &camera_info[CAMERA_3];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = -6555.770996f; pCamera->pos[2] = 166.988892f;
	pCamera->uaxis[0] = 1.0f; pCamera->uaxis[1] = 0.0f; pCamera->uaxis[2] = 0.0f;
	pCamera->vaxis[0] = 0.0f; pCamera->vaxis[1] = 0.0f; pCamera->vaxis[2] = 1.0f;
	pCamera->naxis[0] = 0.0f; pCamera->naxis[1] = -1.0f; pCamera->naxis[2] = 0.0f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	//CAMERA_4 : top view
	pCamera = &camera_info[CAMERA_4];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = 6364.564453f; pCamera->pos[2] = 3915.244629f;
	pCamera->uaxis[0] = -0.997364f; pCamera->uaxis[1] = -0.065447f; pCamera->uaxis[2] = -0.030853f;
	pCamera->vaxis[0] = -0.005446f; pCamera->vaxis[1] = -0.386193f; pCamera->vaxis[2] = 0.922377f;
	pCamera->naxis[0] = -0.071890f; pCamera->naxis[1] = 0.944558f; pCamera->naxis[2] = 0.320276f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	//WORLD_MOVING_CAMERA
	pCamera = &camera_info[WORLD_MOVING_CAMERA];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = -3528.364502f; pCamera->pos[2] = 958.963989f;
	pCamera->uaxis[0] = 0.999360f; pCamera->uaxis[1] = 0.032412f; pCamera->uaxis[2] = 0.014561f;
	pCamera->vaxis[0] = -0.022085f; pCamera->vaxis[1] = 0.325728f; pCamera->vaxis[2] = 0.945191f;
	pCamera->naxis[0] = 0.027511f; pCamera->naxis[1] = -0.965124f; pCamera->naxis[2] = 0.260263f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;
	
	//TIGER_EYE_CAMERA
	pCamera = &camera_info[TIGER_EYE_CAMERA];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = 0.0f; pCamera->pos[2] = 0.0f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	//TIGER_OBSERVE_CAMERA
	pCamera = &camera_info[TIGER_OBSERVE_CAMERA];
	pCamera->pos[0] = 0.0f; pCamera->pos[1] = 0.0f; pCamera->pos[2] = 0.0f;
	pCamera->move = 0;
	pCamera->fovy = TO_RADIAN * scene.camera.fovy, pCamera->aspect_ratio = scene.camera.aspect, pCamera->near_c = 0.1f; pCamera->far_c = 50000.0f;

	set_current_camera(CAMERA_1);
}
/*********************************  END: camera *********************************/

/******************************  START: shader setup ****************************/
// Begin of Callback function definitions
void prepare_shader_program(void) {
	char string[256];

	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram_simple = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram_simple);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram_simple, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram_simple, "u_primitive_color");

	ShaderInfo shader_info_TXPBR[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Background/PBR_Tx.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Background/PBR_Tx.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram_TXPBR = LoadShaders(shader_info_TXPBR);
	glUseProgram(h_ShaderProgram_TXPBR);

	loc_ModelViewProjectionMatrix_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans_TXPBR = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_ModelViewMatrixInvTrans");

	//loc_global_ambient_color = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_global_ambient_color");

	loc_lightCount = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_light_count");

	for (int i = 0; i < NUMBER_OF_LIGHT_SUPPORTED; i++) {
		sprintf(string, "u_light[%d].position", i);
		loc_light[i].position = glGetUniformLocation(h_ShaderProgram_TXPBR, string);
		sprintf(string, "u_light[%d].color", i);
		loc_light[i].color = glGetUniformLocation(h_ShaderProgram_TXPBR, string);
	}

	loc_cameraPos = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_camPos");

	//Textures
	loc_material.diffuseTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_albedoMap");
	loc_material.normalTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_normalMap");
	loc_material.specularTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_metallicRoughnessMap");
	loc_material.emissiveTex = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_emissiveMap");

	loc_texture = glGetUniformLocation(h_ShaderProgram_TXPBR, "u_base_texture");

	ShaderInfo shader_info_skybox[3] = {
		{ GL_VERTEX_SHADER, "Shaders/Background/skybox.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/Background/skybox.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram_skybox = LoadShaders(shader_info_skybox);
	loc_cubemap_skybox = glGetUniformLocation(h_ShaderProgram_skybox, "u_skymap");

	loc_ModelViewProjectionMatrix_SKY = glGetUniformLocation(h_ShaderProgram_skybox, "u_ModelViewProjectionMatrix");
}
/*******************************  END: shder setup ******************************/

/****************************  START: geometry setup ****************************/
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))
#define INDEX_VERTEX_POSITION	0
#define INDEX_NORMAL			1
#define INDEX_TEX_COORD			2

bool b_draw_grid = false;

// axes
GLuint axes_VBO, axes_VAO;
GLfloat axes_vertices[6][3] = {
	{ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }
};
GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } };

void prepare_axes(void) {
	// Initialize vertex buffer object.
	glGenBuffers(1, &axes_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes_vertices), &axes_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &axes_VAO);
	glBindVertexArray(axes_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, axes_VBO);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	fprintf(stdout, " * Loaded axes into graphics memory.\n");
}

void draw_axes(void) {
	if (!b_draw_grid)
		return;

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(8000.0f, 8000.0f, 8000.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(2.0f);
	glBindVertexArray(axes_VAO);
	glUniform3fv(loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
	glLineWidth(1.0f);
	glUseProgram(0);
}

// grid
#define GRID_LENGTH			(100)
#define NUM_GRID_VETICES	((2 * GRID_LENGTH + 1) * 4)
GLuint grid_VBO, grid_VAO;
GLfloat grid_vertices[NUM_GRID_VETICES][3];
GLfloat grid_color[3] = { 0.5f, 0.5f, 0.5f };

void prepare_grid(void) {

	//set grid vertices
	int vertex_idx = 0;
	for (int x_idx = -GRID_LENGTH; x_idx <= GRID_LENGTH; x_idx++)
	{
		grid_vertices[vertex_idx][0] = x_idx;
		grid_vertices[vertex_idx][1] = -GRID_LENGTH;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;

		grid_vertices[vertex_idx][0] = x_idx;
		grid_vertices[vertex_idx][1] = GRID_LENGTH;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;
	}

	for (int y_idx = -GRID_LENGTH; y_idx <= GRID_LENGTH; y_idx++)
	{
		grid_vertices[vertex_idx][0] = -GRID_LENGTH;
		grid_vertices[vertex_idx][1] = y_idx;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;

		grid_vertices[vertex_idx][0] = GRID_LENGTH;
		grid_vertices[vertex_idx][1] = y_idx;
		grid_vertices[vertex_idx][2] = 0.0f;
		vertex_idx++;
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &grid_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, grid_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grid_vertices), &grid_vertices[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &grid_VAO);
	glBindVertexArray(grid_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, grid_VAO);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	fprintf(stdout, " * Loaded grid into graphics memory.\n");
}

void draw_grid(void) {
	if (!b_draw_grid)
		return;

	glUseProgram(h_ShaderProgram_simple);
	ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(100.0f, 100.0f, 100.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glLineWidth(1.0f);
	glBindVertexArray(grid_VAO);
	glUniform3fv(loc_primitive_color, 1, grid_color);
	glDrawArrays(GL_LINES, 0, NUM_GRID_VETICES);
	glBindVertexArray(0);
	glLineWidth(1.0f);
	glUseProgram(0);
}

//sun_temple
GLuint* sun_temple_VBO;
GLuint* sun_temple_VAO;
int* sun_temple_n_triangles;
int* sun_temple_vertex_offset;
GLfloat** sun_temple_vertices;
GLuint* sun_temple_texture_names;

void initialize_lights(void) { // follow OpenGL conventions for initialization
	glUseProgram(h_ShaderProgram_TXPBR);

	glUniform1i(loc_lightCount, scene.n_lights);

	for (int i = 0; i < scene.n_lights; i++) {
		glUniform4f(loc_light[i].position,
			scene.light_list[i].pos[0],
			scene.light_list[i].pos[1],
			scene.light_list[i].pos[2],
			0.0f);
		
		glUniform3f(loc_light[i].color,
			scene.light_list[i].color[0],
			scene.light_list[i].color[1],
			scene.light_list[i].color[2]);
	}

	glUseProgram(0);
}

bool readTexImage2D_from_file(char* filename) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap, * tx_pixmap_32;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	if (tx_pixmap == NULL)
		return false;
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	//fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);
	GLenum format, internalFormat;
	if (tx_bits_per_pixel == 32) {
		format = GL_BGRA;
		internalFormat = GL_RGBA;
	}
	else if (tx_bits_per_pixel == 24) {
		format = GL_BGR;
		internalFormat = GL_RGB;
	}
	else {
		fprintf(stdout, " * Converting texture from %d bits to 32 bits...\n", tx_bits_per_pixel);
		tx_pixmap = FreeImage_ConvertTo32Bits(tx_pixmap);
		format = GL_BGRA;
		internalFormat = GL_RGBA;
	}

	width = FreeImage_GetWidth(tx_pixmap);
	height = FreeImage_GetHeight(tx_pixmap);
	data = FreeImage_GetBits(tx_pixmap);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	//fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap);

	return true;
}

void prepare_sun_temple(void) {
	int n_bytes_per_vertex, n_bytes_per_triangle;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	// VBO, VAO malloc
	sun_temple_VBO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);
	sun_temple_VAO = (GLuint*)malloc(sizeof(GLuint) * scene.n_materials);

	sun_temple_n_triangles = (int*)malloc(sizeof(int) * scene.n_materials);
	sun_temple_vertex_offset = (int*)malloc(sizeof(int) * scene.n_materials);

	// vertices
	sun_temple_vertices = (GLfloat**)malloc(sizeof(GLfloat*) * scene.n_materials);

	for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++) {
		MATERIAL* pMaterial = &(scene.material_list[materialIdx]);
		GEOMETRY_TRIANGULAR_MESH* tm = &(pMaterial->geometry.tm);

		// vertex
		sun_temple_vertices[materialIdx] = (GLfloat*)malloc(sizeof(GLfloat) * 8 * tm->n_triangle * 3);

		int vertexIdx = 0;
		for (int triIdx = 0; triIdx < tm->n_triangle; triIdx++) {
			TRIANGLE tri = tm->triangle_list[triIdx];
			for (int triVertex = 0; triVertex < 3; triVertex++) {
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].x;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].y;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.position[triVertex].z;

				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].x;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].y;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.normal_vetcor[triVertex].z;

				sun_temple_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].u;
				sun_temple_vertices[materialIdx][vertexIdx++] = tri.texture_list[triVertex][0].v;
			}
		}

		// # of triangles
		sun_temple_n_triangles[materialIdx] = tm->n_triangle;

		if (materialIdx == 0)
			sun_temple_vertex_offset[materialIdx] = 0;
		else
			sun_temple_vertex_offset[materialIdx] = sun_temple_vertex_offset[materialIdx - 1] + 3 * sun_temple_n_triangles[materialIdx - 1];

		glGenBuffers(1, &sun_temple_VBO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, sun_temple_VBO[materialIdx]);
		glBufferData(GL_ARRAY_BUFFER, sun_temple_n_triangles[materialIdx] * 3 * n_bytes_per_vertex,
			sun_temple_vertices[materialIdx], GL_STATIC_DRAW);

		// As the geometry data exists now in graphics memory, ...
		free(sun_temple_vertices[materialIdx]);

		// Initialize vertex array object.
		glGenVertexArrays(1, &sun_temple_VAO[materialIdx]);
		glBindVertexArray(sun_temple_VAO[materialIdx]);

		glBindBuffer(GL_ARRAY_BUFFER, sun_temple_VBO[materialIdx]);
		glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(0));
		glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
		glVertexAttribPointer(INDEX_NORMAL, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(3 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_NORMAL);
		glVertexAttribPointer(INDEX_TEX_COORD, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), BUFFER_OFFSET(6 * sizeof(float)));
		glEnableVertexAttribArray(INDEX_TEX_COORD);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		if ((materialIdx > 0) && (materialIdx % 100 == 0))
			fprintf(stdout, " * Loaded %d sun temple materials into graphics memory.\n", materialIdx / 100 * 100);
	}
	fprintf(stdout, " * Loaded %d sun temple materials into graphics memory.\n", scene.n_materials);

	// textures
	sun_temple_texture_names = (GLuint*)malloc(sizeof(GLuint) * scene.n_textures);
	glGenTextures(scene.n_textures, sun_temple_texture_names);

	for (int texId = 0; texId < scene.n_textures; texId++) {
		glBindTexture(GL_TEXTURE_2D, sun_temple_texture_names[texId]);

		bool bReturn = readTexImage2D_from_file(scene.texture_file_name[texId]);

		if (bReturn) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
	fprintf(stdout, " * Loaded sun temple textures into graphics memory.\n");

	free(sun_temple_vertices);
}

void bindTexture(GLint tex, int glTextureId, int texId) {
	if (INVALID_TEX_ID != texId) {
		glActiveTexture(GL_TEXTURE0 + glTextureId);
		glBindTexture(GL_TEXTURE_2D, sun_temple_texture_names[texId]);
		glUniform1i(tex, glTextureId);
	}
}

void draw_sun_temple(void) {
	glUseProgram(h_ShaderProgram_TXPBR);
	ModelViewMatrix = ViewMatrix;
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	ModelViewMatrixInvTrans = glm::transpose(glm::inverse(glm::mat3(ModelViewMatrix)));

	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);

	for (int materialIdx = 0; materialIdx < scene.n_materials; materialIdx++) {
		// set material
		int diffuseTexId = scene.material_list[materialIdx].diffuseTexId;
		int normalMapTexId = scene.material_list[materialIdx].normalMapTexId;
		int specularTexId = scene.material_list[materialIdx].specularTexId;;
		int emissiveTexId = scene.material_list[materialIdx].emissiveTexId;

		bindTexture(loc_material.diffuseTex, TEXTURE_INDEX_DIFFUSE, diffuseTexId);
		bindTexture(loc_material.normalTex, TEXTURE_INDEX_NORMAL, normalMapTexId);
		bindTexture(loc_material.specularTex, TEXTURE_INDEX_SPECULAR, specularTexId);
		bindTexture(loc_material.emissiveTex, TEXTURE_INDEX_EMISSIVE, emissiveTexId);

		glEnable(GL_TEXTURE_2D);

		glBindVertexArray(sun_temple_VAO[materialIdx]);
		glDrawArrays(GL_TRIANGLES, 0, 3 * sun_temple_n_triangles[materialIdx]);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
	}
	glUseProgram(0);
}

// skybox
GLuint skybox_VBO, skybox_VAO;
GLuint skybox_texture_name;

GLfloat cube_vertices[72][3] = {
	// vertices enumerated clockwise
	// 6*2*3 * 2 (POS & NORM)

	// position
	-1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f, //right
	 1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f, //left
	 1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f,

	-1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f, //top
	 1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f, //bottom
	 1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f, //back
	-1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f, //front
	 1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,

	 // normal
	 0.0f, 0.0f, -1.0f,      0.0f, 0.0f, -1.0f,     0.0f, 0.0f, -1.0f,
	 0.0f, 0.0f, -1.0f,      0.0f, 0.0f, -1.0f,     0.0f, 0.0f, -1.0f,

	-1.0f, 0.0f,  0.0f,     -1.0f, 0.0f,  0.0f,    -1.0f, 0.0f,  0.0f,
	-1.0f, 0.0f,  0.0f,     -1.0f, 0.0f,  0.0f,    -1.0f, 0.0f,  0.0f,

	 1.0f, 0.0f,  0.0f,      1.0f, 0.0f,  0.0f,     1.0f, 0.0f,  0.0f,
	 1.0f, 0.0f,  0.0f,      1.0f, 0.0f,  0.0f,     1.0f, 0.0f,  0.0f,

	 0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f,     0.0f, 0.0f, 1.0f,
	 0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f,     0.0f, 0.0f, 1.0f,

	 0.0f, 1.0f, 0.0f,      0.0f, 1.0f, 0.0f,     0.0f, 1.0f, 0.0f,
	 0.0f, 1.0f, 0.0f,      0.0f, 1.0f, 0.0f,     0.0f, 1.0f, 0.0f,

	 0.0f, -1.0f, 0.0f,      0.0f, -1.0f, 0.0f,     0.0f, -1.0f, 0.0f,
	 0.0f, -1.0f, 0.0f,      0.0f, -1.0f, 0.0f,     0.0f, -1.0f, 0.0f
};

void readTexImage2DForCubeMap(const char* filename, GLenum texture_target) {
	FREE_IMAGE_FORMAT tx_file_format;
	int tx_bits_per_pixel;
	FIBITMAP* tx_pixmap;

	int width, height;
	GLvoid* data;

	tx_file_format = FreeImage_GetFileType(filename, 0);
	// assume everything is fine with reading texture from file: no error checking
	tx_pixmap = FreeImage_Load(tx_file_format, filename);
	tx_bits_per_pixel = FreeImage_GetBPP(tx_pixmap);

	//fprintf(stdout, " * A %d-bit texture was read from %s.\n", tx_bits_per_pixel, filename);

	width = FreeImage_GetWidth(tx_pixmap);
	height = FreeImage_GetHeight(tx_pixmap);
	FreeImage_FlipVertical(tx_pixmap);
	data = FreeImage_GetBits(tx_pixmap);

	glTexImage2D(texture_target, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);
	//fprintf(stdout, " * Loaded %dx%d RGBA texture into graphics memory.\n\n", width, height);

	FreeImage_Unload(tx_pixmap);
}

void prepare_skybox(void) { // Draw skybox.
	glGenVertexArrays(1, &skybox_VAO);
	glGenBuffers(1, &skybox_VBO);

	glBindVertexArray(skybox_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_VBO);
	glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(GLfloat), &cube_vertices[0][0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(INDEX_VERTEX_POSITION);
	glVertexAttribPointer(INDEX_VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), BUFFER_OFFSET(0));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glGenTextures(1, &skybox_texture_name);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_name);

	readTexImage2DForCubeMap("Scene/Cubemap/px.png", GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	readTexImage2DForCubeMap("Scene/Cubemap/nx.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
	readTexImage2DForCubeMap("Scene/Cubemap/py.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
	readTexImage2DForCubeMap("Scene/Cubemap/ny.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
	readTexImage2DForCubeMap("Scene/Cubemap/pz.png", GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
	readTexImage2DForCubeMap("Scene/Cubemap/nz.png", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	fprintf(stdout, " * Loaded cube map textures into graphics memory.\n\n");

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void draw_skybox(void) {
	glUseProgram(h_ShaderProgram_skybox);

	glUniform1i(loc_cubemap_skybox, TEXTURE_INDEX_SKYMAP);

	ModelViewMatrix = ViewMatrix * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f);
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(20000, 20000, 20000));
	//ModelViewMatrix = glm::scale(ViewMatrix, glm::vec3(20000.0f, 20000.0f, 20000.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_SKY, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);

	glBindVertexArray(skybox_VAO);
	glActiveTexture(GL_TEXTURE0 + TEXTURE_INDEX_SKYMAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_texture_name);

	glFrontFace(GL_CW);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 2 * 3);
	glBindVertexArray(0);
	glDisable(GL_CULL_FACE);
	glUseProgram(0);
}

void prepare_tiger(void) { // vertices enumerated clockwise
	int i, n_bytes_per_vertex, n_bytes_per_triangle, tiger_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_TIGER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/tiger/Tiger_%d%d_triangles_vnt.geom", i / 10, i % 10);
		tiger_n_triangles[i] = read_geometry(&tiger_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		tiger_n_total_triangles += tiger_n_triangles[i];

		if (i == 0)
			tiger_vertex_offset[i] = 0;
		else
			tiger_vertex_offset[i] = tiger_vertex_offset[i - 1] + 3 * tiger_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &tiger_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glBufferData(GL_ARRAY_BUFFER, tiger_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_TIGER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, tiger_vertex_offset[i] * n_bytes_per_vertex,
			tiger_n_triangles[i] * n_bytes_per_triangle, tiger_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_TIGER_FRAMES; i++)
		free(tiger_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &tiger_VAO);
	glBindVertexArray(tiger_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, tiger_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//호랑이 이동속성 설정
#define TIGER_TOTAL_MOVING_TIME 70600//ms, round-trip
#define TIGER_ROUTE1_TIME 3000//직진
#define TIGER_ROUTE2_TIME 3000//점프
#define TIGER_ROUTE3_TIME 9000//직진
#define TIGER_ROUTE4_TIME 3000//좌우
#define TIGER_ROUTE5_TIME 9000//직진
#define TIGER_ROUTE6_TIME 3000//계단
#define TIGER_ROUTE7_TIME 1800//계단위 직진
#define TIGER_ROUTE8_TIME 9000//계단위 가로

void set_tiger_MVP() {
	float tiger_x, tiger_y, tiger_z;
	unsigned int t = ((long long)tiger_timestamp_scene * 100) % TIGER_TOTAL_MOVING_TIME;
	if (t < TIGER_ROUTE1_TIME) {
		//x=0, z=30, -6500<y<-6000을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, 500.0 / TIGER_ROUTE1_TIME * t - 6500, 30));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE1_TIME) < TIGER_ROUTE2_TIME) {
		//-6000<y<-5600 까지 포물선을 그리며 점프한다.
		tiger_y = 400.0 / TIGER_ROUTE2_TIME * t - 6000;
		tiger_z = -1.0 / 100 * (tiger_y + 5600) * (tiger_y + 6000) + 60;
		float tiger_z_prime = -1.0 / 50 * tiger_y - 116;
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, tiger_y, tiger_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE2_TIME) < TIGER_ROUTE3_TIME) {
		//x=0, z=30, -5600<y<-4100을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, 1500.0 / TIGER_ROUTE3_TIME * t - 5600, 30));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE3_TIME) < TIGER_ROUTE4_TIME) {
		//0<x<500, y=-4100, z=30을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500.0 / TIGER_ROUTE4_TIME * t, -4100, 30));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE4_TIME) < TIGER_ROUTE5_TIME) {
		//x=500, z=30, -4100<y<-2600을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500.0, 1500.0 / TIGER_ROUTE5_TIME * t - 4100, 30));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE5_TIME) < TIGER_ROUTE6_TIME) {
		//x=500, z=27/40*y+1785(-2600<y<-2200)을 따라 걷는다.
		tiger_y = 400.0 / TIGER_ROUTE6_TIME * t - 2600;
		tiger_z = 27.0 / 40 * tiger_y + 1785;
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500.0, tiger_y, tiger_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE6_TIME) < TIGER_ROUTE7_TIME) {
		//x=500, -2200<y<-1900, z=270을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500, 300.0 / TIGER_ROUTE7_TIME * t - 2200, 270));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE7_TIME) < TIGER_ROUTE8_TIME) {
		//-500<x<500, y=-1900, z=270을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-1000.0 / TIGER_ROUTE8_TIME * t + 500, -1900, 270));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	//7~1ROUTES를 되돌아간다.
	else if ((t -= TIGER_ROUTE8_TIME) < TIGER_ROUTE7_TIME) {
		//x=-500, -2200<y<-1900, z=270을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-500, -300.0 / TIGER_ROUTE7_TIME * t - 1900, 270));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE7_TIME) < TIGER_ROUTE6_TIME) {
		//x=-500, z=27/40*y+1785(-2600<y<-2200)을 따라 걷는다.
		tiger_y = -400.0 / TIGER_ROUTE6_TIME * t - 2200;
		tiger_z = 27.0 / 40 * tiger_y + 1785;
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-500.0, tiger_y, tiger_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE6_TIME) < TIGER_ROUTE5_TIME) {
		//x=-500, z=30, -4100<y<-2600을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-500.0, -1500.0 / TIGER_ROUTE5_TIME * t - 2600, 30));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE5_TIME) < TIGER_ROUTE4_TIME) {
		//-500<x<0, y=-4100, z=30을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(500.0 / TIGER_ROUTE4_TIME * t - 500, -4100, 30));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, -90.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE4_TIME) < TIGER_ROUTE3_TIME) {
		//x=0, z=30, -5600<y<-4100을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, -1500.0 / TIGER_ROUTE3_TIME * t - 4100, 30));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if ((t -= TIGER_ROUTE3_TIME) < TIGER_ROUTE2_TIME) {
		//-6000<y<-5600 까지 포물선을 그리며 점프한다.
		tiger_y = -400.0 / TIGER_ROUTE2_TIME * t - 5600;
		tiger_z = -1.0 / 100 * (tiger_y + 5600) * (tiger_y + 6000) + 60;
		float tiger_z_prime = -1.0 / 50 * tiger_y - 116;
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, tiger_y, tiger_z));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
	else if (t -= TIGER_ROUTE2_TIME < TIGER_ROUTE1_TIME) {
		//x=0, z=30, -6500<y<-6000을 따라 걷는다.
		ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, -500.0 / TIGER_ROUTE1_TIME * t - 6000, 30));
		ModelViewMatrix = glm::rotate(ModelViewMatrix, 180.0f * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(2.0f, -2.0f, 2.0f));
		ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	}
}

#define TIGER_EYE_CAMERA_UPDOWN 1200 //ms, round trip
float up_down = 0;
void update_tiger_related_camera() {
	float tiger_eye_x = 0, tiger_eye_y = 88, tiger_eye_z = 62;
	float tiger_observe_x = 0, tiger_observe_y = -150, tiger_observe_z = 200;
	Camera* te_pCamera = &camera_info[TIGER_EYE_CAMERA];
	Camera* to_pCamera = &camera_info[TIGER_OBSERVE_CAMERA];
	unsigned int t = ((long long)tiger_timestamp_scene * 100) % TIGER_TOTAL_MOVING_TIME;
	up_down = 5 * sinf(2 * PI / TIGER_EYE_CAMERA_UPDOWN * (t % TIGER_EYE_CAMERA_UPDOWN));

	if (t < TIGER_ROUTE1_TIME) {
		//x=0, z=30, -6500<y<-6000을 따라 걷는다.
		tiger_eye_y += 500.0 / TIGER_ROUTE1_TIME * t - 6500; 
		tiger_eye_z += 30;
		tiger_observe_y += 500.0 / TIGER_ROUTE1_TIME * t - 6500;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = -1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = -1.0f; to_pCamera->naxis[2] = 0.0f;
	}
	else if ((t -= TIGER_ROUTE1_TIME) < TIGER_ROUTE2_TIME) {
		//-6000<y<-5600 까지 포물선을 그리며 점프한다.
		glm::vec4 tiger_eye_mc(0.0f, tiger_eye_y, tiger_eye_z, 1.0f);
		tiger_eye_y = 400.0 / TIGER_ROUTE2_TIME * t - 6000;
		tiger_eye_z = -1.0 / 100 * (tiger_eye_y + 5600) * (tiger_eye_y + 6000) + 60;
		float tiger_z_prime = -1.0 / 50 * tiger_eye_y - 116;

		glm::mat4 tiger_eye_mat = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, tiger_eye_y, tiger_eye_z));
		tiger_eye_mat = glm::rotate(tiger_eye_mat, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_eye_coord = tiger_eye_mat * tiger_eye_mc;

		tiger_eye_y = tiger_eye_coord.y;
		tiger_eye_z = tiger_eye_coord.z;

		glm::vec4 tiger_observe_mc(0.0f, tiger_observe_y, tiger_observe_z, 1.0f);
		tiger_observe_y = 400.0 / TIGER_ROUTE2_TIME * t - 6000;
		tiger_observe_z = -1.0 / 100 * (tiger_observe_y + 5600) * (tiger_observe_y + 6000) + 60;

		glm::mat4 tiger_observe_mat = glm::translate(glm::mat4(1.0), glm::vec3(0, tiger_observe_y, tiger_observe_z));
		tiger_observe_mat = glm::rotate(tiger_observe_mat, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_observe_coord = tiger_observe_mat * tiger_observe_mc;

		tiger_observe_y = tiger_observe_coord.y;
		tiger_observe_z = tiger_observe_coord.z;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;

		glm::vec3 v = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(tiger_z_prime),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(tiger_z_prime),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, -1.0f, 0.0f);;

		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = v.x; te_pCamera->vaxis[1] = v.y; te_pCamera->vaxis[2] = v.z;
		te_pCamera->naxis[0] = n.x; te_pCamera->naxis[1] = n.y; te_pCamera->naxis[2] = n.z;

		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = v.x; to_pCamera->vaxis[1] = v.y; to_pCamera->vaxis[2] = v.z;
		to_pCamera->naxis[0] = n.x; to_pCamera->naxis[1] = n.y; to_pCamera->naxis[2] = n.z;
	}
	else if ((t -= TIGER_ROUTE2_TIME) < TIGER_ROUTE3_TIME) {
		//x=0, z=30, -5600<y<-4100을 따라 걷는다.
		tiger_eye_y += 1500.0 / TIGER_ROUTE3_TIME * t - 5600; 
		tiger_eye_z += 30;

		tiger_observe_y += 1500.0 / TIGER_ROUTE3_TIME * t - 5600;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = -1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = -1.0f; to_pCamera->naxis[2] = 0.0f;
	}
	else if ((t -= TIGER_ROUTE3_TIME) < TIGER_ROUTE4_TIME) {
		//rotate -90
		tiger_eye_x = tiger_eye_y;
		tiger_eye_y = 0;
		tiger_observe_x = tiger_observe_y;
		tiger_observe_y = 0;

		//0<x<500, y=-4100, z=30을 따라 걷는다.
		tiger_eye_x += 500.0 / TIGER_ROUTE4_TIME * t;
		tiger_eye_y += -4100; 
		tiger_eye_z += 30;

		tiger_observe_x += 500.0 / TIGER_ROUTE4_TIME * t;
		tiger_observe_y += -4100;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 0.0f; te_pCamera->uaxis[1] = -1.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = -1.0f; te_pCamera->naxis[1] = 0.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 0.0f; to_pCamera->uaxis[1] = -1.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = -1.0f; to_pCamera->naxis[1] = 0.0f; to_pCamera->naxis[2] = 0.0f;

	}
	else if ((t -= TIGER_ROUTE4_TIME) < TIGER_ROUTE5_TIME) {
		//x=500, z=30, -4100<y<-2600을 따라 걷는다.
		tiger_eye_x += 500; 
		tiger_eye_y += 1500.0 / TIGER_ROUTE5_TIME * t - 4100;
		tiger_eye_z += 30;

		tiger_observe_x += 500;
		tiger_observe_y += 1500.0 / TIGER_ROUTE5_TIME * t - 4100;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = -1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = -1.0f; to_pCamera->naxis[2] = 0.0f;

	}
	else if ((t -= TIGER_ROUTE5_TIME) < TIGER_ROUTE6_TIME) {
		//x=500, z=27/40*y+1785(-2600<y<-2200)을 따라 걷는다.
		glm::vec4 tiger_eye_mc(0.0f, tiger_eye_y, tiger_eye_z, 1.0f);
		tiger_eye_x = 500;
		tiger_eye_y = 400.0 / TIGER_ROUTE6_TIME * t - 2600;
		tiger_eye_z = 27.0 / 40 * tiger_eye_y + 1785;

		glm::mat4 tiger_eye_mat = glm::translate(glm::mat4(1.0), glm::vec3(tiger_eye_x, tiger_eye_y, tiger_eye_z));
		tiger_eye_mat = glm::rotate(tiger_eye_mat, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_eye_coord = tiger_eye_mat * tiger_eye_mc;

		tiger_eye_x = tiger_eye_coord.x;
		tiger_eye_y = tiger_eye_coord.y;
		tiger_eye_z = tiger_eye_coord.z;

		glm::vec4 tiger_observe_mc(0.0f, tiger_observe_y, tiger_observe_z, 1.0f);
		tiger_observe_x = 500;
		tiger_observe_y = 400.0 / TIGER_ROUTE6_TIME * t - 2600;
		tiger_observe_z = 27.0 / 40 * tiger_observe_y + 1785;

		glm::mat4 tiger_observe_mat = glm::translate(glm::mat4(1.0), glm::vec3(tiger_eye_x, tiger_observe_y, tiger_observe_z));
		tiger_observe_mat = glm::rotate(tiger_observe_mat, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_observe_coord = tiger_observe_mat * tiger_observe_mc;

		tiger_observe_x = tiger_observe_coord.x;
		tiger_observe_y = tiger_observe_coord.y;
		tiger_observe_z = tiger_observe_coord.z;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;

		glm::vec3 v = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(27.0 / 40),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(27.0 / 40),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, -1.0f, 0.0f);;

		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = v.x; te_pCamera->vaxis[1] = v.y; te_pCamera->vaxis[2] = v.z;
		te_pCamera->naxis[0] = n.x; te_pCamera->naxis[1] = n.y; te_pCamera->naxis[2] = n.z;

		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = v.x; to_pCamera->vaxis[1] = v.y; to_pCamera->vaxis[2] = v.z;
		to_pCamera->naxis[0] = n.x; to_pCamera->naxis[1] = n.y; to_pCamera->naxis[2] = n.z;
	}
	else if ((t -= TIGER_ROUTE6_TIME) < TIGER_ROUTE7_TIME) {
		//x=500, -2200<y<-1900, z=270을 따라 걷는다.
		tiger_eye_x += 500;
		tiger_eye_y += 300.0 / TIGER_ROUTE7_TIME * t - 2200;
		tiger_eye_z += 270;

		tiger_observe_x += 500;
		tiger_observe_y += 300.0 / TIGER_ROUTE7_TIME * t - 2200;
		tiger_observe_z += 270;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = -1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = -1.0f; to_pCamera->naxis[2] = 0.0f;
	}
	else if ((t -= TIGER_ROUTE7_TIME) < TIGER_ROUTE8_TIME) {
		//rotate
		tiger_eye_x = -tiger_eye_y;
		tiger_eye_y = 0;

		tiger_observe_x = -tiger_observe_y;
		tiger_observe_y = 0;

		//-500<x<500, y=-1900, z=270을 따라 걷는다.
		tiger_eye_x += -1000.0 / TIGER_ROUTE8_TIME * t + 500;
		tiger_eye_y += -1900;
		tiger_eye_z += 270;

		tiger_observe_x += -1000.0 / TIGER_ROUTE8_TIME * t + 500;
		tiger_observe_y += -1900;
		tiger_observe_z += 270;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 0.0f; te_pCamera->uaxis[1] = 1.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 1.0f; te_pCamera->naxis[1] = 0.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 0.0f; to_pCamera->uaxis[1] = 1.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 1.0f; to_pCamera->naxis[1] = 0.0f; to_pCamera->naxis[2] = 0.0f;
	}
	//7~1ROUTES를 되돌아간다.
	else if ((t -= TIGER_ROUTE8_TIME) < TIGER_ROUTE7_TIME) {
		//x=-500, -2200<y<-1900, z=270을 따라 걷는다.
		tiger_eye_x += -500;
		tiger_eye_y += -300.0 / TIGER_ROUTE7_TIME * t - 1900 - 88 - 88;
		tiger_eye_z += 270;

		tiger_observe_x += -500;
		tiger_observe_y += -300.0 / TIGER_ROUTE7_TIME * t - 1900 + 150 + 150;
		tiger_observe_z += 270;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = 1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = 1.0f; to_pCamera->naxis[2] = 0.0f;
	}
	else if ((t -= TIGER_ROUTE7_TIME) < TIGER_ROUTE6_TIME) {
		//x=-500, z=27/40*y+1785(-2600<y<-2200)을 따라 걷는다.
		glm::vec4 tiger_eye_mc(0.0f, tiger_eye_y - 88 - 88, tiger_eye_z, 1.0f);
		tiger_eye_x = -500;
		tiger_eye_y = -400.0 / TIGER_ROUTE6_TIME * t - 2200;
		tiger_eye_z = 27.0 / 40 * tiger_eye_y + 1785;

		glm::mat4 tiger_eye_mat = glm::translate(glm::mat4(1.0), glm::vec3(tiger_eye_x, tiger_eye_y, tiger_eye_z));
		tiger_eye_mat = glm::rotate(tiger_eye_mat, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_eye_coord = tiger_eye_mat * tiger_eye_mc;

		tiger_eye_x = tiger_eye_coord.x;
		tiger_eye_y = tiger_eye_coord.y;
		tiger_eye_z = tiger_eye_coord.z;

		glm::vec4 tiger_observe_mc(0.0f, tiger_observe_y + 150 + 150, tiger_observe_z, 1.0f);
		tiger_observe_x = -500;
		tiger_observe_y = -400.0 / TIGER_ROUTE6_TIME * t - 2200;
		tiger_observe_z = 27.0 / 40 * tiger_observe_y + 1785;

		glm::mat4 tiger_observe_mat = glm::translate(glm::mat4(1.0), glm::vec3(tiger_eye_x, tiger_observe_y, tiger_observe_z));
		tiger_observe_mat = glm::rotate(tiger_observe_mat, atanf(27.0 / 40), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_observe_coord = tiger_observe_mat * tiger_observe_mc;

		tiger_observe_x = tiger_observe_coord.x;
		tiger_observe_y = tiger_observe_coord.y;
		tiger_observe_z = tiger_observe_coord.z;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;

		glm::vec3 v = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(27.0 / 40),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(27.0 / 40),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 1.0f, 0.0f);;

		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = v.x; te_pCamera->vaxis[1] = v.y; te_pCamera->vaxis[2] = v.z;
		te_pCamera->naxis[0] = n.x; te_pCamera->naxis[1] = n.y; te_pCamera->naxis[2] = n.z;

		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = v.x; to_pCamera->vaxis[1] = v.y; to_pCamera->vaxis[2] = v.z;
		to_pCamera->naxis[0] = n.x; to_pCamera->naxis[1] = n.y; to_pCamera->naxis[2] = n.z;
	}
	else if ((t -= TIGER_ROUTE6_TIME) < TIGER_ROUTE5_TIME) {
		tiger_eye_x += -500;
		tiger_eye_y += -1500.0 / TIGER_ROUTE5_TIME * t - 2600 - 88 - 88;
		tiger_eye_z += 30;

		tiger_observe_x += -500;
		tiger_observe_y += -1500.0 / TIGER_ROUTE5_TIME * t - 2600 + 150 + 150;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = 1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = 1.0f; to_pCamera->naxis[2] = 0.0f;

	}
	else if ((t -= TIGER_ROUTE5_TIME) < TIGER_ROUTE4_TIME) {
		//rotate
		tiger_eye_x = -(tiger_eye_y - 88 - 88);
		tiger_eye_y = 0;

		tiger_observe_x = -(tiger_observe_y + 150 + 150);
		tiger_observe_y = 0;

		//x=0, z=30, -5600<y<-4100을 따라 걷는다.
		tiger_eye_x += 500.0 / TIGER_ROUTE4_TIME * t - 500;
		tiger_eye_y += -4100;
		tiger_eye_z += 30;

		tiger_observe_x += 500.0 / TIGER_ROUTE4_TIME * t - 500;
		tiger_observe_y += -4100;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = 0.0f; te_pCamera->uaxis[1] = -1.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = -1.0f; te_pCamera->naxis[1] = 0.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = 0.0f; to_pCamera->uaxis[1] = -1.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = -1.0f; to_pCamera->naxis[1] = 0.0f; to_pCamera->naxis[2] = 0.0f;
	}
	else if ((t -= TIGER_ROUTE4_TIME) < TIGER_ROUTE3_TIME) {
		//x=0, z=30, -5600<y<-4100을 따라 걷는다.
		tiger_eye_y += -1500.0 / TIGER_ROUTE3_TIME * t - 4100 - 88 - 88;
		tiger_eye_z += 30;

		tiger_observe_y += -1500.0 / TIGER_ROUTE3_TIME * t - 4100 + 150 + 150;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = 1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = 1.0f; to_pCamera->naxis[2] = 0.0f;

	}
	else if ((t -= TIGER_ROUTE3_TIME) < TIGER_ROUTE2_TIME) {
		//-6000<y<-5600 까지 포물선을 그리며 점프한다.
		glm::vec4 tiger_eye_mc(0.0f, tiger_eye_y - 88 - 88, tiger_eye_z, 1.0f);
		tiger_eye_y = -400.0 / TIGER_ROUTE2_TIME * t - 5600;
		tiger_eye_z = -1.0 / 100 * (tiger_eye_y + 5600) * (tiger_eye_y + 6000) + 60;
		float tiger_z_prime = -1.0 / 50 * tiger_eye_y - 116;

		glm::mat4 tiger_eye_mat = glm::translate(glm::mat4(1.0), glm::vec3(0, tiger_eye_y, tiger_eye_z));
		tiger_eye_mat = glm::rotate(tiger_eye_mat, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_eye_coord = tiger_eye_mat * tiger_eye_mc;

		tiger_eye_y = tiger_eye_coord.y;
		tiger_eye_z = tiger_eye_coord.z;

		glm::vec4 tiger_observe_mc(0.0f, tiger_observe_y + 150 + 150, tiger_observe_z, 1.0f);
		tiger_observe_y = -400.0 / TIGER_ROUTE2_TIME * t - 5600;
		tiger_observe_z = -1.0 / 100 * (tiger_observe_y + 5600) * (tiger_observe_y + 6000) + 60;

		glm::mat4 tiger_observe_mat = glm::translate(glm::mat4(1.0), glm::vec3(0, tiger_observe_y, tiger_observe_z));
		tiger_observe_mat = glm::rotate(tiger_observe_mat, atanf(tiger_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec4 tiger_observe_coord = tiger_observe_mat * tiger_observe_mc;

		tiger_observe_y = tiger_observe_coord.y;
		tiger_observe_z = tiger_observe_coord.z;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;

		glm::vec3 v = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(tiger_z_prime),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 n = glm::mat3(glm::rotate(glm::mat4(1.0), atanf(tiger_z_prime),
			glm::vec3(1.0f, 0.0f, 0.0f))) * glm::vec3(0.0f, 1.0f, 0.0f);;

		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = v.x; te_pCamera->vaxis[1] = v.y; te_pCamera->vaxis[2] = v.z;
		te_pCamera->naxis[0] = n.x; te_pCamera->naxis[1] = n.y; te_pCamera->naxis[2] = n.z;

		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = v.x; to_pCamera->vaxis[1] = v.y; to_pCamera->vaxis[2] = v.z;
		to_pCamera->naxis[0] = n.x; to_pCamera->naxis[1] = n.y; to_pCamera->naxis[2] = n.z;
	}
	else if (t -= TIGER_ROUTE2_TIME < TIGER_ROUTE1_TIME) {
		//x=0, z=30, -6500<y<-6000을 따라 걷는다.
		tiger_eye_y += -500.0 / TIGER_ROUTE1_TIME * t - 6000 - 88 - 88;
		tiger_eye_z += 30;

		tiger_observe_y += -500.0 / TIGER_ROUTE1_TIME * t - 6000 + 150 + 150;
		tiger_observe_z += 30;

		te_pCamera->pos[0] = tiger_eye_x;
		te_pCamera->pos[1] = tiger_eye_y;
		te_pCamera->pos[2] = tiger_eye_z + up_down;
		te_pCamera->uaxis[0] = -1.0f; te_pCamera->uaxis[1] = 0.0f; te_pCamera->uaxis[2] = 0.0f;
		te_pCamera->vaxis[0] = 0.0f; te_pCamera->vaxis[1] = 0.0f; te_pCamera->vaxis[2] = 1.0f;
		te_pCamera->naxis[0] = 0.0f; te_pCamera->naxis[1] = 1.0f; te_pCamera->naxis[2] = 0.0f;

		to_pCamera->pos[0] = tiger_observe_x;
		to_pCamera->pos[1] = tiger_observe_y;
		to_pCamera->pos[2] = tiger_observe_z;
		to_pCamera->uaxis[0] = -1.0f; to_pCamera->uaxis[1] = 0.0f; to_pCamera->uaxis[2] = 0.0f;
		to_pCamera->vaxis[0] = 0.0f; to_pCamera->vaxis[1] = 0.0f; to_pCamera->vaxis[2] = 1.0f;
		to_pCamera->naxis[0] = 0.0f; to_pCamera->naxis[1] = 1.0f; to_pCamera->naxis[2] = 0.0f;
	}
	if (is_tiger_related_camera) {
		update_vp_matrix();
	}
}

void draw_tiger(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	set_tiger_MVP();
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(tiger_VAO);
	glDrawArrays(GL_TRIANGLES, tiger_vertex_offset[cur_frame_tiger], 3 * tiger_n_triangles[cur_frame_tiger]);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void prepare_ben(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ben_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_BEN_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/ben/ben_vn%d%d.geom", i / 10, i % 10);
		ben_n_triangles[i] = read_geometry(&ben_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		ben_n_total_triangles += ben_n_triangles[i];

		if (i == 0)
			ben_vertex_offset[i] = 0;
		else
			ben_vertex_offset[i] = ben_vertex_offset[i - 1] + 3 * ben_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &ben_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glBufferData(GL_ARRAY_BUFFER, ben_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_BEN_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, ben_vertex_offset[i] * n_bytes_per_vertex,
			ben_n_triangles[i] * n_bytes_per_triangle, ben_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_BEN_FRAMES; i++)
		free(ben_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &ben_VAO);
	glBindVertexArray(ben_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ben_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//ben 이동속성 설정
#define BEN_TOTAL_MOVING_TIME 10000//ms, round-trip
#define BEN_TOTAL_MOVING_SPEED 0.8
float get_ben_xy_rotate_angle(float ben_x_prime, float ben_y_prime) {
	if (ben_x_prime == 0) {
		if (ben_y_prime > 0) return 0.0;
		return 180.0;
	}
	float theta = atan2f(ben_y_prime, ben_x_prime) * TO_DEGREE;
	if (theta == 0) {
		if (ben_x_prime > 0) return 90.0;
		return -90.0;
	}
	return -90.0 + theta;
}
void set_ben_MVP() {
	float ben_x, ben_y, ben_z;
	float ben_x_prime, ben_y_prime, ben_z_prime;
	unsigned int t = ((long long)_timestamp_scene * 100) % BEN_TOTAL_MOVING_TIME;
	ben_x = 500 * cos(BEN_TOTAL_MOVING_SPEED * PI / 1000 * t);
	ben_y = 500 * sin(BEN_TOTAL_MOVING_SPEED * PI / 1000 * t);
	ben_z = (float)t / 10 + 200;
	ben_x_prime = -PI / 2 * BEN_TOTAL_MOVING_SPEED * sin(BEN_TOTAL_MOVING_SPEED * PI / 1000 * t);
	ben_y_prime = PI / 2 * BEN_TOTAL_MOVING_SPEED * cos(BEN_TOTAL_MOVING_SPEED * PI / 1000 * t);
	ben_z_prime = 0.1;

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(ben_x, ben_y, ben_z));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, get_ben_xy_rotate_angle(ben_x_prime, ben_y_prime) * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, atanf(ben_z_prime), glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(400.0f, 400.0f, -400.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
}

void draw_ben(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	set_ben_MVP();
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(ben_VAO);
	glDrawArrays(GL_TRIANGLES, ben_vertex_offset[cur_frame_ben], 3 * ben_n_triangles[cur_frame_ben]);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void prepare_spider(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, spider_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	for (i = 0; i < N_SPIDER_FRAMES; i++) {
		sprintf(filename, "Data/dynamic_objects/spider/spider_vnt_%d%d.geom", i / 10, i % 10);
		spider_n_triangles[i] = read_geometry(&spider_vertices[i], n_bytes_per_triangle, filename);
		// assume all geometry files are effective
		spider_n_total_triangles += spider_n_triangles[i];

		if (i == 0)
			spider_vertex_offset[i] = 0;
		else
			spider_vertex_offset[i] = spider_vertex_offset[i - 1] + 3 * spider_n_triangles[i - 1];
	}

	// initialize vertex buffer object
	glGenBuffers(1, &spider_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glBufferData(GL_ARRAY_BUFFER, spider_n_total_triangles * n_bytes_per_triangle, NULL, GL_STATIC_DRAW);

	for (i = 0; i < N_SPIDER_FRAMES; i++)
		glBufferSubData(GL_ARRAY_BUFFER, spider_vertex_offset[i] * n_bytes_per_vertex,
			spider_n_triangles[i] * n_bytes_per_triangle, spider_vertices[i]);

	// as the geometry data exists now in graphics memory, ...
	for (i = 0; i < N_SPIDER_FRAMES; i++)
		free(spider_vertices[i]);

	// initialize vertex array object
	glGenVertexArrays(1, &spider_VAO);
	glBindVertexArray(spider_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, spider_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//spider 이동속성 설정
#define SPIDER_TOTAL_MOVING_TIME 20000//ms, round-trip
void set_spider_MVP() {
	float spider_x, spider_y, spider_z;
	//원환소용돌이선
	unsigned int t = ((long long)_timestamp_scene * 100) % SPIDER_TOTAL_MOVING_TIME;
	spider_x = (500 + 100*sin(2 * PI * 20 / SPIDER_TOTAL_MOVING_TIME * t)) * cos(2 * PI / SPIDER_TOTAL_MOVING_TIME * t);
	spider_y = (500 + 100*sin(2 * PI * 20 / SPIDER_TOTAL_MOVING_TIME * t)) * sin(2 * PI / SPIDER_TOTAL_MOVING_TIME * t);
	spider_z = 100*cos(2 * PI * 20 / SPIDER_TOTAL_MOVING_TIME * t) + 500;

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(spider_x, spider_y, spider_z));
	/*ModelViewMatrix = glm::rotate(ModelViewMatrix, -90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));*/
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(60.0f, 60.0f, 60.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
}

void draw_spider(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	set_spider_MVP();
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(spider_VAO);
	glDrawArrays(GL_TRIANGLES, spider_vertex_offset[cur_frame_spider], 3 * spider_n_triangles[cur_frame_spider]);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void prepare_ironman(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, ironman_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/ironman_vnt.geom");
	ironman_n_triangles = read_geometry(&ironman_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	ironman_n_total_triangles += ironman_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &ironman_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glBufferData(GL_ARRAY_BUFFER, ironman_n_total_triangles * 3 * n_bytes_per_vertex, ironman_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(ironman_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &ironman_VAO);
	glBindVertexArray(ironman_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, ironman_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_ironman(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, -2300, 30));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(60.0f, 60.0f, 60.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(ironman_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
	glBindVertexArray(0);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(0, -3600, 30));
	ModelViewMatrix = glm::rotate(ModelViewMatrix, 90 * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(-60.0f, 60.0f, -60.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(ironman_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * ironman_n_triangles);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void prepare_optimus(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, optimus_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/optimus_vnt.geom");
	optimus_n_triangles = read_geometry(&optimus_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	optimus_n_total_triangles += optimus_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &optimus_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glBufferData(GL_ARRAY_BUFFER, optimus_n_total_triangles * 3 * n_bytes_per_vertex, optimus_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(optimus_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &optimus_VAO);
	glBindVertexArray(optimus_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, optimus_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_optimus(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(300, -5800, 30));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(-0.3f, 0.3f, 0.3f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(-300, -5800, 30));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(0.3f, 0.3f, 0.3f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(optimus_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * optimus_n_triangles);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void prepare_dragon(void) {
	int i, n_bytes_per_vertex, n_bytes_per_triangle, dragon_n_total_triangles = 0;
	char filename[512];

	n_bytes_per_vertex = 8 * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	sprintf(filename, "Data/static_objects/dragon_vnt.geom");
	dragon_n_triangles = read_geometry(&dragon_vertices, n_bytes_per_triangle, filename);
	// assume all geometry files are effective
	dragon_n_total_triangles += dragon_n_triangles;


	// initialize vertex buffer object
	glGenBuffers(1, &dragon_VBO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glBufferData(GL_ARRAY_BUFFER, dragon_n_total_triangles * 3 * n_bytes_per_vertex, dragon_vertices, GL_STATIC_DRAW);

	// as the geometry data exists now in graphics memory, ...
	free(dragon_vertices);

	// initialize vertex array object
	glGenVertexArrays(1, &dragon_VAO);
	glBindVertexArray(dragon_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, dragon_VBO);
	glVertexAttribPointer(LOC_VERTEX, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(LOC_TEXCOORD, 2, GL_FLOAT, GL_FALSE, n_bytes_per_vertex, BUFFER_OFFSET(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_dragon(void) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(h_ShaderProgram_TXPBR);

	ModelViewMatrix = glm::translate(ViewMatrix, glm::vec3(300, -2400, 1700));
	ModelViewMatrix = glm::scale(ModelViewMatrix, glm::vec3(90.0f, 90.0f, 90.0f));
	ModelViewProjectionMatrix = ProjectionMatrix * ModelViewMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix_TXPBR, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	glUniformMatrix4fv(loc_ModelViewMatrix_TXPBR, 1, GL_FALSE, &ModelViewMatrix[0][0]);
	glUniformMatrix3fv(loc_ModelViewMatrixInvTrans_TXPBR, 1, GL_FALSE, &ModelViewMatrixInvTrans[0][0]);
	glFrontFace(GL_CW);

	glBindVertexArray(dragon_VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3 * dragon_n_triangles);
	glBindVertexArray(0);

	glUseProgram(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
/*****************************  END: geometry setup *****************************/

//debug
void print_camera_coord() {
	printf("\n%f %f %f\n", current_camera->pos[0], current_camera->pos[1], current_camera->pos[2]);
	printf("%f %f %f\n", current_camera->uaxis[0], current_camera->uaxis[1], current_camera->uaxis[2]);
	printf("%f %f %f\n", current_camera->vaxis[0], current_camera->vaxis[1], current_camera->vaxis[2]);
	printf("%f %f %f\n\n", current_camera->naxis[0], current_camera->naxis[1], current_camera->naxis[2]);
}

/********************  START: callback function definitions *********************/
void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//print_camera_coord();

	update_tiger_related_camera();


	draw_grid();
	draw_axes();
	draw_sun_temple();
	draw_skybox();
	draw_tiger();
	draw_ben();
	draw_spider();
	draw_ironman();
	draw_optimus();
	draw_dragon();

	glutSwapBuffers();
}

void special(int key, int x, int y) {
	if (!is_world_moving_camera) {
		return;
	}
	int active_key = glutGetModifiers();

	switch (key) {
	case GLUT_KEY_LEFT: {
		//ALT키와 LEFT키를 같이 누르는 경우에 n축 양의 둘레로 회전한다.
		if (active_key == GLUT_ACTIVE_ALT) {
			rotate_world_moving_camera(CAM_ROTATING_SPEED, N_AXIS);
			update_vp_matrix();
			glutPostRedisplay();
			break;
		}
		//SHIFT키와 LEFT키를 같이 누르는 경우에 v축 양의 둘레로 회전한다.
		if (active_key == GLUT_ACTIVE_SHIFT) {
			rotate_world_moving_camera(CAM_ROTATING_SPEED, V_AXIS);
			update_vp_matrix();
			glutPostRedisplay();
			break;
		}
		move_world_moving_camera(-CAM_MOVING_SPEED, U_MOVING);
		update_vp_matrix();
		glutPostRedisplay();
		break;
	}
	case GLUT_KEY_RIGHT:
		if (active_key == GLUT_ACTIVE_ALT) {
			rotate_world_moving_camera(-CAM_ROTATING_SPEED, N_AXIS);
			update_vp_matrix();
			glutPostRedisplay();
			break;
		}
		if (active_key == GLUT_ACTIVE_SHIFT) {
			rotate_world_moving_camera(-CAM_ROTATING_SPEED, V_AXIS);
			update_vp_matrix();
			glutPostRedisplay();
			break;
		}
		move_world_moving_camera(CAM_MOVING_SPEED, U_MOVING);
		update_vp_matrix();
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		//CTRL키와 DOWN키를 같이 누르는 경우에 n축으로 카메라가 이동한다.
		if (active_key == GLUT_ACTIVE_CTRL) {
			move_world_moving_camera(CAM_MOVING_SPEED, N_MOVING);
			update_vp_matrix();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		}
		//ALT키와 DOWN키를 같이 누르는 경우에 u축 양의 둘레로 회전한다.
		if (active_key == GLUT_ACTIVE_ALT) {
			rotate_world_moving_camera(CAM_ROTATING_SPEED, U_AXIS);
			update_vp_matrix();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		}
		move_world_moving_camera(-CAM_MOVING_SPEED, V_MOVING);
		update_vp_matrix();
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		if (active_key == GLUT_ACTIVE_CTRL) {
			move_world_moving_camera(-CAM_MOVING_SPEED, N_MOVING);
			update_vp_matrix();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		}
		if (active_key == GLUT_ACTIVE_ALT) {
			rotate_world_moving_camera(-CAM_ROTATING_SPEED, U_AXIS);
			update_vp_matrix();
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
			break;
		}
		move_world_moving_camera(CAM_MOVING_SPEED, V_MOVING);
		update_vp_matrix();
		ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
		glutPostRedisplay();
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'a':
		is_world_moving_camera = true;
		is_tiger_related_camera = false;
		set_current_camera(WORLD_MOVING_CAMERA);
		glutPostRedisplay();
		break;
	case 's':
		animation_mode = 1 - animation_mode;
		break;
	case 'f':
		b_draw_grid = b_draw_grid ? false : true;
		glutPostRedisplay();
		break;
	case 'u':
		is_world_moving_camera = false;
		is_tiger_related_camera = false;
		set_current_camera(CAMERA_1);
		glutPostRedisplay();
		break;
	case 'i':
		is_world_moving_camera = false;
		is_tiger_related_camera = false;
		set_current_camera(CAMERA_2);
		glutPostRedisplay();
		break;
	case 'o':
		is_world_moving_camera = false;
		is_tiger_related_camera = false;
		set_current_camera(CAMERA_3);
		glutPostRedisplay();
		break;
	case 'p':
		is_world_moving_camera = false;
		is_tiger_related_camera = false;
		set_current_camera(CAMERA_4);
		glutPostRedisplay();
		break;
	case 't':
		is_world_moving_camera = false;
		is_tiger_related_camera = true;
		set_current_camera(TIGER_EYE_CAMERA);
		glutPostRedisplay();
		break;
	case 'g':
		is_world_moving_camera = false;
		is_tiger_related_camera = true;
		set_current_camera(TIGER_OBSERVE_CAMERA);
		glutPostRedisplay();
		break;
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void mouse(int button, int state, int x, int y) {
	int active_key = glutGetModifiers();
	switch (button) {
	case MOUSE_WHEELED: 
		if (active_key == GLUT_ACTIVE_CTRL && current_camera->fovy * TO_DEGREE < 60) {
			current_camera->fovy += 1 * TO_RADIAN;
			ProjectionMatrix = glm::perspective(current_camera->fovy, current_camera->aspect_ratio, current_camera->near_c, current_camera->far_c);
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
		}
		break;
	case MOUSE_WHEELED - 1: //휠을 위로 회전하는 경우를 나타낸다.
		if (active_key == GLUT_ACTIVE_CTRL && current_camera->fovy * TO_DEGREE > 20) {
			current_camera->fovy -= 1 * TO_RADIAN;
			ProjectionMatrix = glm::perspective(current_camera->fovy, current_camera->aspect_ratio, current_camera->near_c, current_camera->far_c);
			ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
			glutPostRedisplay();
		}
		break;
	default:
		break;
	}
}

void reshape(int width, int height) {
	float aspect_ratio;

	glViewport(0, 0, width, height);

	ProjectionMatrix = glm::perspective(current_camera->fovy, current_camera->aspect_ratio, current_camera->near_c, current_camera->far_c);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;
	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &axes_VAO);
	glDeleteBuffers(1, &axes_VBO);

	glDeleteVertexArrays(1, &grid_VAO);
	glDeleteBuffers(1, &grid_VBO);

	glDeleteVertexArrays(scene.n_materials, sun_temple_VAO);
	glDeleteBuffers(scene.n_materials, sun_temple_VBO);
	glDeleteTextures(scene.n_textures, sun_temple_texture_names);

	glDeleteVertexArrays(1, &skybox_VAO);
	glDeleteBuffers(1, &skybox_VBO);

	glDeleteVertexArrays(1, &tiger_VAO);
	glDeleteBuffers(1, &tiger_VBO);

	free(sun_temple_n_triangles);
	free(sun_temple_vertex_offset);

	free(sun_temple_VAO);
	free(sun_temple_VBO);

	free(sun_temple_texture_names);
}
/*********************  END: callback function definitions **********************/

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutCloseFunc(cleanup);
}

void initialize_OpenGL(void) {
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	ViewMatrix = glm::mat4(1.0f);
	ProjectionMatrix = glm::mat4(1.0f);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	initialize_lights();
}

void prepare_scene(void) {
	prepare_axes();
	prepare_tiger();
	prepare_grid();
	prepare_sun_temple();
	prepare_skybox();
	prepare_ben();
	prepare_spider();
	prepare_ironman();
	prepare_optimus();
	prepare_dragon();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program();
	initialize_OpenGL();
	prepare_scene();
	initialize_camera();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "********************************************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "********************************************************************************\n\n");
}

void print_message(const char* m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char* program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "********************************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n********************************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 9
void drawScene(int argc, char* argv[]) {
	char program_name[64] = "Sogang CSE4170 Sun Temple Scene";
	char messages[N_MESSAGE_LINES][256] = { 
		"    - Keys used:",
		"		'f' : draw x, y, z axes and grid",
		"		'1' : set the camera for bronze statue view",
		"		'2' : set the camera for bronze statue view",
		"		'3' : set the camera for tree view",	
		"		'4' : set the camera for top view",
		"		'5' : set the camera for front view",
		"		'6' : set the camera for side view",
		"		'ESC' : program close",
	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(900, 600);
	glutInitWindowPosition(20, 20);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}
