//=============================================================================================
// Mintaprogram: Zöld háromszög. Ervenyes 2018. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Gyõri Kristóf
// Neptun : HV0R9S
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"
class Hermit {
	vec3 p0;	// First Point
	vec3 p1;	// Last Point
	std::vector<vec3> cps;
	float t0 = 0;
	float t1 = 1;
public:	
	void AddPoints(vec3 add) {
		cps.push_back(add);
	}

	vec3 r(float t) {
		p0 = cps[0];
		p1 = cps[1];
		vec3 a0 = p0;														// First point: p0
		vec3 v0 = p1 - p0;														// r'(t0)
		vec3 v1 = p1; // a3*((t - t0)*(t - t0)) * 3 + a2*((t - t0)) * 2 + a1;		// r'(t1)
		
		return getHermit(p0, v0, t0, p1, v1, t1, t);
	}

	vec3 getHermit(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1 - p0) * 3)*(1.0 / ((t1 - t0)*(t1 - t0)))) - ((v1 + (v0 * 2))*(1.0 / (t1 - t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt = (a3*((t - t0)*(t - t0)*(t - t0))) + (a2*((t - t0)*(t - t0))) + (a1*((t - t0))) + a0;
		return rt;
	}
};
class CatmullRom {
private: 
	std::vector<vec3> cps;	// Control Points
	std::vector<float> ts;	// parameter (knot) values 
	int controlPointsNum = 5;
	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1-p0)*3)*(1.0/((t1-t0)*(t1-t0)))) - ((v1 + (v0*2))*(1.0/(t1-t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt =(a3*((t - t0)*(t - t0)*(t - t0))) + (a2*((t - t0)*(t - t0))) + (a1*((t - t0))) + a0;
		return rt;
	}

	vec3 v(int i) {
		if (i == 0 || i == controlPointsNum - 1) {
			return cps[i];
		}

		else {
			vec3 tag1 = (cps[i + 1] - cps[i])*(1.0 / (ts[i + 1] - ts[i]));
			printf("tag1 = (%lf, %lf)\n", tag1.x, tag1.y);
			vec3 tag2 = (cps[i] - cps[i - 1])*(1.0 / (ts[i] - ts[i - 1]));
			printf("tag2 = (%lf, %lf)\n", tag2.x, tag2.y);
			vec3 vi = (tag1 + tag2)*(1.0 / 2.0);
			printf("(%lf, %lf)\n", vi.x, vi.y);
			return vi;
		}
	}

public:
	void AddControlPoint(vec3 cp, float f) {
		cps.push_back(cp);
		ts.push_back(f);
	}

	vec3 r(float t) {
		
		for (int i = 0; i < cps.size() - 1; i++) {
			if (ts[i] <= t && t <= ts[i + 1]) {
				vec3 v0 = cps[0];
				vec3 v1 = cps[1];
				//printf("cps[%d]=(%lf, %lf)\n", i, cps[i].x, cps[i].y);
				//printf("cps[%d]=(%lf, %lf)\n", i+1, cps[i+1].x, cps[i+1].y);
				return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
			}
		}
	}

	void ToString() {
		for (vec3 i : cps) {
			printf("%lf");
		}
	}
};
CatmullRom catmullRom;
Hermit hermit;
int clickNum = 0;

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(0, 1, 0, 1);	// computed color is the color of the primitive
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders
unsigned int vao;	   // virtual world on the GPU
bool val = false;
std::vector<float> vert;

int vertexNum = 0;
void DrawSpline() {
	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	unsigned int vbo;		// vertex buffer object
	glGenBuffers(1, &vbo);	// Generate 1 buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// Geometry with 24 bytes (6 floats or 3 x 2 coordinates)
	//float vertices[] = { -0.8f, -0.8f, -0.6f, 1.0f, 0.8f, -0.2f };
	float* vertices = new float[12];
	// float vertices[] = { -0.73, 0.63,-0.77, 0.67, -0.77, 0.67 , -0.64, 0.60, -0.64, 0.60,  -0.44, 0.49, -0.44, 0.49, -0.27, 0.41, -0.22, 0.42 };
	int i = 0;
	for (float t = 0; t <= 1.0; t += 0.2) {
		printf("%lf, %lf\n", hermit.r(t).x, hermit.r(t).y);
		
		vertices[i++] = hermit.r(t).x;
		vertices[i++] = hermit.r(t).y;
		printf("vertices[i]=%lf, %lf\n", vertices[i-2], vertices[i-1]);
	}
	
	//float* vertices = &vert[0];
	glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
		sizeof(vertices),			// # bytes
		vertices,	      			// address
		GL_STATIC_DRAW);	// we do not change later

	glEnableVertexAttribArray(0);  // AttribArray 0			--> COORDS
	glVertexAttribPointer(0,       // vbo -> AttribArray 0
		2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
		0, NULL); 		     // stride, offset: tightly packed

							 // create program for the GPU
	gpuProgram.Create(vertexSource, fragmentSource, "outColor");
	delete[] vertices;
}

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	//DrawSpline();
	
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	// Set color to (0, 1, 0) = green
	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, 0.0f, 1.0f, 0.0f); // 3 floats

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
		                      0, 1, 0, 0,    // row-major!
		                      0, 0, 1, 0,
		                      0, 0, 0, 1 };

	location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	glBindVertexArray(vao);  // Draw call
	//glDrawArrays(GL_TRIANGLES, 0 /*startIdx*/, 3 /*# Elements*/);
	glDrawArrays(GL_LINE_STRIP, 0 /*startIdx*/, 6 /*# Elements*/);

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: 
		buttonStat = "pressed";
		printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);
		clickNum++;
		if (clickNum <= 2) {
			hermit.AddPoints(vec3(pX/1000.0, pY/1000.0, 0));
			if (clickNum == 2) {
				DrawSpline();
			}
		}
		break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   
		//printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   
		
		break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}

