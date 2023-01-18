#include <math.h>
#include "core.h"
#include "uart.h"
#include "gpu.h"

#include "imgui/imgui.h"
#include "imgui/imgui_sw.h"
#include <stdio.h>

int main()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    imgui_sw::bind_imgui_painting();
    imgui_sw::make_style_fast();


	// Set up frame buffers
	uint8_t *framebufferA = GPUAllocateBuffer(640*480);
	uint8_t *framebufferB = GPUAllocateBuffer(640*480);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(VIDEOMODE_640PALETTED, VIDEOOUT_ENABLED)); // Mode 1 (640x480), video output on

	// Set up buffer for 32 bit imgui render output
	// We try to align it to a cache boundary to support future DMA copies
	uint32_t *imguiframebuffer = (uint32_t*)malloc(640*480*4 + 128);
	imguiframebuffer = (uint32_t*)E32AlignUp((uint32_t)imguiframebuffer, 64);

	uint32_t cycle = 0;
	uint32_t prevvblankcount = GPUReadVBlankCounter();
	do {
		// Select next r/w pair
		uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;
		uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;

		uint32_t vblankcount = GPUReadVBlankCounter();
		if (vblankcount > prevvblankcount)
		{
			// Clear write buffer
			GPUClearScreen((uint8_t*)imguiframebuffer, VIDEOMODE_640PALETTED, 0x0F0F0F0F);

			// Demo
			io.DisplaySize = ImVec2(640, 480);
			io.DeltaTime = 1.0f / 60.0f;
			ImGui::NewFrame();

			//ImGui::ShowDemoWindow(NULL);

			ImGui::SetNextWindowSize(ImVec2(150, 100));
			ImGui::Begin("Test");
			ImGui::Text("Hello, world!");
			ImGui::Text("Frame: %d", (int)cycle);       
			ImGui::End();

			ImGui::Render();
			imgui_sw::paint_imgui((uint32_t*)imguiframebuffer, 640, 480);

			// Convert to a coherent image for our 8bpp display
			for (uint32_t i=0;i<640*480;++i)
			{
				uint32_t B = imguiframebuffer[i];
				writepage[i] = ((B&0xFF000000)>>24) ^ ((B&0x00FF0000)>>16) ^ ((B&0x0000FF00)>>8) ^ (B&0x000000FF);
			}

			// Flush data cache at last pixel so we can see a coherent image when we scan out
			CFLUSH_D_L1;

			// Flip
			GPUSetVPage((uint32_t)readpage);

			// Swap to next page
			++cycle;
		}
	} while (1);

    ImGui::DestroyContext(); // Won't reach here, but oh well...

	return 0;
}
