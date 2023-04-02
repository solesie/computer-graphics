#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#define TO_RADIAN		0.017453292519943296 

int rightbuttonpressed = 0;
int leftbuttonpressed = 0, center_selected = 0;

float r, g, b; // Background color
float px, py, qx, qy; // Line (px, py) --> (qx, qy)
int n_object_points = 6;
float object[6][2], object_center_x, object_center_y;
float rotation_angle_in_degree;
int window_width, window_height;

void convert_window_coord_to_openGL_coord(int x, int y, float* converted_x, float* converted_y) {
	float ex = (float)window_width / 2;
	float ey = (float)window_height / 2;
	*converted_x = (x - ex) / 250;
	*converted_y = (-y + ey) / 250;
}

/* xy좌표계를 그리기 위해 필요하다. */
void draw_axes() {
	glLineWidth(3.0f);
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(1.0f, 0.0f);
	glVertex2f(0.975f, 0.025f);
	glVertex2f(1.0f, 0.0f);
	glVertex2f(0.975f, -0.025f);
	glVertex2f(1.0f, 0.0f);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glVertex2f(0.0f, 1.0f);
	glVertex2f(0.025f, 0.975f);
	glVertex2f(0.0f, 1.0f);
	glVertex2f(-0.025f, 0.975f);
	glVertex2f(0.0f, 1.0f);
	glEnd();
	glLineWidth(1.0f);

	glPointSize(7.0f);
	glBegin(GL_POINTS);
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glEnd();
	glPointSize(1.0f);
}

/* 직선을 그리기 위해 필요하다. */
void draw_line(float px, float py, float qx, float qy) {
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2f(px, py); 
	glVertex2f(qx, qy);
	glEnd();
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex2f(px, py);
	glColor3f(1.0f, 1.0f, 1.0f);
	glVertex2f(qx, qy);
	glEnd();
	glPointSize(1.0f);
}

/* 육각형을 그리기 위해 필요하다. */
void draw_object(void) {
	glBegin(GL_LINE_LOOP);
	glColor3f(0.0f, 1.0f, 0.0f);
	for (int i = 0; i < 6; i++)
		glVertex2f(object[i][0], object[i][1]);
	glEnd();
	glPointSize(5.0f);
	glBegin(GL_POINTS);
	glColor3f(1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 6; i++)
		glVertex2f(object[i][0], object[i][1]);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex2f(object_center_x, object_center_y);
	glEnd();
	glPointSize(1.0f);
}

void display(void) {
	glClearColor(r, g, b, 1.0f); 
	glClear(GL_COLOR_BUFFER_BIT);

	draw_axes();
	draw_line(px, py, qx, qy);
	draw_object();
	glFlush();
}

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 'r':
		r = 1.0f; g = b = 0.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case 'g':
		g = 1.0f; r = b = 0.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case 'b':
		b = 1.0f; r = g = 0.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case 's':
		r = 250.0f / 255.0f, g = 128.0f / 255.0f, b = 114.0f / 255.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case 'q':
		glutLeaveMainLoop(); 
		break;
	}
}

void special(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_LEFT:
		r -= 0.1f;
		if (r < 0.0f) r = 0.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		r += 0.1f;
		if (r > 1.0f) r = 1.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		g -= 0.1f;
		if (g < 0.0f) g = 0.0f;
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		g += 0.1f;
		if (g > 1.0f) g = 1.0f;  
		fprintf(stdout, "$$$ The new window background color is (%5.3f, %5.3f, %5.3f).\n", r, g, b);
		glutPostRedisplay();
		break;
	}
}

int prevx, prevy;
void mousepress(int button, int state, int x, int y) {
	//motion callback에서 어느 기준으로 마우스를 움직이고 있는지 알 필요가 있다.
	prevx = x;
	prevy = y;
	if (state == GLUT_UP) {
		switch (button) {
		case GLUT_LEFT_BUTTON:{
			leftbuttonpressed = 0;
			break;
		}
		case GLUT_RIGHT_BUTTON: {
			rightbuttonpressed = 0;
			break;
		}
		case MOUSE_WHEELED: {
			//원점 이동
			float delx = -px;
			float dely = -py;
			float tempqx = qx + delx;
			float tempqy = qy + dely;
			//회전
			tempqx = tempqx * cos(-rotation_angle_in_degree * TO_RADIAN)
				- tempqy * sin(-rotation_angle_in_degree * TO_RADIAN);
			tempqy = tempqx * sin(-rotation_angle_in_degree * TO_RADIAN)
				+ tempqy * cos(-rotation_angle_in_degree * TO_RADIAN);
			//원위치
			qx = tempqx - delx;
			qy = tempqy - dely;
			glutPostRedisplay();
			break;
		}
		case MOUSE_WHEELED - 1: {//휠을 위로 회전하는 경우를 나타낸다.
			//원점 이동
			float delx = -px;
			float dely = -py;
			float tempqx = qx + delx;
			float tempqy = qy + dely;
			//회전
			tempqx = tempqx * cos(rotation_angle_in_degree * TO_RADIAN)
				- tempqy * sin(rotation_angle_in_degree * TO_RADIAN);
			tempqy = tempqx * sin(rotation_angle_in_degree * TO_RADIAN)
				+ tempqy * cos(rotation_angle_in_degree * TO_RADIAN);
			//원위치
			qx = tempqx - delx;
			qy = tempqy - dely;
			glutPostRedisplay();
			break;
		}
		}
	}

	if (state == GLUT_DOWN) {
		int active_key = glutGetModifiers();
		switch (button) {
		case GLUT_LEFT_BUTTON: {
			leftbuttonpressed = 1;
			if (active_key == GLUT_ACTIVE_CTRL) {
				for (int i = 0; i < 6; ++i) {
					float temp = object[i][0];
					object[i][0] = object[i][1];
					object[i][1] = temp;
				}
				object_center_x = object_center_y = 0.0f;
				for (int i = 0; i < n_object_points; i++) {
					object_center_x += object[i][0];
					object_center_y += object[i][1];
				}
				object_center_x /= n_object_points;
				object_center_y /= n_object_points;
				glutPostRedisplay();
			}
			if (active_key == GLUT_ACTIVE_ALT) {
				float temp = px;
				px = py;
				py = temp;

				temp = qx;
				qx = qy;
				qy = temp;

				glutPostRedisplay();
			}
			break;
		}
		case GLUT_RIGHT_BUTTON:{
			rightbuttonpressed = 1;
			break;
		}
		}
	}
}

void mousemove(int x, int y) {
	float converted_x, converted_y;
	float converted_prevx, converted_prevy;
	convert_window_coord_to_openGL_coord(x, y, &converted_x, &converted_y);
	convert_window_coord_to_openGL_coord(prevx, prevy, &converted_prevx, &converted_prevy);
	int active_key = glutGetModifiers();

	if (active_key == GLUT_ACTIVE_SHIFT && leftbuttonpressed) {
		if ((converted_prevx <= px + 0.02 && px - 0.02 <= converted_prevx)
			&& (converted_prevy <= py + 0.02 && py - 0.02 <= converted_prevy)) {
			px = converted_x;
			py = converted_y;
			//계속 누르고 있는 상태이므로 클릭을 시작한 위치를 갱신해야 한다.
			prevx = x;
			prevy = y;
			glutPostRedisplay();
		}
	}

	if (active_key == GLUT_ACTIVE_ALT && rightbuttonpressed) {
		float delx = converted_x - converted_prevx;
		float dely = converted_y - converted_prevy;
		object[0][0] += delx;
		object[0][1] += dely;
		object[1][0] += delx;
		object[1][1] += dely;
		object[2][0] += delx;
		object[2][1] += dely;
		object[3][0] += delx;
		object[3][1] += dely;
		object[4][0] += delx;
		object[4][1] += dely;
		object[5][0] += delx;
		object[5][1] += dely;
		object_center_x = object_center_y = 0.0f;
		for (int i = 0; i < n_object_points; i++) {
			object_center_x += object[i][0];
			object_center_y += object[i][1];
		}
		object_center_x /= n_object_points;
		object_center_y /= n_object_points;

		//계속 누르고 있는 상태이므로 클릭을 시작한 위치를 갱신해야 한다.
		prevx = x;
		prevy = y;
		glutPostRedisplay();
	}

	if (active_key == GLUT_ACTIVE_CTRL && rightbuttonpressed) {
		float delx = converted_x - converted_prevx;
		//delx가 음수인 경우 1보다 작은수 양수를, 양수인 경우 1보다 큰 양수를 얻어야한다.
		float s = delx + 1;
		while (s < 0) {
			++s;
		}

		object[0][0] = s * (object[0][0] - object_center_x) + object_center_x;
		object[0][1] = s * (object[0][1] - object_center_y) + object_center_y;
		object[1][0] = s * (object[1][0] - object_center_x) + object_center_x;
		object[1][1] = s * (object[1][1] - object_center_y) + object_center_y;
		object[2][0] = s * (object[2][0] - object_center_x) + object_center_x;
		object[2][1] = s * (object[2][1] - object_center_y) + object_center_y;
		object[3][0] = s * (object[3][0] - object_center_x) + object_center_x;
		object[3][1] = s * (object[3][1] - object_center_y) + object_center_y;
		object[4][0] = s * (object[4][0] - object_center_x) + object_center_x;
		object[4][1] = s * (object[4][1] - object_center_y) + object_center_y;
		object[5][0] = s * (object[5][0] - object_center_x) + object_center_x;
		object[5][1] = s * (object[5][1] - object_center_y) + object_center_y;

		//계속 누르고 있는 상태이므로 클릭을 시작한 위치를 갱신해야 한다.
		prevx = x;
		prevy = y;
		glutPostRedisplay();
	}
}
	
void reshape(int width, int height) {
	// DO NOT MODIFY THIS FUNCTION!!!
	window_width = width, window_height = height;
	glViewport(0.0f, 0.0f, window_width, window_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-window_width / 500.0f, window_width / 500.0f,  -window_height / 500.0f, window_height / 500.0f, -1.0f, 1.0f);

	glutPostRedisplay();
}


void close(void) {
	fprintf(stdout, "\n^^^ The control is at the close callback function now.\n\n");
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mousepress);
	glutMotionFunc(mousemove);
	glutReshapeFunc(reshape);
 	glutCloseFunc(close);
}

void initialize_renderer(void) {
	register_callbacks();
	r = 250.0f / 255.0f, g = 128.0f / 255.0f, b = 114.0f / 255.0f; // Background color = Salmon
	px = -1.0f, py = 0.20f, qx = -0.6f, qy = 0.20f;
	rotation_angle_in_degree = 1.0f; // 1 degree
	
	float sq_cx = 0.55f, sq_cy = -0.45f, sq_side = 0.15f;
	object[0][0] = sq_cx + sq_side;
	object[0][1] = sq_cy + sq_side;
	object[1][0] = sq_cx;
	object[1][1] = sq_cy + 1.5 * sq_side;
	object[2][0] = sq_cx - sq_side;
	object[2][1] = sq_cy + sq_side;
	object[3][0] = sq_cx - sq_side;
	object[3][1] = sq_cy - sq_side;
	object[4][0] = sq_cx + 0.2 * sq_side;
	object[4][1] = sq_cy - 0.8 * sq_side;
	object[5][0] = sq_cx + sq_side;
	object[5][1] = sq_cy - sq_side;
	object_center_x = object_center_y = 0.0f;
	for (int i = 0; i < n_object_points; i++) {
		object_center_x += object[i][0];
		object_center_y += object[i][1];
	}
	object_center_x /= n_object_points;
	object_center_y /= n_object_points;
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = TRUE;
	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void greetings(char *program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 4
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 Simple 2D Transformations";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'r', 'g', 'b', 's', 'q'",
		"    - Special keys used: LEFT, RIGHT, UP, DOWN",
		"    - Mouse used: SHIFT/L-click and move, ALT/R-click and move, CTRL/R-click and move",
		"    - Wheel used: up and down scroll"
		"    - Other operations: window size change"
	};

	glutInit(&argc, argv);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE); // <-- Be sure to use this profile for this example code!
 //	glutInitContextProfile(GLUT_CORE_PROFILE);

	glutInitDisplayMode(GLUT_RGBA);

	glutInitWindowSize(750, 750);
	glutInitWindowPosition(500, 200);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

   // glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_EXIT); // default
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	
	glutMainLoop();
	fprintf(stdout, "^^^ The control is at the end of main function now.\n\n");
}