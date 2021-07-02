#include "Version.cmake.h"
#include <System/tPrint.h>
#include <System/tFile.h>

int main(int argc, char** argv)
{
	tPrintf("Bip39Version %d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

	const char* shaMesg = "abc";
	tuint256 shaComp = tHash::tHashStringSHA256(shaMesg);
	tuint256 shaCorr("BA7816BF 8F01CFEA 414140DE 5DAE2223 B00361A3 96177A9C B410FF61 F20015AD", 16);
	tPrintf
	(
		"Computing SHA-256 of [%s]\n"
		"Computed: %0_64|256X\n"
		"Expected: %0_64|256X\n",
		shaMesg, shaComp, shaCorr
	);

	return 0;
}
