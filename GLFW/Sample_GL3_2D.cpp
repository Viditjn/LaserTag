#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

typedef struct COLOR
{
    float r;
    float g;
    float b; 
}Color;

typedef struct Sprite {
    string name;
    COLOR color;
    float x,y;
    VAO* object;
    int status;
    float height,width;
    float x_speed,y_speed;
    float angle; //Current Angle (Actual rotated angle of the object)
    int inAir;
    float radius;
    int fixed;
    int isRotating;
    float remAngle; //the remaining angle to finish animation
    int tone;
    float curr_angle;
}Sprite;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

map <string, Sprite> BUCKET;
map <string, Sprite> CANNON;
map <string, Sprite> BRICKS;
map <string, Sprite> SCORE1;
map <string, Sprite> SCORE2;
map <string, Sprite> SCORE3;
map <string, Sprite> SCORE4;
map <string, Sprite> LASER;
map <string, Sprite> START_WINDOW;
map <string, Sprite> MIRROR;
map <string, Sprite> TEXT;

float x_change = 0; //For the camera pan
float y_change = 0; //For the camera pan
float zoom_camera = 1;

int playerScore = 0;
int gameOver = 0;
int bricks_speed = 1;
int start = 0;
int pause = 0;
int t1=0,t2=0;
double prev_click = 0;
float x_zoom = 400.0f;
float y_zoom = 300.0f; 
double time_curr = glfwGetTime();

GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/
int playerStatus = 0; // Ready 1, 0 not ready 
int key_p=0,key_s=0,key_f=0,key_a=0,key_d=0,key_ctrl=0,key_alt=0,key_right=0,key_left=0,key_n=0,key_m=0;

double cannon_angle = 0;

void mousescroll(GLFWwindow* window, double xoffset, double yoffset)
{
    if (yoffset==-1) { 
        zoom_camera /= 1.1; //make it bigger than current size
    }
    else if(yoffset==1){
        zoom_camera *= 1.1; //make it bigger than current size
    }
    if (zoom_camera<=1) {
        zoom_camera = 1;
    }
    if (zoom_camera>=4) {
        zoom_camera=4;
    }
    if(x_change-400.0f/zoom_camera<-400)
        x_change=-400+400.0f/zoom_camera;
    else if(x_change+400.0f/zoom_camera>400)
        x_change=400-400.0f/zoom_camera;
    if(y_change-300.0f/zoom_camera<-300)
        y_change=-300+300.0f/zoom_camera;
    else if(y_change+300.0f/zoom_camera>300)
        y_change=300-300.0f/zoom_camera;
    Matrices.projection = glm::ortho((float)(-400.0f/zoom_camera+x_change), (float)(400.0f/zoom_camera+x_change), (float)(-300.0f/zoom_camera+y_change), (float)(300.0f/zoom_camera+y_change), 0.1f, 500.0f);
}

void check_pan(){
    if(x_change-400.0f/zoom_camera<-400)
        x_change=-400+400.0f/zoom_camera;
    else if(x_change+400.0f/zoom_camera>400)
        x_change=400-400.0f/zoom_camera;
    if(y_change-300.0f/zoom_camera<-300)
        y_change=-300+300.0f/zoom_camera;
    else if(y_change+300.0f/zoom_camera>300)
        y_change=300-300.0f/zoom_camera;
}


void keys()
{
    if(key_s==1)
    {
      if(CANNON["cannon_small"].y+5 < 290)
      {
        CANNON["cannon_small"].y += 5;
        CANNON["cannon_big"].y += 5;
      } 
    }

    if(key_f==1)
    {
        if(CANNON["cannon_small"].y-5 > -250)
        {
          CANNON["cannon_small"].y -= 5;
          CANNON["cannon_big"].y -= 5;
        }      
    }

    if(key_a==1)
    {
      if(CANNON["cannon_small"].curr_angle<60-5)
        CANNON["cannon_small"].curr_angle+=5;
      else
        CANNON["cannon_small"].curr_angle = 60;
    }

    if(key_d==1)
    {
      if(CANNON["cannon_small"].curr_angle>-60+5)
        CANNON["cannon_small"].curr_angle-=5;
      else
        CANNON["cannon_small"].curr_angle = -60;             
    }

    if(key_alt==1 && key_left==1)
      if(BUCKET["bucket_1"].x -5 > -370)
        BUCKET["bucket_1"].x -= 5;
    
    if(key_alt==1 && key_right)
      if(BUCKET["bucket_1"].x + 5 < 370)
        BUCKET["bucket_1"].x += 5;

    if(key_ctrl==1 && key_left==1)
      if(BUCKET["bucket_2"].x -5 > -370)
        BUCKET["bucket_2"].x -= 5;
    
    if(key_ctrl==1 && key_right==1)
      if(BUCKET["bucket_2"].x + 5 < 370)
        BUCKET["bucket_2"].x += 5;
    return;
}
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_P:
              if(start == 0)
                start = 1;
              break;

            case GLFW_KEY_S:
              key_s = 1;  
              break;
            case GLFW_KEY_F:
              key_f = 1;
              break;
            case GLFW_KEY_A:
              key_a = 1;
              break;
            case GLFW_KEY_D:
              key_d = 1;
              break;
            case GLFW_KEY_N:
              if(bricks_speed < 5)
                bricks_speed += 1;
              else
                bricks_speed = 5; 
              break;

            case GLFW_KEY_M:
              if(bricks_speed > 1)
                bricks_speed -= 1;
              else
                bricks_speed = 1;
              break;

            case GLFW_KEY_RIGHT_CONTROL:
              key_ctrl = 1;
              break;
            
            case GLFW_KEY_RIGHT_ALT:
              key_alt = 1;
              break;
              
            case GLFW_KEY_SPACE:
              time_curr = glfwGetTime();
              if(time_curr - prev_click < 1.0f  )
                break;
              else
                prev_click = time_curr; 
              for(map<string,Sprite>::iterator it=LASER.begin();it!=LASER.end();it++)
              {
                string current = it->first;
                if(LASER[current].inAir==0)
                {
                  LASER[current].inAir = 1;
                  LASER[current].curr_angle = CANNON["cannon_small"].curr_angle;
                  LASER[current].x = CANNON["cannon_small"].x;
                  LASER[current].y = CANNON["cannon_small"].y;
                  break;
                }
              }
              break;
            case GLFW_KEY_UP:
                mousescroll(window,0,+1);
                check_pan();
                break;
            
            case GLFW_KEY_DOWN:
                mousescroll(window,0,-1);
                check_pan();
                break;
            
            case GLFW_KEY_RIGHT:
                key_right = 1;
                if(key_ctrl == 0 && key_alt==0)
                {
                  x_change+=10;
                  check_pan();
                }
                break;
            
            case GLFW_KEY_LEFT:
                key_left = 1;
                if(key_ctrl == 0 && key_alt==0)
                {
                  x_change-=10;
                  check_pan();
                }
                break;    
            
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
              
            
            default:
                break;
        }
    }
     else if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            case GLFW_KEY_S:
              key_s = 0;  
              break;
            case GLFW_KEY_F:
              key_f = 0;
              break;
            case GLFW_KEY_A:
              key_a = 0;
              break;
            case GLFW_KEY_D:
              key_d = 0;
              break;
            case GLFW_KEY_RIGHT_CONTROL:
              key_ctrl = 0;
              break;
            
            case GLFW_KEY_RIGHT_ALT:
              key_alt = 0;
              break;
              
            case GLFW_KEY_RIGHT:
              key_right = 0;
              break;
              
            case GLFW_KEY_LEFT:
              key_left = 0;
              break;
              break;      
            default:
                break;

        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}
double mouse_initial_X,mouse_initial_Y,mouse_x,mouse_y;
int mouse_clicked = 0,right_mouse_clicked=0;

void mouse_release(GLFWwindow* window, int button){ 
    mouse_clicked=0;
    float ratio_zoom = x_zoom/y_zoom;
    glfwGetCursorPos(window,&mouse_x,&mouse_y);
    if((mouse_initial_X*ratio_zoom - x_zoom) > (BUCKET["bucket_1"].x -BUCKET["bucket_1"].width*0.5) && (ratio_zoom*mouse_initial_X - x_zoom) < (BUCKET["bucket_1"].x +BUCKET["bucket_1"].width*0.5)
      && (-mouse_initial_Y+y_zoom) > (BUCKET["bucket_1"].y -BUCKET["bucket_1"].height*0.5) && (-mouse_initial_Y+y_zoom) < (BUCKET["bucket_1"].y +BUCKET["bucket_1"].height*0.5))
    {
        if((ratio_zoom*mouse_x - x_zoom) > -x_zoom && (ratio_zoom*mouse_x - x_zoom) < x_zoom)
          BUCKET["bucket_1"].x = ratio_zoom*mouse_x - x_zoom;
    }
    else if((mouse_initial_X*ratio_zoom - x_zoom) > (BUCKET["bucket_2"].x -BUCKET["bucket_2"].width*0.5) && (ratio_zoom*mouse_initial_X - x_zoom) < (BUCKET["bucket_2"].x +BUCKET["bucket_1"].width*0.5)
      && (-mouse_initial_Y+y_zoom) > (BUCKET["bucket_2"].y -BUCKET["bucket_2"].height*0.5) && (-mouse_initial_Y+y_zoom) < (BUCKET["bucket_2"].y +BUCKET["bucket_2"].height*0.5))
    {
        if((ratio_zoom*mouse_x - x_zoom) > -x_zoom && (ratio_zoom*mouse_x - x_zoom) < x_zoom)
          BUCKET["bucket_2"].x = ratio_zoom*mouse_x - x_zoom;
    }
    else if((mouse_initial_X*ratio_zoom - x_zoom) > (CANNON["cannon_small"].x -CANNON["cannon_small"].width*0.5) && (ratio_zoom*mouse_initial_X - x_zoom) < (CANNON["cannon_small"].x +CANNON["cannon_small"].width*0.5)
      && (-mouse_initial_Y+y_zoom) > (CANNON["cannon_small"].y -CANNON["cannon_small"].height*0.5) && (-mouse_initial_Y+y_zoom) < (CANNON["cannon_small"].y +CANNON["cannon_small"].height*0.5))
    {
        if((-mouse_x + y_zoom) > -y_zoom && (-mouse_y + y_zoom) < y_zoom)
        {
          CANNON["cannon_big"].y = -mouse_y + y_zoom;
          CANNON["cannon_small"].y = -mouse_y + y_zoom;
        }
    }
    else if((mouse_initial_X*ratio_zoom - x_zoom) > (CANNON["cannon_big"].x -CANNON["cannon_big"].width*0.5) && (ratio_zoom*mouse_initial_X - x_zoom) < (CANNON["cannon_big"].x +CANNON["cannon_big"].width*0.5)
      && (-mouse_initial_Y+y_zoom) > (CANNON["cannon_big"].y -CANNON["cannon_big"].height*0.5) && (-mouse_initial_Y+y_zoom) < (CANNON["cannon_big"].y +CANNON["cannon_big"].height*0.5))
    {
        if((-mouse_x + y_zoom) > -y_zoom && (-mouse_y + y_zoom) < y_zoom)
        {
          CANNON["cannon_big"].y = -mouse_y + y_zoom;
          CANNON["cannon_small"].y = -mouse_y + y_zoom;
        }
    }
    else
    {
      float d1 = mouse_x*ratio_zoom - x_zoom - CANNON["cannon_small"].x;
      float d2 = -mouse_y + y_zoom - CANNON["cannon_small"].y;
      if(abs(atan(d2/d1)*180.0f/M_PI) <= 60.0f)
        CANNON["cannon_small"].curr_angle = atan(d2/d1)*180.0f/M_PI;
      if(atan(d2/d1)*180.0f/M_PI > 60)
        CANNON["cannon_small"].curr_angle = 60;
      if(atan(d2/d1)*180.0f/M_PI < -60)
        CANNON["cannon_small"].curr_angle = -60;
      time_curr = glfwGetTime();
      if(time_curr - prev_click < 1.0f  )
        return;
      else
        prev_click = time_curr; 
      for(map<string,Sprite>::iterator it=LASER.begin();it!=LASER.end();it++)
      {
          string current = it->first;
          if(LASER[current].inAir==0)
          {
            LASER[current].inAir = 1;
            LASER[current].curr_angle = CANNON["cannon_small"].curr_angle;
            LASER[current].x = CANNON["cannon_small"].x;
            LASER[current].y = CANNON["cannon_small"].y;
            return;
          }
      }
    }
    
}


/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                mouse_clicked=1;
                glfwGetCursorPos(window,&mouse_initial_X,&mouse_initial_Y);
            }
            if (action == GLFW_RELEASE) {
                if(start == 0)
                {
                  start = 1;
                  break;
                }
                mouse_release(window,button);
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                right_mouse_clicked=1;
            }
            if (action == GLFW_RELEASE) {
                right_mouse_clicked=0;
            }
            break;
        default:
            break;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-x_zoom/zoom_camera, x_zoom/zoom_camera, -y_zoom/zoom_camera, y_zoom/zoom_camera, 0.1f, 500.0f);
}

VAO *triangle, *rectangle;



// Creates the rectangle object used in this sample code
void createRectangle (string name, int tone, COLOR colorA, COLOR colorB, COLOR colorC, COLOR colorD, float x, float y, float height, float width, string component)
{
    // GL3 accepts only Triangles. Quads are not supported
    float w=width/2,h=height/2;
    GLfloat vertex_buffer_data [] = {
        -w,-h,0, // vertex 1
        -w,h,0, // vertex 2
        w,h,0, // vertex 3

        w,h,0, // vertex 3
        w,-h,0, // vertex 4
        -w,-h,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        colorA.r,colorA.g,colorA.b, // color 1
        colorB.r,colorB.g,colorB.b, // color 2
        colorC.r,colorC.g,colorC.b, // color 3

        colorC.r,colorC.g,colorC.b, // color 4
        colorD.r,colorD.g,colorD.b, // color 5
        colorA.r,colorA.g,colorA.b // color 6
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    Sprite vishsprite = {};
    vishsprite.color = colorA;
    vishsprite.name = name;
    vishsprite.object = rectangle;
    vishsprite.x=x;
    vishsprite.y=y;
    vishsprite.height=height;
    vishsprite.width=width;
    vishsprite.status=1;
    vishsprite.inAir=0;
    vishsprite.x_speed=0;
    vishsprite.y_speed=0;
    vishsprite.radius=(sqrt(height*height+width*width))/2;
    vishsprite.tone=tone;
    vishsprite.curr_angle = 0;

    if(component=="cannon")
      CANNON[name] = vishsprite;
    else if(component=="bucket")
      BUCKET[name] = vishsprite;
    else if(component=="brick")
      BRICKS[name] = vishsprite;
    else if(component=="laser")
      LASER[name] = vishsprite;
    else if(component=="start")
      START_WINDOW[name] = vishsprite;
    else if(component=="mirror")
      MIRROR[name] = vishsprite;
    else if(component=="onesPlace")
      SCORE1[name] = vishsprite;
    else if(component=="tensPlace")
      SCORE2[name] = vishsprite;
    else if(component=="hundPlace")
      SCORE3[name] = vishsprite;
    else if(component=="thoPlace")
      SCORE4[name] = vishsprite;
    else if(component=="score")
      TEXT[name] = vishsprite;
}


float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

int check_laser(Sprite object_laser)
{
  if(object_laser.x > 400 || object_laser.x < -400 || object_laser.y > 300 || object_laser.y < -250)
    return 1;
  return 0;
}

int check_collision_brick(Sprite object_brick)
{
  //cout << "entry" << endl;
  for(map<string,Sprite>::iterator it=LASER.begin();it!=LASER.end();it++)
  {
    string current = it->first;
    if(LASER[current].inAir==1)
    {
      float d1 = abs(object_brick.x - LASER[current].x);
      float d2 = abs(object_brick.y - LASER[current].y);
      float w1 = abs(object_brick.width+LASER[current].width)*0.5;
      float w2 = abs(object_brick.height+LASER[current].height)*0.5;
      if(d1 < w1 && d2 < w2)
      {
        LASER[current].inAir = 0;
        return 1;
      }
    }
    return 0;
  }
}

void check_collision_mirror(Sprite* object_laser)
{
  for(map<string,Sprite>::iterator it=MIRROR.begin();it!=MIRROR.end();it++)
  {
    string current = it->first;
    float x1 = object_laser->x + object_laser->width*0.5*cos(object_laser->curr_angle*M_PI/180.0f);
    float y1 = object_laser->y + object_laser->width*0.5*sin(object_laser->curr_angle*M_PI/180.0f);
    float d1 = (x1 - MIRROR[current].x)/cos(MIRROR[current].curr_angle*M_PI/180.0f);
    float d2 = (y1 - MIRROR[current].y)/sin(MIRROR[current].curr_angle*M_PI/180.0f);
    if(d1 > -MIRROR[current].width*0.5f && d1 < MIRROR[current].width*0.5f  && d2 > -MIRROR[current].width*0.5f && d2 < MIRROR[current].width*0.5f && abs(d1-d2) <= 5.0f)
    {
      object_laser->curr_angle = 2*MIRROR[current].curr_angle - object_laser->curr_angle;
      return ;
    }
  }
  return ;
}

int check_intersection()
{
  if(abs(BUCKET["bucket_1"].x - BUCKET["bucket_2"].x) < BUCKET["bucket_1"].width)
    return 1;
  return 0;
}

//void setStrokes(char val, int charNo, map<string,Sprite> curChar){
void setStrokes(char val, map<string,Sprite> &curChar){  
    curChar["top"].status=0;
    curChar["bottom"].status=0;
    curChar["middle"].status=0;
    curChar["left1"].status=0;
    curChar["left2"].status=0;
    curChar["right1"].status=0;
    curChar["right2"].status=0;
    curChar["middle1"].status=0;
    curChar["middle2"].status=0;
    /*Score["diagonal1"].status=0;
    Score["diagonal2"].status=0;
    Score["diagonal3"].status=0;
    Score["diagonal4"].status=0;
    */if(val=='0' || val=='2' || val=='3' || val=='5' || val=='6'|| val=='7' || val=='8' || val=='9' || val=='P' || val=='I' || val=='O' || val=='N' || val=='T' || val=='S' || val=='E'){
        curChar["top"].status=1;
    }
    if(val=='2' || val=='3' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9' || val=='P' || val=='S' || val=='Y' || val=='E'){
        curChar["middle"].status=1;
    }
    if(val=='0' || val=='2' || val=='3' || val=='5' || val=='6' || val=='8' || val=='9' || val=='O' || val=='S' || val=='I' || val=='Y' || val=='U' || val=='L' || val=='E' || val=='W'){
        curChar["bottom"].status=1;
    }
    if(val=='0' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9' || val=='P' || val=='O' || val=='N' || val=='S' || val=='Y' || val=='U' || val=='L' || val=='E' || val=='W'){
        curChar["left1"].status=1;
    }
    if(val=='0' || val=='2' || val=='6' || val=='8' || val=='P' || val=='O' || val=='N' || val=='U' || val=='L' || val=='E' || val=='W'){
        curChar["left2"].status=1;
    }
    if(val=='0' || val=='1' || val=='2' || val=='3' || val=='4' || val=='7' || val=='8' || val=='9' || val=='P' || val=='O' || val=='N' || val=='Y' || val=='U' || val=='W'){
        curChar["right1"].status=1;
    }
    if(val=='0' || val=='1' || val=='3' || val=='4' || val=='5' || val=='6' || val=='7' || val=='8' || val=='9' || val=='O' || val=='N' || val=='S' || val=='Y' || val=='U' || val=='W'){
        curChar["right2"].status=1;
    }
    if(val=='I' || val=='T'){
        curChar["middle1"].status=1;
    }
    if(val=='I' || val=='T' || val=='W'){
        curChar["middle2"].status=1;
    }
    //SCORE=curChar;
}

void setStroke(char val){
    TEXT["top"].status=0;
    TEXT["bottom"].status=0;
    TEXT["middle"].status=0;
    TEXT["left1"].status=0;
    TEXT["left2"].status=0;
    TEXT["right1"].status=0;
    TEXT["right2"].status=0;
    TEXT["middle1"].status=0;
    TEXT["middle2"].status=0;
    TEXT["diagonal1"].status=0;
    TEXT["diagonal2"].status=0;
    TEXT["diagonal3"].status=0;
    TEXT["diagonal4"].status=0;
    TEXT["diagonal5"].status=0;
    TEXT["diagonal6"].status=0;
    TEXT["diagonal7"].status=0;
    if(val=='A'|| val=='G' || val=='E' || val=='F' || val=='T' || val=='R' || val=='P' || val=='O' || val=='C' || val=='S' || val=='0' || val=='2' || val=='3' || val=='5' || val=='6'|| val=='7' || val=='8' || val=='9'){
        TEXT["top"].status=1;
    }
    if(val=='A' || val=='E' || val=='F' || val=='P' || val=='S' || val=='R' || val=='2' || val=='3' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9'){
        TEXT["middle"].status=1;
    }
    if(val=='G' || val=='W' || val=='U' || val=='E' || val=='L' || val=='O' || val=='C' || val=='S' || val=='0' || val=='2' || val=='3' || val=='5' || val=='6' || val=='8' || val=='9'){
        TEXT["bottom"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='R' || val=='V' || val=='E' || val=='F' || val=='L' || val=='N' || val=='P' || val=='O' || val=='C' || val=='S' || val=='0' || val=='4' || val=='5' || val=='6' || val=='8' || val=='9' ){
        TEXT["left1"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='E' || val=='R' || val=='F' || val=='L' || val=='N' || val=='P' || val=='O' || val=='C' || val=='0' || val=='2' || val=='6' || val=='8'){
        TEXT["left2"].status=1;
    }
    if(val=='A' || val=='M' || val=='W' || val=='U' || val=='N' || val=='V' || val=='P' || val=='R' || val=='O' || val=='0' || val=='1' || val=='2' || val=='3' || val=='4' || val=='7' || val=='8' || val=='9'){
        TEXT["right1"].status=1;
    }
    if(val=='A' || val=='G' || val=='M' || val=='W' || val=='U' || val=='N' || val=='O' || val=='S' || val=='0' || val=='1' || val=='3' || val=='4' || val=='5' || val=='6' || val=='7' || val=='8' || val=='9'){
        TEXT["right2"].status=1;
    }
    if(val=='T' || val=='I')
    {
       TEXT["middle1"].status=1;
    }
    if(val=='W' || val=='T' || val=='I' || val=='Y')
    {
        TEXT["middle2"].status=1;
    }
    if(val=='M' || val=='N' || val=='Y')
    {
        TEXT["diagonal1"].status=1;
    }
    if(val=='M' || val=='Y')
    {
     TEXT["diagonal2"].status=1;
    }
    if(val=='N')
    {
       TEXT["diagonal4"].status=1;
    }
    if(val=='V')
    {
        TEXT["diagonal5"].status=1;
        TEXT["diagonal6"].status=1; 
    }
    if(val=='R')
        TEXT["diagonal7"].status=1;
}


COLOR red = {255.0/255.0,51.0/255.0,51.0/255.0};
COLOR blue = {0,0,1};
int time_temp = 1;
int collision = 0;
double new_mouse_pos_x,new_mouse_pos_y,mouse_pos_x, mouse_pos_y;

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw (GLFWwindow* window)
{
  keys();
  time_temp++;
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;
  glfwGetCursorPos(window, &new_mouse_pos_x, &new_mouse_pos_y);
  if(right_mouse_clicked==1){
      x_change+=new_mouse_pos_x-mouse_pos_x;
      y_change-=new_mouse_pos_y-mouse_pos_y;
      check_pan();
  }
  Matrices.projection = glm::ortho((float)(-x_zoom/zoom_camera+x_change), (float)(x_zoom/zoom_camera+x_change), (float)(-y_zoom/zoom_camera+y_change), (float)(y_zoom/zoom_camera+y_change), 0.1f, 500.0f);
  glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y); 
  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!

  // Load identity to model matrix
  /* Render your scene */

  if(start == 0)
  { 
    if(t1==0)
    {  
      cout << "Press P Or Click Left Mouse Botton To Start" << endl;
      t1=1;
    }
    for(map<string, Sprite>::iterator it=START_WINDOW.begin();it!=START_WINDOW.end();it++)
    {
      string current = it->first;
      glm::mat4 MVP;
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 ObjectTransform;
      glm::mat4 translateObject = glm::translate (glm::vec3(START_WINDOW[current].x, START_WINDOW[current].y, 0.0f)); // glTranslatef
      ObjectTransform=translateObject;
      Matrices.model *= ObjectTransform;
      MVP = VP * Matrices.model; // MVP = p * V * M
        
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(START_WINDOW[current].object);
      if(START_WINDOW[current].name == "laser")
        START_WINDOW[current].x += 5;
        if(START_WINDOW[current].x > 400)
          START_WINDOW[current].x = -265; 
    } 
    int k;
    TEXT["diagonal1"].curr_angle = (atan(0.5)*180/M_PI);
    TEXT["diagonal2"].curr_angle = -(atan(0.5)*180/M_PI);
    TEXT["diagonal3"].curr_angle = -(atan(0.5)*180/M_PI);
    TEXT["diagonal4"].curr_angle = (atan(0.5)*180/M_PI);
    TEXT["diagonal5"].curr_angle = (atan(0.5)*180/M_PI);
    TEXT["diagonal6"].curr_angle = -(atan(0.5)*180/M_PI);
    TEXT["diagonal7"].curr_angle = (atan(1)*180/M_PI);
    for(k=1;k<=7;k++)
    {
        float translation;
        float translation1=0;
        if(k==1)
        {
            setStroke('W');
            translation=-90;
        }
        else if(k==2)
        {
            setStroke('E');          
            translation=-60;
        }
        else if(k==3)
        {
            setStroke('L');          
            translation=-30;
        }
        else if(k==4)
        {
            setStroke('C');          
            translation=0;
        }
        else if(k==5)
        {
            setStroke('O');          
            translation=30;
        }
        else if(k==6)
        {
            setStroke('M');

            translation=60;
        }
        else if(k==7)
        {
          setStroke('E');          
          translation=90;
        }
        for(map<string,Sprite>::iterator it=TEXT.begin();it!=TEXT.end();it++){
            string current = it->first; 
            if(TEXT[current].status==1)
            {
                glm::mat4 MVP;  // MVP = Projection * View * Model

                Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
                glm::mat4 ObjectTransform;
                glm::mat4 translateObject = glm::translate (glm::vec3(TEXT[current].x+translation,TEXT[current].y+translation1  ,0.0f)); // glTranslatef
                glm::mat4 rotateTriangle = glm::rotate((float)((TEXT[current].curr_angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
                ObjectTransform=translateObject*rotateTriangle;
                Matrices.model *= ObjectTransform;
                MVP = VP * Matrices.model; // MVP = p * V * M

                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(TEXT[current].object);
            }
        }
    }
  }
  else if(gameOver==0)
  {

  // for cannon
    for(map<string,Sprite>::iterator it=CANNON.begin();it!=CANNON.end();it++){
        string current = it->first; //The name of the current object
        //if(CANNON[current].status==0)
        //    continue;
        glm::mat4 MVP;  // MVP = Projection * View * Model

        Matrices.model = glm::mat4(1.0f);

        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(CANNON[current].x, CANNON[current].y, 0.0f)); // glTranslatef
        glm::mat4 rotateObject = glm::rotate((float)(CANNON[current].curr_angle*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
        ObjectTransform=translateObject*rotateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(CANNON[current].object);
        //glPopMatrix (); 
    }
    // for buckets 
    for(map<string, Sprite>::iterator it=BUCKET.begin();it!=BUCKET.end();it++)
    { 
      string current = it->first;
      glm::mat4 MVP;
      Matrices.model = glm::mat4(1.0f);
      glm::mat4 ObjectTransform;
      glm::mat4 translateObject = glm::translate (glm::vec3(BUCKET[current].x, BUCKET[current].y, 0.0f)); // glTranslatef
      ObjectTransform=translateObject;
      Matrices.model *= ObjectTransform;
      MVP = VP * Matrices.model; // MVP = p * V * M
        
      glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

      draw3DObject(BUCKET[current].object);
    }

    //for laser
    for(map<string,Sprite>::iterator it=LASER.begin();it!=LASER.end();it++){

       string current = it->first; //The name of the current object
       if(LASER[current].inAir==0)
          continue;
       else
       {
          LASER[current].x_speed = 1.0f*cos(LASER[current].curr_angle*M_PI/180.0f);
          LASER[current].x_speed = 1.0f*sin(LASER[current].curr_angle*M_PI/180.0f);
          LASER[current].x +=  5.0f*cos(LASER[current].curr_angle*M_PI/180.0f);
          LASER[current].y +=  5.0f*sin(LASER[current].curr_angle*M_PI/180.0f);
          check_collision_mirror(&LASER[current]);
          //cout << temp << endl;
          /*if(temp!=0)
            LASER[current].curr_angle = 2*MIRROR[temp].curr_angle - LASER[current].curr_angle;*/
          if(check_laser(LASER[current])) 
          {
            LASER[current].inAir=0;
            continue;
          }
          glm::mat4 MVP;  // MVP = Projection * View * Model

          Matrices.model = glm::mat4(1.0f);

          glm::mat4 ObjectTransform;
          glm::mat4 translateObject = glm::translate (glm::vec3(LASER[current].x, LASER[current].y, 0.0f)); // glTranslatef
          glm::mat4 rotateObject = glm::rotate((float)(LASER[current].curr_angle*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
          ObjectTransform=translateObject*rotateObject;
          Matrices.model *= ObjectTransform;
          MVP = VP * Matrices.model; // MVP = p * V * M
        
         glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

         draw3DObject(LASER[current].object);

      }
    }
    // for bricks
    int brick_temp = rand()%18;
    int t = 0;
    for(map<string,Sprite>::iterator it2=BRICKS.begin();it2!=BRICKS.end();it2++)
    { 
        string current = it2->first;
        if(BRICKS[current].inAir==0 && time_temp%(100-(15*(bricks_speed-1))) ==0 && t==brick_temp)
        {
          BRICKS[current].inAir = 1;
          time_temp = 1;
        }
        t++;
    }

    for(map<string,Sprite>::iterator it2=BRICKS.begin();it2!=BRICKS.end();it2++){
      string current = it2->first; //The name of the current object
      if(BRICKS[current].inAir==0)
          continue;
       else
       {
          if(BRICKS[current].y - bricks_speed > -270)
            BRICKS[current].y -= bricks_speed;
          else
          {
            BRICKS[current].y = 320;
            BRICKS[current].inAir = 0;
          }

          if(check_collision_brick(BRICKS[current])==1)
          {
            BRICKS[current].inAir = 0;
            BRICKS[current].y = 310;
            if(BRICKS[current].tone == 0)
              playerScore += 10;
            if((BRICKS[current].tone == 1 || BRICKS[current].tone == 2) && playerScore > 0)
              playerScore -= 10;
            if(BRICKS[current].tone == 3)
              playerScore += 50;
          }
          collision = check_intersection();
          if(BRICKS[current].tone == 1 && BRICKS[current].x < (BUCKET["bucket_2"].x +BUCKET["bucket_2"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_2"].x - BUCKET["bucket_2"].width*0.5) && BRICKS[current].y == -260 && collision == 0 && playerScore > 0)
              playerScore -= 10; 

          if(BRICKS[current].tone == 2 && BRICKS[current].x < (BUCKET["bucket_1"].x +BUCKET["bucket_1"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_1"].x - BUCKET["bucket_1"].width*0.5) && BRICKS[current].y == -260 && collision == 0  && playerScore > 0)
              playerScore -= 10;

          if(BRICKS[current].tone == 2 && BRICKS[current].x < (BUCKET["bucket_2"].x +BUCKET["bucket_2"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_2"].x - BUCKET["bucket_2"].width*0.5) && BRICKS[current].y == -260 && collision == 0)
          {
            playerScore += 10;
            BRICKS[current].inAir = 0;
            BRICKS[current].y = 310;
            //cout << playerScore << "red" << endl;
          }
          if(BRICKS[current].tone == 1 && BRICKS[current].x < (BUCKET["bucket_1"].x +BUCKET["bucket_1"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_1"].x - BUCKET["bucket_1"].width*0.5) && BRICKS[current].y == -260 && collision == 0)
          {
            playerScore += 10;
            BRICKS[current].inAir = 0;
            BRICKS[current].y = 320;
            //cout << playerScore << "blue" << endl;
          }
          if(BRICKS[current].tone == 0 && BRICKS[current].x < (BUCKET["bucket_1"].x +BUCKET["bucket_1"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_1"].x - BUCKET["bucket_1"].width*0.5) && BRICKS[current].y == -260)
          {
            gameOver = 1;
            //start = 0;
            break;
          }
          if(BRICKS[current].tone == 0 && BRICKS[current].x < (BUCKET["bucket_2"].x +BUCKET["bucket_2"].width*0.5) 
          && BRICKS[current].x > (BUCKET["bucket_2"].x - BUCKET["bucket_2"].width*0.5) && BRICKS[current].y == -260)
          {
            gameOver = 1;
            //start = 0;
            break;
          }

          glm::mat4 MVP;  // MVP = Projection * View * Model

          Matrices.model = glm::mat4(1.0f);

          glm::mat4 ObjectTransform;
          glm::mat4 translateObject = glm::translate (glm::vec3(BRICKS[current].x, BRICKS[current].y, 0.0f)); // glTranslatef
          ObjectTransform=translateObject;
          Matrices.model *= ObjectTransform;
          MVP = VP * Matrices.model; // MVP = p * V * M
        
         glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

         draw3DObject(BRICKS[current].object);
         

      }
    }

    // mirrors
    for(map<string,Sprite>::iterator it=MIRROR.begin();it!=MIRROR.end();it++){
        string current = it->first; //The name of the current object
        if(current == "mirror_1")
          MIRROR[current].curr_angle = -20;
        if(current == "mirror_2")
          MIRROR[current].curr_angle = 50;
        if(current == "mirror_3")
          MIRROR[current].curr_angle = -40;
        if(current == "mirror_4")
          MIRROR[current].curr_angle = 30;
        glm::mat4 MVP;  // MVP = Projection * View * Model

        Matrices.model = glm::mat4(1.0f);

        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(MIRROR[current].x, MIRROR[current].y, 0.0f)); // glTranslatef
        glm::mat4 rotateObject = glm::rotate((float)(MIRROR[current].curr_angle*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
        ObjectTransform=translateObject*rotateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

        draw3DObject(MIRROR[current].object);
    }

    //score
    for(map<string, Sprite>::iterator it=SCORE1.begin();it!=SCORE1.end();it++)
    {
      string current = it->first;
      SCORE1[current].inAir = 0;
      SCORE2[current].inAir = 0;
      SCORE3[current].inAir = 0;
      SCORE4[current].inAir = 0;
    }
    char curr_score[7];
    int tempPlace = 1;
    //char scoreLabel = '0';
    int te;
    for(te=0;te<4;te++){
        map <string, Sprite> tempMap;
        if(te==0)
          tempMap = SCORE1;
        if(te==1)
          tempMap = SCORE2;
        if(te==2)
          tempMap = SCORE3;
        if(te==3)
          tempMap = SCORE4;
        setStrokes('0'+(playerScore/tempPlace)%10,tempMap);
        for(map<string,Sprite>::iterator it2=tempMap.begin();it2!=tempMap.end();it2++){
            if(it2->second.status==0)
                continue;
            
            string current = it2->first;

            glm::mat4 MVP;  // MVP = Projection * View * Model
            Matrices.model = glm::mat4(1.0f);

            glm::mat4 ObjectTransform;
            //glm::mat4 translateObject = glm::translate (glm::vec3(base_x+it2->second.x, base_y+it2->second.y, 0.0f)); // glTranslatef
            glm::mat4 translateObject = glm::translate (glm::vec3(tempMap[current].x, tempMap[current].y, 0.0f)); // glTranslatef
            ObjectTransform=translateObject;
            Matrices.model *= ObjectTransform;
            MVP = VP * Matrices.model; // MVP = p * V * M

            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

            draw3DObject(it2->second.object);
            //glPopMatrix (); 
        }
        tempPlace*=10; //Next character
    }
  } 
  else if(gameOver==1)
  {
    if(t2==0)
    {  
      cout << "GAME OVER" << endl;
      cout << "YOUR FINAL SCORE IS :" << " " << playerScore << endl;
      t2 = 1;
    }
    int k;
    for(k=1;k<=8;k++)
    {
        float translation;
        float translation1=0;
        if(k==1)
        {
            setStroke('G');
            translation=-90;
        }
        else if(k==2)
        {
            setStroke('A');          
            translation=-60;
        }
        else if(k==3)
        {
            setStroke('M');          
            translation=-30;
        }
        else if(k==4)
        {
            setStroke('E');          
            translation=0;
        }
        else if(k==5)
        {
            setStroke('O');          
            translation=60;
        }
        else if(k==6)
        {
            setStroke('V');

            translation=90;
        }
        else if(k==7)
        {
          setStroke('E');          
          translation=120;
        }
        else if(k==8)
        {
          setStroke('R');          
          translation=150;
        }
        for(map<string,Sprite>::iterator it=TEXT.begin();it!=TEXT.end();it++){
            string current = it->first; 
            if(TEXT[current].status==1)
            {
                glm::mat4 MVP;  // MVP = Projection * View * Model

                Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
                glm::mat4 ObjectTransform;
                glm::mat4 translateObject = glm::translate (glm::vec3(TEXT[current].x+translation,TEXT[current].y+translation1  ,0.0f)); // glTranslatef
                glm::mat4 rotateTriangle = glm::rotate((float)((TEXT[current].curr_angle)*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
                ObjectTransform=translateObject*rotateTriangle;
                Matrices.model *= ObjectTransform;
                MVP = VP * Matrices.model; // MVP = p * V * M

                glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
                draw3DObject(TEXT[current].object);
            }
        }
    }
  } 

  //  Don't change unless you are sure!!
  //glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  //draw3DObject(CANNON[cannon_small].object);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  Matrices.model = glm::mat4(1.0f);


  // Increment angles
  float increments = 1;
  
  //camera_rotation_angle++; // Simulating camera rotation
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); // mouse scroll


    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models

  COLOR grey = {168.0/255.0,168.0/255.0,168.0/255.0};
    COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
    COLOR lightgreen = {57/255.0,230/255.0,0/255.0};
    COLOR black = {30/255.0,30/255.0,21/255.0};
    COLOR cratebrown2 = {102/255.0,68/255.0,0/255.0};
    COLOR skyblue2 = {113/255.0,185/255.0,209/255.0};
    COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
    COLOR skyblue = {132/255.0,217/255.0,245/255.0};
    COLOR lightpink = {255/255.0,122/255.0,173/255.0};
    COLOR darkpink = {255/255.0,51/255.0,119/255.0};

  createRectangle("cannon_small",10000,gold,gold,lightgreen,lightgreen,-300,0,20,40,"start");
  createRectangle("cannon_big",10000,black,red,blue,black,-360,0,60,80,"start");  
  createRectangle("laser",10000,red,red,red,red,-265,0,10,30,"start");
	
  createRectangle("cannon_small",10000,cratebrown2,cratebrown2,cratebrown2,cratebrown2,-360,0,10,30,"cannon");
  createRectangle("cannon_big",10000,blue,blue,red,red,-380,0,30,40,"cannon");

  createRectangle("bucket_1",1,blue,blue,blue,blue,-200,-280,40,60,"bucket");
  createRectangle("bucket_2",2,red,red,red,red,200,-280,40,60,"bucket");
  createRectangle("boundary",10000,black,black,black,black,0,-250,1,800,"bucket");

  createRectangle("mirror_1",10000,black,black,black,black,-150,200,3,60,"mirror");
  createRectangle("mirror_2",10000,black,black,black,black,-150,-50,3,60,"mirror");
  createRectangle("mirror_3",10000,black,black,black,black,200,100,3,60,"mirror");
  createRectangle("mirror_4",10000,black,black,black,black,200,-100,3,60,"mirror");

  
  createRectangle("laser_1",10000,red,red,red,red,LASER["laser_1"].x,LASER["laser_1"].y,5,40,"laser");
  createRectangle("laser_2",10000,red,red,red,red,LASER["laser_2"].x,LASER["laser_2"].y,5,40,"laser");
  createRectangle("laser_3",10000,red,red,red,red,LASER["laser_3"].x,LASER["laser_3"].y,5,40,"laser");
  createRectangle("laser_4",10000,red,red,red,red,LASER["laser_4"].x,LASER["laser_4"].y,5,40,"laser");
  createRectangle("laser_5",10000,red,red,red,red,LASER["laser_5"].x,LASER["laser_5"].y,5,40,"laser");
  
  createRectangle("brick_1",2,red,red,red,red,-250,310,20,20,"brick");
  createRectangle("brick_2",2,red,red,red,red,-200,310,20,20,"brick");
  createRectangle("brick_3",2,red,red,red,red,-50,310,20,20,"brick");
  createRectangle("brick_4",2,red,red,red,red,150,310,20,20,"brick");
  createRectangle("brick_5",2,red,red,red,red,260,310,20,20,"brick");
  createRectangle("brick_6",2,red,red,red,red,350,310,20,20,"brick");
  createRectangle("brick_a",1,blue,blue,blue,blue,-260,310,20,20,"brick");
  createRectangle("brick_b",1,blue,blue,blue,blue,-210,310,20,20,"brick");
  createRectangle("brick_c",1,blue,blue,blue,blue,-30,310,20,20,"brick");
  createRectangle("brick_G",3,gold,gold,gold,gold,90,310,20,20,"brick");
  createRectangle("brick_d",1,blue,blue,blue,blue,70,310,20,20,"brick");
  createRectangle("brick_e",1,blue,blue,blue,blue,280,310,20,20,"brick");
  createRectangle("brick_f",1,blue,blue,blue,blue,370,310,20,20,"brick");
  createRectangle("brick_A",0,black,black,black,black,-270,310,20,20,"brick");
  createRectangle("brick_B",0,black,black,black,black,-220,310,20,20,"brick");
  createRectangle("brick_C",0,black,black,black,black,-70,310,20,20,"brick");
  createRectangle("brick_D",0,black,black,black,black,50,310,20,20,"brick");
  createRectangle("brick_E",0,black,black,black,black,240,310,20,20,"brick");
  createRectangle("brick_F",0,black,black,black,black,330,310,20,20,"brick");


  int t;
  string temp;
  int offset=0;
  int x = -45;
  for(t=0;t<4;t++)
  {
      if(t==0)
      {
        offset = 0;
        temp = "onesPlace";
      }
      else if(t==1)
      {
        offset = -15;
        temp = "tensPlace";
      }
      else if(t==2)
      {
        offset = -30;
        temp = "hundPlace";
      }
      else if(t==3)
      {
        offset = -45;
        temp = "thoPlace";
      }
      createRectangle("right1",100,black,black,black,black,380+offset,280,10,2,temp);
      createRectangle("right2",100,black,black,black,black,380+offset,269,10,2,temp);
      createRectangle("top",100,black,black,black,black,375+offset,286,2,10,temp);
      createRectangle("middle",100,black,black,black,black,375+offset,276,2,10,temp);
      createRectangle("bottom",100,black,black,black,black,375+offset,264,2,10,temp);
      createRectangle("left1",100,black,black,black,black,370+offset,280,10,2,temp);
      createRectangle("left2",100,black,black,black,black,370+offset,269,10,2,temp);
      createRectangle("middle1",100,black,black,black,black,375+offset,280,10,2,temp);
      createRectangle("middle2",100,black,black,black,black,375+offset,269,10,2,temp);
  }
    int height1 = 2;
    int width1 = 20;
    createRectangle("top",0,black,black,black,black,0,20,height1,width1,"score");
    createRectangle("bottom",0,black,black,black,black,0,-20,height1,width1,"score");
    createRectangle("middle",0,black,black,black,black,0,0,height1,width1,"score");
    createRectangle("left1",0,black,black,black,black,-10,10,width1,height1,"score");
    createRectangle("left2",0,black,black,black,black,-10,-10,width1,height1,"score");
    createRectangle("right1",0,black,black,black,black,10,10,width1,height1,"score");
    createRectangle("right2",0,black,black,black,black,10,-10,width1,height1,"score");
    createRectangle("middle1",0,black,black,black,black,0,10,width1,height1,"score");
    createRectangle("middle2",0,black,black,black,black,0,-10,width1,height1,"score");
    createRectangle("diagonal1",0,black,black,black,black,-5,10,width1/2+13,height1,"score");
    createRectangle("diagonal2",0,black,black,black,black,5,10,width1/2+13,height1,"score");
    createRectangle("diagonal3",0,black,black,black,black,-5,-10,width1/2+13,height1,"score");
    createRectangle("diagonal4",0,black,black,black,black,5,-10,width1/2+13,height1,"score");
    createRectangle("diagonal5",0,black,black,black,black,-5/2,-10,sqrt(5)*width1/2,height1,"score");
    createRectangle("diagonal6",0,black,black,black,black,5/2,-10,sqrt(5)*width1/2,height1,"score");
    createRectangle("diagonal7",0,black,black,black,black,0,-10,sqrt(2)*width1,height1,"score");
 
  /*createRectangle("brick_7",10000,red,red,red,red,300,310,20,20,"brick");
  createRectangle("brick_8",10000,red,red,red,red,350,310,20,20,"brick");
  createRectangle("brick_9",10000,red,red,red,red,-350,310,20,20,"brick");
  createRectangle("brick_10",10000,red,red,red,red,-250,310,20,20,"brick");
  createRectangle("brick_11",10000,red,red,red,red,250,310,20,20,"brick");
  createRectangle("brick_12",10000,red,red,red,red,-50,310,20,20,"brick");
  createRectangle("brick_g",10000,blue,blue,blue,blue,0,310,20,20,"brick");
  createRectangle("brick_h",10000,blue,blue,blue,blue,100,310,20,20,"brick");
  createRectangle("brick_i",10000,blue,blue,blue,blue,200,310,20,20,"brick");
  createRectangle("brick_j",10000,blue,blue,blue,blue,300,310,20,20,"brick");
  createRectangle("brick_k",10000,blue,blue,blue,blue,350,310,20,20,"brick");
  createRectangle("brick_l",10000,blue,blue,blue,blue,-350,310,20,20,"brick");
  */

	
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (123/255.0f,201/255.0f,227/255.0f,0.4f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;
  

  GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

  double last_update_time = glfwGetTime(), current_time;
  glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);


    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        
        draw(window);

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
