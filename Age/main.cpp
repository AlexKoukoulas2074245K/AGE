#include "common.h"
#include "memory.h"
#include "cpu.h"

#include <iostream>
#include <fstream>
#include <vector>

void loadRom(const char* const arg, Memory& memory)
{
	std::ifstream file;
	file.open(arg, std::ios::binary|std::ios::ate);
	if (file.is_open())
	{
		std::ifstream::pos_type pos = file.tellg();
		std::vector<char> programData((int)pos);

		file.seekg(0, std::ios::beg);
		file.read(&programData[0], pos);
		
		memory.fillRom(programData);
	}
}

int main(int argc, char* argv[])
{
	Memory memory;
	Cpu cpu(memory);

	memory.setPcRef(cpu.getPC());

	loadRom(argv[1], memory);
	std::getchar();

	return 0;
}