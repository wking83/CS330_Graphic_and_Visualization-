///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
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

	modelView = translation * rotationX * rotationY * rotationZ * scale;

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
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
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

	bReturn = CreateGLTexture(
		"../../Utilities/textures/desk.jpg",
		"desk");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/coffee.jpg",
		"coffee");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/mug.jpg",
		"mug");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/wood_light_seamless.jpg",
		"floor");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/keyboard.jpg",
		"keyboard");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/screen.jpg",
		"screen");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/mug_handle.jpg",
		"handle");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/paper_book.jpg",
		"paper_book");




	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL mugMaterial;
	mugMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);   
	mugMaterial.ambientStrength = 0.2f;
	mugMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);   
	mugMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f); 
	mugMaterial.shininess = 4.0f;                          
	mugMaterial.tag = "mug";

	m_objectMaterials.push_back(mugMaterial);


	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);   
	plasticMaterial.ambientStrength = 0.25f;                          
	plasticMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);       
	plasticMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);     
	plasticMaterial.shininess = 30.0f;                                
	plasticMaterial.tag = "plastic";

	m_objectMaterials.push_back(plasticMaterial);

	OBJECT_MATERIAL handleMaterial;
	handleMaterial.ambientColor = glm::vec3(0.4f, 0.2f, 0.05f);   
	handleMaterial.ambientStrength = 0.3f;
	handleMaterial.diffuseColor = glm::vec3(0.96f, 0.45f, 0.18f); 
	handleMaterial.specularColor = glm::vec3(0.03f, 0.015f, 0.01f); 
	handleMaterial.shininess = 6.0f;                            
	handleMaterial.tag = "mugHandle";

	m_objectMaterials.push_back(handleMaterial);

	OBJECT_MATERIAL screenMaterial;
	screenMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);   
	screenMaterial.ambientStrength = 0.2f;                              
	screenMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);       
	screenMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);       
	screenMaterial.shininess = 128.0f;                            
	screenMaterial.tag = "screen";

	m_objectMaterials.push_back(screenMaterial);

	OBJECT_MATERIAL bookCoverMaterial;
	bookCoverMaterial.ambientColor = glm::vec3(0.2f, 0.25f, 0.3f);   
	bookCoverMaterial.ambientStrength = 0.3f;                            
	bookCoverMaterial.diffuseColor = glm::vec3(0.5f, 0.7f, 1.0f);    
	bookCoverMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f); 
	bookCoverMaterial.shininess = 8.0f;                           
	bookCoverMaterial.tag = "book_cover";

	m_objectMaterials.push_back(bookCoverMaterial);

	OBJECT_MATERIAL bookSideMaterial;
	bookSideMaterial.ambientColor = glm::vec3(0.85f, 0.85f, 0.8f);   
	bookSideMaterial.ambientStrength = 0.2f;                          
	bookSideMaterial.diffuseColor = glm::vec3(0.75f, 0.75f, 0.7f);     
	bookSideMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);   
	bookSideMaterial.shininess = 4.0f;                                 
	bookSideMaterial.tag = "book_side";

	m_objectMaterials.push_back(bookSideMaterial);




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
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	//ambient
	m_pShaderManager->setVec3Value("lightSources[0].direction", 0.0f, -1.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.55f, 0.55f, 0.5f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.65f, 0.65f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.0f);



	//computer screen light	

	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 2.77f, -0.4f);         
	m_pShaderManager->setVec3Value("lightSources[1].spotDirection", 0.0f, -0.1f, 0.8f);        
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.04f, 0.05f, 0.1f);        
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.1f, 0.15f, 0.4f);          
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.1f, 0.1f, 0.2f);         
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 3.0f);                   
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05);               





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
	//load scene textures
	LoadSceneTextures();
	DefineObjectMaterials();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh(.1f);
	m_basicMeshes->LoadPrismMesh();
	
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
	{
		/*** Set needed transformations before drawing the basic mesh.  ***/
		/*** This same ordering of code should be used for transforming ***/
		/*** and drawing all the basic 3D shapes.						***/
		/******************************************************************/
		// set the XYZ scale for the mesh
		scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);

		// set the XYZ rotation for the mesh
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 0.0f;

		// set the XYZ position for the mesh
		positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

		// set the transformations into memory to be used on the drawn meshes
		SetTransformations(
			scaleXYZ,
			XrotationDegrees,
			YrotationDegrees,
			ZrotationDegrees,
			positionXYZ);

		//SetShaderColor(1, 1, 1, 1);

		// draw the mesh with transformation values
		SetShaderTexture("floor");
		SetTextureUVScale(1.0, 1.0);
		SetShaderMaterial("wood");
		m_basicMeshes->DrawPlaneMesh();
		/****************************************************************/
	}
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//mug body
	scaleXYZ = glm::vec3(.8f, 1.8f, 0.8f); 
	positionXYZ = glm::vec3(-5.5f, 0.5f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	
	SetShaderTexture("coffee");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawCylinderMesh(true, false, false); //drawing top of cyl to look like coffee
	//SetShaderColor(0.33f, 0.35f, 0.36f, 1.0f);
	SetShaderTexture("mug");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("mug");
	m_basicMeshes->DrawCylinderMesh();

	//top lip torus
	scaleXYZ = glm::vec3(.745f, .745f, 0.27f); 
	positionXYZ = glm::vec3(-5.5f, 2.29f, 4.0f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.33f, 0.35f, 0.36f, 1.0f);
	SetShaderTexture("mug");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("mug");
	m_basicMeshes->DrawTorusMesh();

	//Handle
	scaleXYZ = glm::vec3(0.6f, 0.6f, 0.75f);
	positionXYZ = glm::vec3(-4.8f, 1.5f, 4.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 270.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("handle");
	SetTextureUVScale(1.0, 1.0);
	//SetShaderColor(0.96f, 0.45f, 0.18f, 1.0f);
	SetShaderMaterial("mugHandle");
	m_basicMeshes->DrawHalfTorusMesh();

	//table
	scaleXYZ = glm::vec3(11.0f, 0.5f, 11.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.13f, 0.11f, 0.10f, 1.0f);
	SetShaderTexture("desk");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();

	//computer keyboard
	scaleXYZ = glm::vec3(6.0f, 0.2f, 4.0f);
	positionXYZ = glm::vec3(0.0f, 0.6f, 2.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("keyboard");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_top);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	//computer screen
	scaleXYZ = glm::vec3(6.0f, 0.2f, 4.0f);
	positionXYZ = glm::vec3(0.0f, 2.77f, -0.40f);
	XrotationDegrees = 80.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("screen");
	SetShaderMaterial("screen");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::box_top);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawBoxMesh();

	//HINGE
	scaleXYZ = glm::vec3(.15f, 6.0f, .15f);
	positionXYZ = glm::vec3(3.0f, 0.75f, -0.02f);
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	//mouse
	scaleXYZ = glm::vec3(0.6f, 0.25f, 1.1f);  
	positionXYZ = glm::vec3(4.5f, 0.5f, 1.5f);  
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.5f, 0.5f, 0.5f, 1.0f);
	SetShaderMaterial("plastic");  
	m_basicMeshes->DrawHalfSphereMesh();

	//mouse scroll
	scaleXYZ = glm::vec3(0.05f, 0.05f, 0.15f);      
	positionXYZ = glm::vec3(4.5f, 0.75f, 1.0f);          
	XrotationDegrees = 0.0f;                               
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);            
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();

	
	// Pages 
	scaleXYZ = glm::vec3(2.0f, 0.5f, 3.0f);
	positionXYZ = glm::vec3(7.5f, 0.8f, 2.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("paper_book");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();

	// Top Cover
	scaleXYZ = glm::vec3(2.01f, 0.05f, 3.01f);
	positionXYZ = glm::vec3(7.5f, 1.05f, 2.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("book_cover");
	SetShaderColor(0.3f, 0.5f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Bottom Cover
	scaleXYZ = glm::vec3(2.01f, 0.05f, 3.01f);
	positionXYZ = glm::vec3(7.5f, 0.55f, 2.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("book_cover");
	SetShaderColor(0.3f, 0.5f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// side cover
	scaleXYZ = glm::vec3(0.05f, 0.55f, 3.01f);
	positionXYZ = glm::vec3(6.49f, 0.8f, 2.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("book_side");
	SetShaderColor(0.65f, 0.65f, 0.6f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}
