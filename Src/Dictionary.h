// Distionary.h
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

#pragma once


typedef const char*			WordListType[2048];


namespace Bip39
{


namespace Dictionary
{
	extern WordListType WordList_english;
	extern WordListType WordList_czech;
	extern WordListType WordList_portuguese;
	extern WordListType WordList_italian;
	extern WordListType WordList_french;
	extern WordListType WordList_spanish;
	extern WordListType WordList_japanese;
	extern WordListType WordList_korean;
	extern WordListType WordList_chinese_simplified;
	extern WordListType WordList_chinese_traditional;

	extern const char* Languages[];//		= { "english", "czech", "portuguese", "italian", "french", "spanish", "japanese", "korean", "chinese_simplified", "chinese_traditional" };
	enum class Language       	  {  English,   Czech,   Portuguese,   Italian,   French,   Spanish,   Japanese,   Korean,   Chinese_Simplified,   Chinese_Traditional  };
	extern const int NumLanguages;//		= sizeof(Languages)/sizeof(*Languages);
}


}
