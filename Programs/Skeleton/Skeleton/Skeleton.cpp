//=============================================================================================
// Triangle with smooth color and interactive polyline 
//=============================================================================================
//#include "MyClasses.h"

#include "framework.h"

class Hermit {
	vec3 p0;	// First Point
	vec3 p1;	// Last Point
	std::vector<vec4> cps;
	float t0 = 0;
	float t1 = 1;
	vec2   wTranslate;
public:
	mat4 Minv() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-wTranslate.x, -wTranslate.y, 0, 1); // model matrix
	}
	void AddPoints(int cX, int cY) {
		//vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv() * Minv();
		//cps.push_back(wVertex);
	}
	vec3 r(float t) {
		p0 = vec3(cps[0].x, cps[0].y, 0);
		p1 = vec3(cps[1].x, cps[1].y, 0);
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

class Triangle {
	unsigned int vao;	// vertex array object id
	float sx, sy;		// scaling
	vec2 wTranslate;	// translation
	float phi;
public:
	Triangle() { Animate(0); }

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		unsigned int vbo[2];		// vertex buffer objects
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

									// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		float vertexCoords[] = { -8, -8,  -6, 10,  8, -2 };	// vertex data on the CPU
		glBufferData(GL_ARRAY_BUFFER,      // copy to the GPU
			sizeof(vertexCoords), // number of the vbo in bytes
			vertexCoords,		   // address of the data array on the CPU
			GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
								   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0,			// Attribute Array 0
			2, GL_FLOAT,  // components/attribute, component type
			GL_FALSE,		// not in fixed point format, do not normalized
			0, NULL);     // stride and offset: it is tightly packed

						  // vertex colors: vbo[1] -> Attrib Array 1 -> vertexColor of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		float vertexColors[] = { 1, 0, 0,  0, 1, 0,  0, 0, 1 };	// vertex data on the CPU
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexColors), vertexColors, GL_STATIC_DRAW);	// copy to the GPU

																							// Map Attribute Array 1 to the current bound vertex buffer (vbo[1])
		glEnableVertexAttribArray(1);  // Vertex position
									   // Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}

	void Animate(float t) {
		sx = 1;
		sy = 1;
		wTranslate = vec2(0, 0);
		phi = t;
	}

	mat4 M() {
		mat4 Mscale(sx, 0, 0, 0,
			0, sy, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 1); // model matrix

		mat4 Mrotate(cosf(phi), sinf(phi), 0.0f, 0.0f,
			-sinf(phi), cosf(phi), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		mat4 Mtranslate(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 0,
			wTranslate.x, wTranslate.y, 0, 1); // model matrix

		return Mscale * Mrotate * Mtranslate;
	}

	void Draw() {
		// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
		mat4 MVPTransform = M() * camera.V() * camera.P();
		MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

		glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
		glDrawArrays(GL_TRIANGLES, 0, 3);	// draw a single triangle with vertices defined in vao
	}
};

class LineStrip {
	GLuint vao, vbo;        // vertex array object, vertex buffer object
	float  vertexData[100]; // interleaved data of coordinates and colors
	int    nVertices;       // number of vertices
	vec2   wTranslate;
public:
	LineStrip() {
		nVertices = 0;
	}

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

	

};

class Hermite : public DrawableObject {
private:
	std::vector<GLfloat> vertexDatas;
	std::vector<vec3> controlPoints;
	vec3 r0;	// Start
	vec3 r1;	// End
	float t0	;
	float t1;
	vec3 v0;
	vec3 v1;
public: 
	Hermite() {
		t0 = 0;
		t1 = 2;
	}
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo); // Generate 1 vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0
		glEnableVertexAttribArray(1);  // attribute array 1
									   // Map attribute array 0 to the vertex data of the interleaved vbo
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)); // attribute array, components/attribute, component type, normalize?, stride, offset
																										// Map attribute array 1 to the color data of the interleaved vbo
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
	}
	
	// Kontrolpontok hozzáadása
	void AddCps(vec3 point) {
		controlPoints.push_back(point);

		if (controlPoints.size() == 2) {
			
			r0 = controlPoints[0];
			r1 = controlPoints[1];
			//v1 = r1;
		}
	}

	void AddPoint(float cX, float cY) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//if (nVertices >= 20) return;

		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv() * Minv();
		// fill interleaved data
		vertexDatas.push_back(wVertex.x);
		vertexDatas.push_back(wVertex.y);
		vertexDatas.push_back(1);
		vertexDatas.push_back(0.5);
		vertexDatas.push_back(0);
		nVertices++;
		// copy data to the GPU
		glBufferData(GL_ARRAY_BUFFER, nVertices * 5 * sizeof(float), &vertexDatas[0], GL_STATIC_DRAW);

		printf("Coords: %lf, %lf\n", cX, cY);
	}
	bool ok = true;
	void Draw() {	

		if (nVertices > 0 || controlPoints.size() >= 2) {
			if (ok) {
				for (float t = t0; t <= t1; t += 0.05) {
					vec3 c = getHermit(r0, v0, t0, r1, v1, t1, t);
					AddPoint(c.x, c.y);
				}
				ok = false;
			}
			
			
			// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
			mat4 MVPTransform = M() * camera.V() * camera.P();
			MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

			glBindVertexArray(vao);
			glDrawArrays(GL_LINE_STRIP, 0, nVertices);		
		}
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
		//printf("tag1 = (%lf, %lf)\n", tag1.x, tag1.y);
		vec3 tag2 = (cps[i] - cps[i - 1])*(1.0 / (ts[i] - ts[i - 1]));
		//printf("tag2 = (%lf, %lf)\n", tag2.x, tag2.y);
		vec3 vi = (tag1 + tag2)*(1.0 / 2.0);
		//printf("(%lf, %lf)\n", vi.x, vi.y);
		return vi;
	}
public: 
	void Create() {
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo); // Generate 1 vertex buffer object
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0
		glEnableVertexAttribArray(1);  // attribute array 1
									   // Map attribute array 0 to the vertex data of the interleaved vbo
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(0)); // attribute array, components/attribute, component type, normalize?, stride, offset
																										// Map attribute array 1 to the color data of the interleaved vbo
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
	}

	void AddPoint(float cX, float cY) {
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		//if (nVertices >= 20) return;

		vec4 wVertex = vec4(cX, cY, 0, 1) * camera.Pinv() * camera.Vinv() * Minv();
		// fill interleaved data
		vertexDatas.push_back(wVertex.x);
		vertexDatas.push_back(wVertex.y);
		vertexDatas.push_back(1);
		vertexDatas.push_back(0.5);
		vertexDatas.push_back(0);
		nVertices++;
		// copy data to the GPU
		glBufferData(GL_ARRAY_BUFFER, nVertices * 5 * sizeof(float), &vertexDatas[0], GL_STATIC_DRAW);

		printf("Coords: %lf, %lf\n", cX, cY);
	}
	void AddControlPoints(vec3 cp, float t) {
		cps.push_back(cp);
		ts.push_back(t);
	}
	
	vec3 r(float t) {
		for (int i = 0; i < cps.size() - 1 ; i++)
		{
			if (ts[i] <= t && t <= ts[i + 1]) {		 
				if (i == 0) {
					v0 = cps[1];
					v1 = v(i + 1);
				}
				else {
					v0 = v(i);
					if (i == cps.size() - 2) {
						v1 = cps[i + 1] - cps[i];
					}
					else
					{
						v1 = v(i + 1);
					}
					
				}
				return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);	
			}
		}
	}

	bool ok = true;
	void Draw() {

		if (cps.size() >= 4) {
			if (ok) {
				for (float t = ts[0]; t <= ts.size() - 1; t += 0.05) {
					vec3 c = r(t);
					AddPoint(c.x, c.y);
				}
				ok = false;
			}
		}
		if (nVertices > 0) {
			

				// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
				mat4 MVPTransform = M() * camera.V() * camera.P();
				MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

				glBindVertexArray(vao);
				glDrawArrays(GL_LINE_STRIP, 0, nVertices);
			}
		}
			
	
};

// The virtual world: collection of two objects
//Triangle triangle;
CatmullRom lineStrip;

// Initialization, create an OpenGL context
void onInitialization() {
	// Position and size of the photograph on screen
	glViewport(0, 0, windowWidth, windowHeight);

	// Width of lines in pixels
	glLineWidth(2.0f);

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

	//triangle.Draw();
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

int i = 0;
// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / windowHeight;
		if (i < 4) {
			lineStrip.AddControlPoints(vec3(cX, cY, 0), (float)(i));
			i++;
		}
		printf("Mouse coords: %lf, %lf", cX, cY);
		//lineStrip.AddPoint(cX, cY);
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
