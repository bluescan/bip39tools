#include "Version.cmake.h"
#include <System/tPrint.h>
#include <System/tFile.h>

int main(int argc, char** argv)
{
	tuint256 thehash = tHash::tHashData256((uint8*)"hello", 5);
	tPrintf("Bip39Version %d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);
	tPrintf("256 Bit Hash of Hello: \n", Version::Major, Version::Minor, Version::Revision);
	tPrintf("[%0_256|256b]\n", thehash);

	uint64 u64 = 0x170F1234B8F0B8F0LL;
	tPrintf("Binary [0001 0111 0000 1111 0001 0010 0011 0100 1011 1000 1111 0000 1011 1000 1111 0000 (64 bit):\n");
	tPrintf("       [%0_64|64b]\n", u64);

	return 0;
}
