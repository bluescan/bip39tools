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


// Given the number of words, returns how many bits of entropy it represents. This does not include the checksum bits.
// Returns 0 on failure.
int GetNumEntropyBits(int numWords);

// Given the number of words, returns how many bits are for the checksum.
// Returns 0 on failure.
int GetNumChecksumBits(int numWords);

// FullBits just refers to the number of entropy bits plus the number of checksum bits.
// Returns 0 on failure.
int GetNumFullBits(int numWords);

// Given the number of entropy bits, returns how many words would be needed. Returns 0 on failure.
int GetNumWords(int numEntropyBits);

// Given the number of full bits (ENT+CS), returns how many words would be needed. Returns 0 on failure.
int GetNumWordsFromFullBits(int numFullBits);

// Compute ENT+CS from just ENT. This is the workhorse that computes the SHA-256 for the CS.
bool ComputeFullBitsFromEntropy(tuint512& fullBits, int& numFullBits, tbit256& entropy, int numEntropyBits);

// Fills in the supplied word list given the entropy you want to represent as a mnemonic. This basically just calls
// ComputeFullBitsFromEntropy and converts the result into the words (in the given language). Returns success.
bool ComputeWordsFromEntropy
(
	tList<tStringItem>& words,
	tbit256& entropy,			// We pass a ref here so we don't stamp critical info all over memory,
	int numEntropyBits,
	Bip39::Dictionary::Language
);

// Returns the full compliment of bits (ENT and CS) directly from the words.
bool GetFullBits
(
	tuint512& fullBits,
	int& numFullBits,
	const tList<tStringItem>& words,
	Bip39::Dictionary::Language
);

// Splits ENT+CS into ENT and CS.
bool SplitFullBits
(
	tbit256& entropy,	int& numENTBits,
	uint32& checksum,	int& numCSBits,
	tuint512 fullBits,
	int numFullBits
);

// Convenience. Performs GetFullBits followed by SplitFullBits and returns the entropy. Wallets
// could use this.
bool GetEntropyFromWords
(
	tbit256& entropy,
	int& numEntropyBits,
	const tList<tStringItem>& words,
	Bip39::Dictionary::Language
);

// Returns true if the CS is valid for the supplied word list.
bool ValidateMnemonic(const tList<tStringItem>& words, Bip39::Dictionary::Language);

// Overwrite the entropyBits in memory many times to help make it more secure.
void ClearEntropy(tbit256& entropyBits);


}
