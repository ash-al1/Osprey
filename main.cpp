#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "implot/implot.h"
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <iomanip>

#include "SignalGui.h"
#include "SDRFactory.h"
#include "SDRDevice.h"

#include "USRPDevice.h"
#include "SimulationDevice.h"

#include "theme.h"

#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1080

namespace po = boost::program_options;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void print_device_list() {
    std::cout << "\nSupported devices:" << std::endl;
    auto devices = SDRFactory::getSupportedDevices();
    for (const auto& dev : devices) {
        std::cout << "  - " << dev << std::endl;
    }
}

int main(int argc, char* argv[]) {
    SDRConfig config;
    std::string mode_str;
    bool list_devices = false;
    bool auto_detect = false;
    
    po::options_description desc("Signal Processing Application - SDR GUI");
    desc.add_options()
        ("help,h", "Show help message")
        ("list-devices,l", po::bool_switch(&list_devices), "List supported devices")
        ("auto,a", po::bool_switch(&auto_detect), "Auto-detect connected devices")
        
        // Device selection
        ("device,d", po::value<std::string>(&config.device_type)->default_value("simulation"),
         "Device type (e.g., usrp, simulation, rtlsdr)")
        ("serial,s", po::value<std::string>(&config.serial_number)->default_value(""),
         "Device serial number or identifier")
        
        // Radio parameters
        ("freq,f", po::value<double>(&config.frequency)->default_value(100e6),
         "Center frequency in Hz (e.g., 2.45e9 for 2.45 GHz)")
        ("rate,r", po::value<double>(&config.sample_rate)->default_value(1e6),
         "Sample rate in samples/second (e.g., 10e6 for 10 MS/s)")
        ("gain,g", po::value<double>(&config.gain)->default_value(20.0),
         "Gain in dB")
        ("bandwidth,b", po::value<double>(&config.bandwidth)->default_value(0.0),
         "Bandwidth in Hz (0 = auto)")
        ("antenna", po::value<std::string>(&config.antenna)->default_value(""),
         "Antenna selection (device-specific)")
        
        // Performance options
        ("buffer-size", po::value<size_t>(&config.buffer_size)->default_value(8192),
         "Buffer size in samples")
        
        // Legacy option for backward compatibility
        ("mode", po::value<std::string>(&mode_str)->default_value(""),
         "Legacy: Mode selection (sim or usrp)");
    
    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        
        // Help
        if (vm.count("help")) {
            std::cout << "\n" << desc << std::endl;
            print_device_list();
            std::cout << "\nExamples:" << std::endl;
            std::cout << "  " << argv[0] << " --device simulation" << std::endl;
            std::cout << "  " << argv[0] << " --device usrp --serial 32C1EC6 --freq 2.45e9 --rate 10e6 --gain 40" << std::endl;
            std::cout << "  " << argv[0] << " --device rtlsdr --freq 433e6 --rate 2e6" << std::endl;
            return 0;
        }
        
        // List devices
        if (list_devices) {
            print_device_list();
            return 0;
        }
        
        // Handle legacy mode option
        if (!mode_str.empty()) {
            std::cout << "Note: --mode is deprecated. Use --device instead." << std::endl;
            if (mode_str == "sim") {
                config.device_type = "simulation";
            } else if (mode_str == "usrp") {
                config.device_type = "usrp";
                if (config.serial_number.empty()) {
                    config.serial_number = "32C1EC6";  // Default serial
                }
            } else {
                std::cerr << "Unknown mode: " << mode_str << std::endl;
                return 1;
            }
        }
        
        // Auto-detect devices
        if (auto_detect) {
            std::cout << "Auto-detecting devices..." << std::endl;
            auto detected = SDRFactory::detectDevices();
            if (detected.empty()) {
                std::cout << "No devices detected. Using simulation mode." << std::endl;
                config.device_type = "simulation";
            } else {
                std::cout << "Found " << detected.size() << " device(s):" << std::endl;
                for (size_t i = 0; i < detected.size(); ++i) {
                    std::cout << "  [" << i << "] " << detected[i].device_type 
                             << " - " << detected[i].serial_number << std::endl;
                }
                // Use first detected device
                config = detected[0];
                std::cout << "Using: " << config.device_type << std::endl;
            }
        }
        
    } catch(const po::error& e) {
        std::cerr << "Error parsing arguments: " << e.what() << std::endl;
        std::cout << desc << std::endl;
        return 1;
    }
    
    // Validate device type
    if (!SDRFactory::isDeviceSupported(config.device_type)) {
        std::cerr << "Error: Unsupported device type '" << config.device_type << "'" << std::endl;
        print_device_list();
        return 1;
    }
    
    // Print configuration
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Device:      " << config.device_type << std::endl;
    if (!config.serial_number.empty()) {
        std::cout << "  Serial:      " << config.serial_number << std::endl;
    }
    std::cout << "  Frequency:   " << std::fixed << std::setprecision(3) 
              << config.frequency / 1e6 << " MHz" << std::endl;
    std::cout << "  Sample rate: " << config.sample_rate / 1e6 << " MS/s" << std::endl;
    std::cout << "  Gain:        " << config.gain << " dB" << std::endl;
    if (config.bandwidth > 0) {
        std::cout << "  Bandwidth:   " << config.bandwidth / 1e6 << " MHz" << std::endl;
    }
    std::cout << "  Buffer size: " << config.buffer_size << " samples" << std::endl;
    std::cout << std::endl;
    
    // GLFW initialization
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }
    
    // OpenGL version
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 
                                          "SDR Signal Processing", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    
	// VSync
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

	// Try to load GLAD & OpenGL not through GLFW
	// This is really scuffed but Spectrolysis uses GLAD so we will too
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		glfwTerminate();
		return 1;
	}
    
    // Initialize Dear ImGui; set GUI flags
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	SetDraculaTheme();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    // Initialize ImPlot
    ImPlot::CreateContext();
    
    // Initialize SignalGui with the device
    SignalGui gui;
    if (!gui.Initialize(config)) {
        std::cerr << "Failed to initialize SignalGui with " << config.device_type << std::endl;
        
        // Fall back to simulation
        std::cout << "Falling back to simulation mode..." << std::endl;
        config.device_type = "simulation";
        if (!gui.Initialize(config)) {
            std::cerr << "Failed to initialize simulation device" << std::endl;
            return 1;
        }
    }
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll events
        glfwPollEvents();
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Update GUI
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
    
    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    return 0;
}
