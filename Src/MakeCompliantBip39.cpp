// MakeCompliantBip39.cpp
//
// Takes words you enter, extracts the entropy, and sets the CS bits to be either:
// a) Bip-39 compliant,
// b) All zeros.
// Finally it re-outputs a set of words that have the same entropy, but modified checksum bits.
//
// The first case (a) is useful if you have a mnemonic sentence that has an invalid checksum and you want to use it with
// a wallet that not only checks the CS, but also refuses to use your entropy unless it is Bip-39 compliant. More
// generally this mode takes an invalid sentence and makes it valid.
//
// The second case (b) is useful if you have a wallet that is not Bip-39 compliant and, indeed, requires the mnemonic
// sentence to also be non-compliant by having all the CS bits cleared. The Helium (HNT) mobile Android and iOS
// wallets (as of Oct 19, 2021) are examples of this strange requirement.
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
#include <iostream>
#include <string>
#include <System/tCmdLine.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include "Bip39/Bip39.h"


namespace Comply
{
	Bip39::Dictionary::Language QueryUserLanguage();
	void ComplyMnemonic(Bip39::Dictionary::Language language);
	void ComplyMnemonic(tList<tStringItem>& words, bool clearCS);

	int QueryUserNumWords();
	void QueryUserWords(tList<tStringItem>& words, int numWords, Bip39::Dictionary::Language);

	tString InputString();
	tString InputStringBip39Word(int wordNum, Bip39::Dictionary::Language = Bip39::Dictionary::Language::English);

	int InputInt();				// Returns -1 if couldn't read an integer >= 0.
	int InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal = -1, int* inputCount = nullptr);
};


int Comply::QueryUserNumWords()
{
	tPrintf("How many words do you have?\n");
	int numAvailWords = InputIntRanged
	(
		"Number of Words [12, 15, 18, 21, 24]: ",
		[](int w) -> bool { return (w == 12) || (w == 15) || (w == 18) || (w == 21) || (w == 24); },
		24
	);

	return numAvailWords;
}


void Comply::QueryUserWords(tList<tStringItem>& words, int numWords, Bip39::Dictionary::Language language)
{
	tPrintf("Enter words. You may only enter the first 4 letters if you like.\n");
	for (int w = 0; w < numWords; w++)
	{
		tString fullWord = InputStringBip39Word(w+1, language);
		tPrintf("Entered Word: %s\n", fullWord.Chars());
		if (fullWord.IsEmpty())
		{
			tPrintf("Critical error entering word. Exiting.\n");
			exit(1);
		}
		words.Append(new tStringItem(fullWord));
	}
}


Bip39::Dictionary::Language Comply::QueryUserLanguage()
{
	tPrintf("Language?\n");
	for (int l = 0; l < Bip39::Dictionary::GetNumLanguages(); l++)
		tPrintf("%d=%s\n", l, Bip39::Dictionary::GetLanguageName(Bip39::Dictionary::Language(l)).Chars());

	int langInt = Comply::InputIntRanged("Language [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]: ", [](int l) -> bool { return (l >= 0) && (l <= 9); }, 0);
	Bip39::Dictionary::Language	language = Bip39::Dictionary::Language(langInt);
	tPrintf("Language Set To %s\n", Bip39::Dictionary::GetLanguageName(language).Chars());
	return language;
}


int Comply::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


int Comply::InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal, int* inputCount)
{
	int val = -1;
	const int maxTries = 100;
	int tries = 0;
	do
	{
		tPrintf("%s", question);
		val = Comply::InputInt();
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


tString Comply::InputString()
{
	char str[128];
	str[0] = '\0';
	char line[128];
	std::cin.getline(line, 128);
	int numRead = sscanf(line, "%s", str);
	if (numRead >= 1)
		return tString(str);

	return tString();
}


tString Comply::InputStringBip39Word(int wordNum, Bip39::Dictionary::Language lang)
{
	tAssert(lang == Bip39::Dictionary::Language::English);
	tString word;
	const int maxTries = 100;
	for (int tries = 0; tries < maxTries; tries++)
	{
		tPrintf("Enter Word %d: ", wordNum);
		word = Comply::InputString();

		// Word may not be the full word at this point. It's possible the user only entered the first
		// few digits. If they entered 4, success is guaranteed. GetFullWord results in an empty string
		// if a unique match isn't found, so it can be returned directly.
		word = Bip39::Dictionary::GetFullWord(word, lang);
		if (word.IsEmpty())
			tPrintf("Invalid word. Try again.\n");
		else
			break;
	}

	if (word.IsEmpty())
	{
		tPrintf("Too many attepts. Giving up.\n");
		exit(1);
	}

	return word;
}


void Comply::ComplyMnemonic(Bip39::Dictionary::Language language)
{
	int numWords = QueryUserNumWords();

	tList<tStringItem> words;
	QueryUserWords(words, numWords, language);
	tAssert(words.GetNumItems() == numWords);

	// Ask user if they want the zero mode.
	tPrintf("Clear Checksum? Hitting enter selects NO and outputs a valid Bip-39 mnemonic.\n");
	int zeroCS = Comply::InputIntRanged("0=No 1=Yes [0, 1]: ", [](int a) -> bool { return (a == 0) || (a == 1); }, 0);
	bool clearCS = zeroCS ? true : false;

	tbit256 entropy;
	int numEntropyBits;
	Bip39::GetEntropyFromWords(entropy, numEntropyBits, words, language);

	tbit512 fullBits;
	int numFullBits;
	Bip39::ComputeFullBitsFromEntropy(fullBits, numFullBits, entropy, numEntropyBits, clearCS);

	words.Clear();
	Bip39::ComputeWordsFromFullBits(words, fullBits, numFullBits, language);

	// Tell the user the new words.
	tPrintf("\nNew words are:\n");
	int wordNum = 1;
	for (tStringItem* word = words.First(); word; word = word->Next(), wordNum++)
		tPrintf("Word %02d: %s\n", wordNum, word->Chars());
	tPrintf("\n");
}


void Comply::ComplyMnemonic(tList<tStringItem>& words, bool clearCS)
{
	// Modify words so we have a list of full-words (in case user only entered first 4 letters).
	for (tStringItem* wrd = words.First(); wrd; wrd = wrd->Next())
	{
		tString fullword = Bip39::Dictionary::GetFullWord(*wrd, Bip39::Dictionary::Language::English);
		if (fullword.IsEmpty())
		{
			tPrintf("The word %s is not a valid English BIP-0039 mnemonic word.\n", wrd->Chars());
			tPrintf("The mnemonic phrase is INVALID\n");
			return;
		}

		*wrd = fullword;
	}

	tPrintf("Full words entered:\n");
	int wordNum = 1;
	for (tStringItem* wrd = words.First(); wrd; wrd = wrd->Next())
		tPrintf("Word %2d: %s\n", wordNum++, wrd->Chars());

	tbit256 entropy;
	int numEntropyBits;
	Bip39::GetEntropyFromWords(entropy, numEntropyBits, words, Bip39::Dictionary::Language::English);

	tbit512 fullBits;
	int numFullBits;
	Bip39::ComputeFullBitsFromEntropy(fullBits, numFullBits, entropy, numEntropyBits, clearCS);

	words.Clear();
	Bip39::ComputeWordsFromFullBits(words, fullBits, numFullBits, Bip39::Dictionary::Language::English);

	// Tell the user the new words.
	tPrintf("\nNew words are:\n");
	wordNum = 1;
	for (tStringItem* word = words.First(); word; word = word->Next(), wordNum++)
		tPrintf("Word %02d: %s\n", wordNum, word->Chars());
}


int main(int argc, char** argv)
{
	tPrintf("makecompliantbip39 V%d.%d.%d. Use -h for help.\n", Version::Major, Version::Minor, Version::Revision);
	tSystem::tSetChannels(tSystem::tChannel_Systems | tSystem::tChannel_Verbosity1);

	tCmdLine::tParam wordParams[24];
	tCmdLine::tOption zeroChecksum("Force clear checksum bits.", 'z');
	tCmdLine::tOption help("Display usage.", 'h');
	tCmdLine::tParse(argc, argv);

	if (help)
	{
		tCmdLine::tPrintUsage
		(
			nullptr,
			u8"Takes words you enter, extracts the entropy, and sets the CS bits to be either:\n"
			"a) Bip-39 compliant,\n"
			"b) All zeros.\n"
			"Finally it re-outputs a set of words that have the same entropy, but modified\n"
			"checksum bits.\n"
			"\n"
			"Case (a) is useful if you have a mnemonic sentence that has an invalid checksum\n"
			"and you want to use it with a wallet that not only checks the CS, but also\n"
			"refuses to use your entropy unless it is Bip-39 compliant. Generally this mode\n"
			"takes an invalid mnemonic and makes it valid. This is the default behavior.\n"
			"\n"
			"Case (b) is useful if you have a wallet that is not Bip39 compliant and further\n"
			"requires the mnemonic sentence to also be non-compliant by checking that all\n"
			"the CS bits are cleared. The Helium (HNT) mobile Android and iOS wallets (as of\n"
			"Oct 19, 2021) are examples of this non-standard requirement.\n"
			"\n"
			"You may enter your current words as parameters in the command line. You are\n"
			"required to supply 12, 15, 18, 21, or 24 words. If you do not supply them, an\n"
			"interactive mode is entered requesting them."
		);
		return 0;
	}

	// If the words were entered on the command line we validate them and skip interactive entry.
	// For this use-case currently only English is supported.
	int numWordParams = tCmdLine::tGetNumPresentParameters();
	if (numWordParams > 0)
	{
		if (Bip39::IsValidNumWords(numWordParams))
		{
			tPrintf("Using English words entered on command line.\nOnly first 4 letters of each required.\n");
			tList<tStringItem> words;
			for (int w = 0; w < numWordParams; w++)
				words.Append(new tStringItem(wordParams[w].Param));

			Comply::ComplyMnemonic(words, zeroChecksum);
			return 0;
		}
		else
		{
			tPrintf("You supplied %d words. BIP-0039 requires 12, 15, 18, 21, or 24 words.\n", numWordParams);
			return 2;
		}
	}

ChooseLanguage:
	Bip39::Dictionary::Language language = Comply::QueryUserLanguage();
	Comply::ComplyMnemonic(language);

	// Go again?
	int again = Comply::InputIntRanged("Go Again? 0=No 1=Yes [0, 1]: ", [](int a) -> bool { return (a == 0) || (a == 1); });
	if (again == 1)
		goto ChooseLanguage;

	// Return error code of last checked mnemonic.
	return 0;
}
