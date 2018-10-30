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
//=============================================================================================
// Triangle with smooth color and interactive polyline 
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL
const char * vertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1
	out vec3 color;									// output attribute

	void main() {
		color = vertexColor;														// copy color from input to output
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * fragmentSource = R"(
	#version 330
    precision highp float;

	in vec3 color;				// variable input: interpolated color of vertex shader
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";

// 2D camera
class Camera2D {
	vec2 wCenter; // center in world coordinates
	vec2 wSize;   // width and height in world coordinates

public:
	Camera2D() : wCenter(0, 0), wSize(20, 20) { }

	mat4 V() { return TranslateMatrix(-wCenter); }

	mat4 P() { // projection matrix: 
		return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y));
	}

	mat4 Vinv() { return TranslateMatrix(wCenter); }

	mat4 Pinv() { // inverse projection matrix
		return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2));
	}

	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }
};

// 2D camera
Camera2D camera;
GPUProgram gpuProgram; // vertex and fragment shaders
class DrawableObject {

protected:
	GLuint vao, vbo;        // vertex array object, vertex buffer object
	float*  vertexData; // interleaved data of coordinates and colors
	int    nVertices = 0;       // number of vertices
	vec2   wTranslate;

public:
	mat4 M() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			wTranslate.x, wTranslate.y, 0, 1); // model matrix
	}
	mat4 Minv() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-wTranslate.x, -wTranslate.y, 0, 1); // model matrix
	}

	void AddTranslation(vec2 wT) { wTranslate = wTranslate + wT; }

	virtual void Create() = 0;

	virtual void AddPoint(float cX, float cY) = 0;

	virtual void Draw() {
		if (nVertices > 0) {
			// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
			mat4 MVPTransform = M() * camera.V() * camera.P();
			MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

			glBindVertexArray(vao);
			glDrawArrays(GL_LINE_STRIP, 0, nVertices);
		}
	}
};

class CatmullRom : public DrawableObject {
	std::vector<vec3> cps;
	std::vector<float> ts;
	std::vector<GLfloat> vertexDatas;

	vec3 v0;
	vec3 v1;

	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1 - p0) * 3)*(1.0 / ((t1 - t0)*(t1 - t0)))) - ((v1 + (v0 * 2))*(1.0 / (t1 - t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt = (a3*((t - t0)*(t - t0)*(t - t0))) + (a2*((t - t0)*(t - t0))) + (a1*((t - t0))) + a0;
		return rt;
	}

	vec3 v(int i) {
		vec3 tag1 = (cps[i + 1] - cps[i])*(1.0 / (ts[i + 1] - ts[i]));
		vec3 tag2 = (cps[i] - cps[i - 1])*(1.0 / (ts[i] - ts[i - 1]));
		vec3 vi = (tag1 + tag2)*(1.0 / 2.0);
		return vi;
	}
public:
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		
		glGenBuffers(2, &vbo); // Generate 1 vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0
		glEnableVertexAttribArray(1);  // attribute array 1
									   // Map attribute array 0 to the vertex data of the interleaved vbo
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)); // attribute array, components/attribute, component type, normalize?, stride, offset
																										// Map attribute array 1 to the color data of the interleaved vbo
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

	}

	void FlushVertexData() {
			vertexDatas.clear();
			nVertices = 0;
		
	}
	void AddPoint(float cX, float cY) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv() * Minv();
		// fill interleaved data
		vertexDatas.push_back(wVertex.x);
		vertexDatas.push_back(wVertex.y);
		vertexDatas.push_back(1);
		vertexDatas.push_back(0.25);
		vertexDatas.push_back(0.1);
		nVertices++;

		copyDataToGPU();
	}

	void copyDataToGPU() {
		// copy data to the GPU
		glBufferData(GL_ARRAY_BUFFER, nVertices * 5 * sizeof(float), &vertexDatas[0], GL_DYNAMIC_DRAW);
	}
	void AddControlPoints(vec3 cp, float t) {
		cps.push_back(cp);
		ts.push_back(t);
	}

	vec3 r(float t) {
		for (int i = 0; i < cps.size() - 1; i++)
		{
			if (cps.size() == 0 || cps.size() == 1) return NULL;
			else if (ts[i] <= t && t <= ts[i + 1]) {

				// The first control point
				if (i == 0) {
					v0 = cps[1] - cps[0];			// Init in manual way the first v(0) vector
					if (cps.size() > 2) {
						v1 = v(i + 1);
					}
					else {
						v1 = vec3(-1, -1);					// Linear spline if there are 2 cps 
					}

				}
				else {
					v0 = v(i);
					// The last control point
					if (i >= cps.size() - 2) {
						v1 = cps[i + 1] - cps[i];	// Init in manual way the last v(n) vector
					}
					else
						v1 = v(i + 1);
				}

				
				return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
			}
		}
	}
	bool ok = true;
	

	
	void Draw() {	
		
		if (ts.size() != 0) {
			for (float t = ts[0]; t < ts.size(); t += 0.01) {
				vec3 c = r(t);
				AddPoint(c.x, c.y);
			}
		}

		if (nVertices > 0) {
			// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
			mat4 MVPTransform = M() * camera.V() * camera.P();
			MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

			glBindVertexArray(vao);
			glDrawArrays(GL_POINTS, 0, nVertices);
		}
	}

	void SayVertexPoints() {
		printf(" ------------------- \n");
	}
};

// The virtual world: collection of two objects
CatmullRom lineStrip;

// Initialization, create an OpenGL context
void onInitialization() {
	// Position and size of the photograph on screen
	glViewport(0, 0, windowWidth, windowHeight);

	// Width of lines in pixels
	glLineWidth(4.0f);
	glPointSize(4.0f);

	// Create objects by setting up their vertex data on the GPU
	lineStrip.Create();
	//triangle.Create();

	// create program for the GPU
	gpuProgram.Create(vertexSource, fragmentSource, "fragmentColor");

	printf("\nUsage: \n");
	printf("Mouse Left Button: Add control point to polyline\n");
	printf("Key 's': Camera pan -x\n");
	printf("Key 'd': Camera pan +x\n");
	printf("Key 'x': Camera pan -y\n");
	printf("Key 'e': Camera pan +y\n");
	printf("Key 'z': Camera zoom in\n");
	printf("Key 's': Camera zoom out\n");
	printf("Key 'j': Line strip move -x\n");
	printf("Key 'k': Line strip move +x\n");
	printf("Key 'm': Line strip move -y\n");
	printf("Key 'i': Line strip move +y\n");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen


	lineStrip.Draw();

	glutSwapBuffers();									// exchange the two buffers
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	switch (key) {
	case 's': camera.Pan(vec2(-1, 0)); break;
	case 'd': camera.Pan(vec2(+1, 0)); break;
	case 'e': camera.Pan(vec2(0, 1)); break;
	case 'x': camera.Pan(vec2(0, -1)); break;
	case 'z': camera.Zoom(0.9f); break;
	case 'Z': camera.Zoom(1.1f); break;
	case 'j': lineStrip.AddTranslation(vec2(-1, 0)); break;
	case 'k': lineStrip.AddTranslation(vec2(+1, 0)); break;
	case 'i': lineStrip.AddTranslation(vec2(0, 1)); break;
	case 'm': lineStrip.AddTranslation(vec2(0, -1)); break;
	}
	glutPostRedisplay();
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

static float i = 0.0f;		// NumberOfClicks
// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / windowHeight;
		lineStrip.FlushVertexData();
		lineStrip.AddControlPoints(vec3(cX, cY, 0), i);
		i++;
		glutPostRedisplay();     // redraw
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	float sec = time / 1000.0f;				// convert msec to sec

											//triangle.Animate(sec);					// animate the triangle object

	glutPostRedisplay();					// redraw the scene
}
