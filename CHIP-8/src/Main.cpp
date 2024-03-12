
#include"Chip8.h"

#include<vector>
#include<array>
#include <chrono>
#include <cassert>
#include <iostream>
#define GLUT_STATIC_LIB
#include <windows.h> 


#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
// Link library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")
using namespace DirectX;
// global declarations
IDXGISwapChain* swapchain;             // the pointer to the swap chain interface
ID3D11Device* dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext* devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView* backbuffer;    
// Define the functionality of the rasterizer stage.
ID3D11RasterizerState* rasterizerState = nullptr;
D3D11_VIEWPORT viewport = { 0 };
ID3D11VertexShader* vertex = nullptr;
ID3D11PixelShader* pixel= nullptr;
unsigned int clientWidth = 0;
unsigned int clientHeight = 0;
bool vsync = true;
// Safely release a COM object.
template<typename T>
inline void SafeRelease(T& ptr)
{
	if (ptr != NULL)
	{
		ptr->Release();
		ptr = NULL;
	}
}
void CleanD3D() 
{
	SafeRelease(vertex);
	SafeRelease(pixel);

	SafeRelease(backbuffer);
	SafeRelease(rasterizerState);
	SafeRelease(swapchain);
	SafeRelease(devcon);
	SafeRelease(dev);
}
DXGI_RATIONAL QueryRefreshRate(UINT screenWidth, UINT screenHeight, BOOL vsync)
{
	DXGI_RATIONAL refreshRate = { 0, 1 };
	if (vsync)
	{
		IDXGIFactory* factory;
		IDXGIAdapter* adapter;
		IDXGIOutput* adapterOutput;
		DXGI_MODE_DESC* displayModeList;

		// Create a DirectX graphics interface factory.
		HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Could not create DXGIFactory instance."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to create DXGIFactory.");
		}

		hr = factory->EnumAdapters(0, &adapter);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapters."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapters.");
		}

		hr = adapter->EnumOutputs(0, &adapterOutput);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to enumerate adapter outputs."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to enumerate adapter outputs.");
		}

		UINT numDisplayModes;
		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, nullptr);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		displayModeList = new DXGI_MODE_DESC[numDisplayModes];
		assert(displayModeList);

		hr = adapterOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numDisplayModes, displayModeList);
		if (FAILED(hr))
		{
			MessageBox(0,
				TEXT("Failed to query display mode list."),
				TEXT("Query Refresh Rate"),
				MB_OK);

			throw new std::exception("Failed to query display mode list.");
		}

		// Now store the refresh rate of the monitor that matches the width and height of the requested screen.
		for (UINT i = 0; i < numDisplayModes; ++i)
		{
			if (displayModeList[i].Width == screenWidth && displayModeList[i].Height == screenHeight)
			{
				refreshRate = displayModeList[i].RefreshRate;
			}
		}

		delete[] displayModeList;
		SafeRelease(adapterOutput);
		SafeRelease(adapter);
		SafeRelease(factory);
	}

	return refreshRate;
}

// this function initializes and prepares Direct3D for use
int InitD3D(HWND hWnd)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferDesc.RefreshRate = QueryRefreshRate(clientWidth, clientHeight, vsync);
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = hWnd;                                // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode


	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	UINT createDeviceFlags = 0;
#if _DEBUG
	createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
	// create a device, device context and swap chain using the information in the scd struct
	auto check = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		createDeviceFlags,
		featureLevels,
		_countof(featureLevels),
		D3D11_SDK_VERSION,
		&scd,
		&swapchain,
		&dev,
		NULL,
		&devcon);
	if (FAILED(check)) return -1;

	// get the address of the back buffer
	ID3D11Texture2D* pBackBuffer;
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

	// use the back buffer address to create the render target
	check = dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
	if (FAILED(check)) return -1;
	SafeRelease(pBackBuffer);
	// set the render target as the back buffer
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);


	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.ScissorEnable = FALSE;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;

	// Create the rasterizer state object.
	check = dev->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	if (FAILED(check)) return -1;
	viewport.Width = static_cast<float>(clientWidth);
	viewport.Height = static_cast<float>(clientHeight);
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Load the compiled vertex shader.
	ID3DBlob* vertexShaderBlob;
	LPCWSTR compiledVertexShaderObject = L"./../x64/Debug/vertex.cso";

	auto hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
	if (FAILED(hr))
	{
		return 1;
	}

	hr = dev->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &vertex);
	if (FAILED(hr))
	{
		return 1;
	}

	ID3DBlob* pixleShaderBlob;
	compiledVertexShaderObject = L"./../x64/Debug/pixel.cso";

	 hr = D3DReadFileToBlob(compiledVertexShaderObject, &pixleShaderBlob);
	if (FAILED(hr))
	{
		return 1;
	}

	hr = dev->CreatePixelShader(pixleShaderBlob->GetBufferPointer(), pixleShaderBlob->GetBufferSize(), nullptr, &pixel);
	if (FAILED(hr))
	{
		return 1;
	}
	devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devcon->VSSetShader(vertex, nullptr, 0);
	devcon->PSSetShader(pixel, nullptr, 0);
	devcon->RSSetState(rasterizerState);
	devcon->RSSetViewports(1, &viewport);
	devcon->OMSetRenderTargets(1, &backbuffer, nullptr);
	return 0;
}


void Present(bool vSync)
{
	if (vSync)
	{
		swapchain->Present(1, 0);
	}
	else
	{
		swapchain->Present(0, 0);
	}
}

// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4])
{
	devcon->ClearRenderTargetView(backbuffer, clearColor);
}

void Render()
{
	assert(dev);
	assert(devcon);
	Clear(Colors::CornflowerBlue);
	devcon->Draw(6, 0);
	Present(vsync);
}


// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam);

std::array<unsigned char, 16>keys;
int modifier = 10;
int display_width = SCREEN_W * modifier;
int display_height = SCREEN_H * modifier;
std::array<int, 3> colors = { 0x00,0x88,0xFF };
std::vector<int> colorLevels ;
int maxColorLevelIndex;
int maxColorIndex;
int framePersistence = 3;


std::array<Byte, ScreenDataSize> m_PhysDisplay;
Chip8 chip;


typedef unsigned __int8 u8;
u8 screenData[SCREEN_H][SCREEN_W][3];
// Setup Texture
#if 0
void setupTexture()
{
	// Clear screen
	for (int y = 0; y < SCREEN_H; ++y)
		for (int x = 0; x < SCREEN_W; ++x)
			screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;

	// Create a texture 
	glTexImage2D(GL_TEXTURE_2D, 0, 3, SCREEN_W, SCREEN_H, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

	// Set up the texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// Enable textures
	glEnable(GL_TEXTURE_2D);
}


void reshape_window(GLsizei w, GLsizei h)
{
	glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, h, 0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);
	// Resize quad
	display_width = w;
	display_height = h;


}
#endif


void updateTexture()
{
	// Update pixels
	auto& screen = chip.GetScreen();
	auto& updated = chip.GetUpdated();
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x)
		{
			int index = (y * 64) + x;
			// if updated and draws 1 then max
			if (!updated[index] || screen[index] != 1)
			{
				//if (!m_Updated[index])
				//{
				//	screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = colors[0];	
				//}
				continue;
			}
			
			m_PhysDisplay[index] = maxColorLevelIndex;
			screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = colors[2]; 
		}
	std::vector<std::vector<int>>dimmedPixels(maxColorIndex);
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x)
		{
			int index = (y * 64) + x;
			if (screen[index] == 1) continue;
			auto prevLevel = m_PhysDisplay[index];
			if (prevLevel == 0) continue; // skip if fully off

			auto currentLevel = prevLevel - 1;
			m_PhysDisplay[index] = currentLevel;

			if (colorLevels[currentLevel] != colorLevels[prevLevel]) {
				auto colorIndex =colorLevels[currentLevel];

				dimmedPixels[colorIndex].push_back(x);
				dimmedPixels[colorIndex].push_back(y);
			}

		}
	for (auto color = 0; color < dimmedPixels.size(); color++) {
		auto pixels = dimmedPixels[color];
		for (int j = 0; j < pixels.size(); j += 2) {
			auto  x = pixels[j];
			auto  y = pixels[j + 1];

			screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = colors[color];
		}
	}
#if 0

	// Update Texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
	glTexCoord2d(1.0, 0.0); 	glVertex2d(display_width, 0.0);
	glTexCoord2d(1.0, 1.0); 	glVertex2d(display_width, display_height);
	glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, display_height);
	glEnd();
#endif // 0

}


auto lastCycleTime = std::chrono::high_resolution_clock::now();
// amount of time one frame executes
float cycleDelay = 1.5;
void display()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();
	if (dt < cycleDelay) return;
	lastCycleTime = currentTime;
	chip.Emulate(keys);
	//debug_render();
	if (chip.IsUpdateScreen())
	{
#if 0

		glClear(GL_COLOR_BUFFER_BIT);
	
		updateTexture();
	
		glutSwapBuffers();
	
#endif // 0

		// Clear framebuffer
		chip.SetUpdateScreen(false);
	}
}


void keyboardDown(unsigned char key, int x, int y)
{
	if (key == 27)    // esc
		exit(0);

	if (key == '1')			keys[0x1] = 1;
	else if (key == '2')	keys[0x2] = 1;
	else if (key == '3')	keys[0x3] = 1;
	else if (key == '4')	keys[0xC] = 1;

	else if (key == 'q')	keys[0x4] = 1;
	else if (key == 'w')	keys[0x5] = 1;
	else if (key == 'e')	keys[0x6] = 1;
	else if (key == 'r')	keys[0xD] = 1;

	else if (key == 'a')	keys[0x7] = 1;
	else if (key == 's')	keys[0x8] = 1;
	else if (key == 'd')	keys[0x9] = 1;
	else if (key == 'f')	keys[0xE] = 1;

	else if (key == 'z')	keys[0xA] = 1;
	else if (key == 'x')	keys[0x0] = 1;
	else if (key == 'c')	keys[0xB] = 1;
	else if (key == 'v')	keys[0xF] = 1;

	//printf("Press key %c\n", key);
}

void keyboardUp(unsigned char key, int x, int y)
{
	if (key == '1')			keys[0x1] = 0;
	else if (key == '2')	keys[0x2] = 0;
	else if (key == '3')	keys[0x3] = 0;
	else if (key == '4')	keys[0xC] = 0;
	else if (key == 'q')	keys[0x4] = 0;
	else if (key == 'w')	keys[0x5] = 0;
	else if (key == 'e')	keys[0x6] = 0;
	else if (key == 'r')	keys[0xD] = 0;
	else if (key == 'a')	keys[0x7] = 0;
	else if (key == 's')	keys[0x8] = 0;
	else if (key == 'd')	keys[0x9] = 0;
	else if (key == 'f')	keys[0xE] = 0;
	else if (key == 'z')	keys[0xA] = 0;
	else if (key == 'x')	keys[0x0] = 0;
	else if (key == 'c')	keys[0xB] = 0;
	else if (key == 'v')	keys[0xF] = 0;
}

HWND CreateWin(HINSTANCE hInstance)
{
	RECT wr = { 0, 0, 500, 400 };    // set the size, but not the position
	//Calculates the required size of the window rectangle, based on the desired client - rectangle size.
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);    // adjust the size

	
	// this struct holds information for the window class
	WNDCLASSEX wc;

	// clear out the window class for use
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	// fill in the struct with the needed information
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName = L"WindowClass1";

	// register the window class
	RegisterClassEx(&wc);
	// the handle for the window, filled by a function
	HWND hWnd;
	// create the window and use the result as the handle
	clientWidth = wr.right - wr.left;
	clientHeight = wr.bottom - wr.top;
	hWnd = CreateWindowEx(NULL,
		L"WindowClass1",    // name of the window class
		L"Our First Windowed Program",   // title of the window
		WS_OVERLAPPEDWINDOW,    // window style
		300,    // x-position of the window
		300,    // y-position of the window
		clientWidth,    // width of the window
		clientHeight,    // height of the window
		NULL,    // we have no parent window, NULL
		NULL,    // we aren't using menus, NULL
		hInstance,    // application handle
		NULL);    // used with multiple windows, NULL

	// display the window on the screen
	
	return hWnd;
}

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	
	auto hWnd = CreateWin(hInstance);
	ShowWindow(hWnd, nShowCmd);
    auto check = InitD3D(hWnd);
	if (check != 0)
	{
		MessageBox(nullptr, TEXT("Failed to create DirectX device and swap chain."), TEXT("Error"), MB_OK);
		return -1;
	}
	// this struct holds Windows event messages
	MSG msg{};

	while (true)
	{
		// Check to see if any messages are waiting in the queue
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// translate keystroke messages into the right format
			TranslateMessage(&msg);

			// send the message to the WindowProc function
			DispatchMessage(&msg);

			// check to see if it's time to quit
			if (msg.message == WM_QUIT)
				break;
		}
		else
		{
			Render();
		}
	}


	CleanD3D();
	// return this part of the WM_QUIT message to Windows
	return msg.wParam;
}
// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// sort through and find what code to run for the message given
	switch (message)
	{
		// this message is read when the window is closed
	case WM_DESTROY:
	{
		// close the application entirely
		PostQuitMessage(0);
		return 0;
	} break;
	}

	// Handle any messages the switch statement didn't
	return DefWindowProc(hWnd, message, wParam, lParam);
}
#if 0
int main(int argc, char** argv)
{

	keys.fill(0);
	chip.Init();


	for (int i = 0; i < colors.size(); i++)
	{
		for (int j = 0; j < framePersistence; j++)
		{
			colorLevels.push_back(i);
		}
	}
	maxColorIndex = colors.size() - 1;
	maxColorLevelIndex = colorLevels.size() - 1;
	// Load fontset

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

	glutInitWindowSize(display_width, display_height);
	glutInitWindowPosition(320, 320);
	glutCreateWindow("myChip8 by Laurence Muller");

	glutReshapeFunc(reshape_window);
	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutKeyboardFunc(keyboardDown);
	glutKeyboardUpFunc(keyboardUp);
	setupTexture();

	// working directory is a project directory

	//file.open("./../games/pong2.c8", std::ios::binary);
	//file.open("./../JamesGriffin CHIP-8-Emulator master roms/MAZE", std::ios::binary);
	//file.open("./../JamesGriffin CHIP-8-Emulator master roms/PONG2", std::ios::binary);
	//file.open("./../kripod chip8-roms master programs/SQRT Test [Sergey Naydenov, 2010].ch8", std::ios::binary);
	//file.open("./../games/Pong (1 player).ch8", std::ios::binary);
	//file.open("./../games/Space Invaders [David Winter] (alt).ch8", std::ios::binary);
	//file.open("./../games/Space Invaders [David Winter] (alt).ch8", std::ios::binary);
	//file.open("./../test_opcode.ch8", std::ios::in | std::ios::binary);
	//file.open("./../c8_test.c8", std::ios::in | std::ios::binary);
	//file.open("./../2-ibm-logo.ch8", std::ios::in | std::ios::binary);

	try
	{
		chip.LoadROM("./../JamesGriffin CHIP-8-Emulator master roms/PONG2");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what();
	}



	glutMainLoop();
}

#endif // 0


