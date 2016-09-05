#include "common.h"
#include "memory.h"
#include "cpu.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

#define CURR_ADDRESS_TO_BREAK 0x0055

static const char* DEBUG_FLAG = "-d";

void loadRom(const char* const arg, Memory& memory)
{
	std::ifstream file;
	file.open(arg, std::ios::binary|std::ios::ate);
	if (file.is_open())
	{
		std::ifstream::pos_type pos = file.tellg();
		std::vector<char> programData(static_cast<int>(pos));

		file.seekg(0, std::ios::beg);
		file.read(&programData[0], pos);
		
		memory.fillRom(programData);
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: <rom_file_path> [-d] (debug mode)" << std::endl;
		return -1;
	}

	bool debugFlag = false;
	if (argc > 2)
	{
		for (size_t i = 2; i < static_cast<size_t>(argc); ++i)
		{	
			debugFlag = strcmp(argv[i], DEBUG_FLAG) == 0;
		}
	}

	Memory memory;
	Cpu cpu(memory);

	memory.setPcRef(cpu.getPC());
	loadRom(argv[1], memory);
	
	for (;;)
	{
		if (*cpu.getPC() >= CURR_ADDRESS_TO_BREAK)
		{
			const auto b = false;
		}
		
		if (*cpu.getPC() == CURR_ADDRESS_TO_BREAK)
		{
			const auto c = false;
		}
		cpu.emulateCycle();

#if defined (_DEBUG) || defined (DEBUG)
		if (debugFlag && *cpu.getPC() >= CURR_ADDRESS_TO_BREAK)
		{
			if (*cpu.getPC() < 0x0095 || *cpu.getPC() > 0x00A7)
			cpu.printRegisters();
		}
#endif
	}
	
	std::getchar();
	return 0;
}