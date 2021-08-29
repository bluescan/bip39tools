// Bip39.h
//
// Bip39 interface.
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
#include <Foundation/tBitField.h>
#include "Dictionary/Dictionary.h"
namespace Bip39
{


// Given the number of word, how many bits of entropy does it represent. Note this does not include the checksum bits.
// Returns 0 on failure.
int GetNumEntropyBits(int numWords);

// Given the number of entropy bits,how many words would be needed. Returns 0 on failure.
int GetNumEntropyWords(int numBits);

// Fills in the supplied word list given the entropy you want to represent as a mnemonic.
// Returns success.
bool ComputeWordsFromEntropy
(
	tList<tStringItem>& words,
	tbit256& entropy,			// We pass a ref here so we don't stamp critical info all over memory,
	int entropyBits,
	Bip39::Dictionary::Language
);

// Overwrite the entropyBits in memory a few times to help make it more secure.
void ClearEntropy(tbit256& entropyBits);


}
