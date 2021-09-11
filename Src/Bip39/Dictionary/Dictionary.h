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


namespace Bip39
{


namespace Dictionary
{
	// Do not modify this enum without also updating the EnumNames array in Dictionary.cpp. I'm not enabling
	// RTTI for something so trivial as this.
	enum class Language { English, Czech, Portuguese, Italian, French, Spanish, Japanese, Korean, Chinese_Simplified, Chinese_Traditional };
	int GetNumLanguages();

	// Given language enum, returns the string name of the language (all lower case).
	tString GetLanguageName(Language);

	// Finds the full unique word (if it exists) given just a partial word prefix. If it can't find a unique match it
	// returns an empty string. In English Bip39 words are uniquely identifiable with the first 4 letters. Ex. If you
	// passed "abou" to this function it would return "about". Since "above" is also in the list, if you passed "abo"
	// it would return an empty string since there are 2 possibilites. Further, if you passed "abouz", it would return
	// the correct "about" as it assumes the "z" is a typo. That is, it returns as soon as a unique match is made as
	// the prexix is culled one character at a time. Note that the '4' is not hardcoded into any of the logic for this
	// function. This also allows something like "aba" to go to "abandon", since in this case only 3 letters are needed
	// to uniquely identify the word.
	//
	// If you enter the full exact word in the first place, it still works and returns it. Prefix is case insensitive.
	// Returned word is always lower-case. As a final example, the word "golf" will be turned into "gold". This is
	// because even though 'golf' is not a valid Bip39 word, the first 3 letters (gol) still uniquely identify (in this
	// case) the word "gold". It couldn't be anything else.
	tString GetFullWord(const tString& prefix, Language);

	// This function returns matches to the supplied prefix (if there are any). This can be useful if you
	// want, for example, to display the list of contender words as the user types -- that sort of thing.
	// Accordingly, if you enter the empty string for the prefix, all 2048 words will match. Prefix is
	// case-insensitive. Returned words are always lower-case.
	void GetMatchingWords(tList<tStringItem>& words, const tString& prefix, Language);

	// This returns the 11 bits associated with the supplied word in the given language. This function
	// expects the full word to be entered. If you want to only enter the first few letters (usually 4)
	// you should call GetFullWord first. Since only 11 bits should be returned if everything succeeded
	// the value 0xFFFFFFFF is reserved to indicate failure. Failure is returned if either the word doesn't
	// exist in the dictionary, or if the empty string was entered. FullWord is case-insensitive.
	uint32 GetBits(const tString& fullWord, Language);

	// This function takes in the 11 least significant bits and returns the associated word (lower-case).
	// If a vaue greater than 2047 is entered (more than 11 bits), returns an empty string.
	tString GetWord(uint32 bits, Language);

	// This is only needed during dev to generate the C++ dictionary header files.
	void DevGenerateWordListHeaders();
}


}
