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

// These two should not be defined when compiling. They are for testing/dev purposes only.
// #define DEV_AUTO_GENERATE
// #define DEV_GEN_WORDLIST
#include "Version.cmake.h"
#include <iostream>
#include <string>
#include <System/tCommand.h>
#include "WordListEnglish.h"
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#ifdef DEV_AUTO_GENERATE
#include <Math/tRandom.h>
#include <System/tTime.h>
#endif
tCommand::tOption ConciseOutput	("Concise output.",	'c',	"concise");
tCommand::tOption NormalOutput	("Normal output.",	'n',	"normal");
tCommand::tOption VerboseOutput	("Verbose output.",	'v',	"verbose");


namespace tBip
{
	bool SelfTest();
	int QueryUserNumWords();
	int GetNumEntropyBits(int numWords);

	enum class Method { Auto, Simple, Parallel, Extractor };
	const char* MedthodNames[] = { "Auto", "Simple", "Parallel", "Extractor" };
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
	int InputInt();				// Returns -1 if couldn't read an integer >= 0.

	uint64 ChConc = tSystem::tChannel_Verbosity0;
	uint64 ChNorm = tSystem::tChannel_Verbosity1;
	uint64 ChVerb = tSystem::tChannel_Verbosity2;

	int NumMnemonicWords		= 0;
	int NumEntropyBitsNeeded	= 0;
	int NumEntropyBitsGenerated	= 0;
	int RollCount				= 1;
	tbit256 Entropy;
};


int tBip::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


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
	tPrintf(ChNorm | ChVerb, "How many words do you want in your mnemonic phrase?\n");
	tPrintf(ChNorm | ChVerb, "Valid answers are 0, 12, 15, 18, 21, and 24. Enter 0 for self-test.\n");
	tPrintf(ChNorm | ChVerb, "Most recovery wallets will accept 12 or 24. Press Enter after answering.\n");

	const char* wordOpts = "[0, 12, 15, 18, 21, 24]: ";

	int numWords = -1;
	do
	{
		tPrintf("Number of Words %s", wordOpts);
		numWords = InputInt();
	}
	while ((numWords != 0) && (numWords != 12) && (numWords != 15) && (numWords != 18) && (numWords != 21) && (numWords != 24));
	return numWords;
}


tBip::Method tBip::QueryUserMethod()
{
	#ifdef DEV_AUTO_GENERATE
	const char* methodOpts = "[0, 1, 2, 3]: ";
	#else
	const char* methodOpts = "[1, 2, 3]: ";
	#endif

	tPrintf(ChVerb | ChNorm, "What method should be used to generate your phrase?\n\n");
	#ifdef DEV_AUTO_GENERATE
	tPrintf(ChVerb | ChNorm, "0) DevAuto\n   Do not use. For development only.\n\n");
	#endif

	tPrintf
	(
		ChVerb | ChNorm,
		"1) Simple\n"
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

	int method = -1;
	do
	{
		#ifdef DEV_AUTO_GENERATE
		tPrintf("Method: 0=Auto 1=Simple 2=Parallel 3=Extractor %s", methodOpts);
		#else
		tPrintf("Method: 1=Simple 2=Parallel 3=Extractor %s", methodOpts);
		#endif

		method = InputInt();
	}
	#ifdef DEV_AUTO_GENERATE
	while ((method != 0) && (method != 1) && (method != 2) && (method != 3));
	#else
	while ((method != 1) && (method != 2) && (method != 3));
	#endif

	return Method(method);
}


void tBip::QueryUserEntropyBits_Simple()
{
	int roll = 0;
	do
	{
		tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
		roll = InputInt();
		RollCount++;
	}
	while ((roll < 1) || (roll > 4));

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
}


void tBip::QueryUserEntropyBits_Parallel()
{
	uint32 base36 = 0;
	do
	{
		int roll1 = 0;
		do
		{
			tPrintf("Roll#%03d Left die  [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll1 = InputInt();
		}
		while ((roll1 < 1) || (roll1 > 6));

		int roll2 = 0;
		do
		{
			tPrintf("Roll#%03d Right die [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll2 = InputInt();
		}
		while ((roll2 < 1) || (roll2 > 6));
		RollCount++;		
		roll1--;
		roll2--;

		base36 = (roll1*6) + roll2;
		tPrintf(ChVerb, "Base36 Number: %d\n", base36);
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

	NumEntropyBitsGenerated += bitCount;
}


void tBip::QueryUserEntropyBits_Extractor()
{
	int roll1 = 0;
	int roll2 = 0;

	do 
	{
		do
		{
			tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll1 = InputInt();
			RollCount++;
		}
		while ((roll1 < 1) || (roll1 > 6));

		do
		{
			tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll2 = InputInt();
			RollCount++;
		}
		while ((roll2 < 1) || (roll2 > 6));
	}
	while (roll1 == roll2);
	tAssert(roll1 != roll2);
	bool bit = (roll1 < roll2) ? false : true;
	tPrintf(ChVerb, "Generated a %s\n", bit ? "1" : "0");

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


int tBip::GetNumEntropyBits(int numWords)
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
	// Some utf-8 tests.
	// This works so long as /utf-8 is specified in the compiler options. That flag makes source files be interpreted as utf-8 as well as compiling output string literals to utf-8.
	//	FILE* ff = fopen("testutf8.txt", "wb");
	//	fprintf(ff, u8"UTF8STR: 的一是在不了有\n");
	//	fclose(ff);
	//	return 0;

	tCommand::tParse(argc, argv);
	tSystem::tSetChannels(tSystem::tChannel_Systems | tBip::ChNorm);
	if (VerboseOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | tBip::ChVerb);
	else if (NormalOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | tBip::ChNorm);
	else if (ConciseOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | tBip::ChConc);

	if (ConciseOutput)
		tPrintf("dice2bip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);
	else
		tCommand::tPrintUsage(nullptr, "This program generates a valid BIP-39 passphrase using dice.", Version::Major, Version::Minor, Version::Revision);

	#ifdef DEV_GEN_WORDLIST
	tBip::GenerateWordListHeader();
	return 0;
	#endif

	int numWords = tBip::QueryUserNumWords();
	if (numWords == 0)
		return tBip::SelfTest() ? 0 : 1;
	tPrintf("A %d-word mnemonic will be created.\n", numWords);

	tBip::NumEntropyBitsNeeded = tBip::GetNumEntropyBits(numWords);
	tPrintf(tBip::ChVerb, "Your %d-word mnemonic phrase will contain %d bits of entropy.\n", numWords, tBip::NumEntropyBitsNeeded);

	tBip::Method method = tBip::QueryUserMethod();
	tPrintf("Using %s method.\n", tBip::MedthodNames[ int(method) ]);

	tBip::NumEntropyBitsGenerated = 0;
	tBip::Entropy.Clear();

	while (tBip::NumEntropyBitsGenerated < tBip::NumEntropyBitsNeeded)
	{
		switch (method)
		{
			#ifdef DEV_AUTO_GENERATE
			case tBip::Method::Auto:
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
		tPrintf("Progress: %d of %d bits.\n", tBip::NumEntropyBitsGenerated, tBip::NumEntropyBitsNeeded);
		tPrintf(tBip::ChVerb, "Entropy: %0_256|256b\n", tBip::Entropy);
	}

	tAssert(tBip::NumEntropyBitsGenerated == tBip::NumEntropyBitsNeeded);

	// From BIP-39
	//
	// First, an initial entropy of ENT bits is generated. A checksum is generated by taking the first
	// ENT / 32 bits of its SHA256 hash. This checksum is appended to the end of the initial entropy.
	// Next, these concatenated bits are split into groups of 11 bits, each encoding a number from
	// 0-2047, serving as an index into a wordlist. Finally, we convert these numbers into words and
	// use the joined words as a mnemonic sentence.
	//
	// Compute the SHA-256 hash of the entropy.
	uint8 entropyByteArray[32];
	int numEntropyBytes = tBip::NumEntropyBitsGenerated/8;
	for (int b = 0; b < numEntropyBytes; b++)
		entropyByteArray[b] = tBip::Entropy.GetByte(numEntropyBytes - b - 1);
	tuint256 sha256 = tHash::tHashDataSHA256(entropyByteArray, numEntropyBytes);
	tPrintf(tBip::ChVerb | tBip::ChNorm, "SHA256: %0_64|256X\n", sha256);

	// How many of the first bits do we need?
	int numHashBitsNeeded = tBip::NumEntropyBitsNeeded / 32;
	uint8 firstBits = sha256.GetByte(31);
	firstBits >>= (8-numHashBitsNeeded);
	tPrintf(tBip::ChVerb, "The first %d bits of the sha are: %08b\n", numHashBitsNeeded, firstBits);

	// We now need to store the entropy and the first bits of the sha in a single variable. We make one
	// big enough for the 24-word case: 264 bits. Just for efficiency, we'll use 288, since internally
	// the fixed int class uses 32-bit ints to store their value (and 288 is divisible by 32).
	// Actually, we'll just use 512, as the tPrintf supports that size.
	tuint512 entropyAndChecksum;
	entropyAndChecksum.MakeZero();
	for (int r = 0; r < tBip::Entropy.GetNumElements(); r++)
		entropyAndChecksum.RawElement(r) = tBip::Entropy.GetElement(r);

	entropyAndChecksum <<= numHashBitsNeeded;
	entropyAndChecksum |= firstBits;
	tPrintf(tBip::ChVerb, "EntropyAndChecksum\n");
	tPrintf(tBip::ChVerb, "%0_512|512b\n", entropyAndChecksum);

	// Next we make an array for our word indices. We will be filling it in backwards to
	// avoid extra shift operations. We just shift by 11 each time.
	int wordIndices[24];
	for (int w = 0; w < numWords; w++)
	{
		int wordIndex = entropyAndChecksum & tuint512(0x000007FF);
		tAssert((wordIndex >= 0) && (wordIndex < 2048));
		wordIndices[w] = wordIndex;
		entropyAndChecksum >>= 11;
	}

	// And finally we print out the words in the correct order.
	for (int w = numWords-1; w >= 0; w--)
	{
		int wordIndex = wordIndices[w];
		tPrintf(tBip::ChVerb, "Word %02d with index %04d is: %s\n", numWords - w, wordIndex, Bip39WordListEnglish[wordIndex]);
		tPrintf(tBip::ChNorm | tBip::ChConc, "Word %02d: %s\n", numWords - w, Bip39WordListEnglish[wordIndex]);
	}

	// Before exiting let's clear the entropy variables.
	// @todo Make sure this can't get optimized away.
	tPrintf(tBip::ChVerb, "Erasing Memory\n");
	tBip::Entropy.Clear();
	for (int w = 0; w < numWords; w++)
		wordIndices[w] = -1;
	for (int b = 0; b < 32; b++)
		entropyByteArray[b] = 0;
	entropyAndChecksum.MakeZero();
	
	return 0;
}
