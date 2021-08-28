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
#include <Foundation/tString.h>



typedef const char*			DictionaryWords[2048];


namespace Bip39
{


namespace Dictionary
{
	extern const char* LanguageNames[];
	enum class Language { English, Czech, Portuguese, Italian, French, Spanish, Japanese, Korean, Chinese_Simplified, Chinese_Traditional };
	extern const int NumLanguages;

	DictionaryWords* GetDictionary(Language);

	// Finds the full unique word (if it exists) given just a partial word prefix. If it can't find a unique
	// match it returns an empty string. In English Bip39 words are uniquely identifiable with the first 4
	// letters. Ex. If you passed "abou" to this function it would return "about". Since "above" is also in
	// the list, if you passed "abo" it would return an empty string since there are 2 possibilites. Further,
	// if you passed "abouz", it would return the correct "about" as it assumes the "z" is a typo. That is,
	// it returns as soon as a unique match is made as the prexix is culled one character at a time. Note
	// that the '4' is not hardcoded into any of the logic for this function. Lastly, if you enter the full
	// exact word in the first place, it still works and returns it.
	tString GetFullWord(const tString& prefix);

	// This returns the 11 bits associated with the supplied word in the given language. 
	//uint32 GetBits(
}


}
