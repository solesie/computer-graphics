#include "Simple2DTransformation.h"

int crushed_car_clock_limit;
int gameover_sword_clock_limit;
int gameover_human_clock_limit;
std::vector<float> rotation_swords_limits;
std::vector<float> rotation_swords_count;
std::vector<int> moving_swords_count;

void display(void) {
	glm::mat4 ModelMatrix;
	glClear(GL_COLOR_BUFFER_BIT);

	ModelMatrix = glm::mat4(1.0f);
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_road();
	
	//draw airplane
	int airplane_x = (timestamp % 3202) / 2 - 800; // -800 <= airplane_x <= 800
	if (airplane_x <= 200) {
		//y=0.09x+250 을 따라 비행한다.
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(airplane_x, 0.09 * airplane_x + 250, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, 90 * TO_RADIAN + atanf(0.09), glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.5, 1.5, 1.0f));
	}
	else {
		//y=-0.5x+368 을 따라 추락한다.
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(airplane_x, -0.5 * airplane_x + 368, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, (airplane_x % 360) * 5 * TO_RADIAN, glm::vec3(0.0f, 0.0f, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(300.0 / airplane_x, 300.0 / airplane_x, 1.0f));
	}
	ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
	glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
	draw_airplane();
	
	if (timestamp % 50 == 0) {
		cars.addNewCar();
	}
	cars.removeOutOfBoundCars();
	if (!gameover) {
		//draw house
		house_x = (timestamp % 4802) / 2 - 1200; // -1200 <= house_x <= 1200
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2 * (float)house_x, -10.0, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(80, 5, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_house();

		//draw human
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(human_cur_x, -90 - (human_cur_line * 67.5), 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.9, 0.9, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_human_without_foots();
		//draw human foots
		int foot_clock = timestamp % 120;
		if (foot_clock < 60) {
			draw_human_right_foot();
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(human_cur_x, -90 - (human_cur_line * 67.5) + 0.09 * foot_clock, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.9, 0.9, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_human_left_foot();
		}
		else {
			draw_human_left_foot();
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(human_cur_x, -90 - (human_cur_line * 67.5) + 0.09 * (foot_clock - 60), 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.9, 0.9, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_human_right_foot();
		}

		//draw cars
		std::vector<CarInfoForDrawing> carsInfoForDrawing = cars.getCarsInfoForDrawing();
		for (int i = 0; i < carsInfoForDrawing.size(); ++i) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(carsInfoForDrawing.at(i).getX(), carsInfoForDrawing.at(i).getY(), 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0, 2.0, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			carsInfoForDrawing.at(i).getCarType() == CAR1_TYPE ? draw_car() : draw_car2();
		}

		//draw swords
		std::vector<SwordInfoForDrawing> swordInfoForDrawing = swords.getSwordsInfoForDrawing();
		for (int i = 0; i < swordInfoForDrawing.size(); ++i) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(swordInfoForDrawing.at(i).getX(), swordInfoForDrawing.at(i).getY(), 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.2, 6.5, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_sword();
		}

		gameover = cars.haveCrushedCar();
	}
	else {
		//draw stopped house
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2 * (float)house_x, -10.0, 0.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(80, 5, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_house();

		//draw non-crushed cars
		std::vector<CarInfoForDrawing> carsInfoForDrawing = cars.getCarsInfoForDrawing();
		for (int i = 0; i < carsInfoForDrawing.size(); ++i) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(carsInfoForDrawing.at(i).getX(), carsInfoForDrawing.at(i).getY(), 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0, 2.0, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			carsInfoForDrawing.at(i).getCarType() == CAR1_TYPE ? draw_car() : draw_car2();
		}

		//draw crushed car
		if (crushedCar == nullptr) {
			crushedCar = cars.getcrushedCar();
			cars.removeCrushedCar();
		}

		if (crushed_car_clock_limit < 180) {
			++crushed_car_clock_limit;
		}
		if (crushedCar->getCarType() == CAR1_TYPE) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(crushedCar->getX() + crushed_car_clock_limit, crushedCar->getY() + 100 * sinf(crushed_car_clock_limit * TO_RADIAN), 0.0f));
			ModelMatrix = glm::rotate(ModelMatrix, crushed_car_clock_limit * TO_RADIAN, glm::vec3(0.0, 0.0, 1.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0, 2.0, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_car();
		}
		if (crushedCar->getCarType() == CAR2_TYPE) {
			ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(crushedCar->getX() + (float)crushed_car_clock_limit / 2, crushedCar->getY() - (float)crushed_car_clock_limit / 2, 0.0f));
			ModelMatrix = glm::scale(ModelMatrix, glm::vec3(sinf(47.3 * crushed_car_clock_limit * TO_RADIAN), 2.0, 1.0f));
			ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
			glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
			draw_car2();
		}

		//draw gameover sword
		if (gameoveredSwords.empty()) {
			gameoveredSwords = swords.getGameoveredSwords();
		}
		if (gameover_sword_clock_limit < 1000) {
			++gameover_sword_clock_limit;
		}
		//draw stopped sword
		if (gameover_sword_clock_limit < 100) {
			for (int i = 0; i < gameoveredSwords.size(); ++i) {
				ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(gameoveredSwords.at(i)->getX(), gameoveredSwords.at(i)->getY(), 0.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.2, 6.5, 1.0f));
				ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
				glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
				draw_sword();
			}
		}
		//draw rotating sword
		else if (gameover_sword_clock_limit < 400) {
			//draw rotate sword
			float human_x = human_cur_x;
			float human_y = -90 - (human_cur_line * 67.5);
			if (rotation_swords_limits.empty()) {
				for (int i = 0; i < gameoveredSwords.size(); ++i) {
					float sword_x = gameoveredSwords.at(i)->getX();
					float sword_y = gameoveredSwords.at(i)->getY();
					float rotation_angle_sword = atanf((sword_y - human_y) / (sword_x - human_x));
					rotation_angle_sword += rotation_angle_sword < 0 ? 270 * TO_RADIAN : 90 * TO_RADIAN;
					rotation_swords_limits.push_back(rotation_angle_sword * TO_DEGREE);
				}
				rotation_swords_count = std::vector<float>(gameoveredSwords.size(), 0.0);
			}
			for (int i = 0; i < gameoveredSwords.size(); ++i) {
				float sword_x = gameoveredSwords.at(i)->getX();
				float sword_y = gameoveredSwords.at(i)->getY();
				if (rotation_swords_count.at(i) < rotation_swords_limits.at(i))
					++rotation_swords_count.at(i);
				ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sword_x, sword_y, 0.0f));
				ModelMatrix = glm::rotate(ModelMatrix, rotation_swords_count.at(i) * TO_RADIAN, glm::vec3(0.0, 0.0, 1.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.2, 6.5, 1.0f));
				ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
				glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
				draw_sword();
			}
		}
		//draw moving sword
		else if (gameover_sword_clock_limit <= 1000) {
			float human_x = human_cur_x;
			float human_y = -90 - (human_cur_line * 67.5);
			if (moving_swords_count.empty()) {
				moving_swords_count = std::vector<int>(gameoveredSwords.size(), 0);
			}
			for (int i = 0; i < gameoveredSwords.size(); ++i) {
				float sword_x = gameoveredSwords.at(i)->getX();
				float sword_y = gameoveredSwords.at(i)->getY();
				if (rotation_swords_limits.at(i) >= 180.0 && sword_x + moving_swords_count.at(i) < human_x) {
					moving_swords_count.at(i) += 4;
				}
				if (180 > rotation_swords_limits.at(i) && rotation_swords_limits.at(i) >= 90.0 && sword_x + moving_swords_count.at(i) > human_x) {
					moving_swords_count.at(i) -= 4;
				}
				
				float new_x = sword_x + moving_swords_count.at(i);
				float new_y = (sword_y - human_y) / (sword_x - human_x) * (new_x - human_x) + human_y;
				ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(new_x, new_y, 0.0f));
				ModelMatrix = glm::rotate(ModelMatrix, rotation_swords_limits.at(i) * TO_RADIAN, glm::vec3(0.0, 0.0, 1.0f));
				ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.2, 6.5, 1.0f));
				ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
				glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
				draw_sword();
			}
		}


		//draw gameover human
		if (gameover_human_clock_limit < 90) {
			++gameover_human_clock_limit;
		}
		ModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(human_cur_x, -90 - (human_cur_line * 67.5), 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, gameover_human_clock_limit * TO_RADIAN, glm::vec3(0.0, 0.0, 1.0f));
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.9, 0.9, 1.0f));
		ModelViewProjectionMatrix = ViewProjectionMatrix * ModelMatrix;
		glUniformMatrix4fv(loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);
		draw_human_without_foots();
		draw_human_right_foot();
		draw_human_left_foot();
	}
	
	glFlush();	
}   

void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27: // ESC key
		glutLeaveMainLoop(); // Incur destuction callback for cleanups.
		break;
	}
}

void special(int key, int x, int y) {
	if (gameover) {
		return;
	}
	switch (key) {
	case GLUT_KEY_LEFT:
		if (-500 < human_cur_x) {
			human_cur_x -= 10;
			human_cur_leftest -= 10;
			human_cur_rightest -= 10;
		}
		break;
	case GLUT_KEY_RIGHT:
		if (human_cur_x < 500) {
			human_cur_x += 10;
			human_cur_leftest += 10;
			human_cur_rightest += 10;
		}
		break;
	case GLUT_KEY_DOWN:
		if (human_cur_line < 3 && human_cur_line >= 0) {
			++human_cur_line;
		}
		break;
	case GLUT_KEY_UP:
		if (human_cur_line <= 3 && human_cur_line > 0) {
			--human_cur_line;
		}
		break;
	}
}

int leftbuttonpressed = 0;
void mouse(int button, int state, int x, int y) {
	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN))
		leftbuttonpressed = 1;
	else if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP))
		leftbuttonpressed = 0;
}

void motion(int x, int y) {
	static int delay = 0;
	static float tmpx = 0.0, tmpy = 0.0;
	float dx, dy;
	if (leftbuttonpressed) {
		centerx =  x - win_width/2.0f, centery = (win_height - y) - win_height/2.0f;
		if (delay == 8) {	
			dx = centerx - tmpx;
			dy = centery - tmpy;
	  
			if (dx > 0.0) {
				rotate_angle = atan(dy / dx) + 90.0f*TO_RADIAN;
			}
			else if (dx < 0.0) {
				rotate_angle = atan(dy / dx) - 90.0f*TO_RADIAN;
			}
			else if (dx == 0.0) {
				if (dy > 0.0) rotate_angle = 180.0f*TO_RADIAN;
				else  rotate_angle = 0.0f;
			}
			tmpx = centerx, tmpy = centery; 
			delay = 0;
		}
		glutPostRedisplay();
		delay++;
	}
} 
	
void reshape(int width, int height) {
	win_width = width, win_height = height;
	
  	glViewport(0, 0, win_width, win_height);
	ProjectionMatrix = glm::ortho(-win_width / 2.0, win_width / 2.0, 
		-win_height / 2.0, win_height / 2.0, -1000.0, 1000.0);
	ViewProjectionMatrix = ProjectionMatrix * ViewMatrix;

	update_axes();

	glutPostRedisplay();
}

void cleanup(void) {
	glDeleteVertexArrays(1, &VAO_axes);
	glDeleteBuffers(1, &VBO_axes);

	glDeleteVertexArrays(1, &VAO_airplane);
	glDeleteBuffers(1, &VBO_airplane);
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutReshapeFunc(reshape);
	glutTimerFunc(10, timer, 0);
	glutCloseFunc(cleanup);
}

void prepare_shader_program(void) {
	ShaderInfo shader_info[3] = {
		{ GL_VERTEX_SHADER, "Shaders/simple.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/simple.frag" },
		{ GL_NONE, NULL }
	};

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
}

void initialize_OpenGL(void) {
	glEnable(GL_MULTISAMPLE); 
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	glClearColor(220 / 255.0f, 20 / 255.0f, 60 / 255.0f, 1.0f);
	ViewMatrix = glm::mat4(1.0f);
}

void prepare_scene(void) {
	prepare_axes();
	prepare_human();
	prepare_airplane();
	prepare_house();
	prepare_car2();
	prepare_car();
	prepare_sword();
	prepare_road();
}

void initialize_renderer(void) {
	register_callbacks();
	prepare_shader_program(); 
	initialize_OpenGL();
	prepare_scene();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

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

#define N_MESSAGE_LINES 1
void main(int argc, char *argv[]) {
	char program_name[64] = "Sogang CSE4170 Simple2DTransformation_GLSL_3.0";
	char messages[N_MESSAGE_LINES][256] = {
		"    - Keys used: 'ESC', four arrows"
	};

	glutInit (&argc, argv);
 	glutInitDisplayMode(GLUT_RGBA | GLUT_MULTISAMPLE);
	glutInitWindowSize (1200, 800);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop ();
}


