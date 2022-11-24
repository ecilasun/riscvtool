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
	// NOTE: Video scanout buffers have to be aligned at 64 byte boundary
	uint8_t *framebufferA = (uint8_t*)malloc(640*480*3 + 64);
	uint8_t *framebufferB = (uint8_t*)malloc(640*480*3 + 64);
	framebufferA = (uint8_t*)E32AlignUp((uint32_t)framebufferA, 64);
	framebufferB = (uint8_t*)E32AlignUp((uint32_t)framebufferB, 64);

	GPUSetVPage((uint32_t)framebufferA);
	GPUSetVMode(MAKEVMODEINFO(VIDEOMODE_640PALETTED, VIDEOOUT_ENABLED)); // Mode 1 (640x480), video output on

	uint32_t cycle = 0;
	//uint32_t prevvblankcount = GPUReadVBlankCounter();
	do {
		// Select next r/w pair
		uint8_t *readpage = (cycle%2) ? framebufferA : framebufferB;
		uint8_t *writepage = (cycle%2) ? framebufferB : framebufferA;

		// Wait for vsync
		/*while (prevvblankcount == GPUReadVBlankCounter()) { asm volatile ("nop;"); }
		prevvblankcount = GPUReadVBlankCounter();*/

		// Flip
		GPUSetVPage((uint32_t)readpage);

		// Clar to white
		GPUClearScreen(writepage, VIDEOMODE_640PALETTED, 0x0F0F0F0F);

		// Demo
        io.DisplaySize = ImVec2(640, 480);
        io.DeltaTime = 1.0f / 60.0f;
        ImGui::NewFrame();

		ImGui::ShowDemoWindow(NULL);

        ImGui::SetNextWindowSize(ImVec2(150, 100));
        ImGui::Begin("Test");
        ImGui::Text("Hello, world!");
        ImGui::Text("Frame: %d", (int)cycle);       
        ImGui::End();

        ImGui::Render();
        imgui_sw::paint_imgui((uint32_t*)writepage,640,480);

		// Flush data cache at last pixel so we can see a coherent image
		asm volatile( ".word 0xFC000073;");

		// Flip to next page
		++cycle;
	} while (1);

    ImGui::DestroyContext(); // Won't reach here

	return 0;
}
