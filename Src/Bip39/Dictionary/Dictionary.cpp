// Distionary.cpp
//
// Bip39 dictionary namespace.
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

#include <Foundation/tList.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#include "Dictionary.h"
#include "WordList_english.h"
#include "WordList_czech.h"
#include "WordList_portuguese.h"
#include "WordList_italian.h"
#include "WordList_french.h"
#include "WordList_spanish.h"
#include "WordList_japanese.h"
#include "WordList_korean.h"
#include "WordList_chinese_simplified.h"
#include "WordList_chinese_traditional.h"
namespace Bip39
{


namespace Dictionary
{
	const int NumWords = 2048;			// Defined by BIP0039.
	const char* LanguageNames[]			= { "english", "czech", "portuguese", "italian", "french", "spanish", "japanese", "korean", "chinese_simplified", "chinese_traditional" };
	const int NumLanguages				= sizeof(LanguageNames)/sizeof(*LanguageNames);

	typedef const char* DictionaryWords[2048];
	DictionaryWords* GetDictionary(Language);
	tString GetLanguageName(Language);
}


int Dictionary::GetNumLanguages()
{
	return NumLanguages;
}


Dictionary::DictionaryWords* Dictionary::GetDictionary(Language lang)
{
	switch (lang)
	{
		case Bip39::Dictionary::Language::English:				return &Bip39::Dictionary::WordList_english;
		case Bip39::Dictionary::Language::Czech:				return &Bip39::Dictionary::WordList_czech;
		case Bip39::Dictionary::Language::Portuguese:			return &Bip39::Dictionary::WordList_portuguese;
		case Bip39::Dictionary::Language::Italian:				return &Bip39::Dictionary::WordList_italian;
		case Bip39::Dictionary::Language::French:				return &Bip39::Dictionary::WordList_french;
		case Bip39::Dictionary::Language::Spanish:				return &Bip39::Dictionary::WordList_spanish;
		case Bip39::Dictionary::Language::Japanese:				return &Bip39::Dictionary::WordList_japanese;
		case Bip39::Dictionary::Language::Korean:				return &Bip39::Dictionary::WordList_korean;
		case Bip39::Dictionary::Language::Chinese_Simplified:	return &Bip39::Dictionary::WordList_chinese_simplified;
		case Bip39::Dictionary::Language::Chinese_Traditional:	return &Bip39::Dictionary::WordList_chinese_traditional;
	}

	return nullptr;
}


tString Dictionary::GetLanguageName(Language lang)
{
	return LanguageNames[int(lang)];
}


void Dictionary::GetMatchingWords(tList<tStringItem>& words, const tString& prefix, Language lang)
{
	DictionaryWords* dict = GetDictionary(lang);
	tString prefixLower = prefix;
	prefixLower.ToLower();
	int prefixLen = prefixLower.Length();

	// Slow but don't care.
	for (int w = 0; w < NumWords; w++)
	{
		if (tString((*dict)[w]).Left(prefixLen) == prefixLower)
			words.Append(new tStringItem((*dict)[w]));
	}
}


tString Dictionary::GetFullWord(const tString& prefix, Language lang)
{
	// This function is implemented naively. It's slow but correct. No need to optimize with only 2048 words.
	tString prefixLower = prefix;
	prefixLower.ToLower();

	// We start with finding the words that contain the full prefix, and whittle down from there.
	while (prefixLower.Length() >= 1)
	{
		tList<tStringItem> words;
		GetMatchingWords(words, prefixLower, lang);
		if (words.NumItems() == 1)
			return *words.First();
		else if (words.NumItems() > 1)
		{
			// If there are multiple results we can only return a single result if there is an exact match.
			// This happens in cases like "fat" where it would also match "fatal", "father", and "fatigue".
			// We can use GetBits since it requires an exact match.
			uint32 bits = GetBits(prefixLower, lang);
			if (bits != 0xFFFFFFFF)
				return prefixLower;

			return tString();
		}

		// By whittling down the prefix, it allows typos after the initial uniquely
		// specified characters. For example, "abanzzz" still works.
		prefixLower.ExtractRightN(1);
	}

	return tString();
}


uint32 Dictionary::GetBits(const tString& fullWord, Language lang)
{
	tString lowerWord = fullWord;
	lowerWord.ToLower();
	DictionaryWords* dict = GetDictionary(lang);

	// Slow but don't care.
	for (uint32 w = 0; w < NumWords; w++)
	{
		if (tString((*dict)[w]) == lowerWord)
			return w;
	}

	return 0xFFFFFFFF;
}


tString Dictionary::GetWord(uint32 bits, Language lang)
{
	if (bits >= NumWords)
		return tString();

	DictionaryWords* dict = GetDictionary(lang);
	return (*dict)[bits];
}


void Dictionary::DevGenerateWordListHeaders()
{
	for (int lang = 0; lang < GetNumLanguages(); lang++)
	{
		tString language = GetLanguageName( Language(lang) );
		tString srcFile;
		tsPrintf(srcFile, "../Reference/WordLists/%s.txt", language.Chars());

		tString words;
		tSystem::tLoadFile(srcFile, words, '_');
		words.Replace('\n', '_');

		tString dstFile;
		tsPrintf(dstFile, "../Src/Bip39/Dictionary/WordList_%s.h", language.Chars());
		tFileHandle file = tSystem::tOpenFile(dstFile, "wb");
		if (!file)
		{
			tPrintf("Error generating header for language %s\n", language.Chars());
			continue;
		}

		// Prologue.
		tfPrintf
		(
			file,
			"// Generated by bip39tools\n"
			"#pragma once\n"
			"namespace Bip39 { namespace Dictionary {\n"
			"const char* WordList_%s[] =\n"
			"{\n",
			language.Chars()
		);

		int num = 0;
		do
		{
			tString word = words.ExtractLeftC('_');
			tfPrintf(file, "\t\"%s\"", word.Chars());
			if (num == 2047)
				tfPrintf(file, "\n");
			else
				tfPrintf(file, ",\n");
			num++;
			// tPrintf("Word %d [%s]\n", num, word.Chars());
		}
		while (!words.IsEmpty());
		tAssert(num == 2048);

		// Epilogue.
		tfPrintf(file, "};\n");
		tfPrintf(file, "} }\n");
		tSystem::tCloseFile(file);
	}
}


}
