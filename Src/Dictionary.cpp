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

	void GetAllWordsWithPrefix(tList<tStringItem>& words, const tString& prefix, Language);
}


DictionaryWords* Dictionary::GetDictionary(Language lang)
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


void Dictionary::GetAllWordsWithPrefix(tList<tStringItem>& words, const tString& prefix, Language lang)
{
	DictionaryWords* dict = GetDictionary(lang);
	int prefixLen = prefix.Length();

	// Slow but don't care.
	for (int w = 0; w < NumWords; w++)
	{
		if (tString((*dict)[w]).Left(prefixLen) == prefix)
			words.Append(new tStringItem((*dict)[w]));
	}

	tPrintf("GetAllWithPrefix %s\n", prefix.Chars());
	for (tStringItem* wrd = words.First(); wrd; wrd = wrd->Next())
		tPrintf("Matching Word: %s\n", wrd->Chars());
	tPrintf("\n\n");
}


tString Dictionary::GetFullWord(const tString& pre)
{
	// This function is implemented naively. It's slow but correct. No need to optimize with only 2048 words.
	//int numChars = prefix.Length();
	tString prefix = pre;

	// We start with finding the words that contain the full prefix, and whittle down from there.
	while (prefix.Length() >=1)
	{
		tList<tStringItem> words;
		GetAllWordsWithPrefix(words, prefix, Language::English);
		if (words.NumItems() == 1)
			return *words.First();
		else if (words.NumItems() > 1)
			return tString();

		prefix.ExtractRight(1);
	}

	return tString();
}


}
