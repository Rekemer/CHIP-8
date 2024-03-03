#include<stdint.h>
#include<vector>
#include<array>
#include<fstream>
#include<cassert>
#include <time.h>
#include <algorithm>
using Byte = uint8_t;
using Word = uint16_t;
Byte delayTimer = 0;
Byte soundTimer = 0;
constexpr size_t MEMORY_SIZE= 4096;
std::array<Byte, MEMORY_SIZE> m_Memory;
std::array<Byte, 16> m_VRegisters;
// must be 12 bits register
Word m_AddressI;
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
std::array<unsigned char, 16>key;
// two bytes make up opCode, so programm counter increments by 2
Word GetOpCode(Byte a, Byte b)
{
	Word res = 0;
	res = a;
	res = res << 8;
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
		switch (opCode & 0x0FFF)
		{
		case 0x00E0: // clears the screen
			m_UpdateScreen = true;
			m_ScreenData.fill(0);
			break;
		case 0x00EE: // returns from subroutine
			m_ProgramCounter = m_Stack[m_StackPtr];
			break;
		default:
			break;
		}
		break;
	case 0x1000: 
		m_ProgramCounter = opCode & 0x0FFF; // go to address 0x1NNN
		break;
	case 0x2000: // call subroutine at address 0x2NNN
		m_Stack[m_StackPtr++] = m_ProgramCounter;
		m_ProgramCounter = opCode & 0x0FFF;
		break;
	case 0x3000: // 0x3XNN skip next instruction if VX == NN
	{
		Word v = (opCode >> 8) & 0x000F;
		Word nn = opCode & 0x00FF;

		bool check = m_VRegisters[v] == nn;
		
		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
	}
		break;
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
	}
		break;
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
	}
		break;
	case 0x6000: // 0x6XNN Sets VX to NN
	{
		Word x = (opCode >> 8) & 0x000F;
		Word nn = opCode  & 0x00FF;
		m_VRegisters[x] = nn; m_ProgramCounter += 2;
	}
	break;
	case 0x7000:// 0x7XNN Adds NN to VX
	{
		Word x = (opCode >> 8) & 0x000F;
		Word nn = opCode & 0x00FF;
		m_VRegisters[x] += nn; m_ProgramCounter += 2;
	}
		break;
	case 0x8000: 
		switch (opCode & 0x000F)
		{
		case 0x0000: // 0x8XY0: sets VX to VY 
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			m_VRegisters[x] = m_VRegisters[y];
			break;
		case 0x0001: // 0x8XY1: sets VX to VY | VX
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			m_VRegisters[x] |= m_VRegisters[y]; m_ProgramCounter += 2;
			break;
		case 0x0002: // 0x8XY2: sets VX to VY & VX	
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			m_VRegisters[x] &= m_VRegisters[y]; m_ProgramCounter += 2;
			break;
		case 0x0003: // 0x8XY3: sets VX to VY xor VX
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			m_VRegisters[x] ^= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;
		case 0x0004: // 0x8XY4: adds VY to VX. VF is set to 1 when there's an overflow,  and to 0 when there is not
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			// a + b <= max
			// a <= max - b if that is not the case then we have overflow situation
			bool isOverflow = m_VRegisters[x] > (0xFF - m_VRegisters[y]);
			m_VRegisters[0xF] = (Byte)isOverflow;
			m_VRegisters[x] += m_VRegisters[y];
			m_ProgramCounter += 2;
			break;	  
					 

		case 0x0005: // 0x8XY5: substract VY from VX
					 // VF is set to 0 when there's an overflow,
					// and to 1 when there is not
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			// a - b >= min
			// a  >= min - b if that is not the case then we have overflow situation
			bool isUnderflow = m_VRegisters[x] < m_VRegisters[y];
			m_VRegisters[0xF] = (Byte)!isUnderflow;
			m_VRegisters[x] -= m_VRegisters[y];
			m_ProgramCounter += 2;
			break;		 
		case 0x0006: // 0x8XY6: stores the least significant bit of VX in VF
					 // and then shifts VX to right by 1
			Word x = (opCode << 8) & 0x000F;
			m_VRegisters[0xF] = m_VRegisters[x] & 0x1;
			m_VRegisters[x] >>= 1;
			m_ProgramCounter += 2;
			break;

		case 0x0007: // 0x8XY7: Sets VX to VY minus VX.
					 // VF is set to 0 when there's an underflow, 
					 // and 1 when there is not
			Word x = (opCode << 8) & 0x000F;
			Word y = (opCode << 4) & 0x000F;
			bool isUnderflow = m_VRegisters[x] < m_VRegisters[y];
			m_VRegisters[0xF] = (Byte)!isUnderflow;
			m_VRegisters[x] = m_VRegisters[y] - m_VRegisters[x];
			m_ProgramCounter += 2;
			break;
		case 0x000E: // 0x8XYE: Stores the most significant bit of VX in VF 
					 // and then shifts VX to the left by 1
			Word x = (opCode << 8) & 0x000F;
			m_VRegisters[0xF] = (m_VRegisters[x] << 7 ) & 0x1;
			m_VRegisters[x] <<= 1;
			m_ProgramCounter += 2;
			break;
		default:	
			break;	
		}
		break;
	case 0x9000:// 0x9XY0: Skips the next instruction if VX does not equal VY.

		Word x = (opCode << 8) & 0x000F;
		Word y = (opCode << 4) & 0x000F;
		bool check = m_VRegisters[x] == m_VRegisters[y];
		if (check)
		{
			m_ProgramCounter += 4;
		}
		else
		{
			m_ProgramCounter += 2;
		}
		break;
	case 0xA000: // 0xANNN: Sets I to the address NNN
		Word nnn = opCode & 0x0FFF;
		m_AddressI = nnn;
		m_ProgramCounter += 2;
		break;
	case 0xB000: // 0xBNNN: Jumps to the address NNN plus V0
		Word nnn = opCode & 0x0FFF;
		m_ProgramCounter = nnn + m_VRegisters[0];
		m_ProgramCounter += 2;
		break;
	case 0xC000: // 0xCXNN: Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN
		auto rand = Random();
		Word x = (opCode << 8) & 0x000F;
		Word nn = (opCode) & 0x00FF;
		m_VRegisters[x] = rand & nn;
		m_ProgramCounter += 2;
		break;
	case 0xD000:
		/*
		 0xDXYN:
			Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
			Each row of 8 pixels is read as bit-coded starting from memory location I; 
			I value does not change after the execution of this instruction. 
			As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, 
			and to 0 if that does not happen.
		*/
		break;
	case 0xE000:
		switch (opCode & 0x00F0)
		{
		case 0x0090: //0xEX9E: Skips the next instruction if the key stored in VX is pressed
			Word x = (opCode << 8) & 0x000F;
			if (key[m_VRegisters[x]] == 1)
			{
				m_ProgramCounter += 4;
			}
			else
			{
				m_ProgramCounter += 2;
			}
			break;
		case 0x00A0: //0xEXA1: Skips the next instruction if the key stored in VX is not pressed
			Word x = (opCode << 8) & 0x000F;
			if (key[m_VRegisters[x]] == 0)
			{
				m_ProgramCounter += 4;
			}
			else
			{
				m_ProgramCounter += 2;
			}
			break;
		default:
			break;
		}
		break;
	case 0xF000:
		switch (opCode & 0x00FF) // 0xFX__
		{
		case 0x0007: //Sets VX to the value of the delay timer
			Word x = (opCode << 8) & 0x000F;
			m_VRegisters[x] = delayTimer;
			break;
		case 0x000A: //A key press is awaited, and then stored in VX
			Word x = (opCode << 8) & 0x000F;
			auto check = std::find(key.begin(), key.end(), 1);
			if (check != key.end())
			{
				m_VRegisters[x] = *check;
			}
			else
			{
				return;
			}
			m_ProgramCounter += 2;
			break;
		case 0x0015: // Sets the delay timer to VX
			break;
		case 0x0018: // Sets the sound timer to VX
			break;
		case 0x001E: // Adds VX to I. VF is not affected
			break;
		case 0x0029: //Sets I to the location of the sprite for the character in VX
			break;
		case 0x0033: // Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, 
					 //and the ones digit at location I+2
			break;
		case 0x0055: //Stores from V0 to VX (including VX) 
					 //in memory, starting at address I. The offset from I is increased by 1 for each value written, 
					 //but I itself is left unmodified
			break;
		case 0x0065: //Fills from V0 to VX (including VX) with values from 
					 //memory, starting at address I. The offset from I is increased by 1 for each value read, 
					 //but I itself is left unmodified
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
int main()
{
	srand(time(NULL));
	m_AddressI = 0;
	// the game loads into this address
	m_ProgramCounter = 0x200;
	m_VRegisters.fill(0);
	// working directory is a project directory
	std::fstream file{ "./../games/Soccer.ch8" };

	if (file.is_open())
	{
		file.seekg(0, std::ios::end);
		auto fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		auto memory = (char*)m_Memory.data();
		file.read(memory, fileSize);
		file.close();
	}
	else
	{
		assert(false);
	}
}