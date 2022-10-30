// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <sstream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;

// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#define N 10
#define M 6
// ATTN 1A is the general place in the program where you have to change the code base to satisfy a Task of Project 1A.
// ATTN 1B for Project 1B. ATTN 1C for Project 1C. Focus on the ones relevant for the assignment you're working on.

typedef struct Vertex {
	float Position[4];
	float Color[4];
	void SetCoords(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = coords[3];
	}
	void SetColor(float *color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
};

// ATTN: use POINT structs for cleaner code (POINT is a part of a vertex)
// allows for (1-t)*P_1+t*P_2  avoiding repeat for each coordinate (x,y,z)
typedef struct point {
	float x, y, z;
	point(const float x = 0, const float y = 0, const float z = 0) : x(x), y(y), z(z){};
	point(float *coords) : x(coords[0]), y(coords[1]), z(coords[2]){};
	point operator -(const point& a) const {
		return point(x - a.x, y - a.y, z - a.z);
	}
	point operator +(const point& a) const {
		return point(x + a.x, y + a.y, z + a.z);
	}
	point operator *(const float& a) const {
		return point(x * a, y * a, z * a);
	}
	point operator /(const float& a) const {
		return point(x / a, y / a, z / a);
	}
	float* toArray() {
		float array[] = { x, y, z, 1.0f };
		return array;
	}
};

// Function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], unsigned short[], size_t, size_t, int);
void createObjects(void);
void pickVertex(void);
void moveVertex(void);
void renderScene(void);
void catmullroncurve(void);
void cleanup(void);
static void mouseCallback(GLFWwindow*, int, int, int);
static void keyCallback(GLFWwindow*, int, int, int, int);

// GLOBAL VARIABLES
int i = 0;
float origColor[4]; 
GLFWwindow* window;
const GLuint window_width = 1024, window_height = 768;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

// Program IDs
GLuint programID;
GLuint pickingProgramID;

// Uniform IDs
GLuint MatrixID;
GLuint ViewMatrixID;
GLuint ModelMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorArrayID;
GLuint pickingColorID;

GLuint gPickedIndex;
std::string gMessage;

// ATTN: INCREASE THIS NUMBER AS YOU CREATE NEW OBJECTS
const GLuint NumObjects = 320; // Number of objects types in the scene
const int tessellation_no = 17;

// Keeps track of IDs associated with each object
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];

size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumVert[NumObjects];	// Useful for glDrawArrays command
size_t NumIdcs[NumObjects];	// Useful for glDrawElements command

// Initialize ---  global objects -- not elegant but ok for this project
const size_t IndexCount = 10;
Vertex Vertices[IndexCount] = 
{
	{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { -0.5f, 2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 0.5f, 2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { -0.5f, -2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 0.5f, -2.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
	{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
};
GLushort Indices[IndexCount] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};

Vertex pVertices[6][321];
Vertex c2Vertices[40];
Vertex c1Vertices[40];
Vertex tempVertices[5];
Vertex bVerticesArray[N * tessellation_no];
Vertex tVerticesArray[N * tessellation_no];
Vertex movingPoint[1];
Vertex tangentPoints[2];
Vertex normalPoints[2];
Vertex binormalPoints[2];

int curr_level = 0;
int prev_max_i;
float colorBlue[] = {0.0f, 0.0f, 1.0f, 1.0f};
float colorYellow[] = {1.0f, 1.0f, 0.0f, 1.0f};
float colorRed[] = {1.0f, 0.0f, 0.0f, 1.0f};
float colorGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};
unsigned short pIndices[321];
unsigned short catIndices[170];
bool task2 = false;
bool task3 = false;
bool shouldZTranslate = false;
bool shouldSplitView = false;
bool showAxes = false;
int pointCount = 0;
// ATTN: DON'T FORGET TO INCREASE THE ARRAY SIZE IN THE PICKING VERTEX SHADER WHEN YOU ADD MORE PICKING COLORS
float pickingColor[IndexCount];

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // FOR MAC

	// ATTN: Project 1A, Task 0 == Change the name of the window
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Avulapati,Niranjan(21921323)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI display
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetMouseButtonCallback(window, mouseCallback);
	glfwSetKeyCallback(window, keyCallback);

	return 0;
}

void initOpenGL(void) {
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	for(int i = 0; i < 320; i++) {
        pIndices[i] = i;
    }

	// Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	//glm::mat4 ProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for Project 1, use an ortho camera :
	gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(
		glm::vec3(0, 0, -5), // Camera is at (0,0,-5) below the origin, in World Space
		glm::vec3(0, 0, 0), // and looks at the origin
		glm::vec3(0, 1, 0)  // Head is looking up at the origin (set to 0,-1,0 to look upside-down)
	);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("p1_StandardShading.vertexshader", "p1_StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("p1_Picking.vertexshader", "p1_Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	
	// Get a handle for our "pickingColorID" uniform
	pickingColorArrayID = glGetUniformLocation(pickingProgramID, "PickingColorArray");
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");

	// Define pickingColor array for picking program
	// use a for-loop here
	pickingColor[0] = 0 / 255.0f;
	pickingColor[1] = 1 / 255.0f;
	pickingColor[2] = 2 / 255.0f;
	pickingColor[3] = 3 / 255.0f;
	pickingColor[4] = 4 / 255.0f;
	pickingColor[5] = 5 / 255.0f;
	pickingColor[6] = 6 / 255.0f;
	pickingColor[7] = 7 / 255.0f;
	pickingColor[8] = 8 / 255.0f;
	pickingColor[9] = 9 / 255.0f;


	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	// for several objects of the same type use a for-loop
	int obj = 0;  // initially there is only one type of object 
	VertexBufferSize[obj] = sizeof(Vertices);
	IndexBufferSize[obj] = sizeof(Indices);
	NumIdcs[obj] = IndexCount;
	//printf("\nOBJ: %d", obj);
	// createVAOs(Vertices, Indices, obj);
	createVAOs(Vertices, Indices, sizeof(Vertices), sizeof(Indices), 0);
	// createVAOs(c2Vertices, pIndices, sizeof(c2Vertices), sizeof(pIndices), 3);
}

// this actually creates the VAO (structure) and the VBO (vertex data buffer)
void createVAOs(Vertex Vertices[], unsigned short Indices[], size_t BufferSize, size_t IdxBufferSize, int ObjectId) {

	NumVert[ObjectId] = IdxBufferSize / (sizeof(GLubyte));

	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, BufferSize, Vertices, GL_STATIC_DRAW);

	// Create buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
	    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
	    glBufferData(GL_ELEMENT_ARRAY_BUFFER, IdxBufferSize, Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}

void createObjects(void) {
	// ATTN: DERIVE YOUR NEW OBJECTS HERE:  each object has
	// an array of vertices {pos;color} and
	// an array of indices (no picking needed here) (no need for indices)
	// ATTN: Project 1A, Task 1 == Add the points in your scene

	for(int i = 0; i <= N - 1; i++) {
        pVertices[0][i] = Vertices[i];
        // pVertices[0][i].SetColor(colorBlue);
    }
    pVertices[0][N] = pVertices[0][0];

    int max_i = N * pow(2, curr_level);
    int i;
    if(curr_level != 0) {
        for(int j = 1; j <= curr_level; j++) {
            prev_max_i = N * pow(2, j - 1);
            for(i = 0; i < max_i / 2; i++) {

                point *p_k1_i_minus_1;
                if(i == 0) {
                    p_k1_i_minus_1 = new point(pVertices[j-1][prev_max_i - 1].Position);
                } else {
                    p_k1_i_minus_1 = new point(pVertices[j-1][i-1].Position);
                }

                point *p_k1_i = new point(pVertices[j-1][i].Position);

                point *p_k1_i_plus_1;
                if((i + 1) == prev_max_i) {
                    p_k1_i_plus_1 = new point(pVertices[j-1][0].Position);
                } else {
                    p_k1_i_plus_1 = new point(pVertices[j-1][i+1].Position);
                }

                point p_k_2i = ( *p_k1_i_minus_1 * 4 + *p_k1_i * 4) / 8;
                point p_k_2i_plus_1 = (*p_k1_i_minus_1 + *p_k1_i * 6 + *p_k1_i_plus_1) / 8;
                pVertices[j][2*i] = {p_k_2i.x, p_k_2i.y, p_k_2i.z, 1.0f};
                pVertices[j][2*i].SetColor(colorBlue);
                pVertices[j][2*i + 1] = {p_k_2i_plus_1.x, p_k_2i_plus_1.y, p_k_2i_plus_1.z, 1.0f};
                pVertices[j][2*i + 1].SetColor(colorBlue);
            }
            pVertices[j][max_i] = pVertices[j][0];
            pVertices[j][max_i].SetColor(colorBlue);
        }
    }

    if(task2) {
        for(int i = 0; i < N; i++) {
            point *p_i_minus_1, *p_i, *p_i_plus_1, *p_i_plus_2;
            if(i == 0) {
                p_i_minus_1 = new point(Vertices[N - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[i + 2].Position);
            }else if(i == (N - 2)) {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[0].Position);
            } else if(i == (N - 1)) {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[0].Position);
                p_i_plus_2 = new point(Vertices[1].Position);
            } else {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[i + 2].Position);
            }

            point c_i_0 = (*p_i_minus_1 + *p_i * 4 + *p_i_plus_1) / 6;
            c2Vertices[4*i] = {c_i_0.x, c_i_0.y, c_i_0.z, 1.0f};
            c2Vertices[4*i].SetColor(colorYellow);
            point c_i_1 = (*p_i * 2 + *p_i_plus_1) / 3;
            c2Vertices[4*i + 1] = {c_i_1.x, c_i_1.y, c_i_1.z, 1.0f};
            c2Vertices[4*i + 1].SetColor(colorYellow);
            point c_i_2 = (*p_i + *p_i_plus_1 * 2) / 3;
            c2Vertices[4*i + 2] = {c_i_2.x, c_i_2.y, c_i_2.z, 1.0f};
            c2Vertices[4*i + 2].SetColor(colorYellow);
            point c_i_3 = (*p_i + *p_i_plus_1 * 4 + *p_i_plus_2) / 6;
            c2Vertices[4*i + 3] = {c_i_3.x, c_i_3.y, c_i_3.z, 1.0f};
            c2Vertices[4*i + 3].SetColor(colorYellow);
        }
    }

    if(task3) {
        for(int i = 0; i < N; i++) {
            point *p_i_minus_1, *p_i, *p_i_plus_1, *p_i_plus_2;
            if(i == 0) {
                p_i_minus_1 = new point(Vertices[N - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[i + 2].Position);
            }else if(i == (N - 2)) {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[0].Position);
            }else if(i == (N - 1)) {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[0].Position);
                p_i_plus_2 = new point(Vertices[1].Position);
            } else {
                p_i_minus_1 = new point(Vertices[i - 1].Position);
                p_i = new point(Vertices[i].Position);
                p_i_plus_1 = new point(Vertices[i + 1].Position);
                p_i_plus_2 = new point(Vertices[i + 2].Position);
            }

            point c_i_0 = *p_i;
            c1Vertices[4*i] = {c_i_0.x, c_i_0.y, c_i_0.z, 1.0f};
            c1Vertices[4*i].SetColor(colorRed);
            point c_i_1 = (*p_i * 10 + *p_i_plus_1 * 2 - *p_i_minus_1 * 2) / 10;
            c1Vertices[4*i + 1] = {c_i_1.x, c_i_1.y, c_i_1.z, 1.0f};
            c1Vertices[4*i + 1].SetColor(colorRed);
            point c_i_2 = (*p_i_plus_1 * 10 - *p_i_plus_2 * 2 + *p_i * 2) / 10;
            c1Vertices[4*i + 2] = {c_i_2.x, c_i_2.y, c_i_2.z, 1.0f};
            c1Vertices[4*i + 2].SetColor(colorRed);
            point c_i_3 = *p_i_plus_1;
            c1Vertices[4*i + 3] = {c_i_3.x, c_i_3.y, c_i_3.z, 1.0f};
            c1Vertices[4*i + 3].SetColor(colorRed);
        }
    }

    if(task3) {
        point *p1, *p2;
        for(int i = 0; i < N; i++) {
            for(int j = 0; j < 4; j++) {
                for(int k = 0; k < tessellation_no; k++) {
                    for(int l = 0; l < 4; l++) {
                        tempVertices[l].SetCoords(c1Vertices[4 * i + l].Position);
                    }
                    for(int m = 1; m < 4; m++) {
                        for(int n = 0; n < 4 - m; n++) {
                            p1 = new point(tempVertices[n].Position);
                            p2 = new point(tempVertices[n + 1].Position);
                            point newP = (*p1 * (1 - ((float)k / (float)tessellation_no))) + (*p2 * ((float)k / (float)tessellation_no));
                            if(m == 3) {
                                point tanP = *p2 - *p1;
                                tVerticesArray[i * tessellation_no + k] = {tanP.x, tanP.y, tanP.z, 1.0f};
                            }
                            tempVertices[n] = {newP.x, newP.y, newP.z, 1.0f};
                        }
                    }
                    bVerticesArray[i * tessellation_no + k].SetCoords(tempVertices[0].Position);
                    bVerticesArray[i * tessellation_no + k].SetColor(colorGreen);
                }
            }
        }
    }


	// ATTN: Project 1B, Task 1 == create line segments to connect the control points

	// ATTN: Project 1B, Task 2 == create the vertices associated to the smoother curve generated by subdivision

	// ATTN: Project 1B, Task 4 == create the BB control points and apply De Casteljau's for their corresponding for each piece

	// ATTN: Project 1C, Task 3 == set coordinates of yellow point based on BB curve and perform calculations to find
	// the tangent, normal, and binormal
}

void pickVertex(void) {
	// Clear the screen in white
	
	glClearColor(0.0f, 0.0f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);

	glm::mat4 ModelMatrix = glm::mat4(1.0); // initialization
	// ModelMatrix == TranslationMatrix * RotationMatrix;
	glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
	// MVP should really be PVM...
	// Send the MVP to the shader (that is currently bound)
	// as data type uniform (shared by all shader instances)
	glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

	// pass in the picking color array to the shader
	glUniform1fv(pickingColorArrayID, IndexCount, pickingColor);

	// --- enter vertices into VBO and draw
	glEnable(GL_PROGRAM_POINT_SIZE);
	glBindVertexArray(VertexArrayId[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);	// update buffer data
	glDrawElements(GL_POINTS, NumVert[0], GL_UNSIGNED_SHORT, (void*)0);;
	glBindVertexArray(0);

	glUseProgram(0);
	glFlush();
	// --- Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// --- Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];  // 2x2 pixel region
	//glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
	int bufferWidth; int bufferHeight; int currWidth; int currHeight;
	glfwGetFramebufferSize(window, & bufferWidth, & bufferHeight);
	glfwGetWindowSize(window, & currWidth, & currHeight);
	glReadPixels(xpos*(bufferWidth/currWidth), (currHeight - ypos)*(bufferHeight/currHeight), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
       	// window_height - ypos;  
	// OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);

	
	float white[4] = {1.0f,1.0f,1.0f,1.0f};

	if (gPickedIndex >= IndexCount) { 
		// Any number > vertices-indices is background!
		gMessage = "background";
	}
	else {
		if(i==0)
		{
			origColor[0] = Vertices[gPickedIndex].Color[0];
		    origColor[1] = Vertices[gPickedIndex].Color[1];
		    origColor[2] = Vertices[gPickedIndex].Color[2];
		    origColor[3] = Vertices[gPickedIndex].Color[3];
		    Vertices[gPickedIndex].SetColor(white);
		    i = 1;
		}
	    else
	    {
	    	Vertices[gPickedIndex].SetColor(origColor);
	    	i = 0;
	    }
	}
    
}

// ATTN: Project 1A, Task 3 == Retrieve your cursor position, get corresponding world coordinate, and move the point accordingly

// ATTN: Project 1C, Task 1 == Keep track of z coordinate for selected point and adjust its value accordingly based on if certain
// buttons are being pressed
void moveVertex(void) {
	glm::mat4 ModelMatrix = glm::mat4(1.0);
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec4 vp = glm::vec4(viewport[0], viewport[1], viewport[2], viewport[3]);

	int bufferWidth; int bufferHeight; int currWidth; int currHeight;
    glfwGetFramebufferSize(window, &bufferWidth, &bufferHeight);
    glfwGetWindowSize(window, &currWidth, &currHeight);

	if (gPickedIndex >= IndexCount) { 
		// Any number > vertices-indices is background!
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();

		double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glm::vec3 wcoords = glm::unProject(glm::vec3(xpos*(bufferWidth/currWidth),(currHeight - ypos)*(bufferHeight/currHeight), 0.0), ModelMatrix, gProjectionMatrix, vp);
        //std::cout<<"("<<wcoords.x<<", "<<wcoords.y<<", "<<wcoords.z<<") \n";
        float newCoords[4];

        if(shouldZTranslate){
        	newCoords[0] = Vertices[gPickedIndex].Position[0];
	        newCoords[1] = Vertices[gPickedIndex].Position[1];
	        newCoords[2] = wcoords.x;
	        newCoords[3] = 1.0f;
        }
        else{	
	        newCoords[0] = -wcoords.x;
	        newCoords[1] = wcoords.y;
	        newCoords[2] = Vertices[gPickedIndex].Position[2];
	        newCoords[3] = 1.0f;
        }
        // printf("%s", shouldZTranslate ? "true" : "false");
        Vertices[gPickedIndex].SetCoords(newCoords);
        // printf("\n%f %f %f\n", Vertices[gPickedIndex].Position[0],Vertices[gPickedIndex].Position[1],Vertices[gPickedIndex].Position[2]);
	}
}


void renderScene(void) {    
	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	// Re-clear the screen for visible rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(!shouldSplitView){
		glViewport(0.0f, 0.0f, window_width, window_height);
		glUseProgram(programID);
	
		// see comments in pick
		glm::mat4 ModelMatrix = glm::mat4(1.0); 
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		
		glEnable(GL_PROGRAM_POINT_SIZE);

		glBindVertexArray(VertexArrayId[0]);	// Draw Vertices
		glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);		// Update buffer data
		glDrawElements(GL_POINTS, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
		//glDrawElements(GL_LINE_LOOP, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
		// // If don't use indices
		// glDrawArrays(GL_POINTS, 0, NumVerts[0]);	

		// ATTN: OTHER BINDING AND DRAWING COMMANDS GO HERE
		// one set per object:
		// glBindVertexArray(VertexArrayId[<x>]); etc etc
		createVAOs(pVertices[curr_level], pIndices, sizeof(pVertices[curr_level]), sizeof(pIndices), 1);
	    glBindVertexArray(VertexArrayId[1]);
	    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[1]);
	    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pVertices[curr_level]), pVertices[curr_level]);
	    glDrawArrays(GL_POINTS, 0, N * pow(2, curr_level) + 1);
	    if(curr_level!=0)
	    {
	    	glDrawArrays(GL_LINE_LOOP, 0, N * pow(2, curr_level) + 1);
	    }

	    if(task2) {
			createVAOs(c2Vertices, pIndices, sizeof(c2Vertices), sizeof(pIndices), 2);
	        glBindVertexArray(VertexArrayId[2]);
	        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[2]);
	        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c2Vertices), c2Vertices);
	        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
	        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);
	    }

	    if(task3) {
			createVAOs(c1Vertices, pIndices, sizeof(c1Vertices), sizeof(pIndices), 3);
	        glBindVertexArray(VertexArrayId[3]);
	        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[3]);
	        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c1Vertices), c1Vertices);
	        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
	        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);

	        createVAOs(bVerticesArray, pIndices, sizeof(bVerticesArray), sizeof(pIndices), 4);
	        glBindVertexArray(VertexArrayId[4]);
	        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[4]);
	        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bVerticesArray), bVerticesArray);
	        glDrawArrays(GL_LINE_LOOP, 0, N * tessellation_no);

	        if(showAxes) {
                if(pointCount < N * tessellation_no) {
                    movingPoint[0].SetCoords(bVerticesArray[pointCount].Position);
                    movingPoint[0].SetColor(colorYellow);
                    createVAOs(movingPoint, pIndices, sizeof(movingPoint), sizeof(pIndices), 5);
                    glBindVertexArray(VertexArrayId[5]);
                    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[5]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(movingPoint), movingPoint);
                    glDrawElements(GL_POINTS, 1, GL_UNSIGNED_SHORT, (void*)0);

                    tangentPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                    tangentPoints[0].SetColor(colorRed);
                    float angle = atan2(tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0]);
                    angle += 3.14;
                    Vertex vertex[1];
                    vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                    //vertex[0] = {{bVerticesArray[pointCount].Position[0] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[1] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                    tangentPoints[1].SetCoords(vertex[0].Position);
                    tangentPoints[1].SetColor(colorRed);
                    createVAOs(tangentPoints, pIndices, sizeof(tangentPoints), sizeof(pIndices), 6);
                    glBindVertexArray(VertexArrayId[6]);
                    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[6]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tangentPoints), tangentPoints);
                    glDrawArrays(GL_LINE_STRIP, 0, 2);

                    normalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                    normalPoints[0].SetColor(colorGreen);
                    angle = atan2(tVerticesArray[pointCount].Position[0], -tVerticesArray[pointCount].Position[1]);
                    angle += 3.14;
                    vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                    //vertex[0] = {{bVerticesArray[pointCount].Position[0] + (-tVerticesArray[pointCount].Position[1]) * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                    normalPoints[1].SetCoords(vertex[0].Position);
                    normalPoints[1].SetColor(colorGreen);
                    createVAOs(normalPoints, pIndices, sizeof(normalPoints), sizeof(pIndices), 7);
                    glBindVertexArray(VertexArrayId[7]);
                    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[7]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(normalPoints), normalPoints);
                    glDrawArrays(GL_LINE_STRIP, 0, 2);

                    binormalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                    binormalPoints[0].SetColor(colorBlue);
                    //vertex[0] = {{-tVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                    //glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(normalPoints[1].Position[0], normalPoints[1].Position[1], normalPoints[1].Position[2]));
                    glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(-tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0], normalPoints[1].Position[2]));
                    angle = atan2(crossProduct.z, crossProduct.x);
                    angle += 3.14;
                    vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], bVerticesArray[pointCount].Position[1], sin(angle) + crossProduct.z, 1.0f}, {1.0f}};
                    //vertex[0] = {{bVerticesArray[pointCount].Position[0] + crossProduct.x, bVerticesArray[pointCount].Position[1] + crossProduct.y, crossProduct.z, 1.0f}, {1.0f}};
                    binormalPoints[1].SetCoords(vertex[0].Position);
                    binormalPoints[1].SetColor(colorBlue);
                    createVAOs(binormalPoints, pIndices, sizeof(binormalPoints), sizeof(pIndices), 8);
                    glBindVertexArray(VertexArrayId[8]);
                    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[8]);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(binormalPoints), binormalPoints);
                    glDrawArrays(GL_LINE_STRIP, 0, 2);

                    pointCount++;
                } else {
                    pointCount = 0;
                }
            }
	    }
	} else{
		glm::mat4 gProjectionMatrix1 = glm::ortho(-4.0f, 4.0f, -1.5f, 1.5f, 0.0f, 100.0f); // In world coordinates
        glViewport(0.0f, window_height / 2.0f, window_width, window_height / 2.0f);
        glUseProgram(programID);
        {
        	glm::mat4 ModelMatrix = glm::mat4(1.0); 
			glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
			
			glEnable(GL_PROGRAM_POINT_SIZE);

			glBindVertexArray(VertexArrayId[0]);	// Draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);		// Update buffer data
			glDrawElements(GL_POINTS, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
			//glDrawElements(GL_LINE_LOOP, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
			// // If don't use indices
			// glDrawArrays(GL_POINTS, 0, NumVerts[0]);	

			// ATTN: OTHER BINDING AND DRAWING COMMANDS GO HERE
			// one set per object:
			// glBindVertexArray(VertexArrayId[<x>]); etc etc
			createVAOs(pVertices[curr_level], pIndices, sizeof(pVertices[curr_level]), sizeof(pIndices), 1);
		    glBindVertexArray(VertexArrayId[1]);
		    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[1]);
		    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pVertices[curr_level]), pVertices[curr_level]);
		    glDrawArrays(GL_POINTS, 0, N * pow(2, curr_level) + 1);
		    if(curr_level!=0)
		    {
		    	glDrawArrays(GL_LINE_LOOP, 0, N * pow(2, curr_level) + 1);
		    }

		    if(task2) {
				createVAOs(c2Vertices, pIndices, sizeof(c2Vertices), sizeof(pIndices), 2);
		        glBindVertexArray(VertexArrayId[2]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[2]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c2Vertices), c2Vertices);
		        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
		        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);
		    }

		    if(task3) {
				createVAOs(c1Vertices, pIndices, sizeof(c1Vertices), sizeof(pIndices), 3);
		        glBindVertexArray(VertexArrayId[3]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[3]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c1Vertices), c1Vertices);
		        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
		        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);

		        createVAOs(bVerticesArray, pIndices, sizeof(bVerticesArray), sizeof(pIndices), 4);
		        glBindVertexArray(VertexArrayId[4]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[4]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bVerticesArray), bVerticesArray);
		        glDrawArrays(GL_LINE_LOOP, 0, N * tessellation_no);

		        if(showAxes) {
                    if(pointCount < N * tessellation_no) {
                        movingPoint[0].SetCoords(bVerticesArray[pointCount].Position);
                        movingPoint[0].SetColor(colorYellow);
                        createVAOs(movingPoint, pIndices, sizeof(movingPoint), sizeof(pIndices), 5);
                        glBindVertexArray(VertexArrayId[5]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[5]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(movingPoint), movingPoint);
                        glDrawElements(GL_POINTS, 1, GL_UNSIGNED_SHORT, (void*)0);

                        tangentPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        tangentPoints[0].SetColor(colorRed);
                        float angle = atan2(tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0]);
                        angle += 3.14;
                        Vertex vertex[1];
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[1] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        tangentPoints[1].SetCoords(vertex[0].Position);
                        tangentPoints[1].SetColor(colorRed);
                        createVAOs(tangentPoints, pIndices, sizeof(tangentPoints), sizeof(pIndices), 6);
                        glBindVertexArray(VertexArrayId[6]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[6]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tangentPoints), tangentPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        normalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        normalPoints[0].SetColor(colorGreen);
                        angle = atan2(tVerticesArray[pointCount].Position[0], -tVerticesArray[pointCount].Position[1]);
                        angle += 3.14;
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + (-tVerticesArray[pointCount].Position[1]) * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        normalPoints[1].SetCoords(vertex[0].Position);
                        normalPoints[1].SetColor(colorGreen);
                        createVAOs(normalPoints, pIndices, sizeof(normalPoints), sizeof(pIndices), 7);
                        glBindVertexArray(VertexArrayId[7]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[7]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(normalPoints), normalPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        binormalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        binormalPoints[0].SetColor(colorBlue);
                        //vertex[0] = {{-tVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(normalPoints[1].Position[0], normalPoints[1].Position[1], normalPoints[1].Position[2]));
                        glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(-tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0], normalPoints[1].Position[2]));
                        angle = atan2(crossProduct.z, crossProduct.x);
                        angle += 3.14;
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], bVerticesArray[pointCount].Position[1], sin(angle) + crossProduct.z, 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + crossProduct.x, bVerticesArray[pointCount].Position[1] + crossProduct.y, crossProduct.z, 1.0f}, {1.0f}};
                        binormalPoints[1].SetCoords(vertex[0].Position);
                        binormalPoints[1].SetColor(colorBlue);
                        createVAOs(binormalPoints, pIndices, sizeof(binormalPoints), sizeof(pIndices), 8);
                        glBindVertexArray(VertexArrayId[8]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[8]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(binormalPoints), binormalPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        pointCount++;
                    } else {
                        pointCount = 0;
                    }
                }
		    }
        }
		glUseProgram(programID);
		{
			glViewport(0.0f, 0.0f, window_width, window_height / 2.0f);
            glm::mat4 gProjectionMatrix2 = glm::ortho(-4.0f, 4.0f, -1.5f, 1.5f, 0.0f, 100.0f); // In world coordinates
            glm::mat4 gViewMatrix2 = glm::lookAt(
                glm::vec3(-5.0f, 0.0f, 0.0f), // Camera is at (4,3,3), in World Space
                glm::vec3(0.0f, 0.0f, 0.0f), // and looks at the origin
                glm::vec3(0.0f, 1.0f, 0.0f)  // Head is up (set to 0,-1,0 to look upside-down)
            );
            glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
            glm::mat4 MVP = gProjectionMatrix2 * gViewMatrix2 * ModelMatrix;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix2[0][0]);
			
			glEnable(GL_PROGRAM_POINT_SIZE);

			glBindVertexArray(VertexArrayId[0]);	// Draw Vertices
			glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[0]);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);		// Update buffer data
			glDrawElements(GL_POINTS, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
			//glDrawElements(GL_LINE_LOOP, NumIdcs[0], GL_UNSIGNED_SHORT, (void*)0);
			// // If don't use indices
			// glDrawArrays(GL_POINTS, 0, NumVerts[0]);	

			// ATTN: OTHER BINDING AND DRAWING COMMANDS GO HERE
			// one set per object:
			// glBindVertexArray(VertexArrayId[<x>]); etc etc
			createVAOs(pVertices[curr_level], pIndices, sizeof(pVertices[curr_level]), sizeof(pIndices), 1);
		    glBindVertexArray(VertexArrayId[1]);
		    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[1]);
		    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pVertices[curr_level]), pVertices[curr_level]);
		    glDrawArrays(GL_POINTS, 0, N * pow(2, curr_level) + 1);
		    if(curr_level!=0)
		    {
		    	glDrawArrays(GL_LINE_LOOP, 0, N * pow(2, curr_level) + 1);
		    }

		    if(task2) {
				createVAOs(c2Vertices, pIndices, sizeof(c2Vertices), sizeof(pIndices), 2);
		        glBindVertexArray(VertexArrayId[2]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[2]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c2Vertices), c2Vertices);
		        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
		        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);
		    }

		    if(task3) {
				createVAOs(c1Vertices, pIndices, sizeof(c1Vertices), sizeof(pIndices), 3);
		        glBindVertexArray(VertexArrayId[3]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[3]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(c1Vertices), c1Vertices);
		        glDrawElements(GL_POINTS, 40, GL_UNSIGNED_SHORT, (void*)0);
		        glDrawElements(GL_LINE_LOOP, 40, GL_UNSIGNED_SHORT, (void*)0);

		        createVAOs(bVerticesArray, pIndices, sizeof(bVerticesArray), sizeof(pIndices), 4);
		        glBindVertexArray(VertexArrayId[4]);
		        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[4]);
		        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bVerticesArray), bVerticesArray);
		        glDrawArrays(GL_LINE_LOOP, 0, N * tessellation_no);

		        if(showAxes) {
                    if(pointCount < N * tessellation_no) {
                        movingPoint[0].SetCoords(bVerticesArray[pointCount].Position);
                        movingPoint[0].SetColor(colorYellow);
                        createVAOs(movingPoint, pIndices, sizeof(movingPoint), sizeof(pIndices), 5);
                        glBindVertexArray(VertexArrayId[5]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[5]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(movingPoint), movingPoint);
                        glDrawElements(GL_POINTS, 1, GL_UNSIGNED_SHORT, (void*)0);

                        tangentPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        tangentPoints[0].SetColor(colorRed);
                        float angle = atan2(tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0]);
                        angle += 3.14;
                        Vertex vertex[1];
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[1] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        tangentPoints[1].SetCoords(vertex[0].Position);
                        tangentPoints[1].SetColor(colorRed);
                        createVAOs(tangentPoints, pIndices, sizeof(tangentPoints), sizeof(pIndices), 6);
                        glBindVertexArray(VertexArrayId[6]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[6]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(tangentPoints), tangentPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        normalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        normalPoints[0].SetColor(colorGreen);
                        angle = atan2(tVerticesArray[pointCount].Position[0], -tVerticesArray[pointCount].Position[1]);
                        angle += 3.14;
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], sin(angle) + bVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + (-tVerticesArray[pointCount].Position[1]) * 5, bVerticesArray[pointCount].Position[1] + tVerticesArray[pointCount].Position[0] * 5, bVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        normalPoints[1].SetCoords(vertex[0].Position);
                        normalPoints[1].SetColor(colorGreen);
                        createVAOs(normalPoints, pIndices, sizeof(normalPoints), sizeof(pIndices), 7);
                        glBindVertexArray(VertexArrayId[7]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[7]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(normalPoints), normalPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        binormalPoints[0].SetCoords(bVerticesArray[pointCount].Position);
                        binormalPoints[0].SetColor(colorBlue);
                        //vertex[0] = {{-tVerticesArray[pointCount].Position[1], bVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2], 1.0f}, {1.0f}};
                        //glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(normalPoints[1].Position[0], normalPoints[1].Position[1], normalPoints[1].Position[2]));
                        glm::vec3 crossProduct = glm::cross(glm::vec3(tVerticesArray[pointCount].Position[0], tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[2]), glm::vec3(-tVerticesArray[pointCount].Position[1], tVerticesArray[pointCount].Position[0], normalPoints[1].Position[2]));
                        angle = atan2(crossProduct.z, crossProduct.x);
                        angle += 3.14;
                        vertex[0] = {{cos(angle) + bVerticesArray[pointCount].Position[0], bVerticesArray[pointCount].Position[1], sin(angle) + crossProduct.z, 1.0f}, {1.0f}};
                        //vertex[0] = {{bVerticesArray[pointCount].Position[0] + crossProduct.x, bVerticesArray[pointCount].Position[1] + crossProduct.y, crossProduct.z, 1.0f}, {1.0f}};
                        binormalPoints[1].SetCoords(vertex[0].Position);
                        binormalPoints[1].SetColor(colorBlue);
                        createVAOs(binormalPoints, pIndices, sizeof(binormalPoints), sizeof(pIndices), 8);
                        glBindVertexArray(VertexArrayId[8]);
                        glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[8]);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(binormalPoints), binormalPoints);
                        glDrawArrays(GL_LINE_STRIP, 0, 2);

                        pointCount++;
                    } else {
                        pointCount = 0;
                    }
                }

		    }
		}

	}
	// ATTN: Project 1C, Task 2 == Refer to https://learnopengl.com/Getting-started/Transformations and
	// https://learnopengl.com/Getting-started/Coordinate-Systems - draw all the objects associated with the
	// curve twice in the displayed fashion using the appropriate transformations

	glBindVertexArray(0);
	glUseProgram(0);
	// Draw GUI
	TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

// Alternative way of triggering functions on mouse click and keyboard events
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickVertex();
	}
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        pickVertex();
    }
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_1 && action == GLFW_PRESS) {
        if(curr_level == M -1) {
            curr_level = 0;
        } else {
            curr_level += 1;
        }
    }

    if(key == GLFW_KEY_2 && action == GLFW_PRESS) {
        if(!task2) {
            task2 = true;
            createObjects();
        } else {
            task2 = false;
        }
    }

    if(key == GLFW_KEY_3 && action == GLFW_PRESS) {
        if(!task3) {
            task3 = true;
            createObjects();
        } else {
            task3 = false;
        }
    }

    if((key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) && action == GLFW_PRESS) {
    	if(!shouldZTranslate){
    		shouldZTranslate = true;
        	// printf("\n%s", shouldZTranslate ? "true" : "false");
    	} else {
	        shouldZTranslate = false;
	        // printf("\n%s", shouldZTranslate ? "true" : "false");
	    }
	}

	if(key == GLFW_KEY_4 && action == GLFW_PRESS) {
        if(shouldSplitView) {
            shouldSplitView = false;
        } else {
            shouldSplitView = true;
        }
    }

    if(key == GLFW_KEY_5 && action == GLFW_PRESS) {
        if(showAxes) {
            showAxes = false;
        } else {
            showAxes = true;
        }
    }
}

int main(void) {
	// ATTN: REFER TO https://learnopengl.com/Getting-started/Creating-a-window
	// AND https://learnopengl.com/Getting-started/Hello-Window to familiarize yourself with the initialization of a window in OpenGL

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// ATTN: REFER TO https://learnopengl.com/Getting-started/Hello-Triangle to familiarize yourself with the graphics pipeline
	// from setting up your vertex data in vertex shaders to rendering the data on screen (everything that follows)

	// Initialize OpenGL pipeline
	initOpenGL();
	glLineWidth( 3);

	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		// Timing 
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// DRAGGING: move current (picked) vertex with cursor
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
			moveVertex();
		}

		
		// ATTN: Project 1B, Task 2 and 4 == account for key presses to activate subdivision and hiding/showing functionality
		// for respective tasks

		// DRAWING the SCENE
		createObjects();	// re-evaluate curves in case vertices have been moved
		renderScene();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}
