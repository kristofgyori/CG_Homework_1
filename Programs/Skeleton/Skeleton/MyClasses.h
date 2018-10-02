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

class CatmullRom {
private:
	std::vector<vec3> cps;	// Control Points
	std::vector<float> ts;	// parameter (knot) values 
	int controlPointsNum = 5;
	vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
		vec3 a0 = p0;
		vec3 a1 = v0;
		vec3 a2 = (((p1 - p0) * 3)*(1.0 / ((t1 - t0)*(t1 - t0)))) - ((v1 + (v0 * 2))*(1.0 / (t1 - t0)));
		vec3 a3 = (((p0 - p1) * 2)*(1.0 / ((t1 - t0)*(t1 - t0)*(t1 - t0)))) + ((v1 + (v0))*(1.0 / ((t1 - t0)*(t1 - t0))));

		vec3 rt = (a3*((t - t0)*(t - t0)*(t - t0))) + (a2*((t - t0)*(t - t0))) + (a1*((t - t0))) + a0;
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


