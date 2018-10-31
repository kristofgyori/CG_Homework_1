//=============================================================================================
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

#pragma region GLSL
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

// vertex shader in GLSL
const char * textureVertexSource = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = vertexUV;														// copy texture coordinates
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

// fragment shader in GLSL
const char * textureFragmentSource = R"(
	#version 330
    precision highp float;

	uniform sampler2D textureUnit;
	// uniform int isGPUProcedural;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 fragmentC;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
			fragmentC = texture(textureUnit, texCoord);
	}
)";
#pragma endregion


// 2D camera
class Camera2D {
	vec2 wCenter; // center in world coordinates
	vec2 wSize;   // width and height in world coordinates
public:
	Camera2D() : wCenter(0, 0), wSize(20, 20) { }

	mat4 V() { return TranslateMatrix(-wCenter); }
	mat4 P() { return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y)); }

	mat4 Vinv() { return TranslateMatrix(wCenter); }
	mat4 Pinv() { return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2)); }

	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }
};

Camera2D camera;				// 2D camera
GPUProgram gpuProgram;			// vertex and fragment shaders
GPUProgram gPUTextureProgram;	// Shaders program for texture


class CatmullRomSpline {
	GLuint vao;					// vertex array object, vertex buffer object
	unsigned int vbo[2];		// vertex buffer objects

	std::vector<float> colorDatas;
	std::vector<vec3> cps;
	std::vector<float> ts;

	vec2 wTranslate;
	const float epsilon = 0.0001;

public: 
	std::vector<float>  dVertexData;	// derivated vertex 
	std::vector<float>  vertexData;		// interleaved data of coordinates
	std::vector<vec3> getCps() {
		return cps;
	}
	
	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1 - p0) * 3)*(1.0 / ((t1 - t0)*(t1 - t0)))) - ((v1 + (v0 * 2))*(1.0 / (t1 - t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt = (a3*((t - t0)*(t - t0)*(t - t0))) + (a2*((t - t0)*(t - t0))) + (a1*((t - t0))) + a0;
		return rt;
	}

	vec3 DHermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1 - p0) * 3)*(1.0 / ((t1 - t0)*(t1 - t0)))) - ((v1 + (v0 * 2))*(1.0 / (t1 - t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt = (a3*(t - t0)*(t - t0)*3) + (a2*(t - t0)*2) + a1;
		return rt;
	}

	vec3 v(int i) {
		vec3 tag1 = (cps[i + 1] - cps[i])*(1.0 / (ts[i + 1] - ts[i]));
		vec3 tag2 = (cps[i] - cps[i - 1])*(1.0 / (ts[i] - ts[i - 1]));
		vec3 vi = (tag1 + tag2)*(1.0 / 2.0);
		return vi;
	}

	
	void AddControlPoint(vec3 cp) { 
		vec4 wVertex = vec4(cp.x, cp.y, 0, 1) * camera.Pinv() * camera.Vinv() * Minv();
		cps.push_back(vec3(wVertex.x, wVertex.y));
		ts.push_back(cps.size() - 1);
		if (cps.size() > 1) 
		ReDraw();
	}

	vec3 r(float t, bool dr = false) {
		vec3 v0;
		vec3 v1;
		
		if (cps.size() == 0 || cps.size() == 1) {
			return vec3(0, 0, 0);
		}

		for (int i = 0; i < cps.size() - 1; i++) {
			if (ts[i] - epsilon <= t  && t <= ts[i + 1] + epsilon) {		
				if (cps.size() == 2 || i == 0) {
					if (ts[0] - epsilon <= t && t <= ts[1] + epsilon) {
						v0 = cps[1] - cps[0];
						v1 = -cps[1] + v0;
						if (!dr)
						return Hermite(cps[0], v0, ts[0], cps[1], v1, ts[1], t);
						else 
						return DHermite(cps[0], v0, ts[0], cps[1], v1, ts[1], t);
					}
				}
				else {
					
					// The first control point
					if (i == 0) {
						v0 = cps[1] - cps[0];            // Init in manual way the first v(0) vector
						v1 = v(i + 1);
					}
					else {
						v0 = v(i);

						// The last control point
						if (i == cps.size() - 2) {
							v1 = cps[i + 1] - cps[i];    // Init in manual way the last v(n) vector
						}
						else
							v1 = v(i + 1);
					}
					if (!dr)
						return Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
					else
						return DHermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], t);
					
				}
			}
		}
		return vec3(0, 0, 0);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		
		glGenBuffers(2, &vbo[0]);	// Generate 2 vertex buffer objects

		// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		
		// Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
	
		// Data organization of Attribute Array 0 
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,	0, NULL);     

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glEnableVertexAttribArray(1);		   // Vertex position
        
		// Data organization of Attribute Array 1
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); // Attribute Array 1, components/attribute, component type, normalize?, tightly packed
	}

	mat4 M() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1); // translation
	}
	mat4 Minv() {
		return mat4(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1); // inverse translation
	}

	void ClearDatas() {
		vertexData.clear();
		colorDatas.clear();
		dVertexData.clear();
	}
	void ReDraw() {
		ClearDatas();
		float tstep = 0.05;

		for (float t = ts[0]; t < ts.size() - 1; t += tstep) {
			vec3 rt = r(t);
			vertexData.push_back(rt.x);
			vertexData.push_back(rt.y);
			colorDatas.push_back(0);
			colorDatas.push_back(0);
			colorDatas.push_back(1);

			vec3 drdt = r(t, true);
			dVertexData.push_back(drdt.x);
			dVertexData.push_back(drdt.y);
		}
		
		// copy data to the GPU
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), &vertexData[0], GL_DYNAMIC_DRAW);

		// copy colors data to the GPU
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, colorDatas.size() * sizeof(float), &colorDatas[0], GL_DYNAMIC_DRAW);



	}
	void Draw() {
		int ns = vertexData.size()/2;
		if (ns > 2) {
			glBindVertexArray(vao);
			// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
			glUseProgram(gpuProgram.getId());
			mat4 MVPTransform = M() * camera.V() * camera.P();
			MVPTransform.SetUniform(gpuProgram.getId(), "MVP");

			//glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source
			glDrawArrays(GL_LINE_STRIP, 0, ns);	// draw a single triangle with vertices defined in vao
		}
	}
};
CatmullRomSpline catmull;

bool started = false;
static int step = 0;
class MyTrainTexture {
	unsigned int vao, vbo[2];
	vec2 vertices[4], uvs[4];
	Texture * pTexture;
	vec2 wTranslate;	// translation
	
	float phi;
	
	float h = 0;
public:
	MyTrainTexture() {
		vertices[0] = vec2(-1, -1); uvs[0] = vec2(0, 0);
		vertices[1] = vec2(1, -1);  uvs[1] = vec2(1, 0);
		vertices[2] = vec2(1, 1);   uvs[2] = vec2(1, 1);
		vertices[3] = vec2(-1, 1);  uvs[3] = vec2(0, 1);

		wTranslate = vec2(5, -2);
	}

	void Create() {
		glGenVertexArrays(1, &vao);	// create 1 vertex array object
		glBindVertexArray(vao);		// make it active

		glGenBuffers(2, vbo);	// Generate 1 vertex buffer objects

								// vertex coordinates: vbo[0] -> Attrib Array 0 -> vertexPosition of the vertex shader
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);	   // copy to that part of the memory which will be modified 
																					   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // make it active, it is an array
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);	   // copy to that part of the memory which is not modified 
																			   // Map Attribute Array 0 to the current bound vertex buffer (vbo[0])
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);     // stride and offset: it is tightly packed

		int width = 128, height = 128;
		float a = width/2;
		float b = a/2 - 20;
		
		float p1 = 0.05;
		float p2 = 0.03;
		float u = 64;
		float v2 = 40;
		h = 2.0/32;

		std::vector<vec4> image(width * height);
		
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				
				float parabola1 = (1.0 / 2 * p1)*pow(x - u, 2);
				float parabola2 = (1.0 / 2 * p2)*pow(x - u, 2) + v2;
			
				if (y >= parabola1 && y <= parabola2) {
					image[y * width + x] = vec4(1, 0.9, 0.1, 1);
				}
				else {
					image[y * width + x] = vec4(0, 0, 0, 0);
				}
			}
		}
		pTexture = new Texture(width, height, image);
	}

	mat4 M() {
		
		mat4 Mtranslate(1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 0,
			wTranslate.x, wTranslate.y, 0, 1); // translation

		mat4 Mrotate(cosf(phi), sinf(phi), 0, 0,
			-sinf(phi), cosf(phi), 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1); // rotation

		return  Mtranslate*Mrotate;	// model transformation
	}
	void Draw() {
		if (started) {
			glBindVertexArray(vao);	// make the vao and its vbos active playing the role of the data source

			mat4 MVPTransform = M() * camera.V() * camera.P();
			glUseProgram(gPUTextureProgram.getId());
			// set GPU uniform matrix variable MVP with the content of CPU variable MVPTransform
			MVPTransform.SetUniform(gPUTextureProgram.getId(), "MVP");

			pTexture->SetUniform(gPUTextureProgram.getId(), "textureUnit");
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);	// draw two triangles forming a quad
			glDisable(GL_BLEND);
		}
	}
	
	
	void Animate() {	
		if (step < catmull.dVertexData.size()) {

			float Tx = catmull.dVertexData[step];		
			float rx = catmull.vertexData[step++];
			float Ty = catmull.dVertexData[step];
			float ry = catmull.vertexData[step++];
			
			wTranslate.x = rx - (Ty*h);
			wTranslate.y = ry + (Tx*h);

		}
		else {
			step = 0;
			
		}
	}
	void SetTranslate(vec2 pos) {
		wTranslate = pos;
	}
};

MyTrainTexture texture;
// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight); 	// Position and size of the photograph on screen
	glLineWidth(4.0f); // Width of lines in pixels

	// Create objects by setting up their vertex data on the GPU
	catmull.Create();
	texture.Create();

	// create program for the GPU
	gpuProgram.Create(vertexSource, fragmentSource, "fragmentColor");
	gPUTextureProgram.Create(textureVertexSource, textureFragmentSource, "fragmentC");

	printf("\nUsage: \n");
	printf("Mouse Left Button: Add control point to polyline\n");
	printf("Key 's': Camera pan -x\n");
	printf("Key 'd': Camera pan +x\n");
	printf("Key 'x': Camera pan -y\n");
	printf("Key 'e': Camera pan +y\n");
	printf("Key 'z': Camera zoom in\n");
	printf("Key 'Z': Camera zoom out\n");
	printf("Key 'j': Line strip move -x\n");
	printf("Key 'k': Line strip move +x\n");
	printf("Key 'm': Line strip move -y\n");
	printf("Key 'i': Line strip move +y\n");
}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);							// background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
	


	catmull.Draw();
	texture.Draw();
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
	case 32: 
		started = true;
		break;
	}
	glutPostRedisplay();
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {  // GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
		float cY = 1.0f - 2.0f * pY / windowHeight;
		
		catmull.AddControlPoint(vec3(cX, cY));
		
		glutPostRedisplay();     // redraw
	}
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {
}

// Idle event indicating that some time elapsed: do animation here
int ti = 0;
void onIdle() {
	
	if (started && ti++ % 100 == 0) {
		texture.Animate();
	}

	glutPostRedisplay();					// redraw the scene
}
