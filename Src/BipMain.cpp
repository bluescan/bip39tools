#include "Version.cmake.h"
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>


namespace tBip
{
	bool SelfTest();
	int QueryUserNumWords();
	int ComputeNumEntropyBits(int numWords);

	enum class Method { Invalid, Simple, Parallel, Extractor };
	const char* MedthodNames[] = { "Invalid", "Simple", "Parallel", "Extractor" };
	Method QueryUserMethod();

	int QueryUserEntropyBits_Simple();
	int QueryUserEntropyBits_Parallel();
	int QueryUserEntropyBits_Extractor();

	int NumMnemonicWords = 0;

	int EntropyBitIndex = 255;
	tbit256 Entropy;
};


bool tBip::SelfTest()
{
	tPrintf("Performing Self-Test\n");
	const char* shaMesg = "abc";
	tuint256 shaComp = tHash::tHashStringSHA256(shaMesg);
	tuint256 shaCorr("BA7816BF 8F01CFEA 414140DE 5DAE2223 B00361A3 96177A9C B410FF61 F20015AD", 16);
	tPrintf
	(
		"Computing SHA-256 of [%s]\n"
		"Compute %0_64|256X\n"
		"Correct %0_64|256X\n",
		shaMesg, shaComp, shaCorr
	);
	return (shaComp == shaCorr);
}


int tBip::QueryUserNumWords()
{
	tPrintf("How many words do you want in your mnemonic seed phrase?\n");
	tPrintf("Valid answers are 12, 15, 18, 21, and 24. Enter 0 for self-test.\n");
	tPrintf("Most recovery wallets will accept 12 or 24. Press Enter after answering.\n");
	tPrintf("[0, 12, 15, 18, 21, 24]:");

	int numWords = 0;
	while (1)
	{
		scanf("%2d", &numWords);
		if ((numWords != 0) && (numWords != 12) && (numWords != 15) && (numWords != 18) && (numWords != 21) && (numWords != 24))
			tPrintf("Invalid entry %d. Try again.\n[0, 12, 15, 18, 21, 24]", numWords);
		else
			break;
	}
	return numWords;
}


tBip::Method tBip::QueryUserMethod()
{
	tPrintf("What method should be used to generate your seed phrase?.\n\n");
	tPrintf
	(
		"1) Simple\n"
	//   01234567890123456789012345678901234567890123456789012345678901234567890123456789
		"   If you have one Casino-quality 6-sided die that is evenly balanced and has\n"
		"   no bias, this method generates a maximum of 2 bits per roll. Rolls of 5 or 6\n"
		"   are discarded. Expect to roll the die approx 171 times for a 24-word seed.\n"
		"\n"
		"2) Parallel\n"
		"   If you have two Casino-quality 6-sided dice that are evenly balanced and\n"
		"   have no bias, this tool generates a maximum of 5 bits for each roll of two\n"
		"   dice. This is because you can treat the two rolls as a two-digit base-36\n"
		"   number. From 36, 32 is the next lower power of two so each double-roll\n"
		"   generates 5 bits. Expect approximately 58 double-rolls for a 24-word seed.\n"
		"\n"
		"3) Extractor\n"
		"   If you have a low-quality die or a suspected biased die use this bias-\n"
		"   removing method. For the extremely paranoid, this 3rd method will also work\n"
		"   with a good balanced die, removing any and all bias. The method is based on\n"
		"   a Von Neumann extractor. You roll the the SAME die twice in a row. If roll 1\n"
		"   is lower than roll 2, a 0 is generated. If roll 1 is larger than roll 2, a 1\n"
		"   is generated. If equal, re-roll. You can expect approximately 597 individual\n"
		"   rolls to generate a 24-word mnemonic.\n"
	);
	tPrintf("[1, 2, 3]:");
	int method = 0;
	while (1)
	{
		scanf("%1d", &method);
		if ((method != 1) && (method != 2) && (method != 3))
			tPrintf("Invalid method %d. Try again.\n[1, 2, 3]", method);
		else
			break;
	}

	return Method(method);
}


int tBip::QueryUserEntropyBits_Simple()
{
	int roll = 0;
	while ((roll < 1) || (roll > 4))
	{
		tPrintf("Roll die. Enter result. [1, 2, 3, 4, 5, 6]");
		scanf("%1d", &roll);
	}
	switch (roll)
	{
		case 1:				// 00
			Entropy.SetBit(EntropyBitIndex, false); EntropyBitIndex--;
			Entropy.SetBit(EntropyBitIndex, false); EntropyBitIndex--;
			break;
		case 2:				// 01
			Entropy.SetBit(EntropyBitIndex, false); EntropyBitIndex--;
			Entropy.SetBit(EntropyBitIndex, true); EntropyBitIndex--;
			break;
		case 3:				// 10
			Entropy.SetBit(EntropyBitIndex, true); EntropyBitIndex--;
			Entropy.SetBit(EntropyBitIndex, false); EntropyBitIndex--;
			break;
		case 4:				// 11
			Entropy.SetBit(EntropyBitIndex, true); EntropyBitIndex--;
			Entropy.SetBit(EntropyBitIndex, true); EntropyBitIndex--;
			break;
	}
	tPrintf("%0256|256b\n", Entropy);
	return 2;
}


int tBip::QueryUserEntropyBits_Parallel()
{
	//	int rolls[2];
	//	scanf("%1d%1d", &rolls[0], &rolls[1]);
	//	tPrintf("roll1 %d  roll2 %d\n", rolls[0], rolls[1]);
	return 5;
}


int tBip::QueryUserEntropyBits_Extractor()
{
	return 1;
}


int tBip::ComputeNumEntropyBits(int numWords)
{
	switch (numWords)
	{
		case 12:	return 128;
		case 15:	return 160;
		case 18:	return 192;
		case 21:	return 224;
		case 24:	return 256;
	}
	return 0;
}


int main(int argc, char** argv)
{
	tPrintf("Bip39Version %d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

	int numWords = tBip::QueryUserNumWords();
	if (numWords == 0)
		return tBip::SelfTest() ? 0 : 1;

	int numEntropyBitsNeeded = tBip::ComputeNumEntropyBits(numWords);
	tPrintf("Your %d-word mnemonic seed phrase will contain %d bits of entropy.\n", numWords, numEntropyBitsNeeded);

	tBip::Method method = tBip::QueryUserMethod();
	tPrintf("The %s method will be used to generate entropy.\n", tBip::MedthodNames[ int(method) ]);

	int numEntropyBitsGenerated = 0;
	tBip::Entropy.Clear();
	tBip::EntropyBitIndex = 255;

	while (numEntropyBitsGenerated < numEntropyBitsNeeded)
	{
		switch (method)
		{
			case tBip::Method::Simple:
				numEntropyBitsGenerated += tBip::QueryUserEntropyBits_Simple();
				break;
			case tBip::Method::Parallel:
				numEntropyBitsGenerated += tBip::QueryUserEntropyBits_Parallel();
				break;
			case tBip::Method::Extractor:
				numEntropyBitsGenerated += tBip::QueryUserEntropyBits_Extractor();
				break;
		}
		tPrintf("You have generated %d bits of %d required.\n", numEntropyBitsGenerated, numEntropyBitsNeeded);
	}
	
	return 0;
}
