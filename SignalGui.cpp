/*
 * References:
 * ImGui: https://github.com/ocornut/imgui/wiki/Getting-Started
 * ImGui: https://skia.googlesource.com/external/github.com/ocornut/imgui/+/refs/heads/master/imgui.h
 */

#include "SignalGui.h"
#include <iostream>

// Constructor
SignalGui::SignalGui() {
}

// Destructor
SignalGui::~SignalGui() {
}

void SignalGui::Update() {
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_Always);


	ImGui::Begin("SigProc", nullptr,
			ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);

	// Framerate
	ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);

	// Dimensions of each plot
	float quarter_width  = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	float quarter_height = (ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y) * 0.5f;

	// Top Left
	ImGui::BeginChild("TimeDomain", ImVec2(quarter_width, quarter_height), true);
	RenderTimeDomainPlot();
	ImGui::EndChild();
	ImGui::SameLine();

	// Top Right
	ImGui::BeginChild("Frequency", ImVec2(quarter_width, quarter_height), true);
	RenderFrequencyPlot();
	ImGui::EndChild();

	// Bottom Left
	ImGui::BeginChild("Spectrogram", ImVec2(quarter_width, quarter_height), true);
	RenderSpectrogramPlot();
	ImGui::EndChild();
	ImGui::SameLine();

	// Bottom Right
	ImGui::BeginChild("Reserved", ImVec2(quarter_width, quarter_height), true);
	RenderReservedSection();
	ImGui::EndChild();
	ImGui::End();
}

void SignalGui::RenderTimeDomainPlot() {
	ImGui::Text("Time domain");
	if ( ImPlot::BeginPlot("##TimePlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Time [s]", "Amplitude");
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderFrequencyPlot() {
	ImGui::Text("Frequency domain");
	if ( ImPlot::BeginPlot("##FreqPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Frequency [Hz]", "Magnitude [dB]");
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderSpectrogramPlot() {
	ImGui::Text("Spectrogram");
	if ( ImPlot::BeginPlot("##SpectrogramPlot", ImVec2(-1, -1))) {
		ImPlot::SetupAxes("Time [S]", "Frequency [KHz]", ImPlotAxisFlags_NoTickLabels, ImPlotAxisFlags_Lock);
		ImPlot::EndPlot();
	}
}

void SignalGui::RenderReservedSection() {
	ImGui::Text("Reserved");
	ImGui::Text("Future");
}
