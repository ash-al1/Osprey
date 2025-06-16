#pragma once

#include "imgui.h"
#include "implot.h"

class SignalGui {
private:
	static constexpr int WINDOW_WIDTH  = 1200;
	static constexpr int WINDOW_HEIGHT = 800;

public:
	SignalGui();
	~SignalGui();

	void Update();

private:
	void RenderTimeDomainPlot();
	void RenderFrequencyPlot();
	void RenderSpectrogramPlot();
	void RenderReservedSection();
};
