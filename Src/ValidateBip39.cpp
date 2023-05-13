// ValidateBip39.cpp
//
// Test a supplied word list to see if it's valid and the checksum is correct.
// The language is determined by inspection.
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

#include "Version.cmake.h"
#include <iostream>
#include <string>
#include <System/tCmdLine.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include "Bip39/Bip39.h"


namespace Validate
{
	Bip39::Dictionary::Language QueryUserLanguage();
	bool CheckMnemonic(Bip39::Dictionary::Language language);
	bool CheckMnemonic(tList<tStringItem>& words);

	int QueryUserNumWords();
	void QueryUserWords(tList<tStringItem>& words, int numWords, Bip39::Dictionary::Language);

	tString InputString();
	tString InputStringBip39Word(int wordNum, Bip39::Dictionary::Language = Bip39::Dictionary::Language::English);

	int InputInt();				// Returns -1 if couldn't read an integer >= 0.
	int InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal = -1, int* inputCount = nullptr);
};


int Validate::QueryUserNumWords()
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


void Validate::QueryUserWords(tList<tStringItem>& words, int numWords, Bip39::Dictionary::Language language)
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


Bip39::Dictionary::Language Validate::QueryUserLanguage()
{
	tPrintf("Language?\n");
	for (int l = 0; l < Bip39::Dictionary::GetNumLanguages(); l++)
		tPrintf("%d=%s\n", l, Bip39::Dictionary::GetLanguageName(Bip39::Dictionary::Language(l)).Chars());

	int langInt = Validate::InputIntRanged("Language [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]: ", [](int l) -> bool { return (l >= 0) && (l <= 9); }, 0);
	Bip39::Dictionary::Language	language = Bip39::Dictionary::Language(langInt);
	tPrintf("Language Set To %s\n", Bip39::Dictionary::GetLanguageName(language).Chars());
	return language;
}


int Validate::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


int Validate::InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal, int* inputCount)
{
	int val = -1;
	const int maxTries = 100;
	int tries = 0;
	do
	{
		tPrintf("%s", question);
		val = Validate::InputInt();
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


tString Validate::InputString()
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


tString Validate::InputStringBip39Word(int wordNum, Bip39::Dictionary::Language lang)
{
	tAssert(lang == Bip39::Dictionary::Language::English);
	tString word;
	const int maxTries = 100;
	for (int tries = 0; tries < maxTries; tries++)
	{
		tPrintf("Enter Word %d: ", wordNum);
		word = Validate::InputString();

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


bool Validate::CheckMnemonic(Bip39::Dictionary::Language language)
{
	int numWords = QueryUserNumWords();

	tList<tStringItem> words;
	QueryUserWords(words, numWords, language);
	tAssert(words.GetNumItems() == numWords);

	Bip39::ValidateResult result	= Bip39::ValidateMnemonic(words, language, true);
	const char* resultStr			= Bip39::GetValidateResultString(result);
	tPrintf("Validation result: %s\n", resultStr);

	bool valid = (result == Bip39::ValidateResult::Valid);
	tPrintf("The mnemonic phrase is %s\n", valid ? "VALID" : "INVALID");
	return valid;
}


bool Validate::CheckMnemonic(tList<tStringItem>& words)
{
	// Modify words so we have a list of full-words (in case user only entered first 4 letters).
	for (tStringItem* wrd = words.First(); wrd; wrd = wrd->Next())
	{
		tString fullword = Bip39::Dictionary::GetFullWord(*wrd, Bip39::Dictionary::Language::English);
		if (fullword.IsEmpty())
		{
			tPrintf("The word %s is not a valid English BIP-0039 mnemonic word.\n", wrd->Chars());
			tPrintf("The mnemonic phrase is INVALID\n");
			return false;
		}

		*wrd = fullword;
	}

	tPrintf("Checking full words:\n");
	int wordNum = 1;
	for (tStringItem* wrd = words.First(); wrd; wrd = wrd->Next())
		tPrintf("Word %2d: %s\n", wordNum++, wrd->Chars());

	Bip39::ValidateResult result	= Bip39::ValidateMnemonic(words, Bip39::Dictionary::Language::English, true);
	const char* resultStr			= Bip39::GetValidateResultString(result);
	tPrintf("Validation result: %s\n", resultStr);

	bool valid = (result == Bip39::ValidateResult::Valid);
	tPrintf("The mnemonic phrase is %s\n", valid ? "VALID" : "INVALID");
	return valid;
}


int main(int argc, char** argv)
{
	tPrintf("validatebip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

	// The 0 means populate wordParams with all command line parame.
	tCmdLine::tParam wordParams(0);
	tCmdLine::tParse(argc, argv);

	// If the words were entered on the command line we validate them and skip interactive entry.
	// For this use-case currently only English is supported.
	int numWordParams = wordParams.Values.GetNumItems();
	if (numWordParams > 0)
	{
		if (Bip39::IsValidNumWords(numWordParams))
		{
			tPrintf("Checking English words entered on command line.\nOnly first 4 letters of each required.\n");
			tList<tStringItem> words;
			for (tStringItem* word = wordParams.Values.First(); word; word = word->Next())
				words.Append(new tStringItem(*word));

			bool valid = Validate::CheckMnemonic(words);
			return valid ? 0 : 1;
		}
		else
		{
			tPrintf("You supplied %d words. BIP-0039 requires 12, 15, 18, 21, or 24 words.\n", numWordParams);
			return 2;
		}
	}

	bool validated = false;

ChooseLanguage:
	Bip39::Dictionary::Language language = Validate::QueryUserLanguage();
	validated = Validate::CheckMnemonic(language);

	// Go again?
	int again = Validate::InputIntRanged("Go Again? 0=No 1=Yes [0, 1]: ", [](int a) -> bool { return (a == 0) || (a == 1); });
	if (again == 1)
		goto ChooseLanguage;

	// Return error code of last checked mnemonic.
	return validated ? 0 : 1;
}
