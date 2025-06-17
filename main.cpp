/**
 *			Create GUI Window, frames and rendering with OpenGL 3
 *
 * Reference:
 * OpenGL: https://www.glfw.org/docs/latest/window.html
 * ImGui: imgui/examples/example_glfw_opengl3
 * ImPlot
 */

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "implot/implot.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include "SignalGui.h"
#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 800

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main() {
	// You think i have any idea what this does? nope ;]
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	// OpenGL
	const char* glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	// Window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Title", nullptr, nullptr);
	if ( window == nullptr )  {  glfwTerminate(); return -1; }
	if ( !glfwVulkanSupported() ) { printf("GLFW: Vulkan not supported\n"); return -1; }

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsClassic();

	// Renderer backend: OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// ImPlot context
	ImPlot::CreateContext();
	SignalGui gui;

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		// Poll and handle events (inputs, window)
		glfwPollEvents();

		// Start ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		gui.Update();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleaning
	ImPlot::DestroyContext();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
