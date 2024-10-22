///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"
#include "GLFW/glfw3.h"
#include "iostream"
#include "camera.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	float fieldOV = 90.0f; // Field of view camera
	float speedIncrement = 0.1f; // mouse sensitivity for camera movement
	float yawVA = -90.0f; // rotation on y axis
	float pitchSS = 0.0f; // rotation on x axis

	// time between current frame and last frame
	float gDeltaTime = 2.5f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();

	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 20.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 10;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// callback to receive mouse scroll movmment
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	//callback to receive keyboard events
	glfwSetKeyCallback(window, &ViewManager::pressKey_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}
	// print mouse position to the screen
	std::cout << xMousePos << ":" << yMousePos << std::endl;

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	 float xOffset = xMousePos - gLastX;
	 float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	//mouse speed for offsets
	
	xOffset *= speedIncrement;
	yOffset *= speedIncrement;

	//yaw and pitch offsets
	yawVA += xOffset;
	pitchSS += yOffset;

	//stop screen from flipping from pitch

	if (pitchSS > 89.0f)
		pitchSS = 89.0f;
	if (pitchSS < -89.0f)
		pitchSS = -89.0f;

	// front vector  for camera

	glm::vec3 cameraF;
	cameraF.x = cos(glm::radians(yawVA)) * cos(glm::radians(pitchSS));
	cameraF.y = sin(glm::radians(pitchSS));
	cameraF.z = sin(glm::radians(yawVA)) * cos(glm::radians(pitchSS));


	// move the 3D camera according to the front vector
	g_pCamera->Front = glm::normalize(cameraF);
}
/***********************************************************
*Scroll Callback()
*
* This method is called from GLFW whenever
* the mouse scroll wheel is moved within the active GLFW display window
***********************************************************/

void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset) {

	const float adjScrollOffset = yOffset * speedIncrement;

	// field of view developed on scroll adj offset
	fieldOV += adjScrollOffset;

	// set field of view to go no loweer than min value
	if (fieldOV < 1.0f)
		fieldOV = 1.0f; // set min speed

	// set field view to go no higher than max value
	if (fieldOV > 90.0f)
		fieldOV = 90.0f; // set max speed

	// print scroll movements
	std::cout << xOffset << " : " << yOffset << std::endl;

}


/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}
}
/***********************************************************
 *  pressKey_Callback()
 *
 *  This method is called from the GLFW whenever a key is pressed
 *  within the active GLFW window.
 ***********************************************************/

void ViewManager::pressKey_Callback(GLFWwindow* window, int compKey, int Scode, int press, int adj) {
	
	//if P key is pressed change to Perspective view
	if (press == GLFW_PRESS) {
		if (compKey == GLFW_KEY_P) {
			bOrthographicProjection = false;
		}
		// if O key is pressed change to Orthographic view
		else if (compKey == GLFW_KEY_O) {
			bOrthographicProjection = true;
		}
	}

}


/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	if (bOrthographicProjection) {

		// orthographic projection
		float aspect = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
		float orthoHeight = 10.0f;
		projection = glm::ortho(-aspect * orthoHeight, aspect * orthoHeight, -orthoHeight, orthoHeight, 0.1f, 100.0f);

	}
	else
	{

		// define the current projection matrix
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
