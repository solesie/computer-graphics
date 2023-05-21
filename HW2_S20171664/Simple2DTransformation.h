#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>

#include "Shaders/LoadShaders.h"
GLuint h_ShaderProgram; // handle to shader program
GLint loc_ModelViewProjectionMatrix, loc_primitive_color; // indices of uniform variables

// include glm/*.hpp only if necessary
//#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, ortho, etc.
glm::mat4 ModelViewProjectionMatrix;
glm::mat4 ViewMatrix, ProjectionMatrix, ViewProjectionMatrix;

#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f
#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))

#define LOC_VERTEX 0

int win_width = 0, win_height = 0;
float centerx = 0.0f, centery = 0.0f, rotate_angle = 0.0f;
int human_cur_line = 1, human_cur_x = -500;
int human_cur_rightest = human_cur_x + 34, human_cur_leftest = human_cur_x - 34;
unsigned int timestamp = 0;
int house_x;
bool gameover = false;

#define CAR1_TYPE 1
#define CAR2_TYPE 2
#define CAR_START_X 2000
struct CarInfo {
private:
	int carType;
	unsigned int createdTimestamp;
	double velocity;
	int line;
	float getMovedDist() {
		unsigned int collapsedTimestamp = createdTimestamp <= timestamp ? timestamp - createdTimestamp : (long long)timestamp + UINT_MAX - createdTimestamp;
		return collapsedTimestamp * velocity;
	}
public:
	CarInfo() {
		createdTimestamp = timestamp;
		velocity = 50 * (rand() % 8 + 1) / 100;
		line = rand() % 4;
		carType = rand() % 2 + 1;
	}
	float getCarLeftest() {
		return carType == CAR1_TYPE ? CAR_START_X - getMovedDist() - 16 : CAR_START_X - getMovedDist() - 18;
	}
	float getCarRightest() {
		return carType == CAR1_TYPE ? CAR_START_X - getMovedDist() + 16 : CAR_START_X - getMovedDist() + 18;
	}
	bool iscrushedWithHuman() {
		if ((line == human_cur_line) && ((human_cur_leftest <= getCarRightest() && human_cur_rightest > getCarLeftest())
			|| (human_cur_rightest >= getCarLeftest() && human_cur_leftest < getCarRightest()))) {
			return true;
		}
		return false;
	}
	float getX() {
		return CAR_START_X - getMovedDist();
	}
	float getY() {
		return -110 - (line * 67.5);
	}
	int getCarType() {
		return carType;
	}
	bool isOutOfBound() {
		return 2 * CAR_START_X <= getMovedDist();
	}
};
struct CarInfoForDrawing {//DTO
private:
	float x;
	float y;
	int carType;
	bool crushed;
public:
	CarInfoForDrawing(float x, float y, int carType, bool crushed) :x(x), y(y), carType(carType), crushed(crushed){ }
	float getY() {
		return y;
	}
	float getX() {
		return x;
	}
	int getCarType() {
		return carType;
	}
	bool iscrushedCar() {
		return crushed;
	}
};

CarInfoForDrawing* crushedCar = nullptr;

struct Cars {
private:
	std::vector<CarInfo> carList;
public:
	Cars() {}
	void removeOutOfBoundCars() {
		for (int i = 0; i < carList.size(); ++i) {
			if (carList.at(i).isOutOfBound()) {
				carList.erase(carList.begin() + i);
			}
		}
	}
	void removeCrushedCar() {
		for (int i = 0; i < carList.size(); ++i) {
			if (carList.at(i).iscrushedWithHuman()){
				carList.erase(carList.begin() + i);
				break;
			}
		}
	}
	void addNewCar() {
		carList.push_back(CarInfo());
	}
	CarInfoForDrawing* getcrushedCar() {
		for (int i = 0; i < carList.size(); ++i) {
			if (carList.at(i).iscrushedWithHuman()) {
				return new CarInfoForDrawing(carList.at(i).getX(), carList.at(i).getY(), carList.at(i).getCarType(), true);
			}
		}
		return nullptr;
	}
	bool haveCrushedCar() {
		for (int i = 0; i < carList.size(); ++i) {
			if (carList.at(i).iscrushedWithHuman()) {
				return true;
			}
		}
		return false;
	}
	std::vector<CarInfoForDrawing> getCarsInfoForDrawing() {
		std::vector<CarInfoForDrawing> ret;
		for (int i = 0; i < carList.size(); ++i) {
			ret.push_back(CarInfoForDrawing(carList.at(i).getX(), carList.at(i).getY(), carList.at(i).getCarType(), carList.at(i).iscrushedWithHuman()));
		}
		return ret;
	}
};

Cars cars;

struct SwordInfo {
private:
	bool isInitiallyFront;
	unsigned int createdTimestamp;
	int initialX;
	int a, b;
	float getEllipseFabsY(float curX) {
		return sqrtf((-(float)b * b) / ((float)a * a) * (curX * curX) + (float)b * b);
	}
public:
	SwordInfo(int initialX, bool isInitiallyFront, int a, int b) :initialX(initialX), isInitiallyFront(isInitiallyFront), a(a), b(b) {
		createdTimestamp = timestamp;
	}
	//(0,0)기준
	float getCurrentX() {
		unsigned int collapsedTimestamp = createdTimestamp <= timestamp ? timestamp - createdTimestamp : (long long)timestamp + UINT_MAX - createdTimestamp;
		long long movedDistance = collapsedTimestamp * 3;
		if (!isInitiallyFront) {
			int distToMinusA = a + initialX;
			if (movedDistance - distToMinusA < 0) {
				return initialX - movedDistance;
			}

			int remainder = (movedDistance - distToMinusA) % (4 * a);
			if (remainder >= 2 * a) {
				return a - (remainder) % (2 * a);
			}
			return -a + (remainder) % (2 * a);
		}

		//위쪽 타원인 경우
		int distToPlusA = a - initialX;
		if (movedDistance - distToPlusA < 0) {
			return initialX + movedDistance;
		}

		int remainder = (movedDistance - distToPlusA) % (4 * a);
		if (remainder >= 2 * a) {
			return -a + (remainder) % (2 * a);
		}
		return a - (remainder) % (2 * a);
	}
	float getCurrentY() {
		unsigned int collapsedTimestamp = createdTimestamp <= timestamp ? timestamp - createdTimestamp : (long long)timestamp + UINT_MAX - createdTimestamp;
		long long movedDistance = collapsedTimestamp * 3;
		int curX = getCurrentX();
		if (!isInitiallyFront) {
			int distToMinusA = a + initialX;
			if (movedDistance - distToMinusA < 0) {
				return 180 + getEllipseFabsY(curX);
			}
			int remainder = (movedDistance - distToMinusA) % (4 * a);
			if (remainder >= 2 * a) {
				return 180 + getEllipseFabsY(curX);
			}
			return 180 - getEllipseFabsY(curX);
		}

		//위쪽 타원인 경우
		int distToMinusA = a - initialX;
		if (movedDistance - distToMinusA < 0) {
			return 180 - getEllipseFabsY(curX);
		}

		int remainder = (movedDistance - distToMinusA) % (4 * a);
		if (remainder >= 2 * a) {
			return 180 - getEllipseFabsY(curX);
		}
		return 180 + getEllipseFabsY(curX);
	}
};
struct SwordInfoForDrawing {//DTO
private:
	float x;
	float y;
public:
	SwordInfoForDrawing(float x, float y) :x(x), y(y) { }
	float getY() {
		return y;
	}
	float getX() {
		return x;
	}
};

std::vector<SwordInfoForDrawing*> gameoveredSwords;

struct Swords {
private:
	std::vector<SwordInfo> swordList;
	int a, b;
public:
	Swords(int a, int b) :a(a), b(b) {}
	void addNewSword(int initialX, bool isInitiallyFront) {
		swordList.push_back(SwordInfo(initialX, isInitiallyFront, a, b));
	}
	std::vector<SwordInfoForDrawing*> getGameoveredSwords() {
		std::vector<SwordInfoForDrawing*> ret;
		for (int i = 0; i < swordList.size(); ++i) {
			ret.push_back(new SwordInfoForDrawing(swordList.at(i).getCurrentX(), swordList.at(i).getCurrentY()));
		}
		return ret;
	}
	std::vector<SwordInfoForDrawing> getSwordsInfoForDrawing() {
		std::vector<SwordInfoForDrawing> ret;
		for (int i = 0; i < swordList.size(); ++i) {
			ret.push_back(SwordInfoForDrawing(swordList.at(i).getCurrentX(), swordList.at(i).getCurrentY()));
		}
		return ret;
	}
};

Swords swords(305, 50);

void timer(int value) {
	timestamp = (timestamp + 1) % UINT_MAX;
	glutPostRedisplay();
	glutTimerFunc(10, timer, 0);
}

GLfloat axes[4][2];
GLfloat axes_color[3] = { 0.0f, 0.0f, 0.0f };
GLuint VBO_axes, VAO_axes;

void prepare_axes(void) { // Draw axes in their MC.
	axes[0][0] = -win_width / 2.5f; axes[0][1] = 0.0f;
	axes[1][0] = win_width / 2.5f; axes[1][1] = 0.0f;
	axes[2][0] = 0.0f; axes[2][1] = -win_height / 2.5f;
	axes[3][0] = 0.0f; axes[3][1] = win_height / 2.5f;

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_axes);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_axes);
	glBindVertexArray(VAO_axes);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void update_axes(void) {
	axes[0][0] = -win_width / 2.5f; axes[1][0] = win_width / 2.5f;
	axes[2][1] = -win_height / 2.5f;
	axes[3][1] = win_height / 2.5f;

	glBindBuffer(GL_ARRAY_BUFFER, VBO_axes);
	glBufferData(GL_ARRAY_BUFFER, sizeof(axes), axes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void draw_axes(void) {
	glUniform3fv(loc_primitive_color, 1, axes_color);
	glBindVertexArray(VAO_axes);
	glDrawArrays(GL_LINES, 0, 4);
	glBindVertexArray(0);
}

#define HUMAN_HAIR 0
#define HUMAN_HEAD 1
#define HUMAN_EYE 2
#define HUMAN_TORSO 3
#define HUMAN_LEFT_HAND 4
#define HUMAN_RIGHT_HAND 5
#define HUMAN_LEFT_FOOT 6
#define HUMAN_RIGHT_FOOT 7
GLfloat hair[6][2] = { { -25.0, 0.0 }, { -25.0, 50.0 }, { 25.0, 50.0 }, { 25.0, 35.0 }, { -12.5, 35.0 }, { -12.5, 0.0 } };
GLfloat head[4][2] = { { -12.5, 0.0 }, { -12.5, 35.0 }, { 25.0, 35.0 }, { 25.0, 0.0 } };
GLfloat eye[2][2] = { { 20.0, 23.0 }, { 0.0, 23.0 } };
GLfloat torso[6][2] = { { 12.5, 0.0 }, { 25, -25.0 }, { 12.5, -50.0 }, { -12.5, -50.0 }, { -25.0, -25.0 }, { -12.5, 0.0 } };
GLfloat left_hand[360][2];
GLfloat right_hand[360][2];
GLfloat left_foot[180][2];
GLfloat right_foot[180][2];
GLfloat human_color[8][3] = {
	{ 0 / 255.0f,    0 / 255.0f,    0 / 255.0f },  // hair
	{ 251 / 255.0f, 206 / 255.0f, 177 / 255.0f },  // head
	{ 0 / 255.0f,    0 / 255.0f,    0 / 255.0f },  // eye
	{ 230 / 255.0f, 0 / 255.0f,     0 / 255.0f },  // torso
	{ 251 / 255.0f, 206 / 255.0f, 177 / 255.0f },  // left_hand
	{ 251 / 255.0f, 206 / 255.0f, 177 / 255.0f },  // right_hand
	{ 251 / 255.0f, 206 / 255.0f, 177 / 255.0f },  // left_foot
	{ 251 / 255.0f, 206 / 255.0f, 177 / 255.0f },  // right_foot
};
GLuint VBO_human, VAO_human;

void prepare_human() {
	float hand_rad = 9.0, foot_rad = 10.5;
	for (int i = 0; i < 360; ++i) {
		left_hand[i][0] = hand_rad * cos(i * TO_RADIAN) - 25.0;
		left_hand[i][1] = hand_rad * sin(i * TO_RADIAN) - 20.0;
		right_hand[i][0] = hand_rad * cos(i * TO_RADIAN) + 25.0;
		right_hand[i][1] = hand_rad * sin(i * TO_RADIAN) - 20.0;
		if (i < 180) {
			left_foot[i][0] = foot_rad * cos(i * TO_RADIAN) - 12.5;
			left_foot[i][1] = foot_rad * sin(i * TO_RADIAN) - 55.0;
			right_foot[i][0] = foot_rad * cos(i * TO_RADIAN) + 12.5;
			right_foot[i][1] = foot_rad * sin(i * TO_RADIAN) - 55.0;
		}
	}
	GLsizeiptr buffer_size = sizeof(hair) + sizeof(head) + sizeof(eye) + sizeof(torso)
		+ sizeof(left_hand) + sizeof(right_hand) + sizeof(left_foot) + sizeof(right_foot);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_human);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_human);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(hair), hair);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair), sizeof(head), head);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head), sizeof(eye), eye);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head) + sizeof(eye), sizeof(torso), torso);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head) + sizeof(eye) + sizeof(torso),
		sizeof(left_hand), left_hand);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head) + sizeof(eye) + sizeof(torso)
		+ sizeof(left_hand), sizeof(right_hand), right_hand);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head) + sizeof(eye) + sizeof(torso)
		+ sizeof(left_hand) + sizeof(right_hand), sizeof(left_foot), left_foot);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(hair) + sizeof(head) + sizeof(eye) + sizeof(torso)
		+ sizeof(left_hand) + sizeof(right_hand) + sizeof(left_foot), sizeof(right_foot), right_foot);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_human);
	glBindVertexArray(VAO_human);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_human);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_human_without_foots() { // Draw human in its MC.
	glBindVertexArray(VAO_human);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_HAIR]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 4);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_EYE]);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 10, 2);
	glPointSize(1.0);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_TORSO]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 6);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_LEFT_HAND]);
	glDrawArrays(GL_TRIANGLE_FAN, 18, 360);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_RIGHT_HAND]);
	glDrawArrays(GL_TRIANGLE_FAN, 378, 360);

	glBindVertexArray(0);
}

void draw_human_left_foot() { 
	glBindVertexArray(VAO_human);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_LEFT_FOOT]);
	glDrawArrays(GL_TRIANGLE_FAN, 738, 180);

	glBindVertexArray(0);
}

void draw_human_right_foot() {
	glBindVertexArray(VAO_human);

	glUniform3fv(loc_primitive_color, 1, human_color[HUMAN_RIGHT_FOOT]);
	glDrawArrays(GL_TRIANGLE_FAN, 918, 180);

	glBindVertexArray(0);
}

#define AIRPLANE_BIG_WING 0
#define AIRPLANE_SMALL_WING 1
#define AIRPLANE_BODY 2
#define AIRPLANE_BACK 3
#define AIRPLANE_SIDEWINDER1 4
#define AIRPLANE_SIDEWINDER2 5
#define AIRPLANE_CENTER 6
GLfloat big_wing[6][2] = { { 0.0, 0.0 }, { -20.0, 15.0 }, { -20.0, 20.0 }, { 0.0, 23.0 }, { 20.0, 20.0 }, { 20.0, 15.0 } };
GLfloat small_wing[6][2] = { { 0.0, -18.0 }, { -11.0, -12.0 }, { -12.0, -7.0 }, { 0.0, -10.0 }, { 12.0, -7.0 }, { 11.0, -12.0 } };
GLfloat body[5][2] = { { 0.0, -25.0 }, { -6.0, 0.0 }, { -6.0, 22.0 }, { 6.0, 22.0 }, { 6.0, 0.0 } };
GLfloat back[5][2] = { { 0.0, 25.0 }, { -7.0, 24.0 }, { -7.0, 21.0 }, { 7.0, 21.0 }, { 7.0, 24.0 } };
GLfloat sidewinder1[5][2] = { { -20.0, 10.0 }, { -18.0, 3.0 }, { -16.0, 10.0 }, { -18.0, 20.0 }, { -20.0, 20.0 } };
GLfloat sidewinder2[5][2] = { { 20.0, 10.0 }, { 18.0, 3.0 }, { 16.0, 10.0 }, { 18.0, 20.0 }, { 20.0, 20.0 } };
GLfloat center[1][2] = { { 0.0, 0.0 } };
GLfloat airplane_color[7][3] = {
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // big_wing
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // small_wing
	{ 111 / 255.0f,  85 / 255.0f, 157 / 255.0f },  // body
	{ 150 / 255.0f, 129 / 255.0f, 183 / 255.0f },  // back
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder1
	{ 245 / 255.0f, 211 / 255.0f,   0 / 255.0f },  // sidewinder2
	{ 0 / 255.0f,  0 / 255.0f,  0 / 255.0f }   // center
};

GLuint VBO_airplane, VAO_airplane;

void prepare_airplane() {
	GLsizeiptr buffer_size = sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2) + sizeof(center);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(big_wing), big_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing), sizeof(small_wing), small_wing);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing), sizeof(body), body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body), sizeof(back), back);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back),
		sizeof(sidewinder1), sidewinder1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1), sizeof(sidewinder2), sidewinder2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(big_wing) + sizeof(small_wing) + sizeof(body) + sizeof(back)
		+ sizeof(sidewinder1) + sizeof(sidewinder2), sizeof(center), center);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_airplane);
	glBindVertexArray(VAO_airplane);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_airplane);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_airplane() { // Draw airplane in its MC.
	glBindVertexArray(VAO_airplane);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BIG_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SMALL_WING]);
	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_BACK]);
	glDrawArrays(GL_TRIANGLE_FAN, 17, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER1]);
	glDrawArrays(GL_TRIANGLE_FAN, 22, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_SIDEWINDER2]);
	glDrawArrays(GL_TRIANGLE_FAN, 27, 5);

	glUniform3fv(loc_primitive_color, 1, airplane_color[AIRPLANE_CENTER]);
	glPointSize(5.0);
	glDrawArrays(GL_POINTS, 32, 1);
	glPointSize(1.0);
	glBindVertexArray(0);
}

//house
#define HOUSE_ROOF 0
#define HOUSE_BODY 1
#define HOUSE_CHIMNEY 2
#define HOUSE_DOOR 3
#define HOUSE_WINDOW 4

GLfloat roof[3][2] = { { -12.0, 0.0 },{ 0.0, 12.0 },{ 12.0, 0.0 } };
GLfloat house_body[4][2] = { { -12.0, -14.0 },{ -12.0, 0.0 },{ 12.0, 0.0 },{ 12.0, -14.0 } };
GLfloat chimney[4][2] = { { 6.0, 6.0 },{ 6.0, 14.0 },{ 10.0, 14.0 },{ 10.0, 2.0 } };
GLfloat door[4][2] = { { -8.0, -14.0 },{ -8.0, -8.0 },{ -4.0, -8.0 },{ -4.0, -14.0 } };
GLfloat window[4][2] = { { 4.0, -6.0 },{ 4.0, -2.0 },{ 8.0, -2.0 },{ 8.0, -6.0 } };

GLfloat house_color[5][3] = {
	{ 200 / 255.0f, 39 / 255.0f, 42 / 255.0f },
	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
	{ 255 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 233 / 255.0f, 113 / 255.0f, 23 / 255.0f },
	{ 44 / 255.0f, 180 / 255.0f, 49 / 255.0f }
};

GLuint VBO_house, VAO_house;
void prepare_house() {
	GLsizeiptr buffer_size = sizeof(roof) + sizeof(house_body) + sizeof(chimney) + sizeof(door)
		+ sizeof(window);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_house);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_house);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(roof), roof);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof), sizeof(house_body), house_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body), sizeof(chimney), chimney);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body) + sizeof(chimney), sizeof(door), door);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(roof) + sizeof(house_body) + sizeof(chimney) + sizeof(door),
		sizeof(window), window);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_house);
	glBindVertexArray(VAO_house);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_house);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_house() {
	glBindVertexArray(VAO_house);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_ROOF]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 3);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 3, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_CHIMNEY]);
	glDrawArrays(GL_TRIANGLE_FAN, 7, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_DOOR]);
	glDrawArrays(GL_TRIANGLE_FAN, 11, 4);

	glUniform3fv(loc_primitive_color, 1, house_color[HOUSE_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glBindVertexArray(0);
}


////draw cocktail
//#define COCKTAIL_NECK 0
//#define COCKTAIL_LIQUID 1
//#define COCKTAIL_REMAIN 2
//#define COCKTAIL_STRAW 3
//#define COCKTAIL_DECO 4
//
//GLfloat neck[6][2] = { { -6.0, -12.0 },{ -6.0, -11.0 },{ -1.0, 0.0 },{ 1.0, 0.0 },{ 6.0, -11.0 },{ 6.0, -12.0 } };
//GLfloat liquid[6][2] = { { -1.0, 0.0 },{ -9.0, 4.0 },{ -12.0, 7.0 },{ 12.0, 7.0 },{ 9.0, 4.0 },{ 1.0, 0.0 } };
//GLfloat remain[4][2] = { { -12.0, 7.0 },{ -12.0, 10.0 },{ 12.0, 10.0 },{ 12.0, 7.0 } };
//GLfloat straw[4][2] = { { 7.0, 7.0 },{ 12.0, 12.0 },{ 14.0, 12.0 },{ 9.0, 7.0 } };
//GLfloat deco[8][2] = { { 12.0, 12.0 },{ 10.0, 14.0 },{ 10.0, 16.0 },{ 12.0, 18.0 },{ 14.0, 18.0 },{ 16.0, 16.0 },{ 16.0, 14.0 },{ 14.0, 12.0 } };
//
//GLfloat cocktail_color[5][3] = {
//	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
//	{ 0 / 255.0f, 63 / 255.0f, 122 / 255.0f },
//	{ 235 / 255.0f, 225 / 255.0f, 196 / 255.0f },
//	{ 191 / 255.0f, 255 / 255.0f, 0 / 255.0f },
//	{ 218 / 255.0f, 165 / 255.0f, 32 / 255.0f }
//};
//
//GLuint VBO_cocktail, VAO_cocktail;
//void prepare_cocktail() {
//	GLsizeiptr buffer_size = sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw)
//		+ sizeof(deco);
//
//	// Initialize vertex buffer object.
//	glGenBuffers(1, &VBO_cocktail);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
//	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory
//
//	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(neck), neck);
//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck), sizeof(liquid), liquid);
//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid), sizeof(remain), remain);
//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain), sizeof(straw), straw);
//	glBufferSubData(GL_ARRAY_BUFFER, sizeof(neck) + sizeof(liquid) + sizeof(remain) + sizeof(straw),
//		sizeof(deco), deco);
//
//	// Initialize vertex array object.
//	glGenVertexArrays(1, &VAO_cocktail);
//	glBindVertexArray(VAO_cocktail);
//
//	glBindBuffer(GL_ARRAY_BUFFER, VBO_cocktail);
//	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
//
//	glEnableVertexAttribArray(0);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);
//	glBindVertexArray(0);
//}
//
//void draw_cocktail() {
//	glBindVertexArray(VAO_cocktail);
//
//	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_NECK]);
//	glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
//
//	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_LIQUID]);
//	glDrawArrays(GL_TRIANGLE_FAN, 6, 6);
//
//	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_REMAIN]);
//	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);
//
//	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_STRAW]);
//	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);
//
//	glUniform3fv(loc_primitive_color, 1, cocktail_color[COCKTAIL_DECO]);
//	glDrawArrays(GL_TRIANGLE_FAN, 20, 8);
//
//	glBindVertexArray(0);
//}

//draw car2
#define CAR2_BODY 0
#define CAR2_FRONT_WINDOW 1
#define CAR2_BACK_WINDOW 2
#define CAR2_FRONT_WHEEL 3
#define CAR2_BACK_WHEEL 4
#define CAR2_LIGHT1 5
#define CAR2_LIGHT2 6

GLfloat car2_body[8][2] = { { -18.0, -7.0 },{ -18.0, 0.0 },{ -13.0, 0.0 },{ -10.0, 8.0 },{ 10.0, 8.0 },{ 13.0, 0.0 },{ 18.0, 0.0 },{ 18.0, -7.0 } };
GLfloat car2_front_window[4][2] = { { -10.0, 0.0 },{ -8.0, 6.0 },{ -2.0, 6.0 },{ -2.0, 0.0 } };
GLfloat car2_back_window[4][2] = { { 0.0, 0.0 },{ 0.0, 6.0 },{ 8.0, 6.0 },{ 10.0, 0.0 } };
GLfloat car2_front_wheel[8][2] = { { -11.0, -11.0 },{ -13.0, -8.0 },{ -13.0, -7.0 },{ -11.0, -4.0 },{ -7.0, -4.0 },{ -5.0, -7.0 },{ -5.0, -8.0 },{ -7.0, -11.0 } };
GLfloat car2_back_wheel[8][2] = { { 7.0, -11.0 },{ 5.0, -8.0 },{ 5.0, -7.0 },{ 7.0, -4.0 },{ 11.0, -4.0 },{ 13.0, -7.0 },{ 13.0, -8.0 },{ 11.0, -11.0 } };
GLfloat car2_light1[3][2] = { { -18.0, -1.0 },{ -17.0, -2.0 },{ -18.0, -3.0 } };
GLfloat car2_light2[3][2] = { { -18.0, -4.0 },{ -17.0, -5.0 },{ -18.0, -6.0 } };

GLfloat car2_color[7][3] = {
	{ 100 / 255.0f, 141 / 255.0f, 159 / 255.0f },
	{ 235 / 255.0f, 219 / 255.0f, 208 / 255.0f },
	{ 235 / 255.0f, 219 / 255.0f, 208 / 255.0f },
	{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f }
};

GLuint VBO_car2, VAO_car2;
void prepare_car2() {
	GLsizeiptr buffer_size = sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel) + sizeof(car2_light1) + sizeof(car2_light2);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car2_body), car2_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body), sizeof(car2_front_window), car2_front_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window), sizeof(car2_back_window), car2_back_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window), sizeof(car2_front_wheel), car2_front_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel),
		sizeof(car2_back_wheel), car2_back_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel), sizeof(car2_light1), car2_light1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car2_body) + sizeof(car2_front_window) + sizeof(car2_back_window) + sizeof(car2_front_wheel)
		+ sizeof(car2_back_wheel) + sizeof(car2_light1), sizeof(car2_light2), car2_light2);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car2);
	glBindVertexArray(VAO_car2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car2);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_car2() {
	glBindVertexArray(VAO_car2);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_FRONT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_BACK_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 8);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT1]);
	glDrawArrays(GL_TRIANGLE_FAN, 32, 3);

	glUniform3fv(loc_primitive_color, 1, car2_color[CAR2_LIGHT2]);
	glDrawArrays(GL_TRIANGLE_FAN, 35, 3);

	glBindVertexArray(0);
}

//car
#define CAR_BODY 0
#define CAR_FRAME 1
#define CAR_WINDOW 2
#define CAR_LEFT_LIGHT 3
#define CAR_RIGHT_LIGHT 4
#define CAR_LEFT_WHEEL 5
#define CAR_RIGHT_WHEEL 6

GLfloat car_body[4][2] = { { -16.0, -8.0 },{ -16.0, 0.0 },{ 16.0, 0.0 },{ 16.0, -8.0 } };
GLfloat car_frame[4][2] = { { -10.0, 0.0 },{ -10.0, 10.0 },{ 10.0, 10.0 },{ 10.0, 0.0 } };
GLfloat car_window[4][2] = { { -8.0, 0.0 },{ -8.0, 8.0 },{ 8.0, 8.0 },{ 8.0, 0.0 } };
GLfloat car_left_light[4][2] = { { -9.0, -6.0 },{ -10.0, -5.0 },{ -9.0, -4.0 },{ -8.0, -5.0 } };
GLfloat car_right_light[4][2] = { { 9.0, -6.0 },{ 8.0, -5.0 },{ 9.0, -4.0 },{ 10.0, -5.0 } };
GLfloat car_left_wheel[4][2] = { { -10.0, -12.0 },{ -10.0, -8.0 },{ -6.0, -8.0 },{ -6.0, -12.0 } };
GLfloat car_right_wheel[4][2] = { { 6.0, -12.0 },{ 6.0, -8.0 },{ 10.0, -8.0 },{ 10.0, -12.0 } };

GLfloat car_color[7][3] = {
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 0 / 255.0f, 149 / 255.0f, 159 / 255.0f },
	{ 216 / 255.0f, 208 / 255.0f, 174 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 249 / 255.0f, 244 / 255.0f, 0 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f },
	{ 21 / 255.0f, 30 / 255.0f, 26 / 255.0f }
};

GLuint VBO_car, VAO_car;
void prepare_car() {
	GLsizeiptr buffer_size = sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel) + sizeof(car_right_wheel);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(car_body), car_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body), sizeof(car_frame), car_frame);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame), sizeof(car_window), car_window);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window), sizeof(car_left_light), car_left_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light),
		sizeof(car_right_light), car_right_light);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light), sizeof(car_left_wheel), car_left_wheel);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(car_body) + sizeof(car_frame) + sizeof(car_window) + sizeof(car_left_light)
		+ sizeof(car_right_light) + sizeof(car_left_wheel), sizeof(car_right_wheel), car_right_wheel);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_car);
	glBindVertexArray(VAO_car);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_car);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_car() {
	glBindVertexArray(VAO_car);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_FRAME]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_WINDOW]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_LIGHT]);
	glDrawArrays(GL_TRIANGLE_FAN, 16, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_LEFT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 20, 4);

	glUniform3fv(loc_primitive_color, 1, car_color[CAR_RIGHT_WHEEL]);
	glDrawArrays(GL_TRIANGLE_FAN, 24, 4);

	glBindVertexArray(0);
}


// sword
#define SWORD_BODY 0
#define SWORD_BODY2 1
#define SWORD_HEAD 2
#define SWORD_HEAD2 3
#define SWORD_IN 4
#define SWORD_DOWN 5
#define SWORD_BODY_IN 6

GLfloat sword_body[4][2] = { { -6.0, 0.0 },{ -6.0, -4.0 },{ 6.0, -4.0 },{ 6.0, 0.0 } };
GLfloat sword_body2[4][2] = { { -2.0, -4.0 },{ -2.0, -6.0 } ,{ 2.0, -6.0 },{ 2.0, -4.0 } };
GLfloat sword_head[4][2] = { { -2.0, 0.0 },{ -2.0, 16.0 } ,{ 2.0, 16.0 },{ 2.0, 0.0 } };
GLfloat sword_head2[3][2] = { { -2.0, 16.0 },{ 0.0, 19.46 } ,{ 2.0, 16.0 } };
GLfloat sword_in[4][2] = { { -0.3, 0.7 },{ -0.3, 15.3 } ,{ 0.3, 15.3 },{ 0.3, 0.7 } };
GLfloat sword_down[4][2] = { { -2.0, -6.0 } ,{ 2.0, -6.0 },{ 4.0, -8.0 },{ -4.0, -8.0 } };
GLfloat sword_body_in[4][2] = { { 0.0, -1.0 } ,{ 1.0, -2.732 },{ 0.0, -4.464 },{ -1.0, -2.732 } };

GLfloat sword_color[7][3] = {
	{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 155 / 255.0f, 155 / 255.0f, 155 / 255.0f },
{ 0 / 255.0f, 0 / 255.0f, 0 / 255.0f },
{ 139 / 255.0f, 69 / 255.0f, 19 / 255.0f },
{ 255 / 255.0f, 0 / 255.0f, 0 / 255.0f }
};

GLuint VBO_sword, VAO_sword;

void prepare_sword() {
	for (int i = 0; i <= 8; ++i) {
		swords.addNewSword(-304 + i * 76, true);
		swords.addNewSword(-304 + i * 76, false);
	}
	GLsizeiptr buffer_size = sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down) + sizeof(sword_body_in);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sword_body), sword_body);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body), sizeof(sword_body2), sword_body2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2), sizeof(sword_head), sword_head);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head), sizeof(sword_head2), sword_head2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2), sizeof(sword_in), sword_in);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in), sizeof(sword_down), sword_down);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(sword_body) + sizeof(sword_body2) + sizeof(sword_head) + sizeof(sword_head2) + sizeof(sword_in) + sizeof(sword_down), sizeof(sword_body_in), sword_body_in);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_sword);
	glBindVertexArray(VAO_sword);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_sword);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_sword() {
	glBindVertexArray(VAO_sword);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY2]);
	glDrawArrays(GL_TRIANGLE_FAN, 4, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_BODY_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 23, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 8, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_HEAD2]);
	glDrawArrays(GL_TRIANGLE_FAN, 12, 3);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_IN]);
	glDrawArrays(GL_TRIANGLE_FAN, 15, 4);

	glUniform3fv(loc_primitive_color, 1, sword_color[SWORD_DOWN]);
	glDrawArrays(GL_TRIANGLE_FAN, 19, 4);

	glBindVertexArray(0);
}


//road
#define ROAD_ROAD 0
#define ROAD_LINE1 1
#define ROAD_LINE2 2
#define ROAD_LINE3 3
GLfloat road[4][2] = { {-1000.0, -80.0}, {-1000.0, -350.0}, {1000.0, -350.0}, {1000.0, -80.0} };
GLfloat line1[2][2] = { {-1000.0, -147.5}, {1000.0, -147.5} };
GLfloat line2[2][2] = { {-1000.0, -215.0}, {1000.0, -215.0} };
GLfloat line3[2][2] = { {-1000.0, -282.5}, {1000.0, -282.5} };

GLfloat road_color[4][3] = {
	{ 104 / 255.0f, 108 / 255.0f,  94 / 255.0f }, //road
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }, //line1
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }, //line2
	{ 235 / 255.0f, 236 / 255.0f, 240 / 255.0f }  //line3
};

GLuint VBO_road, VAO_road;

void prepare_road() {
	//int size = sizeof(tree_1)+sizeof(tree_2);
	GLsizeiptr buffer_size = sizeof(road) + sizeof(line1) + sizeof(line2) + sizeof(line3);

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO_road);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_road);
	glBufferData(GL_ARRAY_BUFFER, buffer_size, NULL, GL_STATIC_DRAW); // allocate buffer object memory

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(road), road);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(road), sizeof(line1), line1);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(road) + sizeof(line1), sizeof(line2), line2);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(road) + sizeof(line1) + sizeof(line2), sizeof(line3), line3);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO_road);
	glBindVertexArray(VAO_road);

	glBindBuffer(GL_ARRAY_BUFFER, VBO_road);
	glVertexAttribPointer(LOC_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void draw_road() {
	glBindVertexArray(VAO_road);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_ROAD]);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	glLineWidth(8.0);
	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_LINE1]);
	glDrawArrays(GL_LINES, 4, 2);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_LINE2]);
	glDrawArrays(GL_LINES, 6, 8);

	glUniform3fv(loc_primitive_color, 1, road_color[ROAD_LINE3]);
	glDrawArrays(GL_LINES, 8, 10);
	glLineWidth(1.0);

	glBindVertexArray(0);
}