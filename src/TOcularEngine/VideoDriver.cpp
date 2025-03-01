#include "VideoDriver.h"
#include "./../EngineUtilities/Resources/Program.h"
#include "./../EngineUtilities/TResourceManager.h"
#include <stdio.h>
#include <string.h>

#define GLEW_STATIC

#include <GL/glew.h>
#include <GLFW/glfw3.h> //SIEMPRE DESPUES DE INCLUIR GLEW

// Inicializa las variables estaticas
std::string	VideoDriver::m_assetsPath = "";
SceneManager* VideoDriver::privateSceneManager = nullptr;
IODriver* VideoDriver::privateIODriver = nullptr;

VideoDriver::VideoDriver(){
	// Init variables
	m_name = "";
	m_lastShaderUsed = STANDARD_SHADER;
	m_window = nullptr;
	m_clearSceenColor = TOEvector4df(0,0,0,0);

	// Init engine stuff
	privateSceneManager = new SceneManager();
	privateIODriver = nullptr;

	// Iinitialize GLFW
	glfwSetErrorCallback(VideoDriver::glwf_error_callback);
    if(!glfwInit()) std::cout <<"GLFW Initiation Problem (Update your graphics card driver!)"<<std::endl;
}

VideoDriver::~VideoDriver(){
	// Eliminamos las variables del videoDriver
	Drop();
}

// Main functions
bool VideoDriver::CreateWindows(std::string window_name, TOEvector2di dimensions, bool fullscreen){
	m_name = window_name;

	//initialize gflwindow parameters
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_RESIZABLE, GL_FALSE );
	if(fullscreen) glfwWindowHint( GLFW_AUTO_ICONIFY, GL_FALSE );

	//create glfwindow and make it the current window
	GLFWmonitor* monitor = nullptr;
	if(fullscreen){
		glfwWindowHint( GLFW_DECORATED, GL_FALSE );
		monitor = glfwGetPrimaryMonitor();
	}

	m_window = glfwCreateWindow(dimensions.X,dimensions.Y, m_name.c_str(), monitor, NULL);
	glfwMakeContextCurrent(m_window);

	/// Iniciamos glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if(GLEW_OK != err){
		std::cout<<"Something wrong happened in glewInit (Update your graphic card driver!)"<<std::endl;
		return false;
	}
	
	SetReceiver();

	glEnable(GL_TEXTURE_2D);		
	glEnable(GL_DEPTH_TEST);		// Habilitar el test de profundidad
	glDepthFunc(GL_LESS);			// Aceptar el fragmento si está más cerca de la cámara que el fragmento anterior
	glEnable(GL_CULL_FACE);			// Habilitar el culing
	glCullFace(GL_BACK);			// Hacerlo Backface
	glFrontFace(GL_CW);				// Hacer las caras que miran a al camara Counter Clockwise

	initShaders();					// Cargamos los shaders
	privateSceneManager->InitScene();

	return true;
}

bool VideoDriver::Update(){
	//UPDATE IO
	glfwPollEvents();
	ClearScreen();

	privateSceneManager->Update();
	return true;
}

void VideoDriver::BeginDraw(){
	//DRAW 3D SCENE
	privateSceneManager->Draw();

	//DRAW BKG 2D ELEMENTS
	start2DDrawState();		// Preparamos Opengl para pintar 2D
	privateSceneManager->DrawBkg2DElements();
	end2DDrawState();		// Lo volvemos a dejar listo para 3D

}

void VideoDriver::EndDraw(){

	//DRAW 2D ELEMENTS
	start2DDrawState();		// Preparamos Opengl para pintar 2D
	privateSceneManager->Draw2DElements();
	end2DDrawState();		// Lo volvemos a dejar listo para 3D

	glfwSwapBuffers(m_window);
}

void VideoDriver::Minimize(){
	if(m_window != nullptr) glfwIconifyWindow(m_window);
}

void VideoDriver::ClearScreen(){
	// Limpiamos la pantalla
	glClearColor(m_clearSceenColor.X, m_clearSceenColor.Y, m_clearSceenColor.X2, m_clearSceenColor.Y2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void VideoDriver::Drop(){
	// Eliminamos los shaders
	std::map<SHADERTYPE, Program*>::iterator it = m_programs.begin();
	for(;it!=m_programs.end();++it) delete it->second;
	m_programs.clear();

	// Eliminamos el IODriver
	if(privateIODriver != nullptr) delete privateIODriver;
	// Eliminamos el SceneManager
	if(privateSceneManager != nullptr) delete privateSceneManager;

	// Eliminamos la ventana
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void VideoDriver::CloseWindow(){
	if(m_window != nullptr) glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void VideoDriver::SetReceiver(){
	// Le decimos a glfw que metodos tiene que llamar para cada tipo de evento
	glfwSetKeyCallback(m_window, VideoDriver::keyboard_callback);
	glfwSetCursorPosCallback(m_window, VideoDriver::mouse_position_callback);
	glfwSetMouseButtonCallback(m_window, VideoDriver::mouse_button_callback);
	glfwSetScrollCallback(m_window, VideoDriver::mouse_scroll_callback);
	glfwSetWindowCloseCallback(m_window, VideoDriver::window_close_callback);
}

void VideoDriver::SetAssetsPath(std::string newPath){
	 m_assetsPath = newPath;
}

void VideoDriver::SetGlobalBoundingBoxes(bool enable){
	// Iterate all meshes
	privateSceneManager->DrawBoundingBoxes(enable);
}

// Getters
VideoDriver* VideoDriver::GetInstance(){
	static VideoDriver instance;
	return &instance;
}

SceneManager* VideoDriver::GetSceneManager(){
	return privateSceneManager;
}

IODriver* VideoDriver::GetIOManager(){
	return privateIODriver;
}

float VideoDriver::GetTime(){
	return (float) (glfwGetTime()*1000);
}

std::string VideoDriver::GetWindowName(){
	return m_name;
}

TOEvector2di VideoDriver::GetWindowDimensions(){
   	TOEvector2di toRet(0.0f,0.0f);
	glfwGetWindowSize(m_window, &toRet.X, &toRet.Y);
   	return toRet;
}

Program* VideoDriver::GetProgram(SHADERTYPE p){
	return m_programs[p];
}

std::map<SHADERTYPE,Program*>& VideoDriver::GetProgramVector(){
	return m_programs;
}

TOEvector2di VideoDriver::GetCursorPosition(){
	double x, y;
	glfwGetCursorPos(m_window, &x, &y);
	return TOEvector2di((int)x, (int)y);
}

GLFWwindow* VideoDriver::GetWindow(){
	return m_window;
}

std::string VideoDriver::GetAssetsPath(){
	return m_assetsPath;
}

TOEvector2di VideoDriver::GetScreenResolution(){
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    return TOEvector2di(mode->width, mode->height);
}

TOEvector2di VideoDriver::GetWindowResolution(){
	int x = 0;
	int y = 0;
	if(m_window != nullptr) glfwGetWindowSize(m_window, &x, &y); 	
	return TOEvector2di(x, y);
}

// Setters
void VideoDriver::SetClearScreenColor(TOEvector4df color){
	m_clearSceenColor = color;
}

void VideoDriver::SetWindowName(std::string name){
	m_name = name;
	glfwSetWindowTitle(m_window,m_name.c_str());
}

Program* VideoDriver::SetShaderProgram(SHADERTYPE p){
	// Cambiamos el shader que se esta usando y actualizamos m_lastShaderUsed
	Program* toRet = m_programs.find(p)->second;
	if(m_lastShaderUsed != p){
		m_lastShaderUsed = p;
		glUseProgram(toRet->GetProgramID());
	}

	return toRet;
}

SHADERTYPE VideoDriver::GetCurrentProgram(){
	if(m_lastShaderUsed == NONE_SHADER) m_lastShaderUsed = STANDARD_SHADER;
	return m_lastShaderUsed;
}

void VideoDriver::SetIODriver(IODriver* driver){
	privateIODriver = driver;
}

void VideoDriver::SetMouseVisibility(bool visible){
	// A la hora de dejar el raton invisible lo hacer de 1x1 para que no se pueda ver
	// La funcion de glfw para no mostrar el raton no nos funcionaba
	if(visible == 0){
		int w = 1;
		int h = 1;
		unsigned char pixels[w * h * 4];
		memset(pixels, 0x00, sizeof(pixels));
		GLFWimage image;
		image.width = w;
		image.height = h;
		image.pixels = pixels;
		GLFWcursor* newCursor = glfwCreateCursor(&image, 0, 0);
		glfwSetCursor(m_window, newCursor);
	}
	else{
		GLFWcursor* newCursor = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
		glfwSetCursor(m_window, newCursor);
	}
}

void VideoDriver::SetCursorPosition(int x, int y){
	glfwSetCursorPos (m_window, (double) x, (double) y);
}

// Private Functions
void VideoDriver::initShaders(){
	// CARGAMOS EL PROGRAMA STANDAR
	std::map<std::string, GLenum> shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderStand.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderStand.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(STANDARD_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE TEXTO
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderText.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderText.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(TEXT_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE PARTICULAS
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderParticle.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderParticle.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(PARTICLE_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE POLIGONOS 2D
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shader2D.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shader2D.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(TWOD_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE TEXTO 2D
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shader2DText.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shader2DText.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(TWODTEXT_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE SPRITES
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderSprites.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderSprites.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(SPRITE_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE BOUNDIN BOXES
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderBB.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderBB.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(BB_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE DISTORSION
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderDistorsion.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderStand.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(DISTORSION_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE FISHEYE
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderFisheye.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderStand.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(FISHEYE_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE BARREL
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderBarrel.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/ShaderStand.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(BARREL_SHADER, new Program(shaders)));

	// CARGAMOS EL PROGRAMA DE SOMBRAS
	shaders = std::map<std::string, GLenum>();
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shadows.vs", GL_VERTEX_SHADER));
	shaders.insert(std::pair<std::string, GLenum>(m_assetsPath + "/shaders/Shadows.frag", GL_FRAGMENT_SHADER));
	m_programs.insert(std::pair<SHADERTYPE, Program*>(SHADOW_SHADER, new Program(shaders)));
}

void VideoDriver::start2DDrawState(){

	//GUARDAMOS EL ESTADO ACTUAL
	glMatrixMode(GL_PROJECTION);	//activamos la matriz Projection
	glPushMatrix(); 				//guardamos en la pila el estado actual de la matriz Projection
	glLoadIdentity(); 				//cargamos la matriz identidad en la matriz Projection

	//creamos una vista ortografica de la camara
	TOEvector2di dims = GetWindowDimensions();	//cojemos las dimensiones de la ventana
	glOrtho(0, dims.X, dims.Y, 0, -1.0f, 1.0f);				//sup-izq, inf-izq, inf-der, sup-der, near, far

	glMatrixMode(GL_MODELVIEW);		//activamos la matriz ModelView
	glPushMatrix(); 				//guardamos en la pila el estado actual de la matriz ModelView
	glLoadIdentity(); 				//cargamos la matriz identidad en la matriz ModelView

	glPushAttrib(GL_DEPTH_TEST);	//guardamos en la pila el estado actual del test de profundidad
	glDepthMask(GL_FALSE);			//desactivamos la escritura en el buffer de profundidad
	glDisable(GL_DEPTH_TEST);		//desactivamos el test de profundidad
	glDisable(GL_CULL_FACE);		//desactivamos el backface culling

	///////////////////////////////////
	///////////START 2D DRAW///////////
	///////////////////////////////////
}

void VideoDriver::end2DDrawState(){
	///////////////////////////////////
	////////////END 2D DRAW////////////
	///////////////////////////////////
	//VOLVER AL ESTADO ANTERIOR
	glPopAttrib();					//recuperamos de la pila el estado del test de profundidad
	glDepthMask(GL_TRUE); 			//activamos la escritura en el buffer de profundidad
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);		//activamos la matriz ModelView
	glPopMatrix();					//recuperamos el estado anterior de la pila y se lo asignamos
	glMatrixMode(GL_PROJECTION);	//activamos la matriz Projection
	glPopMatrix();					//recuperamos el estado anterior de la pila y se lo asignamos
}

void VideoDriver::keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if(privateIODriver!=nullptr) privateIODriver->UpdateKeyboard(key,action);
}

void VideoDriver::mouse_position_callback(GLFWwindow* window, double xpos, double ypos){
	if(privateIODriver!=nullptr) privateIODriver->UpdateMousePosition(xpos,ypos);
}
void VideoDriver::window_close_callback(GLFWwindow* window){
	if(privateIODriver!=nullptr) privateIODriver->UpdateShouldClose();
}

void VideoDriver::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	if(privateIODriver!=nullptr) privateIODriver->UpdateMouseButtons(button,action);
}

void VideoDriver::mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	if(privateIODriver!=nullptr) privateIODriver->UpdateMouseWheel(xoffset, yoffset);
}

void VideoDriver::glwf_error_callback(int error, const char* description){
    fprintf(stderr, "Error %d: %s\n", error, description);
}

void VideoDriver::EnableClipping(){
	privateSceneManager->SetClipping(true);
}

void VideoDriver::DisableClipping(){
	privateSceneManager->SetClipping(false);
}

void VideoDriver::ChangeShader(SHADERTYPE shader, ENTITYTYPE entity){
	privateSceneManager->ChangeShader(shader, entity);
}

