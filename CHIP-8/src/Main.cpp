#include<stdint.h>
#include<vector>
#include<array>
#include<fstream>
#include<cassert>
#include <time.h>
#include <algorithm>
#define GLUT_STATIC_LIB
#include "glut.h"
using Byte = uint8_t;
using Word = uint16_t;
Byte delayTimer = 0;
Byte soundTimer = 0;
const int SCREEN_W = 64;
const int SCREEN_H= 32;

int modifier = 10;

// Window size
int display_width = SCREEN_W * modifier;
int display_height = SCREEN_H * modifier;

constexpr size_t MEMORY_SIZE= 4096;
std::array<Byte, MEMORY_SIZE> m_Memory;
std::array<Byte, 16> m_VRegisters;
// must be 12 bits register
Word m_AddressI= 0;
Word m_ProgramCounter; // the 16-bit program counter
std::array<Word,16> m_Stack; // to save programm coutner value if subroutine is called
int m_StackPtr = 0;
bool m_UpdateScreen = true;
unsigned char Fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, //0
	0x20, 0x60, 0x20, 0x20, 0x70, //1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
	0x90, 0x90, 0xF0, 0x10, 0x10, //4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
	0xF0, 0x10, 0x20, 0x40, 0x40, //7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
	0xF0, 0x90, 0xF0, 0x90, 0x90, //A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
	0xF0, 0x80, 0x80, 0x80, 0xF0, //C
	0xE0, 0x90, 0x90, 0x90, 0xE0, //D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
	0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};
constexpr int ScreenDataSize = 64 * 32;
std::array<Byte, ScreenDataSize> m_ScreenData;
std::array<unsigned char, 16>m_Keys;
// two bytes make up opCode, so programm counter increments by 2
Word GetOpCode(Byte a, Byte b)
{
	Word res = 0;
	res = a;
	res = a << 8;
	res |= b;
	return res;
}
Byte Random()
{
	return rand() % 256;
}
void ExecuteOpCode(Word opCode)
{
	
	switch (opCode & 0xF000)
	{

	case 0x0000:
		switch (opCode & 0x00FF)
		{
		case 0x00E0: // clears the screen
		{

			m_UpdateScreen = true;
			m_ScreenData.fill(0);
			m_ProgramCounter += 2;
			break;
		}
		case 0x00EE: // returns from subroutine
		{

			m_StackPtr--;
			m_ProgramCounter = m_Stack[m_StackPtr];
			m_ProgramCounter+=2;
			break;
		}
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opCode);
			break;
		}
		break;
	case 0x1000: 
		m_ProgramCounter = opCode & 0x0FFF; // go to address NNN
		break;
	case 0x2000: // call subroutine at address 0x2NNN
		m_Stack[m_StackPtr] = m_ProgramCounter;
		m_StackPtr++;
		m_ProgramCounter = opCode & 0x0FFF;
		break;
	case 0x3000: // 0x3XNN skip next instruction if VX == NN
	{
		Word x = (opCode >> 8) & 0x000F;
		Word nn = opCode & 0x00FF;

		bool check = m_VRegisters[x] == nn;
		
		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
		break;
	}
	case 0x4000: // 0x4XNN skip next instruction if if VX != NN
	{
		Word v = (opCode >> 8) & 0x000F;
		Word nn = opCode & 0x00FF;
		bool check = m_VRegisters[v] != nn;

		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
		break;
	}
	case 0x5000: // 0x5XY0 Skips the next instruction if VX equals VY
	{
		Word x = (opCode >> 8) & 0x000F;
		Word y = (opCode >> 4) & 0x000F;
		bool check = m_VRegisters[x] ==  m_VRegisters[y];
		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
		break;
	}
	case 0x6000: // 0x6XNN Sets VX to NN
	{
		Word x = (opCode >> 8) & 0x000F;
		Word nn = opCode  & 0x00FF;
		m_VRegisters[x] = nn; 
		m_ProgramCounter += 2;
	break;
	}
	case 0x7000:// 0x7XNN Adds NN to VX
	{
		Word x = (opCode >> 8) & 0x000F;
		Word nn = opCode & 0x00FF;
		m_VRegisters[x] += nn; 
		m_ProgramCounter += 2;
		break;
	}
	case 0x8000: 
		switch (opCode & 0x000F)
		{
		case 0x0000: // 0x8XY0: sets VX to VY 
		{
			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			m_VRegisters[x] = m_VRegisters[y];
			m_ProgramCounter += 2;

			break;
		}
		case 0x0001: // 0x8XY1: sets VX to VY | VX
		{

			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			m_VRegisters[x] |= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;
		}
		case 0x0002: // 0x8XY2: sets VX to VY & VX	

		{
			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			m_VRegisters[x] &= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;

		}
		case 0x0003: // 0x8XY3: sets VX to VY xor VX
		{

			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			m_VRegisters[x] ^= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;
		}
		case 0x0004: // 0x8XY4: adds VY to VX. VF is set to 1 when there's an overflow,  and to 0 when there is not

		{
			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			// a + b <= max
			// a <= max - b if that is not the case then we have overflow situation
			bool isOverflow = m_VRegisters[x] > (0xFF - m_VRegisters[y]);
			m_VRegisters[0xF] = (Byte)isOverflow;
			m_VRegisters[x] += m_VRegisters[y];
			m_ProgramCounter += 2;
			break;	  
		}
					 

		case 0x0005: // 0x8XY5: substract VY from VX
					 // VF is set to 0 when there's an overflow,
					// and to 1 when there is not

		{

			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			// a - b >= min
			// a  >= min - b if that is not the case then we have overflow situation
			bool isUnderflow = m_VRegisters[x] < m_VRegisters[y];
			m_VRegisters[0xF] = (Byte)!isUnderflow;
			m_VRegisters[x] -= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;		 
		}

		case 0x0006: // 0x8XY6: stores the least significant bit of VX in VF
					 // and then shifts VX to right by 1

		{
			Word x = (opCode >> 8) & 0x000F;
			m_VRegisters[0xF] = m_VRegisters[x] & 0x1;
			m_VRegisters[x] >>= 1;
			m_ProgramCounter += 2;
			break;

		}

		case 0x0007: // 0x8XY7: Sets VX to VY minus VX.
					 // VF is set to 0 when there's an underflow, 
					 // and 1 when there is not
		{

			Word x = (opCode >> 8) & 0x000F;
			Word y = (opCode >> 4) & 0x000F;
			bool isUnderflow = m_VRegisters[x] > m_VRegisters[y];
			m_VRegisters[0xF] = (Byte)!isUnderflow;
			m_VRegisters[x] = m_VRegisters[y] - m_VRegisters[x];
			m_ProgramCounter += 2;
			break;
		}
		case 0x000E: // 0x8XYE: Stores the most significant bit of VX in VF 
					 // and then shifts VX to the left by 1
		{

			Word x = (opCode >> 8) & 0x000F;
			m_VRegisters[0xF] = (m_VRegisters[x] >> 7 ) & 0x1;
			m_VRegisters[x] <<= 1;
			m_ProgramCounter += 2;
			break;
		}
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opCode);
			break;	
		}
		break;
	case 0x9000:// 0x9XY0: Skips the next instruction if VX does not equal VY.

	{

		Word x = (opCode >> 8) & 0x000F;
		Word y = (opCode >> 4) & 0x000F;
		bool check = m_VRegisters[x] != m_VRegisters[y];
		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
		break;
	}
	case 0xA000: // 0xANNN: Sets I to the address NNN
	{

			Word nnn = opCode & 0x0FFF;
			m_AddressI = nnn;
			m_ProgramCounter += 2;
			break;
	}
	case 0xB000: // 0xBNNN: Jumps to the address NNN plus V0
	{

		Word nnn = opCode & 0x0FFF;
		m_ProgramCounter = nnn + m_VRegisters[0];
		break;
	}
	case 0xC000: // 0xCXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
		
	{
		auto rand = Random() &0xFF;
		Word x = (opCode >> 8) & 0x000F;
		Word nn = (opCode) & 0x00FF;
		m_VRegisters[x] = rand & nn;
		m_ProgramCounter += 2;
		break;
	}	
	case 0xD000:
		/*
		 0xDXYN:
			Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
			Each row of 8 pixels is read as bit-coded starting from memory location I; 
			I value does not change after the execution of this instruction. 
			As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
			and to 0 if that does not happen.
		*/

	{

		Word vx = (opCode >> 8) & 0x000F;
		Word vy = (opCode >> 4) & 0x000F;
		Word height = (opCode ) & 0x000F;

		const Word width = 8;
		m_VRegisters[0xF] = 0;
		Byte xPos = m_VRegisters[vx] ;
		Byte  yPos = m_VRegisters[vy] ;
		for (int y = 0; y < height; y++)
		{
			Word binaryPixelRow = m_Memory[m_AddressI + y];
			for (int x = 0; x < width; x++)
			{
				// test if we draw
				if ((binaryPixelRow & (0x80 >> x)) != 0)
				{
				  auto index = (xPos + x + ((yPos + y) * 64));
				  if (m_ScreenData[index] == 1)
					  m_VRegisters[0xF] = 1;
				  m_ScreenData[index] ^=  1;
				}
			}
		}
		m_ProgramCounter += 2;
		m_UpdateScreen = true;
		break;
	}
	case 0xE000:
		switch (opCode & 0x00F0)
		{
		case 0x0090: //0xEX9E: Skips the next instruction if the key stored in VX is pressed
		{

			Word x = (opCode >> 8) & 0x000F;
			if (m_Keys[m_VRegisters[x]] !=0 )
			{
				m_ProgramCounter += 4;
			}
			else
			{
				m_ProgramCounter += 2;
			}
			break;
		}
		case 0x00A0: //0xEXA1: Skips the next instruction if the key stored in VX is not pressed
			
		{
			Word x = (opCode >> 8) & 0x000F;
			if (m_Keys[m_VRegisters[x]] == 0)
			{
				m_ProgramCounter += 4;
			}
			else
			{
				m_ProgramCounter += 2;
			}
			break;
		}
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opCode);
			break;
		}
		break;
	case 0xF000:
		switch (opCode & 0x00FF) // 0xFX__
		{
		case 0x0007: //Sets VX to the value of the delay timer

		{

			Word x = (opCode >> 8) & 0x000F;
			m_VRegisters[x] = delayTimer;
			m_ProgramCounter += 2;
			break;
		}
		case 0x000A: //A key press is awaited, and then stored in VX
		{

			Word x = (opCode >> 8) & 0x000F;
			auto check = std::find(m_Keys.begin(), m_Keys.end(), 1);
			if (check != m_Keys.end())
			{
				int index = std::distance(m_Keys.begin(), check);

				m_VRegisters[x] = index;
			}
			else
			{
				return;
			}
			m_ProgramCounter += 2;
			break;
		}

		case 0x0015: // Sets the delay timer to VX
		{
			Word x = (opCode >> 8) & 0x000F;
			delayTimer = m_VRegisters[x];
			m_ProgramCounter += 2;
			break;
		}
		case 0x0018: // Sets the sound timer to VX
		{
			Word x = (opCode >> 8) & 0x000F;
			soundTimer= m_VRegisters[x];
			m_ProgramCounter += 2;
			break;
		}

		case 0x001E: // Adds VX to I. VF is not affected
		{
			Word x = (opCode >> 8) & 0x000F;
			//if (x + m_AddressI > 0xFFF)
			//{
			//	m_VRegisters[0xF] = 1;
			//}
			//else m_VRegisters[0xF] = 0;
			m_AddressI += m_VRegisters[x];
			m_ProgramCounter += 2;
			break;
		}

		case 0x0029: //Sets I to the location of the sprite for the character in VX
		{
			Word x = (opCode >> 8) & 0x000F;
			m_AddressI = m_VRegisters[x] * 0x5;
			m_ProgramCounter += 2;
			break;
		}
		case 0x0033: // Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, 
					 //and the ones digit at location I+2
		{
			Word x = (opCode >> 8) & 0x000F;
			m_Memory[m_AddressI] = (m_VRegisters[x] / 100);
			m_Memory[m_AddressI + 1] = (m_VRegisters[x] / 10) % 10;
			m_Memory[m_AddressI + 2] = (m_VRegisters[x] % 100) % 10;
			m_ProgramCounter += 2;
			break;
		}
		case 0x0055: //Stores from V0 to VX (including VX) 
					 //in memory, starting at address I. The offset from I is increased by 1 for each value written, 
					 //but I itself is left unmodified
		{

			Word x = (opCode >> 8) & 0x000F;
			for (int i = 0; i <= x; i++)
			{
				m_Memory[m_AddressI + i] = m_VRegisters[i];
			}
			m_ProgramCounter += 2;
			break;
		}
		case 0x0065: //Fills from V0 to VX (including VX) with values from 
					 //memory, starting at address I. The offset from I is increased by 1 for each value read, 
					 //but I itself is left unmodified
		{
			Word x = (opCode >> 8) & 0x000F;
			for (int i = 0; i <= x; ++i)
				m_VRegisters[i] = m_Memory[m_AddressI + i];
			m_ProgramCounter += 2;
			break;
		}
		default:
			printf("Unknown opcode [0x0000]: 0x%X\n", opCode);
			break;
		}
		break;
	default:
		printf("Unknown opcode [0x0000]: 0x%X\n", opCode);
		break;
	}
}
#define DRAWWITHTEXTURE
typedef unsigned __int8 u8;
u8 screenData[SCREEN_H][SCREEN_W][3];
// Setup Texture
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

void debug_render()
{
	// Draw
	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 64; ++x)
		{
			if (m_ScreenData[(y * 64) + x] == 0)
				printf("O");
			else
				printf(" ");
		}
		printf("\n");
	}
	printf("\n");
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

void updateTexture()
{
	// Update pixels
	for (int y = 0; y < 32; ++y)
		for (int x = 0; x < 64; ++x)
			if (m_ScreenData[(y * 64) + x] == 0)
				screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 0;	// Disabled
			else
				screenData[y][x][0] = screenData[y][x][1] = screenData[y][x][2] = 255;  // Enabled

	// Update Texture
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0);		glVertex2d(0.0, 0.0);
	glTexCoord2d(1.0, 0.0); 	glVertex2d(display_width, 0.0);
	glTexCoord2d(1.0, 1.0); 	glVertex2d(display_width, display_height);
	glTexCoord2d(0.0, 1.0); 	glVertex2d(0.0, display_height);
	glEnd();
}

void emulate()
{
	auto code = GetOpCode(m_Memory[m_ProgramCounter], m_Memory[m_ProgramCounter+1]);
	printf("opCode: 0x%X\n", code);
	if (0x13DC == code)
	{
		printf("opCode: 0x%X\n", code);
	}
	ExecuteOpCode(code);

	if (delayTimer > 0)
	{
		printf("DECREMENT!\n");
		--delayTimer;
	}

	if (soundTimer> 0)
	{
		if (soundTimer == 1)
			printf("BEEP!\n");
		--soundTimer;
	}

}

void display()
{
	emulate();
	//debug_render();
	if (m_UpdateScreen)
	{
		// Clear framebuffer
		glClear(GL_COLOR_BUFFER_BIT);
	
		updateTexture();
	
		// Swap buffers!
		glutSwapBuffers();
	
		// Processed frame
		m_UpdateScreen = false;
	}
}


void keyboardDown(unsigned char key, int x, int y)
{
	if (key == 27)    // esc
		exit(0);

	if (key == '1')			m_Keys[0x1] = 1;
	else if (key == '2')	m_Keys[0x2] = 1;
	else if (key == '3')	m_Keys[0x3] = 1;
	else if (key == '4')	m_Keys[0xC] = 1;

	else if (key == 'q')	m_Keys[0x4] = 1;
	else if (key == 'w')	m_Keys[0x5] = 1;
	else if (key == 'e')	m_Keys[0x6] = 1;
	else if (key == 'r')	m_Keys[0xD] = 1;

	else if (key == 'a')	m_Keys[0x7] = 1;
	else if (key == 's')	m_Keys[0x8] = 1;
	else if (key == 'd')	m_Keys[0x9] = 1;
	else if (key == 'f')	m_Keys[0xE] = 1;

	else if (key == 'z')	m_Keys[0xA] = 1;
	else if (key == 'x')	m_Keys[0x0] = 1;
	else if (key == 'c')	m_Keys[0xB] = 1;
	else if (key == 'v')	m_Keys[0xF] = 1;

	//printf("Press key %c\n", key);
}

void keyboardUp(unsigned char key, int x, int y)
{
	if (key == '1')			m_Keys[0x1] = 0;
	else if (key == '2')	m_Keys[0x2] = 0;
	else if (key == '3')	m_Keys[0x3] = 0;
	else if (key == '4')	m_Keys[0xC] = 0;
	else if (key == 'q')	m_Keys[0x4] = 0;
	else if (key == 'w')	m_Keys[0x5] = 0;
	else if (key == 'e')	m_Keys[0x6] = 0;
	else if (key == 'r')	m_Keys[0xD] = 0;
	else if (key == 'a')	m_Keys[0x7] = 0;
	else if (key == 's')	m_Keys[0x8] = 0;
	else if (key == 'd')	m_Keys[0x9] = 0;
	else if (key == 'f')	m_Keys[0xE] = 0;
	else if (key == 'z')	m_Keys[0xA] = 0;
	else if (key == 'x')	m_Keys[0x0] = 0;
	else if (key == 'c')	m_Keys[0xB] = 0;
	else if (key == 'v')	m_Keys[0xF] = 0;
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	m_AddressI = 0;
	// the game loads into this address
	m_ProgramCounter = 0x200;
	m_VRegisters.fill(0);
	m_ScreenData.fill(0);
	m_Memory.fill(0);
	m_Keys.fill(0);
	m_Stack.fill(0);
	// Load fontset
	for (int i = 0; i < 80; ++i)
		m_Memory[i+ 0x50] = Fontset[i];
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
#if 1

	std::ifstream file;
	//file.open("./../games/pong2.c8", std::ios::binary);
	//file.open("./../test_opcode.ch8", std::ios::in | std::ios::binary);
	file.open("./../2-ibm-logo.ch8", std::ios::in | std::ios::binary);

	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		auto fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		auto memory = (char*)m_Memory.data() + 512;

		file.read(memory, fileSize);
		file.close();
	}
	else
	{
		assert(false);
	}
	#else
	using namespace std;
	ifstream ROM("./../test.txt");
	char byte;
	char byte1;
	if (ROM.is_open()) {
		for (int i = 0x200; ROM.get(byte), ROM.get(byte1); i++) {
			m_Memory[i] = (uint8_t)byte1;
			m_Memory[i] <<= 4;
			m_Memory[i] |= byte;
		}
	}
	ROM.close();
#endif // 0


	glutMainLoop();

}

