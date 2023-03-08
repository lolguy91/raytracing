
#include <iostream>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define _DEBUG

#include <opengl/debug.h>
#include <opengl/shader.h>
#include <opengl/window.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw_gl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

unsigned int shader;
glm::vec3 cpos(1.0f,1.0f,-3.0f);
glm::vec2 crot(0.0f,3.0f);
glm::vec3* raydirs;
bool scene_modified = true;
bool render_done = true;
bool render_done_thread_2 = true;
bool raydir_calculations_done = false;
struct color{
unsigned char r,g,b;
};
double hit_sphere(const glm::vec3 center, double radius, const glm::vec3 rorig, const glm::vec3 rdir) {
	glm::vec3 oc = rorig - center;
	auto a = glm::dot(rdir,rdir);
	auto b = glm::dot(oc, rdir);
	auto c = glm::dot(oc,oc) - radius*radius;
	auto discriminant = b*b - a*c;

    if (discriminant < 0) {
        return -1.0;
    } else {
        return (-b - sqrt(discriminant) ) / a;
    }
}

color perpixel(int x,int y){
	glm::vec3 spherepos = glm::vec3(0,0,0);
	glm::vec3 raydir = raydirs[y*600 + x];
	double hit_t = hit_sphere(spherepos,.5,cpos,raydir);
	
	if(hit_t != -1.0){
		glm::vec3 hitpos = raydir * (float)hit_t;
		glm::vec3 normal = glm::normalize(hitpos - spherepos);
		return {normal.x*255,normal.y*255,normal.z*255};
	}else{
		return {0,raydir.y* 256,0};
	}
}

void updatescreenhalf1(unsigned char* imgbuffer){
	while (1)
	{
		if(scene_modified){
			while(!raydir_calculations_done){}
			render_done = false;
			for(int y = 0;y < 600;y++){
				for(int x = 0;x < 400;x++){	
				unsigned int* buffer = (unsigned int*)imgbuffer;
				color col = perpixel(x,y);
				buffer[y*800 + x] = col.r | (col.g << 8)| (col.b << 16)|(255 << 24);
				}	
			}
			while(!render_done_thread_2){}//wait for thread 2

			scene_modified = false;
			render_done = true;
		}
	}
}
void updatescreenhalf2(unsigned char* imgbuffer){
	while (1)
	{
		if(scene_modified){
			while(!raydir_calculations_done){}
			render_done_thread_2 = false;
			for(int y = 0;y < 600;y++){
				for(int x = 400;x < 800;x++){
				unsigned int* buffer = (unsigned int*)imgbuffer;
				color col = perpixel(x,y);
				buffer[y*800 + x] = col.r | (col.g << 8)| (col.b << 16) ;
				}	
			}
			render_done_thread_2 = true;
		}
	}
}
void updateraydirs(){
	while (1)
	{
		if(scene_modified && render_done){
			raydir_calculations_done = false;
			for(int x = 0;x <= 800;x++){
				for(int y = 0;y <= 600;y++){
				glm::mat4 rotmat(1.0);
				glm::vec3 rdir(x / 800.,y/ 600.,-1.);

				rotmat = glm::rotate(rotmat,glm::radians(crot.x),{1,0,0});
				rotmat = glm::rotate(rotmat,glm::radians(crot.y),{0,1,0});

				rdir = glm::vec3(rotmat * glm::vec4(rdir,0));
				raydirs[y*600 + x] = rdir;
				}	
			}
			raydir_calculations_done = true;
		}
	}
}
int main()
{

	GLFWwindow *window = InitWindow();
	if (!window)
		return -1;


	glfwSetWindowSizeCallback(window, [](GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	});



	float verts[] {
		-1,-1, 	0,0,
		-1,1,  	0,1,
		1,-1,  	1,0,

		1,-1,   1,0,
		1,1,   	1,1,
		-1,1, 	0,1

	};
	unsigned int rid;
	unsigned char* imgbuffer = (unsigned char*)malloc(600*800*4);
	raydirs = (glm::vec3*)malloc(sizeof(glm::vec3)*600*800);

	scene_modified = true;

	GLCall(glGenTextures(1, &rid));
	GLCall(glBindTexture(GL_TEXTURE_2D, rid)); // Bind without slot selection

	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));//settings
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgbuffer));
	GLCall(glBindTexture(GL_TEXTURE0, rid));//unbind

	

	std::thread t3(&updateraydirs);
	std::thread t1(&updatescreenhalf1,imgbuffer);
	std::thread t2(&updatescreenhalf2,imgbuffer);


	ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
	unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
	GLCall(glUseProgram(shader));

	GLCall(int location = glGetUniformLocation(shader, "u_Texture"));
	if (location == -1)
		std::cout << "No active uniform variable with name " << "u_Texture" << " found" << std::endl;
	glUniform1i(location, 0);

	unsigned int vao;
	unsigned int vb;

	GLCall(glGenVertexArrays(1, &vao));//VAO
	GLCall(glBindVertexArray(vao));

	GLCall(glGenBuffers(1, &vb));//VBO
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, vb));
	GLCall(glBufferData(GL_ARRAY_BUFFER,sizeof(verts), &verts, GL_STATIC_DRAW));

	GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (int*) 0));
	GLCall(glEnableVertexAttribArray(0));//first argument

	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (int*) (sizeof(float) * 2)));
	GLCall(glEnableVertexAttribArray(1));//second argument

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfwGL3_Init(window, true);
	ImGui::StyleColorsDark();

	do
	{
		GLCall(glClear(GL_COLOR_BUFFER_BIT ));
		
		GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgbuffer));

		GLCall(glBindVertexArray(vao));
		GLCall(glDrawArrays(GL_TRIANGLES,0,6));
		
		
		ImGui_ImplGlfwGL3_NewFrame();


		if(ImGui::DragFloat3("Camera Translation", &cpos.x,1 , -960.0f, 960.0f)){
			//nope
		}
		if(ImGui::DragFloat2("Camera Rotation", &crot.x,1 , -960.0f, 960.0f)){
			//nope
		}
		if(ImGui::Button("Render")){
			scene_modified = true;
		}
		ImGui::Text("Application average: %.1f FPS", ImGui::GetIO().Framerate);

		ImGui::Render();
		ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));



	// Close OpenGL window and terminate GLFW
	ImGui_ImplGlfwGL3_Shutdown();
	ImGui::DestroyContext();
	glfwTerminate();
	return 0;
}