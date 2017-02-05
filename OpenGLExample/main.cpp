// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include <cstdlib>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// specify that we want the OpenGL core profile before including GLFW headers
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "camera.h"

#define PI 3.14159265359

using namespace std;
using namespace glm;


//Forward definitions
bool CheckGLErrors(string location);
void QueryGLVersion();
string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
void animate(vec3 cartLoc, int &i, vector<vec3> points, float ds);
vector<vec3> subdivision(vector<vec3> points, vector<unsigned int>* indices, vector<vec3>* normals);

void generateSquareXYZCoords(vector<vec3>* vertices, vector<vec3>* normals, 
					vector<unsigned int>* indices);

vec3 archLength(vec3 Bt, int& i, vector<vec3> points, float Ds);
vec3 posOnCurve(vec3 Bt, int &i, vector<vec3> points, float ds);
vec3 tangentTemp(vec3 nextPos, vec3 currPos);
vec3 curvature(vec3 nextPos, vec3 currPos, float ds);
vec3 normal(vec3 cA, vec3 gravity);
vec3 binormal(vec3 normal, vec3 tangent);
mat4 freFrame(vec3 N, vec3 B, vec3 T);
vec3 binormalAtCurrPoint(vec3 nextPos, vec3 currPos, vec3 prevPos);
void createTrack (vector<vec3> points);
float getLength(vec3 v);
int wrap(int i);

int highestPointIndex, lowestPointIndex, decIndex;

vec2 mousePos;
bool leftmousePressed = false;
bool rightmousePressed = false;
bool play = false;

float H;
float dt = 0.04;
float h;
float v;
float ds = 0.009;
float prevT = glfwGetTime();
float distLow;

bool lifting = true;
bool gravityFree = false;
bool decel = false;


struct VertexBuffers{
	enum{ VERTICES=0, NORMALS, INDICES, COUNT};

	GLuint id[COUNT];
};

GLuint vao;
GLuint vaoLine; //vertex array object for the line.
GLuint vaoPos;
GLuint vaoNeg;

VertexBuffers vbo;
VertexBuffers vboLine;//vertex buffer object for the line
VertexBuffers vboNeg;
VertexBuffers vboPos;

//Geometry information
vector<vec3> points, normals, linePoints, lineNormal, XYZPoints, XYZNormals;
vector<vec3> negRail, posRail, posNorm, negNorm; 
vector<unsigned int> indices, lineIndices, XYZIndices, negIndices, posIndices;
vec3 gravity = vec3(0.0f, 9.81f, 0.0f);

Camera* activeCamera;

GLFWwindow* window = 0;

float low;
mat4 winRatio = mat4(1.f);
mat4 M =  mat4(1.f);
mat4 V;
mat4 P;
mat4 MXYZ = mat4(1.0f);

GLuint program;
// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
    cout << "GLFW ERROR " << error << ":" << endl;
    cout << description << endl;
}

// handles keyboard input events
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
	
		if(play == false)
			play = true;
		else
			play = false;
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if( (action == GLFW_PRESS) || (action == GLFW_RELEASE) ){
		if(button == GLFW_MOUSE_BUTTON_LEFT)
			leftmousePressed = !leftmousePressed;
		else if(button == GLFW_MOUSE_BUTTON_RIGHT)
			rightmousePressed = !rightmousePressed;
	}
}

void mousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	vec2 newPos = vec2(xpos/(double)vp[2], -ypos/(double)vp[3])*2.f - vec2(1.f);

	vec2 diff = newPos - mousePos;
	if(leftmousePressed){
		activeCamera->trackballRight(-diff.x);
		activeCamera->trackballUp(-diff.y);
	}
	else if(rightmousePressed){
		float zoomBase = (diff.y > 0) ? 1.f/2.f : 2.f;

		activeCamera->zoom(pow(zoomBase, abs(diff.y)));
	}

	mousePos = newPos;
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	glViewport(0, 0, width, height);

	float minDim = float(std::min(width, height));

	winRatio[0][0] = minDim/float(width);
	winRatio[1][1] = minDim/float(height);
}

void printVec(vec3 vector)
{
	cout << "X: " << vector.x << " Y: " << vector.y << " Z: " << vector.z << endl;
}



//==========================================================================
// TUTORIAL STUFF


//vec2 and vec3 are part of the glm math library. 
//Include in your own project by putting the glm directory in your project, 
//and including glm/glm.hpp as I have at the top of the file.
//"using namespace glm;" will allow you to avoid writing everyting as glm::vec2





//Describe the setup of the Vertex Array Object
bool initVAO(GLuint vao, const VertexBuffers& vbo)
{
	glBindVertexArray(vao);		//Set the active Vertex Array

	glEnableVertexAttribArray(0);		//Tell opengl you're using layout attribute 0 (For shader input)
	glBindBuffer( GL_ARRAY_BUFFER, vbo.id[VertexBuffers::VERTICES] );		//Set the active Vertex Buffer
	glVertexAttribPointer(
		0,				//Attribute
		3,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec3),	//Stride
		(void*)0			//Offset
		);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::NORMALS]);
	glVertexAttribPointer(
		1,				//Attribute
		3,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec3),	//Stride
		(void*)0			//Offset
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.id[VertexBuffers::INDICES]);

	return !CheckGLErrors("initVAO");		//Check for errors in initialize
}


//Loads buffers with data
bool loadBuffer(const VertexBuffers& vbo, 
				const vector<vec3>& points, 
				const vector<vec3> normals, 
				const vector<unsigned int>& indices)
{
	
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::VERTICES]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec3)*points.size(),		//Size of data in array (in bytes)
		&points[0],						//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
										//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::NORMALS]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec3)*normals.size(),	//Size of data in array (in bytes)
		&normals[0],					//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
										//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.id[VertexBuffers::INDICES]);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		sizeof(unsigned int)*indices.size(),
		&indices[0],
		GL_STATIC_DRAW
		);

	return !CheckGLErrors("loadBuffer");	
}

//Compile and link shaders, storing the program ID in shader array
GLuint initShader(string vertexName, string fragmentName)
{	
	string vertexSource = LoadSource(vertexName);		//Put vertex file text into string
	string fragmentSource = LoadSource(fragmentName);		//Put fragment file text into string

	GLuint vertexID = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentID = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
	
	return LinkProgram(vertexID, fragmentID);	//Link and store program ID in shader array
}

//Initialization
void initGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearColor(0.f, 0.f, 0.f, 0.f);		//Color to clear the screen with (R, G, B, Alpha)
}

bool loadUniforms(GLuint program, mat4 perspective, mat4 modelview)
{
	glUseProgram(program);

	glUniformMatrix4fv(glGetUniformLocation(program, "modelviewMatrix"),
						1,
						false,
						&modelview[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(program, "perspectiveMatrix"),
						1,
						false,
						&perspective[0][0]);

	glUseProgram(0);
	return !CheckGLErrors("loadUniforms");
}

//Draws buffers to screen
void render()
{
	glBindVertexArray(vao);		//Use the LINES vertex array
	glUseProgram(program);

	loadBuffer(vbo, points, normals, indices);

	glDrawElements(
			GL_TRIANGLES,		//What shape we're drawing	- GL_TRIANGLES, GL_LINES, GL_POINTS, GL_QUADS, GL_TRIANGLE_STRIP
			indices.size(),		//How many indices
			GL_UNSIGNED_INT,	//Type
			(void*)0			//Offset
			);

	CheckGLErrors("render");
	glUseProgram(0);
	glBindVertexArray(0);
}

void renderLine()
{
	
	glBindVertexArray(vaoLine);
	glUseProgram(program);
	
	
	loadBuffer(vboLine, linePoints, lineNormal, lineIndices);
	

	glDrawElements(
			GL_LINE_LOOP,
			lineIndices.size(),
			GL_UNSIGNED_INT,
			(void*)0
			);
	
	
	
	CheckGLErrors("renderLine");
	glUseProgram(0);
	glBindVertexArray(0);
}
void renderLineTest(GLuint vao, VertexBuffers vbo, vector<vec3> points, vector<vec3> normal, vector<unsigned int> indices)
{
	glBindVertexArray(vao);
	glUseProgram(program);
	
	
	loadBuffer(vbo, points, normal, indices);
	

	glDrawElements(
			GL_LINE_LOOP,
			indices.size(),
			GL_UNSIGNED_INT,
			(void*)0
			);
	
	
	
	CheckGLErrors("renderLineTest");
	glUseProgram(0);
	glBindVertexArray(0);
}
/* XYZ framework of the cube*/
void renderXYZ()
{
	glBindVertexArray(vaoLine);
	glUseProgram(program);
	
	
	loadBuffer(vboLine, XYZPoints, XYZNormals, XYZIndices);
	

	glDrawElements(
			GL_LINES,
			XYZIndices.size(),
			GL_UNSIGNED_INT,
			(void*)0
			);
	
	
	
	CheckGLErrors("renderLine");
	glUseProgram(0);
	glBindVertexArray(0);
	
}
void generateSquare(vector<vec3>* vertices, vector<vec3>* normals, 
					vector<unsigned int>* indices, float width)
{
	vertices->push_back(vec3(-width*0.5f, -width*0.75f, 0.f));
	vertices->push_back(vec3(width*0.5f, -width*0.75f, 0.f));
	vertices->push_back(vec3(width*0.5f, width*0.75f, 0.f));
	vertices->push_back(vec3(-width*0.5f, width*0.75f, 0.f));

	normals->push_back(vec3(1.f, 1.f, 1.f));
	normals->push_back(vec3(1.f, 1.f, 1.f));
	normals->push_back(vec3(1.f, 1.f, 1.f));
	normals->push_back(vec3(1.f, 1.f, 1.f));

	//First triangle
	indices->push_back(0);
	indices->push_back(1);
	indices->push_back(2);
	//Second triangle
	indices->push_back(2);
	indices->push_back(3);
	indices->push_back(0);
}

void generateCube(vector<vec3>* vertices, vector<vec3>* normals, 
					vector<unsigned int>* indices, float width)
{
/* points for generating the cube*/
vertices->push_back(vec3(1.0f, 1.0f, 1.f)); //0
vertices->push_back(vec3(1.0f, -1.0f, 1.f)); //1
vertices->push_back(vec3(-1.0f, -1.0f, 1.f)); //2
vertices->push_back(vec3(-1.0f, 1.0f, 1.f)); //3
vertices->push_back(vec3(-1.0f, -1.0f, -1.f)); //4
vertices->push_back(vec3(-1.0f, 1.0f, -1.f)); //5
vertices->push_back(vec3(1.0f, 1.0f, -1.f)); //6
vertices->push_back(vec3(1.0f, -1.0f, -1.f)); //7



/* points for generating the pyramid*/
//vertices->push_back(vec3(0.0f, 0.0f, 5.f)); //8 front pyramid
//vertices->push_back(vec3(0.0f, -3.5f, 0.0f)); //8 bottom pyramid
//vertices->push_back(vec3(3.5f, 0.0f, 0.0f)); //8 left pyramid
//vertices->push_back(vec3(-3.5f, 0.0f, 0.0f)); //8


/* front of cube*/
indices->push_back(0);
indices->push_back(1);
indices->push_back(2);

indices->push_back(0);
indices->push_back(2);
indices->push_back(3);

/* left side of cube*/
indices->push_back(2);
indices->push_back(3);
indices->push_back(4);

indices->push_back(3);
indices->push_back(4);
indices->push_back(5);

/* back of cube*/
indices->push_back(6);
indices->push_back(7);
indices->push_back(4);

indices->push_back(4);
indices->push_back(5);
indices->push_back(6);

/* right side of cube*/
indices->push_back(0);
indices->push_back(1);
indices->push_back(7);

indices->push_back(0);
indices->push_back(7);
indices->push_back(6);

/* top of cube*/
indices->push_back(0);
indices->push_back(3);
indices->push_back(5);

indices->push_back(0);
indices->push_back(5);
indices->push_back(6);

/* bottom of cube*/
indices->push_back(1);
indices->push_back(2);
indices->push_back(4);

indices->push_back(1);
indices->push_back(4);
indices->push_back(7);

/* Pyramid*/
/* on the front
indices->push_back(0);
indices->push_back(1);
indices->push_back(8);

indices->push_back(1);
indices->push_back(2);
indices->push_back(8);

indices->push_back(2);
indices->push_back(3);
indices->push_back(8);

indices->push_back(0);
indices->push_back(3);
indices->push_back(8);
*/
/* bottom pointing pyramid*/
/*
indices->push_back(1);
indices->push_back(7);
indices->push_back(8);

indices->push_back(1);
indices->push_back(2);
indices->push_back(8);

indices->push_back(2);
indices->push_back(4);
indices->push_back(8);

indices->push_back(4);
indices->push_back(7);
indices->push_back(8);
*/
/*left point pyramid*/
/*
indices->push_back(0);
indices->push_back(1);
indices->push_back(8);

indices->push_back(0);
indices->push_back(6);
indices->push_back(8);

indices->push_back(6);
indices->push_back(7);
indices->push_back(8);

indices->push_back(1);
indices->push_back(7);
indices->push_back(8);
*/
/*right point pyramid*/
/*
indices->push_back(2);
indices->push_back(3);
indices->push_back(8);

indices->push_back(2);
indices->push_back(4);
indices->push_back(8);

indices->push_back(4);
indices->push_back(5);
indices->push_back(8);

indices->push_back(3);
indices->push_back(5);
indices->push_back(8);
*/

/*colors of each point*/

normals->push_back(vec3(0.f, 0.f, 1.f));
normals->push_back(vec3(0.f, 0.f, 1.f));
normals->push_back(vec3(0.f, 0.f, 1.f));
normals->push_back(vec3(0.f, 0.f, 1.f));

normals->push_back(vec3(0.f, 1.f, 1.f));
normals->push_back(vec3(0.f, 1.f, 0.f));
normals->push_back(vec3(0.f, 1.f, 0.f));
normals->push_back(vec3(0.f, 1.f, 0.f));

//normals->push_back(vec3(1.f, 1.f, 1.f));



	
}
void generateSquareXYZCoords(vector<vec3>* vertices, vector<vec3>* normals, 
					vector<unsigned int>* indices)
{
	
	vertices->push_back(vec3(0.0f, 0.0f, 0.0f));
	vertices->push_back(vec3(2.0f, 0.0f, 0.0f));
	vertices->push_back(vec3(0.0f, 2.0f, 0.0f));
	vertices->push_back(vec3(0.0f, 0.0f, 2.0f));
	
	normals->push_back(vec3(1.0f, 1.0f, 1.0f));
	normals->push_back(vec3(1.0f, 0.0f, 0.0f)); //X direction color
	normals->push_back(vec3(0.0f, 1.0f, 0.0f)); //Y direction color
	normals->push_back(vec3(0.0f, 0.0f, 1.0f)); //Z direction color
	
	indices->push_back(0);
	indices->push_back(1);
	
	indices->push_back(0);
	indices->push_back(2);
	
	indices->push_back(0);
	indices->push_back(3);
	
}
/* generates the track*/
void generateLine(vector<vec3>* vertices, vector<vec3>* normals, 
					vector<unsigned int>* indices)
{
	/*Generate Random Track*/
	/*
	vertices->push_back(vec3(-12, 0, 0)); //vert 1
	vertices->push_back(vec3(12, 0, 0)); //vert 3
	vertices->push_back(vec3(30, -30, 0)); //vert 2
	vertices->push_back(vec3(70, 30, 0)); //vert 3
	*/
	
	/* Generate a circle*/
	/*
	vertices->push_back(vec3(-12, 0, 10)); //vert 1
	vertices->push_back(vec3(12, 0, 10)); //vert 3
	vertices->push_back(vec3(12, -12, -10)); //vert 2
	vertices->push_back(vec3(-12, -12, -10)); //vert 3
	*/
	/* Infinity sign*/
	/*
	vertices->push_back(vec3(-12, 0, 10)); //vert 1
	vertices->push_back(vec3(12, -12, -10)); //vert 2
	vertices->push_back(vec3(12, 0, 10)); //vert 3
	vertices->push_back(vec3(-12, -12, -10)); //vert 3
	*/
	/* messing with Z coords on the Infinity Sign*/
	
	/*
	vertices->push_back(vec3(-12, 0, 10)); //vert 1
	vertices->push_back(vec3(12, -12, 10)); //vert 2
	vertices->push_back(vec3(12, 30, -10)); //vert 3
	vertices->push_back(vec3(-12, -12, -10)); //vert 3
	*/
	
	
	vertices->push_back(vec3(0, 0, 0)); //vert 0
	vertices->push_back(vec3(0, 20, -40)); //vert 1
	vertices->push_back(vec3(-10, 20, -40)); //vert 2
	vertices->push_back(vec3(-20, 0, -40)); //vert 3
	vertices->push_back(vec3(-20, 0, 10)); //vert 4
	vertices->push_back(vec3(0, 0, 10)); //vert 4
	/*
	vertices->push_back(vec3(10, 10, 0)); //vert 1
	vertices->push_back(vec3(10, -10, 0)); //vert 2
	vertices->push_back(vec3(0, 0, 0)); //vert 3
	vertices->push_back(vec3(-10, 10, 0)); //vert 3
	*/
	/*
	vertices->push_back(vec3(-10, 0, -10)); //vert 0
	vertices->push_back(vec3(0, 0, -10)); //vert 1
	vertices->push_back(vec3(40, 30, -20)); //vert 2
	vertices->push_back(vec3(90, 30, -20)); //vert 3
	vertices->push_back(vec3(0, 0, 0)); //vert 4
	vertices->push_back(vec3(-10, -5, 5)); //vert 5
	*/
	
	
	
	normals->push_back(vec3(0.f, 1.f, 0.f));
	normals->push_back(vec3(1.f, 0.f, 0.f));
	
	
	normals->push_back(vec3(0.f, 1.f, 0.f));
	normals->push_back(vec3(1.f, 0.f, 0.f));
	
	
	indices->push_back(0);
	indices->push_back(1);
	
	
	indices->push_back(1);
	indices->push_back(2);
	
	indices->push_back(2);
	indices->push_back(3);
	
	
	indices->push_back(3);
	indices->push_back(0);
	

}
GLFWwindow* createGLFWWindow()
{
	// initialize the GLFW windowing system
    if (!glfwInit()) {
        cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
        return NULL;
    }
    glfwSetErrorCallback(ErrorCallback);

    // attempt to create a window with an OpenGL 4.1 core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(1024, 1024, "OpenGL Example", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return NULL;
    }

    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetWindowSizeCallback(window, resizeCallback);
    glfwMakeContextCurrent(window);

    return window;
}

void deleteStuff()
{
	// clean up allocated resources before exit
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(VertexBuffers::COUNT, vbo.id);
	glDeleteVertexArrays(1,&vaoLine);
	glDeleteBuffers(VertexBuffers::COUNT, vboLine.id);
	glDeleteProgram(program);
}

// ==========================================================================
// PROGRAM ENTRY POINT

/* total distance of the curve*/
float totalDistance(vector<vec3>points)
{
	float tot = 0;
	for(int i = 0; i < points.size(); i++)
	{
		
		tot += getLength(points[i]);
	}
	
	return tot;
}
/* finds the highest point on the curve*/
float highestPoint(vector<vec3> points)
{
	float H = -1;
	float h;
	for(int i = 0; i < linePoints.size(); i++)
	{
		h = linePoints[i].y;
		
		if(H < h)
		{
			H = h;
			highestPointIndex = i;
		}
	}
	
	return H;
}
/*find the lowest point on the curve*/
float lowestPoint(vector<vec3> points)
{
	float L = 10000000;
	float l;
	
	for(int i = 0; i < linePoints.size(); i++)
	{
		l = linePoints[i].y;
		
	
		if(L > l)
		{
			
			L = l;
			lowestPointIndex = i;
		}
		distLow += getLength(linePoints[i]);
	}
	
	return L;
}
/* sets the point to start deceleration at*/
float decelPoint(vector<vec3>points, float low)
{
	float nextH;
	float h;
	
	for(int i = 0; i < points.size(); i++)
	{
		h = points[i].y;
		nextH = points[wrap(i+1)].y;
		if(h == nextH && h == low)
			return i;
		
	}
	
	
}
/*sets the starting point*/
int zeroHeight(vector<vec3> points, float low)
{
	float nextH;
	
	float h; 
	for(int i = 0; i < points.size(); i++)
	{
		h = points[i].y;
		nextH = points[wrap(i+1)].y;
		if(h < nextH && h == low)
			return i;
		
	}
}

/*get the distance from the deceleration point to the start point*/
float distanceDecToStart(vector<vec3> points, int startIndex, int decIndex)
{
	float totalDistance = 0;
	for(int i = decIndex; i <= startIndex; i++)
	{
		totalDistance += getLength(points[wrap(i + 1)] - points[i]);
	}
	
	return totalDistance;
}
float velocity(float h)
{
	float v = sqrt(2.0f * gravity.y * (H-h));
	
	return v;
}


int main(int argc, char *argv[])
{   
    window = createGLFWWindow();
    if(window == NULL)
    	return -1;

    //Initialize glad
    if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}

    // query and print out information about our OpenGL environment
    QueryGLVersion();

	initGL();

	//Initialize shader
	program = initShader("vertex.glsl", "fragment.glsl");

	
	//GLuint vboLine; 

	//Generate object ids
	glGenVertexArrays(1, &vao);
	glGenBuffers(VertexBuffers::COUNT, vbo.id);

	glGenVertexArrays(1, &vaoLine);
	glGenBuffers(VertexBuffers::COUNT, vboLine.id);

	initVAO(vao, vbo);
	initVAO(vaoLine, vboLine);

	glGenVertexArrays(1, &vaoPos);
	glGenBuffers(VertexBuffers::COUNT, vboPos.id);

	glGenVertexArrays(1, &vaoNeg);
	glGenBuffers(VertexBuffers::COUNT, vboNeg.id);

	initVAO(vaoPos, vboPos);
	initVAO(vaoNeg, vboNeg);

	//generateSquare(&points, &normals, &indices, 0.5f);
	generateCube(&points, &normals, &indices, 0.5f);
	
	
	/* setup buffers to draw line*/
	generateLine(&linePoints, &lineNormal, &lineIndices);
	
	generateSquareXYZCoords(&XYZPoints, &XYZNormals, &XYZIndices);
	for(int i = 0; i < 10; i++)
	{
		linePoints = subdivision(linePoints, &lineIndices, &lineNormal);
	}
	
	createTrack(linePoints); //creates the positive and negative rails
	
	H = highestPoint(linePoints);
	low = lowestPoint(linePoints);
	int startPoint = zeroHeight(linePoints, low);
	int startDec = decelPoint(linePoints, low);
	float decDist = distanceDecToStart(linePoints, startPoint, startDec);
	
	//cout << decDist << endl;

	float currDist = 0;
	
	Camera cam = Camera(vec3(0, 0, -1), vec3(0, 0, 50));
	activeCamera = &cam;
	//float fovy, float aspect, float zNear, float zFar
	mat4 perspectiveMatrix = perspective(radians(80.f), 1.f, 0.1f, 100.f);
	
	
	int i;

	i = startPoint;

	M = translate(mat4(1.0f), linePoints[i]);
	animate(linePoints[i], i, linePoints, ds);

	
	MXYZ = translate(mat4(1.0f), linePoints[i]);
	
	float vdec;
    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
		glClearColor(0.4, 0.4, 0.4, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//Clear color and depth buffers (Haven't covered yet)
		h = linePoints[i].y;
	
		if(gravityFree)
		{
			
			v = velocity(h);
			if (i >= startDec)
			{
				vdec = v;
				decel = true;
				gravityFree = false;
			}
			
			
		}
		if(lifting)
		{
			v = 2.9;
			if(i < startPoint && i > highestPointIndex)
			{
				lifting = false;
				gravityFree = true;
			}	
		
		}
		if(decel)
		{
			
			currDist = distanceDecToStart(linePoints, startPoint, i);
			v = vdec*(currDist/decDist);
			if(i >= startPoint)
			{
				decel = false;
				lifting = true;
			} 
		}
		
		V = cam.getMatrix();

		
		if(play)
			{	
				ds = v*dt;
				animate(linePoints[i], i, linePoints, ds);		
			}
		
		
        // draw the square for now
        loadUniforms(program, winRatio*perspectiveMatrix*V, M);
        render();
       
        
		
		//Draw the line
		//loadUniforms(program, winRatio*perspectiveMatrix*V, mat4(1.0f));
		
       //renderLine();
		
		
		loadUniforms(program, winRatio*perspectiveMatrix*V, mat4(1.0f));
		
		renderLineTest(vaoNeg, vboNeg, negRail, negNorm, negIndices); //need to make there own normals and indices probably
		
		
		
		loadUniforms(program, winRatio*perspectiveMatrix*V, mat4(1.0f));
		
		renderLineTest(vaoPos, vboPos, posRail, posNorm, posIndices);
		
		
		loadUniforms(program, winRatio*perspectiveMatrix*V, MXYZ);
		renderXYZ(); //XYZ Framework of the Cube;
		
        // scene is rendered to the back buffer, so swap to front for display
        glfwSwapInterval(1);
        glfwSwapBuffers(window);
        
		glfwPollEvents();
	}

	deleteStuff();
	


	glfwDestroyWindow(window);
	glfwTerminate();

   return 0;
}



int wrap(int i)
{
	int s = i;
	if(s >= 0)
		s = i%linePoints.size();
	else
		s = linePoints.size()-1;
	return s;
}
// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS
/* Bt = bead, i = current point the bead is on, points is the curve, Ds the user input distance to travel
 * 
 * Returns the point at which the object should be at on the curve based on the passed in Ds
 * */

/*--------------------- Not sure if this is right (Stuff for Frenet Frame-----------------------------------------*/
vec3 posOnCurve(vec3 Bt, int &i, vector<vec3> points, float ds)
{
	return archLength(Bt, i, points, ds);
}

vec3 tangentTemp(vec3 nextPos, vec3 prevPos)
{
	vec3 T = nextPos - prevPos;
	T = T / getLength(T);
	return T;
}

vec3 tangent(vec3 B, vec3 N)
{
	vec3 T = cross(N,B);
	T = T / getLength(T);
	return T;
	
}

/* probably not needed*/
vec3 curvature(vec3 nextPos, vec3 currPos, float ds)
{
	return ((currPos + nextPos)- (2.0f * nextPos) + (currPos - nextPos)) / (ds*ds);
}

vec3 normal(vec3 cA, vec3 gravity)
{
	vec3 N = cA + gravity;
	N = N / getLength(N);
	return N;
}
/*used for figuring out centripetal acceleration */
vec3 N (vec3 nextPos, vec3 currPos, vec3 prevPos)
{
	vec3 n = (nextPos - (2.0f * currPos) + prevPos);
	return  n;
}
/*centripetal acceleration calculation (a perpedicular)*/
vec3 centAccel (vec3 nextPos, vec3 currPos, vec3 prevPos)
{
	vec3 nVec = N(nextPos, currPos, prevPos);
	float x = 0.5f * getLength(nVec);
	float c = 0.5f * getLength((nextPos - prevPos));

	float k = 1.0f / ((x*x)+(c*c));

	return k * nVec;
}

float getLength(vec3 v)
{
	return sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
}
vec3 binormal(vec3 normal, vec3 tangent)
{
	vec3 B = cross(normal, tangent);
	
	B = B / getLength(B);
	return B;
	
}
vec3 binormalAtCurrPoint(vec3 nextPos, vec3 currPos, vec3 prevPos)
{
	
	vec3 centAcc = centAccel(nextPos, currPos, prevPos);
	vec3 T = tangentTemp(nextPos, prevPos);
	vec3 N = normal(centAcc, gravity);
	
	vec3 B = binormal(N,T);
	B = B / getLength(B);
	
	return B;
}
/* for now move the square along the x axis*/
void animate(vec3 cartLoc, int &i, vector<vec3> points, float ds)
{

	vec3 nextPos= posOnCurve(cartLoc, i, points, ds);
	vec3 prevPos = points[wrap(i-1)];
	vec3 nextPosOnCurve = points[wrap(i+1)];
	
	
	vec3 centAcc = centAccel(nextPos, cartLoc, prevPos);
	vec3 N = normal(centAcc, gravity);
	
	vec3 tempT = tangentTemp(nextPos, prevPos);


	vec3 B = binormal(N, tempT);
	
	
	vec3 T = tangent(B, N);
	
	
	mat4 modelTrans = translate(mat4(1.0f), nextPos);
	
	mat4 frenetFrame = freFrame(N, B, T);
	
	
	M =  modelTrans * frenetFrame;
	MXYZ = modelTrans * frenetFrame;

	

}


/*
							B.x,N.x,T.x,0.0
							B.y,N.y,T.y,0.0
							B.z,N.z,T.z,0.0
							0.0,0.0,0.0,1.0);
*/
mat4 freFrame(vec3 N, vec3 B, vec3 T)
{
	mat4 frenetFrame;
	
	frenetFrame[0][0] = B.x; 
	frenetFrame[1][0] = B.y; 
	frenetFrame[2][0] = B.z;
	
	frenetFrame[0][1] = N.x; 
	frenetFrame[1][1] = N.y; 
	frenetFrame[2][1] = N.z;
	
	frenetFrame[0][2] = T.x; 
	frenetFrame[1][2] = T.y; 
	frenetFrame[2][2] = T.z;
	
	
	return frenetFrame;
}
/*-----------------------------------------------------------------------------------------------------------------*/
vec3 archLength(vec3 Bt, int &io, vector<vec3> points, float Ds)
{
	int i = io;
	vec3 objPos = points[wrap(i+1)] - Bt;
	float objLen = getLength(objPos);
	float DsP;
	vec3 currLine;
	
	/* moves to the next point on the curve  even if Ds = 0 or the distance to move is less than the distance to the next point*/
	if(objLen > Ds)
		{
			i++;
			io = wrap(i);
			return (Bt + (Ds *(objPos/objLen)));
		}
	else
		{
			DsP = objLen;
			i++;
			io = wrap(i);
		}
			currLine = points[wrap(i+1)] - points[wrap(i)];
			/* based on the total distance(Ds) to travel it calculates the next point that the object should be at*/
			while((DsP + getLength(currLine)) < Ds)
			{
				DsP = DsP + getLength(currLine);
				
				currLine = points[wrap(i+1)] - points[wrap(i)];
				i++;
				io = wrap(i);
			}
			
		
		return (Ds - DsP)*(currLine / getLength(currLine)) + points[wrap(io)];
		

}

vector<vec3> subdivision(vector<vec3> points, vector<unsigned int>* indices, vector<vec3>* normals)
{
	vector<vec3> splitPoints;
	vector<vec3> averagedPoints;
	vec3 midPoint;
	indices->clear();
	normals->clear();
	int nextEl;
	/* Splitting*/
	for(int i = 0; i < points.size(); i++)
	{
		splitPoints.push_back(points[i]);
		nextEl = i + 1;
		if(nextEl != points.size())
			midPoint = 0.5f*(points[i] + points[nextEl]);
		else
			midPoint = 0.5f*(points[i] + points[0]);
		
		splitPoints.push_back(midPoint);
	}
	
	/* Averaging */
	for(int j = 0; j < splitPoints.size(); j++)
	{
		nextEl = j+1;
		if(nextEl != splitPoints.size())
			midPoint = 0.5f*(splitPoints[j] + splitPoints[nextEl]);
		else
			midPoint = 0.5f*(splitPoints[j] + splitPoints[0]);
		
		averagedPoints.push_back(midPoint);
		
			
		
		
	
	}
	
	for(int i = 0;  i < averagedPoints.size(); i++)
	{
		indices->push_back(i);
		
		nextEl = i + 1;
		if(nextEl != averagedPoints.size())
			indices->push_back(i+1);
		else
			indices->push_back(0);
	
		normals->push_back(vec3(0.0f,0.0,0.0));
	}
	
	return averagedPoints;
}
/* find the binormal at each point and add it to the current track for one rail and subtract it from the current track for the other rail*/
void createTrack (vector<vec3> points)
{
	int nextEl;
	for(int i = 0; i < points.size(); i++)
	{
		vec3 binormal = binormalAtCurrPoint(points[wrap(i+1)], points[wrap(i)], points[wrap(i-1)]);
		
		/* TEST*/
		
		printVec(binormal);
		
		negRail.push_back((points[i] - binormal));
		posRail.push_back((points[i] + binormal));
		
		negIndices.push_back(i);
		posIndices.push_back(i);
		
		posNorm.push_back(vec3(1.0f, 1.0f, 1.0f));
		negNorm.push_back(vec3(1.0f, 1.0f, 1.0f));
		
		nextEl = i + 1;
		if(nextEl != points.size())
		{
			negIndices.push_back(i+1);
			posIndices.push_back(i+1);
		}
		else
		{
			negIndices.push_back(0);
			posIndices.push_back(0);
		}
		
	}
	
}
// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
    // query opengl version and renderer information
    string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    cout << "OpenGL [ " << version << " ] "
         << "with GLSL [ " << glslver << " ] "
         << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors(string location)
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        cout << "OpenGL ERROR:  ";
        switch (flag) {
        case GL_INVALID_ENUM:
            cout << location << ": " << "GL_INVALID_ENUM" << endl; break;
        case GL_INVALID_VALUE:
            cout << location << ": " << "GL_INVALID_VALUE" << endl; break;
        case GL_INVALID_OPERATION:
            cout << location << ": " << "GL_INVALID_OPERATION" << endl; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << location << ": " << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
        case GL_OUT_OF_MEMORY:
            cout << location << ": " << "GL_OUT_OF_MEMORY" << endl; break;
        default:
            cout << "[unknown error code]" << endl;
        }
        error = true;
    }
    return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
    string source;

    ifstream input(filename.c_str());
    if (input) {
        copy(istreambuf_iterator<char>(input),
             istreambuf_iterator<char>(),
             back_inserter(source));
        input.close();
    }
    else {
        cout << "ERROR: Could not load shader source from file "
             << filename << endl;
    }

    return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
    // allocate shader object name
    GLuint shaderObject = glCreateShader(shaderType);

    // try compiling the source as a shader of the given type
    const GLchar *source_ptr = source.c_str();
    glShaderSource(shaderObject, 1, &source_ptr, 0);
    glCompileShader(shaderObject);

    // retrieve compile status
    GLint status;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
        cout << "ERROR compiling shader:" << endl << endl;
        cout << source << endl;
        cout << info << endl;
    }

    return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    // allocate program object name
    GLuint programObject = glCreateProgram();

    // attach provided shader objects to this program
    if (vertexShader)   glAttachShader(programObject, vertexShader);
    if (fragmentShader) glAttachShader(programObject, fragmentShader);

    // try linking the program with given attachments
    glLinkProgram(programObject);

    // retrieve link status
    GLint status;
    glGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
        cout << "ERROR linking shader program:" << endl;
        cout << info << endl;
    }

    return programObject;
}


// ==========================================================================
