// BipMain.cpp
//
// Generate a valid BIP-39 mnemonic phrase with dice.
//
// Copyright (c) 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include "Version.cmake.h"
#include "WordListEnglish.h"
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>

// This should NOT be defined when compiling. It is for testing purposes only.
#define DEV_AUTO_GENERATE
#ifdef DEV_AUTO_GENERATE
#include <Math/tRandom.h>
#include <System/tTime.h>
#endif


namespace tBip
{
	bool SelfTest();
	int QueryUserNumWords();
	int ComputeNumEntropyBits(int numWords);

	enum class Method { Invalid, Simple, Parallel, Extractor };
	const char* MedthodNames[] = { "Invalid", "Simple", "Parallel", "Extractor" };
	Method QueryUserMethod();

	void QueryUserEntropyBits_Simple();
	void QueryUserEntropyBits_Parallel();
	void QueryUserEntropyBits_Extractor();
	#ifdef DEV_AUTO_GENERATE
	void QueryUserEntropyBits_DevGen();
	#endif

	#ifdef DEV_GEN_WORDLIST
	void GenerateWordListHeader();
	#endif

	int NumMnemonicWords = 0;
	int NumEntropyBitsNeeded = 0;
	int NumEntropyBitsGenerated = 0;
	tbit256 Entropy;
};


#ifdef DEV_GEN_WORDLIST
void tBip::GenerateWordListHeader()
{
	tString words;
	tSystem::tLoadFile(tString("WordListEnglish.txt"), words, '_');
	words.Replace('\n', '_');
	int num = 1;
	tFileHandle hand = tSystem::tOpenFile("WordListEnglish.h", "wb");
	do
	{
		tString word = words.ExtractLeft('_');
		tfPrintf(hand, "\t\"%s\",\n", word.Chars());
		tPrintf("WORD %d IS [%s]\n", num++, word.Chars());
	}
	while (!words.IsEmpty());
	tSystem::tCloseFile(hand);
}
#endif


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
	tPrintf("How many words do you want in your mnemonic phrase?\n");
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
	tPrintf("What method should be used to generate your phrase?.\n\n");

	#ifdef DEV_AUTO_GENERATE
	tPrintf
	(
		"0) DevAuto\n"
		"   Do not use. For development only.\n"
		"\n"
	);
	#endif

	tPrintf
	(
		"1) Simple\n"
	//   01234567890123456789012345678901234567890123456789012345678901234567890123456789
		"   If you have one Casino-quality 6-sided die that is evenly balanced and has\n"
		"   no bias, this method generates a maximum of 2 bits per roll. Rolls of 5 or 6\n"
		"   are discarded. Expect to roll the die approx 171 times for a 24-word phrase.\n"
		"\n"
		"2) Parallel\n"
		"   If you have two Casino-quality 6-sided dice that are evenly balanced and\n"
		"   have no bias, this tool generates a maximum of 5 bits for each roll of two\n"
		"   dice. This is because you can treat the two rolls as a two-digit base-36\n"
		"   number. From 36, 32 is the next lower power of two so each double-roll\n"
		"   generates 5 bits. When rolling the 2 dice, enter the leftmost one first.\n"
		"   Expect approximately 58 double-rolls for a 24-word phrase.\n"
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
		#ifdef DEV_AUTO_GENERATE
		if ((method != 0) && (method != 1) && (method != 2) && (method != 3))
		#else
		if ((method != 1) && (method != 2) && (method != 3))
		#endif
			tPrintf("Invalid method %d. Try again.\n[1, 2, 3]", method);
		else
			break;
	}

	return Method(method);
}


void tBip::QueryUserEntropyBits_Simple()
{
	int roll = 0;
	while ((roll < 1) || (roll > 4))
	{
		tPrintf("Roll die [1, 2, 3, 4, 5, 6] :");
		scanf("%1d", &roll);
	}

	tAssert((NumEntropyBitsNeeded - NumEntropyBitsGenerated) >= 2);
	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;
	switch (roll)
	{
		case 1:				// 00
			Entropy.SetBit(bitIndex-0, false);
			Entropy.SetBit(bitIndex-1, false);
			break;
		case 2:				// 01
			Entropy.SetBit(bitIndex-0, false);
			Entropy.SetBit(bitIndex-1, true);
			break;
		case 3:				// 10
			Entropy.SetBit(bitIndex-0, true);
			Entropy.SetBit(bitIndex-1, false);
			break;
		case 4:				// 11
			Entropy.SetBit(bitIndex-0, true);
			Entropy.SetBit(bitIndex-1, true);
			break;
	}
	NumEntropyBitsGenerated += 2;
	tPrintf("%0_256|256b\n", Entropy);
}


void tBip::QueryUserEntropyBits_Parallel()
{
	uint32 base36 = 0;
	do
	{
		int roll1 = 0;
		do
		{
			tPrintf("Left die  [1, 2, 3, 4, 5, 6] :");
			scanf("%1d", &roll1);
		}
		while ((roll1 < 1) || (roll1 > 6));

		int roll2 = 0;
		do
		{
			tPrintf("Right die [1, 2, 3, 4, 5, 6] :");
			scanf("%1d", &roll2);
		}
		while ((roll2 < 1) || (roll2 > 6));
		roll1--;
		roll2--;

		base36 = (roll1*6) + roll2;
		tPrintf("Base36 Number: %d\n", base36);
	}
	while (base36 >= 32);

	// Since the number of bits required may not be divisible by 5, make sure we don't go over.
	int bitCount = NumEntropyBitsNeeded-NumEntropyBitsGenerated;
	int bitIndex = bitCount - 1;

	if (bitCount > 5) bitCount = 5;
	for (int b = 0; b < bitCount; b++)
	{
		bool bit = (base36 & (1 << b)) ? true : false;
		Entropy.SetBit(bitIndex-b, bit);
	}

	tPrintf("Generated %d bits\n", bitCount);
	NumEntropyBitsGenerated += bitCount;
	tPrintf("%0_256|256b\n", tBip::Entropy);
}


void tBip::QueryUserEntropyBits_Extractor()
{
	int roll1 = 0;
	int roll2 = 0;

	do 
	{
		do
		{
			tPrintf("Roll die   [1, 2, 3, 4, 5, 6] :");
			scanf("%1d", &roll1);
		}
		while ((roll1 < 1) || (roll1 > 6));

		do
		{
			tPrintf("Roll again [1, 2, 3, 4, 5, 6] :");
			scanf("%1d", &roll2);
		}
		while ((roll2 < 1) || (roll2 > 6));
	}
	while (roll1 == roll2);
	tAssert(roll1 != roll2);
	bool bit = (roll1 < roll2) ? false : true;
	// tPrintf("Generated a %s\n", bit ? "1" : "0");

	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;
	Entropy.SetBit(bitIndex-0, bit);

	NumEntropyBitsGenerated += 1;
}


#ifdef DEV_AUTO_GENERATE
void tBip::QueryUserEntropyBits_DevGen()
{
	tMath::tRandom::DefaultGenerator.SetSeed( uint64(tSystem::tGetHardwareTimerCount()) );
	tAssert((NumEntropyBitsNeeded - NumEntropyBitsGenerated) >= 32);
	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;

	uint32 randBits = tMath::tRandom::tGetBits();
	for (int b = 0; b < 32; b++)
	{
		bool bit = (randBits & (1 << b)) ? true : false;
		Entropy.SetBit(bitIndex-b, bit);
	}

	NumEntropyBitsGenerated += 32;
}
#endif


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
	tPrintf("bip39dice V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

	#ifdef DEV_GEN_WORDLIST
	tBip::GenerateWordListHeader();
	return 0;
	#endif

	int numWords = tBip::QueryUserNumWords();
	if (numWords == 0)
		return tBip::SelfTest() ? 0 : 1;

	tBip::NumEntropyBitsNeeded = tBip::ComputeNumEntropyBits(numWords);
	tPrintf("Your %d-word mnemonic phrase will contain %d bits of entropy.\n", numWords, tBip::NumEntropyBitsNeeded);

	tBip::Method method = tBip::QueryUserMethod();
	tPrintf("The %s method will be used to generate entropy.\n", tBip::MedthodNames[ int(method) ]);

	tBip::NumEntropyBitsGenerated = 0;
	tBip::Entropy.Clear();

/*
	tPrintf("First 10 words:\n");
	for (int w = 0; w < 10; w++)
		tPrintf("Word %d: [%s]\n", w, Bip39WordListEnglish[w]);

	tPrintf("Last 10 words:\n");
	for (int w = 2048-10; w < 2048; w++)
		tPrintf("Word %d: [%s]\n", w, Bip39WordListEnglish[w]);
*/
	
	while (tBip::NumEntropyBitsGenerated < tBip::NumEntropyBitsNeeded)
	{
		switch (method)
		{
			#ifdef DEV_AUTO_GENERATE
			case tBip::Method::Invalid:
				tBip::QueryUserEntropyBits_DevGen();
				break;
			#endif
			case tBip::Method::Simple:
				tBip::QueryUserEntropyBits_Simple();
				break;
			case tBip::Method::Parallel:
				tBip::QueryUserEntropyBits_Parallel();
				break;
			case tBip::Method::Extractor:
				tBip::QueryUserEntropyBits_Extractor();
				break;
		}
		tPrintf("You have generated %d bits of %d required.\n", tBip::NumEntropyBitsGenerated, tBip::NumEntropyBitsNeeded);
	}

	tPrintf("%0_256|256b\n", tBip::Entropy);

	// From BIP-39
	//
	// First, an initial entropy of ENT bits is generated. A checksum is generated by taking the first
	// ENT / 32 bits of its SHA256 hash. This checksum is appended to the end of the initial entropy.
	// Next, these concatenated bits are split into groups of 11 bits, each encoding a number from
	// 0-2047, serving as an index into a wordlist. Finally, we convert these numbers into words and
	// use the joined words as a mnemonic sentence.
	
	return 0;
}
