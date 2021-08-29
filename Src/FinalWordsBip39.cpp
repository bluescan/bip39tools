// FinalWordsBip39.cpp
//
// Tool to generate a list of possible last words when you supply all previous words.
// Any of the words may be chosen and results in a valid (correct checksum) BIP-0039 mnemonic.
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
#include <System/tCommand.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#include <Math/tRandom.h>		// Only used to overwrite entropy memory when we're done with it.
#include <System/tTime.h>		// Only used to overwrite entropy memory when we're done with it.
#include "Bip39/Bip39.h"


namespace FinalWords
{
	Bip39::Dictionary::Language QueryUserLanguage();
	void DoFindFinalWords(Bip39::Dictionary::Language);

	//
	// Complete Last Word Functions.
	//
	int QueryUserNumAvailableWords();
	void QueryUserAvailableWords(tList<tStringItem>& words, int numWords);

	// Returns true if a file was saved.
	bool QueryUserSave(const tList<tStringItem>& words, Bip39::Dictionary::Language);

	int InputInt();				// Returns -1 if couldn't read an integer >= 0.
	int InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal = -1, int* inputCount = nullptr);
	tString InputString();
	tString InputStringBip39Word(int wordNum, Bip39::Dictionary::Language = Bip39::Dictionary::Language::English);
};


int FinalWords::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


int FinalWords::InputIntRanged(const char* question, std::function< bool(int) > inRange, int defaultVal, int* inputCount)
{
	int val = -1;
	const int maxTries = 100;
	int tries = 0;
	do
	{
		tPrintf("%s", question);
		val = InputInt();
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


tString FinalWords::InputString()
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


tString FinalWords::InputStringBip39Word(int wordNum, Bip39::Dictionary::Language lang)
{
	tAssert(lang == Bip39::Dictionary::Language::English);
	tString word;
	const int maxTries = 100;
	for (int tries = 0; tries < maxTries; tries++)
	{
		tPrintf("Enter Word %d:", wordNum);
		word = InputString();

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


Bip39::Dictionary::Language FinalWords::QueryUserLanguage()
{
	tPrintf("Language?\n");
	for (int l = 0; l < Bip39::Dictionary::GetNumLanguages(); l++)
		tPrintf("%d=%s\n", l, Bip39::Dictionary::GetLanguageName(Bip39::Dictionary::Language(l)).Chars());

	int langInt = InputIntRanged("Language [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]: ", [](int l) -> bool { return (l >= 0) && (l <= 9); }, 0);
	Bip39::Dictionary::Language	language = Bip39::Dictionary::Language(langInt);
	tPrintf("Language Set To %s\n", Bip39::Dictionary::GetLanguageName(language).Chars());

	if (language >= Bip39::Dictionary::Language::French)
	{
		tPrintf
		(
			"You have chosen a language that has special characters that do not always\n"
			"display correctly in bash, cmd, or powershell. Make sure to use a utf-8 font\n"
			"such as NSimSun or MS Gothic. In Windows command you will need to run\n"
			"\"chcp 65001\" before running this software. In PowerShell you will need to run\n"
			"\"[Console]::OutputEncoding = [System.Text.Encoding]::UTF8\" before running.\n"
			"In bash just set the font correctly.\n"
			"\n"
			"You will be given the option to save your the set of valid last words to a file.\n"
			"Many text editors like VS Code read utf-8 very well.\n"
			"Made sure to wipe the file afterwards and only run this SW on an air-gapped\n"
			"machine.\n"
		);
	}

	return language;
}


bool FinalWords::QueryUserSave(const tList<tStringItem>& words, Bip39::Dictionary::Language language)
{
	bool savedFile = false;

	// Should give option to save if language that doesn't dislay correctly in console chosen.
	if (language >= Bip39::Dictionary::Language::French)
	{
		const char* wordSaveFile = "FinalWordsResult.txt";
		tPrintf
		(
			"Since you chose a language that has special characters, do you want\n"
			"to save it as \"%s\"\n", wordSaveFile
		);

		char saveText[64]; tsPrintf(saveText, "Save to %s? 0=No 1=Yes [0, 1]: ", wordSaveFile);
		int doSave = InputIntRanged(saveText, [](int s) -> bool { return (s == 0) || (s == 1); });
		if (doSave == 1)
		{
			tPrintf("Saving words.\n");
			tFileHandle file = tSystem::tOpenFile(wordSaveFile, "wb");
			int numWords = words.GetNumItems();
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


int FinalWords::QueryUserNumAvailableWords()
{
	tPrintf("How many words do you already have?\n");
	int numAvailWords = InputIntRanged
	(
		"Number of Words [11, 14, 17, 20, 23]: ",
		[](int w) -> bool { return (w == 11) || (w == 14) || (w == 17) || (w == 20) || (w == 23); },
		23
	);

	return numAvailWords;
}


void FinalWords::QueryUserAvailableWords(tList<tStringItem>& words, int numWords)
{
	tPrintf("Enter words. You may only enter the first 4 letters if you like.\n");
	for (int w = 0; w < numWords; w++)
	{
		tString fullWord = InputStringBip39Word(w+1);
		tPrintf("Entered Word: %s\n", fullWord.Chars());
	}
}


void FinalWords::DoFindFinalWords(Bip39::Dictionary::Language language)
{
	// WIP
	if (language != Bip39::Dictionary::Language::English)
	{
		tPrintf("Currently only English supported for complete last word.\n");
		return;
	}

	int numAvailWords = QueryUserNumAvailableWords();

	// Ask user to input the available words.
	tList<tStringItem> words;
	QueryUserAvailableWords(words, numAvailWords);
}


int main(int argc, char** argv)
{
	tPrintf("finalwordsbip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

ChooseLanguage:
	Bip39::Dictionary::Language language = FinalWords::QueryUserLanguage();

	FinalWords::DoFindFinalWords(language);

	// Go again?
	int again = FinalWords::InputIntRanged("Go Again? 0=No 1=Yes [0, 1]: ", [](int a) -> bool { return (a == 0) || (a == 1); });
	if (again == 1)
		goto ChooseLanguage;

	return 0;
}
