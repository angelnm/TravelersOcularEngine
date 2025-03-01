#include "TMesh.h"

#include "./../../TOcularEngine/VideoDriver.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

TMesh::TMesh(std::string meshPath, std::string texturePath){
	m_mesh = nullptr;					// 
	m_texture = nullptr;				//
	m_specularMap = nullptr;			//
	m_bumpMap = nullptr;				//
	m_material = nullptr;				// Recursos del TMesh inicializados a nullptr
	m_visibleBB = false;				// Por defecto no se pinta la bounding box
	m_drawingShadows = false;			// Valor por defecto de pintar las sombras
	m_textureScaleX = 1.0f;				//
	m_textureScaleY = 1.0f;				// Por defecto las texturas no estan escaladas
	m_frameDrawed = 0;					// Ultimo frame en el que se pintado el mesh

	LoadMesh(meshPath);					// Cargamos el mesh
	ChangeTexture(texturePath);			// Cargamos la textura
	m_program = STANDARD_SHADER;		// Programa estandar con el que va a pintarse
}

TMesh::~TMesh(){}

void TMesh::LoadMesh(std::string meshPath){
	// En el caso de pasar un string vacio cargamos un cubo como mesh
	if(meshPath.compare("")==0) meshPath = VideoDriver::GetInstance()->GetAssetsPath() + "/models/cube.obj";
	m_mesh = TResourceManager::GetInstance()->GetResourceMesh(meshPath);
}

void TMesh::ChangeTexture(std::string texturePath){
	if(texturePath.compare("")!=0) m_texture = TResourceManager::GetInstance()->GetResourceTexture(texturePath);
	else m_texture = nullptr;
}

void TMesh::ChangeBumpMap(std::string texturePath){
	if(texturePath.compare("")!=0) m_bumpMap = TResourceManager::GetInstance()->GetResourceTexture(texturePath);
	else m_bumpMap = nullptr;
}

void TMesh::ChangeSpecularMap(std::string texturePath){
	if(texturePath.compare("")!=0) m_specularMap = TResourceManager::GetInstance()->GetResourceTexture(texturePath);
	else m_specularMap = nullptr;
}

void TMesh::SetBBVisibility(bool visible){
	m_visibleBB = visible;
}

void TMesh::BeginDraw(){
	if(m_mesh != nullptr && !m_drawingShadows && CheckClipping()){	// Comprobamos que haya mesh, que no vayan a pintarse las sombras y que el objeto este dentro de la pantalla
		unsigned int currentFrame = TEntity::currentFrame;			// Cargamos el frame en el que se ha pintado el mesh
		if(currentFrame != m_frameDrawed){							//  
			m_frameDrawed = currentFrame;							// En el caso de que sea diferente al del mesh lo actualizamos y pintamos

			SendShaderData();										// Enviamos la informacion a los shaders
			
			GLuint elementsBuffer = m_mesh->GetElementBuffer();		// Cargamos y pintamos los elementos del mesh
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsBuffer);
			glDrawElements(GL_TRIANGLES, m_mesh->GetElementSize(), GL_UNSIGNED_INT, 0);

			if(m_visibleBB) DrawBoundingBox();						// En el caso de que sea necesario pintamos el bounding box
		}
	}
}

void TMesh::EndDraw(){
	m_drawingShadows = false;	
}

void TMesh::DrawShadow(){
	m_drawingShadows = true;
	// Nos guardamos un puntero al programa para pintar sombras y enviamos las variables
	Program* myProgram = VideoDriver::GetInstance()->SetShaderProgram(SHADOW_SHADER);

	/// SEND THE VERTEX (1-Bind, 2-VertexAttribPointer)
    GLuint vertexBuffer = m_mesh->GetVertexBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);									// Bind vertex buffer
	
	GLint posAttrib = glGetAttribLocation(myProgram->GetProgramID(), "Position");	// Get atrib location
	glEnableVertexAttribArray(posAttrib);											// Enable pass
	glVertexAttribPointer(posAttrib,3, GL_FLOAT, GL_FALSE, 0, (void*)0);  			// Pour buffer data to shader

	/// SEND DEPTHMVP UNIFORM (UniformMatrix4)
	glm::mat4 depthMVP = TEntity::DepthWVP * m_stack.top();
	GLuint dMVPID = glGetUniformLocation(myProgram->GetProgramID(), "DepthMVP");	// Get uniform location
	glUniformMatrix4fv(dMVPID, 1, GL_FALSE, &depthMVP[0][0]);						// Send uniform

	// Bind and draw elements depending
	GLuint elementsBuffer = m_mesh->GetElementBuffer();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsBuffer);							// Bind elements buffer
	glDrawElements(GL_TRIANGLES, m_mesh->GetElementSize(), GL_UNSIGNED_INT, 0);		// Draw the elements (triangles)
}

void TMesh::SendShaderData(){
	Program* myProgram = VideoDriver::GetInstance()->SetShaderProgram(m_program);

	// -------------------------------------------------------- ENVIAMOS EL TIME
	float time = VideoDriver::GetInstance()->GetTime();
	GLint timeLocation = glGetUniformLocation(myProgram->GetProgramID(), "frameTime");
	glUniform1f(timeLocation, time/1000); // EN SEGUNDOS

    // -------------------------------------------------------- ENVIAMOS LOS VERTICES
    // BIND VERTEX
    GLuint vertexBuffer = m_mesh->GetVertexBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

	// SEND VERTEX
	GLint posAttrib = glGetAttribLocation(myProgram->GetProgramID(), "VertexPosition");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib,3, GL_FLOAT, GL_FALSE, 0*sizeof(float), 0);

	// -------------------------------------------------------- ENVIAMOS LAS UV
	// BIND UV
    GLuint uvBuffer = m_mesh->GetUvBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, uvBuffer);

	// SEND UV
	GLuint uvAttrib = glGetAttribLocation(myProgram->GetProgramID(), "TextureCoords");
	glEnableVertexAttribArray(uvAttrib);
	glVertexAttribPointer(uvAttrib, 2, GL_FLOAT, GL_FALSE, 0*sizeof(float), 0);

	// SEND UV SCALE
	const glm::vec2 scaleA(m_textureScaleX,m_textureScaleY);
	GLint uvScale = glGetUniformLocation(myProgram->GetProgramID(), "TextureScale");
	glUniform2fv(uvScale,1,&scaleA[0]);

	// -------------------------------------------------------- ENVIAMOS LAS NORMALS
	// BIND NORMALS
    GLuint normalBuffer = m_mesh->GetNormalBuffer();
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);

	// SEND NORMALS
	GLuint normAttrib = glGetAttribLocation(myProgram->GetProgramID(), "VertexNormal");
	glEnableVertexAttribArray(normAttrib);
	glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 0*sizeof(float), 0);

	// -------------------------------------------------------- ENVIAMOS LAS MATRICES
	// SEND MODEL MATRIX
	glm::mat4 model =  m_stack.top();
	GLint mLocation = glGetUniformLocation(myProgram->GetProgramID(), "ModelMatrix");
	glUniformMatrix4fv(mLocation, 1, GL_FALSE, &model[0][0]);

	// SEND NORMAL MATRIX (ROTAMOS LAS NORMALES)
	glm::mat3 normalMatrix = m_stack.top();
	normalMatrix = glm::transpose(glm::inverse(normalMatrix));
	GLint normalMLocation = glGetUniformLocation(myProgram->GetProgramID(), "NormalMatrix");
	glUniformMatrix3fv(normalMLocation, 1, GL_FALSE, &normalMatrix[0][0]);

	// SEND VIEW MATRIX
	GLint vLocation = glGetUniformLocation(myProgram->GetProgramID(), "ViewMatrix");
	glUniformMatrix4fv(vLocation, 1, GL_FALSE, &ViewMatrix[0][0]);

	// SEND MODELVIEW MATRIX
	glm::mat4 modelView = ViewMatrix * m_stack.top();
	GLint mvLocation = glGetUniformLocation(myProgram->GetProgramID(), "ModelViewMatrix");
	glUniformMatrix4fv(mvLocation, 1, GL_FALSE, &modelView[0][0]);

	// SEND PROJECTION MATRIX
	glm::mat4 pMatrix = ProjMatrix;
	GLint pLocation = glGetUniformLocation(myProgram->GetProgramID(), "ProjectionMatrix");
	glUniformMatrix4fv(pLocation, 1, GL_FALSE, &pMatrix[0][0]);

	// SEND MODELVIEWPROJECTION MATRIX
	glm::mat4 mvpMatrix = ProjMatrix * modelView;
	GLint mvpLocation = glGetUniformLocation(myProgram->GetProgramID(), "MVP");
	glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, &mvpMatrix[0][0]);

	// -------------------------------------------------------- ENVIAMOS LA TEXTURA
	TResourceTexture* currentTexture = nullptr;
	if(m_texture != nullptr) currentTexture = m_texture;
	else if(m_mesh != nullptr) currentTexture = m_mesh->GetTexture();

	if(currentTexture != nullptr){
		GLuint TextureID = glGetUniformLocation(myProgram->GetProgramID(), "uvMap");
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, currentTexture->GetTextureId());
		glUniform1i(TextureID, 0); 
	}

	// -------------------------------------------------------- ENVIAMOS EL SPECULAR MAP
	currentTexture = nullptr;
	if(m_specularMap != nullptr) currentTexture = m_specularMap;
	else if(m_mesh != nullptr) currentTexture = m_mesh->GetSpecularMap();

	if(currentTexture != nullptr){
		GLuint TextureID = glGetUniformLocation(myProgram->GetProgramID(), "specularMap");

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, currentTexture->GetTextureId());
		glUniform1i(TextureID, 2); 
	}

	// -------------------------------------------------------- ENVIAMOS EL BUMP MAP
	currentTexture = nullptr;
	if(m_bumpMap != nullptr) currentTexture = m_bumpMap;
	else if(m_mesh != nullptr) currentTexture = m_mesh->GetBumpMap();

	if(currentTexture != nullptr){
		GLuint TextureID = glGetUniformLocation(myProgram->GetProgramID(), "bumpMap");

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, currentTexture->GetTextureId());
		glUniform1i(TextureID, 3); 
	}

	// -------------------------------------------------------- ENVIAMOS EL MATERIAL
	TResourceMaterial* currentMaterial = nullptr;
	if(m_material != nullptr) currentMaterial = m_material;
	if(currentMaterial == nullptr) currentMaterial = m_mesh->GetMaterial();

	if(currentMaterial != nullptr){
		GLuint shininess = glGetUniformLocation(myProgram->GetProgramID(), "Material.Shininess");
		glUniform1f(shininess, currentMaterial->GetShininess());

		GLuint diffuse = glGetUniformLocation(myProgram->GetProgramID(), "Material.Diffuse");
		glUniform3fv(diffuse, 1, &currentMaterial->GetColorDifuse()[0]);

		GLuint specular = glGetUniformLocation(myProgram->GetProgramID(), "Material.Specular");
		glUniform3fv(specular, 1, &currentMaterial->GetColorSpecular()[0]);

		GLuint ambient = glGetUniformLocation(myProgram->GetProgramID(), "Material.Ambient");
		glUniform3fv(ambient, 1, &currentMaterial->GetColorAmbient()[0]);
	}

}

void TMesh::DrawBoundingBox() {
	Program* myProgram = VideoDriver::GetInstance()->SetShaderProgram(BB_SHADER);

	// Cube 1x1x1, centered on origin
	GLfloat vertices[] = {
		-0.5, -0.5, -0.5, 1.0,
		0.5, -0.5, -0.5, 1.0,
		0.5,  0.5, -0.5, 1.0,
		-0.5,  0.5, -0.5, 1.0,
		-0.5, -0.5,  0.5, 1.0,
		0.5, -0.5,  0.5, 1.0,
		0.5,  0.5,  0.5, 1.0,
		-0.5,  0.5,  0.5, 1.0,
	};
	GLuint vbo_vertices;
	glGenBuffers(1, &vbo_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	GLushort elements[] = {
		0, 1, 2, 3,
		4, 5, 6, 7,
		0, 4, 1, 5, 2, 6, 3, 7
	};
	GLuint ibo_elements;
	glGenBuffers(1, &ibo_elements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_mesh->GetCenter()) * glm::scale(glm::mat4(1), m_mesh->GetSize());

	// Apply object's transformation matrix 
	glm::mat4 m = ProjMatrix * ViewMatrix * m_stack.top() * transform;
	GLuint uniform_m = glGetUniformLocation(myProgram->GetProgramID(), "MVP");
	glUniformMatrix4fv(uniform_m, 1, GL_FALSE, &m[0][0]);

	// Send light color
	GLuint linecolor = glGetUniformLocation(myProgram->GetProgramID(), "LineColor");
	glUniform3fv(linecolor, 1, glm::value_ptr(glm::vec3(0.3f, 0.7f, 1.0f)));

	// Send each vertex data
	GLuint attribute_v_coord = glGetAttribLocation(myProgram->GetProgramID(), "VertexPosition");	
	glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
	glEnableVertexAttribArray(attribute_v_coord);

	glVertexAttribPointer(
		attribute_v_coord,  // attribute
		4,                  // number of elements per vertex, here (x,y,z,w)
		GL_FLOAT,           // the type of each element
		GL_FALSE,           // take our values as-is
		0,                  // no extra data between each position
		0                   // offset of first element
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
	glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
	glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4*sizeof(GLushort)));
	glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8*sizeof(GLushort)));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisableVertexAttribArray(attribute_v_coord);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDeleteBuffers(1, &vbo_vertices);
	glDeleteBuffers(1, &ibo_elements);
}

void TMesh::SetTextureScale(float valueX, float valueY){
	m_textureScaleX = valueX;
	m_textureScaleY = valueY;
}

int TMesh::Sign(int v){
	if(v>=0) return 1;
	else return -1;
}

// http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-testing-boxes-ii/
// Posible mejora para el clipping
bool TMesh::CheckClipping(){
	if(!m_checkClipping) return true;

	bool output = true;
	glm::vec3 center = m_mesh->GetCenter();		//
	glm::vec3 size = m_mesh->GetSize();			// Cargamos el centro y el tamanyo del mesh

	glm::mat4 mvpMatrix = ProjMatrix * ViewMatrix * m_stack.top();	// Calculamos la matriz MVP
	// Comprobamos el cliping con los 8 puntos 

	int upDown, leftRight, nearFar;
	upDown = leftRight = nearFar  = 0;

	for(int i=-1; i<=0; i++){
		// +X -X
		for(int j=-1; j<=0; j++){
			// +Y -Y
			for(int k=-1; k<=0; k++){
				// +Z -Z
				// Funcion que comprueba si los ocho lados del bounding box estan fuera de la pantalla
				glm::vec3 point = center + glm::vec3(size.x/2.0f * Sign(i), size.y/2.0f * Sign(j), size.z/2.0f * Sign(k));
				glm::vec4 mvpPoint = mvpMatrix * glm::vec4(point.x, point.y, point.z, 1.0f);

				CheckClippingAreas(mvpPoint, &upDown, &leftRight, &nearFar);
			}
		}
	}

	// Comprobacion clipping
	int sides = 8;
	// En el caso de que alguna de las variables valga 8 o -8 significa que se sale por un lado
	if(upDown == sides || upDown == -sides || leftRight == sides || leftRight == -sides || nearFar == sides){
		output = false;
	}

	return output;
}

bool TMesh::CheckOcclusion(){
	bool output = true;
	glm::vec3 center = m_mesh->GetCenter();
	glm::vec3 size = m_mesh->GetSize();

	glm::mat4 mvpMatrix = ProjMatrix * ViewMatrix * m_stack.top();

	TOEvector2di sizeWindow = VideoDriver::GetInstance()->GetWindowResolution();
	int width = sizeWindow.X;
	int height = sizeWindow.Y;

	int raster, visible;
	raster = visible = 0;

	for(int i=-1; i<=0; i++){
		// +X -X
		for(int j=-1; j<=0; j++){
			// +Y -Y
			for(int k=-1; k<=0; k++){
				// +Z -Z
				glm::vec3 point = center + glm::vec3(size.x/2.0f * Sign(i), size.y/2.0f * Sign(j), size.z/2.0f * Sign(k));
				glm::vec4 mvpPoint = mvpMatrix * glm::vec4(point.x, point.y, point.z, 1.0f);

				glm::vec3 pixelRead(mvpPoint.x, mvpPoint.y, mvpPoint.z);
				pixelRead = pixelRead / mvpPoint.w;
				pixelRead.x = ((pixelRead.x + 1)/2) * width;
				pixelRead.y = ((pixelRead.y + 1)/2) * height;
				pixelRead.z = (pixelRead.z+1)/2;

				float depthBuff = 0.0f;
				glReadPixels(static_cast<GLint>(pixelRead.x), static_cast<GLint>(pixelRead.y), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthBuff);

				if(pixelRead.x >= 0.0f && pixelRead.x <= width && pixelRead.y >= 0.0f && pixelRead.y <= height && mvpPoint.z > 0){
				
					if(depthBuff < pixelRead.z) raster++;
				}else{
					visible++;
				}
			}
		}
	}

	int sides = 8;
	if(raster!=0 && raster == sides - visible){
		output = false;
	}

	return output;
}