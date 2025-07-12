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
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "SignalGui.h"
#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 800

namespace po = boost::program_options;

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int argc, char* argv[]) {
	std::string mode_str;

	po::options_description desc("Signal detector & Modulation recognition");
	desc.add_options()
		("help,h", "Show help message")
		("mode", po::value<std::string>(&mode_str)->default_value("sim"),
		 "Signal source mode: 'sim' for simulation, 'usrp' for USRP hardware");

	po::variables_map vm;
	try	{
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
		if (vm.count("help")) {
			std::cout << desc << std::endl;
			return 0;
		}
	} catch(const po::error& e) {
		std::cerr << "Error parsing arguments: " << e.what() << std::endl;
		std::cout << desc << std::endl;
		return 1;
	}

	SignalGui::Mode mode;
	if (mode_str == "sim") {
		mode = SignalGui::Mode::SIMULATION;
		std::cout << "Mode: sim" << std::endl;
	} else if (mode_str == "usrp") {
		mode = SignalGui::Mode::USRP;
		std::cout << "Mode: usrp" << std::endl;
	} else {
		std::cerr << "Error: Invalid mode " << mode_str << ". Use sim or usrp" << std::endl;
		std::cout << desc << std::endl;
		return 1;
	}

	// GLFW initialization
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
	SignalGui gui(mode);

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
