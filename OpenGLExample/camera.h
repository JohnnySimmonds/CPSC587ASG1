#ifndef CAMERA_H
#define CAMERA_H


#include "glm/glm.hpp"
#include <cstdio>

using namespace glm;

class Camera{
public:
	vec3 dir;
	vec3 right;
	vec3 up;

	vec3 pos;

	Camera():	dir(vec3(0, 0, -1)), right(vec3(1, 0, 0)), up(vec3(0, 1, 0)), 
				pos(vec3(0, 0, 1.f)){}

	Camera(vec3 _dir, vec3 _pos):dir(_dir), pos(_pos){
		right = normalize(cross(dir, vec3(0, 1, 0)));
		up = normalize(cross(right, dir));
	}

	void trackballUp(float radians);
	void trackballRight(float radians);
	void zoom(float factor);
	mat4 getMatrix();
};

#endif