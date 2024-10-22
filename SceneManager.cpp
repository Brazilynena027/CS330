///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	DestroyGLTextures();

	
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{

		//State a material to store in material properties
		OBJECT_MATERIAL material;
		bool bReturn = false;

		//search for materials with a tag
		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{

			//Set diffuse color of the material
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			//Set specular color of material
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			//Set shininess of the material
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
			//Set the ambient color of material
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			//Set the ambient strength of material
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/

	bool bReturn = false;

	// Set bottle with the texture stored in the source file of project
	 bReturn= CreateGLTexture("Source/textures/black_back.jpg", "glass");

	//Set bottle top shape with the texture stored in the source file in project
	 bReturn = CreateGLTexture("Source/textures/Gold_Metal.jpg", "top");

	//Set bottle top shape with the texture stored in the source file in project
	 bReturn = CreateGLTexture("Source/textures/Wood_Table.jpg", "table");

	//Set bottle top shape with the texture stored in the source file in project
	 bReturn = CreateGLTexture("Source/textures/swiss_cheese.jpg", "cheese");

	//Set bottle top shape with the texture stored in the source file in project
	 bReturn = CreateGLTexture("Source/textures/pear.jpg", "pear");


	
	// print statemnent to indicate if image is loading or failing to load
	
	if (bReturn) {
		std::cout << "Texture loaded successfully." << std::endl;
	}
	else {
		std::cout << "Failed to load texture." << std::endl;
	}

	

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();

}




/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	// shapes to be rendered in scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadBoxMesh();
	
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/




	/*******Table for Scene********************************************/
	/******************************************************************/

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(45.0f, 5.0f, 20.0f);

	// set the XYZ rotation for the mesh for the plane
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for the plane
	positionXYZ = glm::vec3(0.0f, -15.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of the plane

	SetShaderColor(1, 1, 1, 1);

	// set texture of shape
	SetShaderTexture("table");

	// set material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values for the plaane
	m_basicMeshes->DrawPlaneMesh();


	/*******Wine Bottle***********************************************/
	/*****************************************************************/

	/*******Base Of Wine Bottle*************************************/

	// set the XYZ scale for the mesh for the base of the Bottle 
	scaleXYZ = glm::vec3(2.5f, 12.25f, 2.0f);

	// set the XYZ rotation for the mesh for the base of the Bottle
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for the base of the Bottle
	positionXYZ = glm::vec3(0.0f, -8.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of cylinder
	SetShaderColor(1, 1, 1, 1);

	// set texture of shape
	SetShaderTexture("glass");

	// set material
	SetShaderMaterial("glass");


	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	/*******Curve for  top of Base for Wine bottle****************/

	// set the XYZ scale for the mesh for  curve on neck of bottle
	scaleXYZ = glm::vec3(2.1f, 1.5f, 0.0f);

	// set the XYZ rotation for the mesh for curve on neck of bottle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for curve on neck of bottle
	positionXYZ = glm::vec3(0.0f, 4.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of cone
	SetShaderColor(1, 0, 0, 1);

	// set texture of shape
	SetShaderTexture("glass");

	//set material
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();




	/******Base of neck for Bottle**********************************/


	// set the XYZ scale for the mesh for  curve on neck of bottle
	scaleXYZ = glm::vec3(1.25f, 3.25f, 0.0f);

	// set the XYZ rotation for the mesh for curve on neck of bottle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for curve on neck of bottle
	positionXYZ = glm::vec3(0.0f, 4.80f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of cone
	SetShaderColor(1, 0, 0, 1);

	// set texture of shape
	SetShaderTexture("glass");

	//set material
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	/*******Neck of Bottle*****************************************/

	// set the XYZ scale for the mesh for small neck of bottle
	scaleXYZ = glm::vec3(0.67f, 2.5f, 0.0f);

	// set the XYZ rotation for the mesh for small neck of bottle
	XrotationDegrees = -10.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for small neck of bottle
	positionXYZ = glm::vec3(0.0f, 8.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set color for small neck of bottle

	SetShaderColor(2, 2, 0, 2);

	// set texture of shape
	SetShaderTexture("top");


	//set material
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();




	/******* Ridge of Bottle*****************************************/

	// set the XYZ scale for the mesh for ridge of bottle
	scaleXYZ = glm::vec3(0.75f, .60f, 0.0f);

	// set the XYZ rotation for the mesh for ridge of  bottle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for ridge of bottle
	positionXYZ = glm::vec3(0.0f, 9.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//set color for ridge of bottle

	SetShaderColor(2, 1, 1, 2);

	//set texture
	SetShaderTexture("top");

	//set material
	SetShaderMaterial("gold");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();



	/********Mouth Piece of Bottle***************************************/


	// set the XYZ scale for the mesh for  bottle cap
	scaleXYZ = glm::vec3(.74f, .75f, 0.0f);

	// set the XYZ rotation for the mesh for bottle cap
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh for bottle cap
	positionXYZ = glm::vec3(0.0f, 9.93f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color of bottle cap
	SetShaderColor(1, 0, 0, 1);

	// set texture of shape
	SetShaderTexture("gold");

	//set material
	SetShaderMaterial("glass");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();


	/******* Pear *********************************************/
	/****************************************************************/


	//*******pear base*********************************************/

	scaleXYZ = glm::vec3(1.75f, 2.0f, 2.0f);

	// Set the XYZ rotation for pear base
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for pear base
	positionXYZ = glm::vec3(-3.5f, -4.2f, 2.5f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations
	(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of pear
	SetShaderColor(0.0f, 1.0f, 0.0f, 1.0f);

	// Set the texture
	SetShaderTexture("pear");

	// Set the material
	SetShaderMaterial("plastic");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();



	/*******Pear Middle******************************************/

	// pear middle

	scaleXYZ = glm::vec3(1.72f, 4.0f, 2.0f);

	// Set the XYZ rotation for pear middle
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -30.0f;

	// Set the XYZ position for pear middle
	positionXYZ = glm::vec3(-3.60f, -4.2f, 2.5f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations
	(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of pear
	SetShaderColor(0.0f, 1.0f, 0.0f, 1.0f);

	// Set the texture
	SetShaderTexture("pear");

	// Set the material
	SetShaderMaterial("plastic");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();



	/******* Pear Stem ******************************************/
	// pear top

	scaleXYZ = glm::vec3(-0.1f, 1.75f, 0.0f);

	// Set the XYZ rotation for pear stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 1.0f;
	ZrotationDegrees = -30.0f;

	// Set the XYZ position for pear stem
	positionXYZ = glm::vec3(-1.75f, -1.0f, 2.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations
	(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of pear stem
	SetShaderColor(0.36f, 0.25f, 0.20f, 1.0f);

	//Set material
	SetShaderMaterial("wood");


	// Draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();






	/******* Cheese *********************************************/
	/****************************************************************/


	/******* Cheese Wedge ******************************************/
	// Cheese wedge base

	scaleXYZ = glm::vec3(4.5f, 3.75f, 3.0f);

	// Set the XYZ rotation for Cheese Wedge
	XrotationDegrees = 45.0f;
	YrotationDegrees = 15.0f;
	ZrotationDegrees = -75.0f;

	// Set the XYZ position for Cheese Wedge
	positionXYZ = glm::vec3(4.60f, -5.75f, 0.90f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Set the color of Cheese Wedge
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f); // Yellow color

	// Set the material
	SetShaderTexture("cheese");

	//Set material
	SetShaderMaterial("plastic");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawPrismMesh();



	/******* Wine Glass*********************************************/
	/****************************************************************/


	/******* Wine Glass Base ******************************************/
	// Wine Glass base
	scaleXYZ = glm::vec3(0.25f, 2.65f,1.0f);

	// Set the XYZ rotation for Wine Glass Base
	XrotationDegrees = 65.0f;
	YrotationDegrees = 60.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for Wine Glass Base
	positionXYZ = glm::vec3(3.0f, -7.75f, 3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations
	(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of Wine Glass Base
	SetShaderColor(0.8f, 0.8f, 0.9f, 0.5f);


	// Set the material
	SetShaderMaterial("");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();


	/******* Wine Glass Stem ******************************************/


	// Wine Glass stem
	scaleXYZ = glm::vec3(-0.30f, 6.45f, 0.0f);

	// Set the XYZ rotation for Wine Glass Stem
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 3.5f;

	// Set the XYZ position for Wine Glass Stem
	positionXYZ = glm::vec3(3.30f, -8.0f, 3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations
	(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Set the color of Wine Glass Stem
	SetShaderColor(0.8f, 0.8f, 0.9f, 0.5f);


	// Set the material
	SetShaderMaterial("glass");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();



	/******* Wine Glass Bowl (Cylinder) ******************************************/

// Wine Glass bowl (cylinder part)
	scaleXYZ = glm::vec3(1.95f, 1.75, 8.60f);

	// Set the XYZ rotation for Wine Glass Bowl
	XrotationDegrees = 90.0f; // Rotate to make it upright
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Set the XYZ position for Wine Glass Bowl
	positionXYZ = glm::vec3(2.75f, .75f, 3.0f);

	// Set the transformations into memory to be used on the drawn meshes
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Set the color of Wine Glass Bowl
	SetShaderColor(0.8f, 0.8f, 0.9f, 0.25f);

	// Set the material
	SetShaderMaterial("glass");

	// Draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
}




	/***********************************************************
	 *  DefineObjectMaterials()
	 *
	 *  This method is used for configuring the various material
	 *  settings for all of the objects within the 3D scene.
	 ***********************************************************/
	void SceneManager::DefineObjectMaterials()
	{
		/*** STUDENTS - add the code BELOW for defining object materials. ***/
		/*** There is no limit to the number of object materials that can ***/
		/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

		OBJECT_MATERIAL woodMaterial;
		woodMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.1f);
		woodMaterial.ambientStrength = 0.4f;
		woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
		woodMaterial.specularColor = glm::vec3(0.6f, 0.5f, 0.4f);
		woodMaterial.shininess = 22.0;
		woodMaterial.tag = "wood";

		m_objectMaterials.push_back(woodMaterial);


		OBJECT_MATERIAL glassMaterial;
		glassMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
		glassMaterial.ambientStrength = 0.2f;
		glassMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
		glassMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
		glassMaterial.shininess = 0.3;
		glassMaterial.tag = "glass";
		m_objectMaterials.push_back(glassMaterial);


		OBJECT_MATERIAL plasticMaterial;
		plasticMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
		plasticMaterial.ambientStrength = 0.3f;
		plasticMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
		plasticMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
		plasticMaterial.shininess = 85.0;
		plasticMaterial.tag = "plastic";

		m_objectMaterials.push_back(plasticMaterial);



	}

	/***********************************************************
	 *  SetupSceneLights()
	 *
	 *  This method is called to add and configure the light
	 *  sources for the 3D scene.  There are up to 4 light sources.
	 ***********************************************************/
	void SceneManager::SetupSceneLights()
	{
		// this line of code is NEEDED for telling the shaders to render 
		// the 3D scene with custom lighting, if no light sources have
		// been added then the display window will be black - to use the 
		// default OpenGL lighting then comment out the following line
		//m_pShaderManager->setBoolValue(g_UseLightingName, true);

		/*** STUDENTS - add the code BELOW for setting up light sources ***/
		/*** Up to four light sources can be defined. Refer to the code ***/
		/*** in the OpenGL Sample for help
		***/



		m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
		m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.3f, 0.3f, 0.3f);
		m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.8f, 0.8f, 0.8f);
		m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.5f, 0.5f, 0.5f);
		m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
		m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 1.0f);

		m_pShaderManager->setVec3Value("lightSources[1].position", 3.0f, 14.0f, 0.0f);
		m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.03f, 0.03f, 0.03f);
		m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.8f, 0.8f, 0.8f);
		m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.5f, 0.5f, 0.5f);
		m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
		m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 1.1f);


		m_pShaderManager->setVec3Value("lightSources[2].position", 3.0f, 14.0f, 0.0f);
		m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.03f, 0.03f, 0.03f);
		m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.8f, 0.8f, 0.8f);
		m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.5f, 0.5f, 0.5f);
		m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 32.0f);
		m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 1.6f);


		m_pShaderManager->setBoolValue("bUseLighting", true);


	}

//*********************************************************************************
