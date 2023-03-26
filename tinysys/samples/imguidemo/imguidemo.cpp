#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"
#include "xadc.h"

#include "imgui/imgui.h"
#include "imgui/imgui_sw.h"
#include <stdio.h>

#define min(_x_,_y_) (_x_) < (_y_) ? (_x_) : (_y_)
#define max(_x_,_y_) (_x_) > (_y_) ? (_x_) : (_y_)

// Bayer ordered dither matrix
const uint8_t dithermatrix[4][4] = {
  { 0, 8, 2,10},
  {12, 4,14, 6},
  { 3,11, 1, 9},
  {15, 7,13, 5}
};

int main()
{
	int target = 0;
	for (int b=0;b<4;++b)
		for (int g=0;g<8;++g)
			for (int r=0;r<8;++r)
				GPUSetPal(target++, r*36, g*36, b*85);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    imgui_sw::bind_imgui_painting();
    imgui_sw::make_style_fast();

	// Set up frame buffers
	uint8_t *framebufferA = GPUAllocateBuffer(FRAME_WIDTH_MODE0 * FRAME_HEIGHT_MODE0);
	uint8_t *framebufferB = GPUAllocateBuffer(FRAME_WIDTH_MODE0 * FRAME_HEIGHT_MODE0);

	struct EVideoContext vx;
	GPUSetVMode(&vx, EVM_320_Pal, EVS_Enable);
	GPUSetWriteAddress(&vx, (uint32_t)framebufferA);
	GPUSetScanoutAddress(&vx, (uint32_t)framebufferB);
	GPUClearScreen(&vx, 0x03030303);

	// Set up buffer for 32 bit imgui render output
	// We try to align it to a cache boundary to support future DMA copies
	uint32_t *imguiframebuffer = (uint32_t*)GPUAllocateBuffer(FRAME_WIDTH_MODE0 * FRAME_HEIGHT_MODE0*4);

	static float temps[] = { 40.f, 40.f, 40.f, 40.f, 40.f, 40.f, 40.f, 40.f, 40.f, 40.f };

	uint32_t cycle = 0;
	uint32_t prevvblankcount = GPUReadVBlankCounter();
	do {
		// Select next r/w pair
		uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;
		uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;
		GPUSetWriteAddress(&vx, (uint32_t)writepage);
		GPUSetScanoutAddress(&vx, (uint32_t)readpage);

		uint32_t vblankcount = GPUReadVBlankCounter();
		if (vblankcount > prevvblankcount)
		{
			uint32_t ADCcode = *XADCTEMP;
			float temp_centigrates = (ADCcode*503.975f)/4096.f-273.15f;

		// Clear write buffer
			//GPUClearScreen((uint8_t*)imguiframebuffer, VIDEOMODE_640PALETTED, 0x0F0F0F0F);

			// Demo
			io.DisplaySize = ImVec2(FRAME_WIDTH_MODE0, FRAME_HEIGHT_MODE0);
			io.DeltaTime = 1.0f / 60.0f;
			ImGui::NewFrame();

			//ImGui::ShowDemoWindow(NULL);

			ImGui::SetNextWindowSize(ImVec2(200, 80));
			ImGui::Begin("Test");
			ImGui::Text("Device temperature: %f C", temp_centigrates);
			ImGui::Text("Frame: %d", (int)cycle);
            ImGui::PlotLines("Curve", temps, IM_ARRAYSIZE(temps));
			ImGui::End();

			for (int i=0;i<9;++i)
				temps[i] = temps[i+1];
			temps[9] = temp_centigrates;

			ImGui::Render();
			imgui_sw::paint_imgui((uint32_t*)imguiframebuffer, FRAME_WIDTH_MODE0, FRAME_HEIGHT_MODE0);

			// Convert to a coherent image for our 8bpp display
			// NOTE: This will not be necessary when we support RGB frame buffers
			// or one could add a GPU function to do this in hardware
			for (int y=0;y<FRAME_HEIGHT_MODE0;++y)
			{
				for (int x=0;x<FRAME_WIDTH_MODE0;++x)
				{
					uint32_t img = imguiframebuffer[x+y*FRAME_WIDTH_MODE0];

					// img -> a, b, g, r
					uint8_t B = (img>>16)&0x000000FF;
					uint8_t G = (img>>8)&0x000000FF;
					uint8_t R = (img)&0x000000FF;

					uint8_t ROFF = min(dithermatrix[x&3][y&3] + R, 255);
					uint8_t GOFF = min(dithermatrix[x&3][y&3] + G, 255);
					uint8_t BOFF = min(dithermatrix[x&3][y&3] + B, 255);

					R = ROFF/36;
					G = GOFF/36;
					B = BOFF/85;

					writepage[x+y*FRAME_WIDTH_MODE0] = (uint8_t)((B<<6) | (G<<3) | R);
				}
			}

			// Flush data cache at last pixel so we can see a coherent image when we scan out
			CFLUSH_D_L1;

			// Swap to next page
			++cycle;
		}
	} while (1);

    ImGui::DestroyContext(); // Won't reach here, but oh well...

	return 0;
}
