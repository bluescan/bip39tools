// Dice2Bip39.cpp
//
// Generate a valid BIP-39 mnemonic phrase with dice.
//
// Copyright (c) 2021, 2023 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

// DEV_AUTO_GENERATE should not be defined when compiling. It is used for testing/dev purposes only.
//#define DEV_AUTO_GENERATE
#include "Version.cmake.h"
#include <iostream>
#include <string>
#include <System/tCmdLine.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#include <Math/tRandom.h>		// Only used to overwrite entropy memory when we're done with it.
#include <System/tTime.h>		// Only used to overwrite entropy memory when we're done with it.
#include "Bip39/Bip39.h"


tCmdLine::tOption ConciseOutput	("Concise output.",	'c',	"concise");
tCmdLine::tOption NormalOutput	("Normal output.",	'n',	"normal");
tCmdLine::tOption VerboseOutput	("Verbose output.",	'v',	"verbose");


namespace Dice2Bip
{
	Bip39::Dictionary::Language QueryUserLanguage();

	//
	// Create Mnemonic Functions.
	//
	void DoCreateMnemonic(Bip39::Dictionary::Language);
	int QueryUserNumWords();

	enum class Method { Auto, Simple, Parallel, Extractor };
	const char* MedthodNames[] = { "Auto", "Simple", "Parallel", "Extractor" };
	Method QueryUserMethod();

	void QueryUserEntropyBits_Simple	(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount);
	void QueryUserEntropyBits_Parallel	(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount);
	void QueryUserEntropyBits_Extractor	(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount);
	#ifdef DEV_AUTO_GENERATE
	void QueryUserEntropyBits_DevGen	(tbit256& entropy, int& numBitsGenerated, int numBitsTotal);
	#endif

	// Returns true if a file was saved.
	bool QueryUserSave(const tList<tStringItem>& words, Bip39::Dictionary::Language);

	int InputInt();				// Returns -1 if couldn't read an integer >= 0.
	int InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal = -1, int* inputCount = nullptr);

	//
	// State.
	//
	uint64 ChConc = tSystem::tChannel_Verbosity0;
	uint64 ChNorm = tSystem::tChannel_Verbosity1;
	uint64 ChVerb = tSystem::tChannel_Verbosity2;
};


int Dice2Bip::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


int Dice2Bip::InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal, int* inputCount)
{
	int val = -1;
	const int maxTries = 100;
	int tries = 0;
	do
	{
		tPrintf("%s", question);
		val = Dice2Bip::InputInt();
		tries++;

		if ((val == -1) && (defaultVal >= 0))
		{
			tPrintf("Using Default %d\n", defaultVal);
			val = defaultVal;
			break;
		}
	}
	while (!inRange(val) && (tries < maxTries));

	if ((tries >= maxTries) && !inRange(val))
	{
		tPrintf("Too many ill-formed inputs. Giving up.\n");
		exit(1);
	}

	if (inputCount)
		(*inputCount)++;

	return val;
}


Bip39::Dictionary::Language Dice2Bip::QueryUserLanguage()
{
	tPrintf("Language?\n");
	for (int l = 0; l < Bip39::Dictionary::GetNumLanguages(); l++)
		tPrintf("%d=%s\n", l, Bip39::Dictionary::GetLanguageName(Bip39::Dictionary::Language(l)).Chars());

	int langInt = Dice2Bip::InputIntRanged("Language [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]: ", [](int l) -> bool { return (l >= 0) && (l <= 9); }, 0);
	Bip39::Dictionary::Language	language = Bip39::Dictionary::Language(langInt);
	tPrintf("Language Set To %s\n", Bip39::Dictionary::GetLanguageName(language).Chars());

	if (language >= Bip39::Dictionary::Language::French)
	{
		tPrintf
		(
			ChVerb | ChNorm,
			"You have chosen a language that has special characters that do not always\n"
			"display correctly in bash, cmd, or powershell. Make sure to use a UTF-8 font\n"
			"such as NSimSun or MS Gothic. In Windows command you will need to run\n"
			"\"chcp 65001\" before running this software. In PowerShell you will need to run\n"
			"\"[Console]::OutputEncoding = [System.Text.Encoding]::UTF8\" before running.\n"
			"In Windows 11 you have the option of setting an OS flag to use UTF-8 that\n"
			"makes the output appear correctly:\n"
			"Settings->Time & Language->Administrative Language Settings->Change Locale\n"
			"Check: Beta: Use Unicode UTF-8 for worldwide language support.\n"
			"\n"
			"In bash just set the font correctly.\n"
			"\n"
			"After completing your dice rolls you will be given the option to save your\n"
			"mnemonic phrase to a file. Many text editors like VS Code read utf-8 very well.\n"
			"Made sure to wipe the file afterwards and only run this SW on an air-gapped\n"
			"machine.\n"
		);
	}

	return language;
}


int Dice2Bip::QueryUserNumWords()
{
	tPrintf(ChNorm | ChVerb, "How many words for your mnemonic phrase?\n");
	int numWords = InputIntRanged
	(
		"Number of Words [12, 15, 18, 21, 24]: ",
		[](int w) -> bool { return (w == 12) || (w == 15) || (w == 18) || (w == 21) || (w == 24); },
		24
	);

	return numWords;
}


Dice2Bip::Method Dice2Bip::QueryUserMethod()
{
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
		"   have no bias, this method generates a maximum of 5 bits for each roll of two\n"
		"   dice. This is because you can treat the two rolls as a two-digit base-6\n"
		"   number. 6^2 is 36 and 32 is the next lower power-of-two so each double-roll\n"
		"   generates 5 bits. When rolling the 2 dice, enter the left-most one first.\n"
		"   With re-rolls expect approximately 58 double-rolls for a 24-word phrase.\n"
		"   You can also use this method with one die rolling it twice in a row.\n"
		"\n"
		"3) Extractor\n"
		"   If you have a low-quality die or a suspected biased die use this bias-\n"
		"   removing method. For the extremely paranoid, this 3rd method will also work\n"
		"   with a balanced die, removing any and all bias. The method is based on\n"
		"   a Von Neumann extractor. You roll the the same die twice in a row. If roll 1\n"
		"   is lower than roll 2, a 0 is generated. If roll 1 is larger than roll 2, a 1\n"
		"   is generated. If equal, re-roll. You can expect approximately 597 individual\n"
		"   rolls to generate a 24-word mnemonic.\n"
	);

	int method = Dice2Bip::InputIntRanged
	(
		#ifdef DEV_AUTO_GENERATE
		"Method 0=Auto 1=Simple 2=Parallel 3=Extractor [0, 1, 2, 3]: ",
		[](int m) -> bool { return (m >= 0) && (m <= 3); }
		#else
		"Method 1=Simple 2=Parallel 3=Extractor [1, 2, 3]: ",
		[](int m) -> bool { return (m >= 1) && (m <= 3); }
		#endif
	);

	return Method(method);
}


void Dice2Bip::QueryUserEntropyBits_Simple(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount)
{
	int roll = 0;
	do
	{
		char rollText[64]; tsPrintf(rollText, "Roll#%03d [1, 2, 3, 4, 5, 6]: ", rollCount);
		roll = Dice2Bip::InputIntRanged
		(
			rollText,
			[](int r) -> bool { return (r >= 0) && (r <= 6); },
			-1, &rollCount
		);
	}
	while (roll >= 5);

	tAssert((numBitsTotal - numBitsGenerated) >= 2);
	int bitIndex = numBitsTotal-numBitsGenerated-1;
	switch (roll)
	{
		case 1:				// 00
			entropy.SetBit(bitIndex-0, false);
			entropy.SetBit(bitIndex-1, false);
			break;
		case 2:				// 01
			entropy.SetBit(bitIndex-0, false);
			entropy.SetBit(bitIndex-1, true);
			break;
		case 3:				// 10
			entropy.SetBit(bitIndex-0, true);
			entropy.SetBit(bitIndex-1, false);
			break;
		case 4:				// 11
			entropy.SetBit(bitIndex-0, true);
			entropy.SetBit(bitIndex-1, true);
			break;
	}
	numBitsGenerated += 2;
}


void Dice2Bip::QueryUserEntropyBits_Parallel(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount)
{
	uint32 base6 = 0;
	do
	{
		char rollLText[64]; tsPrintf(rollLText, "Roll#%03d Left Die  [1, 2, 3, 4, 5, 6]: ", rollCount);
		int rollL = Dice2Bip::InputIntRanged(rollLText, [](int r) -> bool { return (r >= 1) && (r <= 6); });

		char rollRText[64]; tsPrintf(rollRText, "Roll#%03d Right Die [1, 2, 3, 4, 5, 6]: ", rollCount);
		int rollR = Dice2Bip::InputIntRanged(rollRText, [](int r) -> bool { return (r >= 1) && (r <= 6); });

		rollCount++;
		base6 = ((rollL-1)*6) + (rollR-1);
		tPrintf(ChVerb, "Base6 Value: %d\n", base6);
	}
	while (base6 >= 32);

	// Since the number of bits required may not be divisible by 5, make sure we don't go over.
	int bitCount = numBitsTotal-numBitsGenerated;
	int bitIndex = bitCount - 1;

	if (bitCount > 5) bitCount = 5;
	for (int b = 0; b < bitCount; b++)
	{
		bool bit = (base6 & (1 << b)) ? true : false;
		entropy.SetBit(bitIndex-b, bit);
	}

	numBitsGenerated += bitCount;
}


void Dice2Bip::QueryUserEntropyBits_Extractor(tbit256& entropy, int& numBitsGenerated, int numBitsTotal, int& rollCount)
{
	int roll1 = 0;
	int roll2 = 0;

	do
	{
		char roll1Text[64]; tsPrintf(roll1Text, "Roll#%03d [1, 2, 3, 4, 5, 6]: ", rollCount);
		roll1 = Dice2Bip::InputIntRanged(roll1Text, [](int r) -> bool { return (r >= 1) && (r <= 6); }, -1, &rollCount);

		char roll2Text[64]; tsPrintf(roll2Text, "Roll#%03d [1, 2, 3, 4, 5, 6]: ", rollCount);
		roll2 = Dice2Bip::InputIntRanged(roll2Text, [](int r) -> bool { return (r >= 1) && (r <= 6); }, -1, &rollCount);
	}
	while (roll1 == roll2);
	tAssert(roll1 != roll2);

	bool bit = (roll1 < roll2) ? false : true;
	tPrintf(ChVerb, "Generated a %s\n", bit ? "1" : "0");

	int bitIndex = numBitsTotal-numBitsGenerated-1;
	entropy.SetBit(bitIndex-0, bit);

	numBitsGenerated += 1;
}


#ifdef DEV_AUTO_GENERATE
void Dice2Bip::QueryUserEntropyBits_DevGen(tbit256& entropy, int& numBitsGenerated, int numBitsTotal)
{
	tAssert((numBitsTotal - numBitsGenerated) >= 32);
	int bitIndex = numBitsTotal-numBitsGenerated-1;

	uint32 randBits = tMath::tRandom::tGetBits();
	for (int b = 0; b < 32; b++)
	{
		bool bit = (randBits & (1 << b)) ? true : false;
		entropy.SetBit(bitIndex-b, bit);
	}

	numBitsGenerated += 32;
}
#endif


bool Dice2Bip::QueryUserSave(const tList<tStringItem>& words, Bip39::Dictionary::Language language)
{
	bool savedFile = false;

	// Should give option to save if language that doesn't dislay correctly in console chosen.
	if (language >= Bip39::Dictionary::Language::French)
	{
		const char* wordSaveFile = "WordListResult.txt";
		tPrintf
		(
			Dice2Bip::ChVerb | Dice2Bip::ChNorm,
			"Since you chose a language that has special characters, do you want\n"
			"to save it as \"%s\"\n", wordSaveFile
		);

		char saveText[64]; tsPrintf(saveText, "Save to %s? 0=No 1=Yes [0, 1]: ", wordSaveFile);
		int doSave = Dice2Bip::InputIntRanged(saveText, [](int s) -> bool { return (s == 0) || (s == 1); });
		if (doSave == 1)
		{
			tPrintf("Saving words.\n");
			tFileHandle file = tSystem::tOpenFile(wordSaveFile, "wb");
			int wordNum = 1;
			for (tStringItem* word = words.First(); word; word = word->Next(), wordNum++)
				tfPrintf(file, "Word %02d: %s\n", wordNum, word->Chars());
			tSystem::tCloseFile(file);
			savedFile = true;
		}
		else
		{
			tPrintf("Not saving words.\n");
		}
	}

	return savedFile;
}


void Dice2Bip::DoCreateMnemonic(Bip39::Dictionary::Language language)
{
	int numWords = Dice2Bip::QueryUserNumWords();
	tPrintf("A %d-word mnemonic will be created.\n", numWords);

	int numBitsTotal = Bip39::GetNumEntropyBits(numWords);
	tPrintf(Dice2Bip::ChVerb, "Your %d-word mnemonic phrase will contain %d bits of entropy.\n", numWords, numBitsTotal);

	Dice2Bip::Method method = Dice2Bip::QueryUserMethod();
	tPrintf("Using %s method.\n", Dice2Bip::MedthodNames[ int(method) ]);

	tbit256 entropy;
	entropy.Clear();
	int numBitsGenerated = 0;
	int rollCount = 1;

	while (numBitsGenerated < numBitsTotal)
	{
		switch (method)
		{
			#ifdef DEV_AUTO_GENERATE
			case Method::Auto:
				QueryUserEntropyBits_DevGen(entropy, numBitsGenerated, numBitsTotal);
				break;
			#endif
			case Method::Simple:
				QueryUserEntropyBits_Simple(entropy, numBitsGenerated, numBitsTotal, rollCount);
				break;
			case Method::Parallel:
				QueryUserEntropyBits_Parallel(entropy, numBitsGenerated, numBitsTotal, rollCount);
				break;
			case Method::Extractor:
				QueryUserEntropyBits_Extractor(entropy, numBitsGenerated, numBitsTotal, rollCount);
				break;
		}
		tPrintf("Progress: %d of %d bits.\n", numBitsGenerated, numBitsTotal);
		tPrintf(Dice2Bip::ChVerb, "Entropy: %0_256|256b\n", entropy);
	}

	tAssert(numBitsGenerated == numBitsTotal);

	// Just to be fully correct, we check that the entropy is valid for Secp256k1.
	// It is _extremely_ unlikely it will be out of range as the period of the curve
	// is really large... not quite 2^256, but not relatively that far off.
	if (!Bip39::IsValidSecp256k1Range(entropy))
	{
		tPrintf("The generated entropy is larger than the Secp256k1 curve period.\n");
		tPrintf("This is a once in a bazillion-quillion failure.\n");
		tPrintf("You will need to start again.\n");
		tPrintf(ChVerb, "Erasing Memory\n");
		Bip39::ClearEntropy(entropy);
		return;
	}

	tList<tStringItem> words;
	Bip39::ComputeWordsFromEntropy(words, entropy, numBitsTotal, language);

	tPrintf(ChVerb, "Erasing Memory\n");
	Bip39::ClearEntropy(entropy);
	tAssert(numWords == words.GetNumItems());

	// Tell the user the words.
	tPrintf("\n");
	int wordNum = 1;
	for (tStringItem* word = words.First(); word; word = word->Next(), wordNum++)
		tPrintf("Word %02d: %s\n", wordNum, word->Chars());
	tPrintf("\n");

	bool savedFile = Dice2Bip::QueryUserSave(words, language);
	if (savedFile)
		tPrintf("You saved results to a file. If you go again and save it will be overwritten.\n");
}


int main(int argc, char** argv)
{
	// The random number generator is ONLY used to clear the entropy memory so it's not hanging around in RAM.
	tMath::tRandom::DefaultGenerator.SetSeed( uint64(tSystem::tGetHardwareTimerCount()) );

	tCmdLine::tParse(argc, argv);
	tSystem::tChannel channels = tSystem::tChannel_Systems | Dice2Bip::ChNorm;
	if (VerboseOutput)
		channels = tSystem::tChannel_Systems | Dice2Bip::ChVerb;
	else if (NormalOutput)
		channels = tSystem::tChannel_Systems | Dice2Bip::ChNorm;
	else if (ConciseOutput)
		channels = tSystem::tChannel_Systems | Dice2Bip::ChConc;
	tSystem::tSetChannels(channels);

	if (ConciseOutput)
		tPrintf("dice2bip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);
	else
		tCmdLine::tPrintUsage(nullptr, u8"This program generates a valid BIP-39 passphrase using dice.", Version::Major, Version::Minor, Version::Revision);

ChooseLanguage:
	Bip39::Dictionary::Language language = Dice2Bip::QueryUserLanguage();
	Dice2Bip::DoCreateMnemonic(language);

	// Go again?
	int again = Dice2Bip::InputIntRanged("Go Again? 0=No 1=Yes [0, 1]: ", [](int a) -> bool { return (a == 0) || (a == 1); });
	if (again == 1)
		goto ChooseLanguage;

	return 0;
}
