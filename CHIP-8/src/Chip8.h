#include <cstdint>;
#include <array>;
#include <string>;

const int SCREEN_W = 64;
const int SCREEN_H = 32;
using Byte = uint8_t;
using Word = uint16_t;
constexpr static int ScreenDataSize = 64 * 32;
class Chip8
{
private:
	
public:
	void Init();
	void Emulate(std::array<unsigned char, 16>& keys);
	void LoadROM(const std::string& path);
	bool IsUpdateScreen() { return m_UpdateScreen; }
	void SetUpdateScreen(bool updateScreen) { m_UpdateScreen = updateScreen; }
	std::array<Byte, ScreenDataSize>& GetScreen() { return m_ScreenData; }
	std::array<bool, ScreenDataSize>& GetUpdated() { return m_Updated; }
private:
	void ExecuteOpCode(Word opCode, std::array<unsigned char, 16>& keys);
	Word GetOpCode(Byte a, Byte b);
	Byte Random();
private:
	using Byte = uint8_t;
	using Word = uint16_t;
	Byte delayTimer = 0;
	Byte soundTimer = 0;
	static constexpr size_t MEMORY_SIZE = 4096;
	std::array<Byte, MEMORY_SIZE> m_Memory;
	std::array<Byte, 16> m_VRegisters;
	// must be 12 bits register
	Word m_AddressI = 0;
	Word m_ProgramCounter; // the 16-bit program counter
	std::array<Word, 16> m_Stack; // to save programm coutner value if subroutine is called
	int m_StackPtr = 0;
	bool m_UpdateScreen = true;
	unsigned char Fontset[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
		0x20, 0x60, 0x20, 0x20, 0x70, // 1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
		0x90, 0x90, 0xF0, 0x10, 0x10, // 4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
		0xF0, 0x10, 0x20, 0x40, 0x40, // 7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
		0xF0, 0x90, 0xF0, 0x90, 0x90, // A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
		0xF0, 0x80, 0x80, 0x80, 0xF0, // C
		0xE0, 0x90, 0x90, 0x90, 0xE0, // D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
		0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	
	std::array<Byte, ScreenDataSize> m_ScreenData;
	std::array<bool, ScreenDataSize> m_Updated;
};
