// Bip39.h
//
// Bip39 interface.
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
bool ComputeFullBitsFromEntropy
(
	tbit512& fullBits,
	int& numFullBits,
	const tbit256& entropy,
	int numEntropyBits,
	bool clearChecksumBits = false
);

// Convert the full compliment of bits (entropy+CS) into words of the supplied language.
bool ComputeWordsFromFullBits
(
	tList<tStringItem>& words,
	const tbit512& fullBits,		// We pass a ref here so we don't stamp critical info all over memory,
	int numFullBits,
	Bip39::Dictionary::Language
);

// Fills in the supplied word list given the entropy you want to represent as a mnemonic. This basically just calls
// ComputeFullBitsFromEntropy followed by ComputeWordsFromFullBits. Returns success.
bool ComputeWordsFromEntropy
(
	tList<tStringItem>& words,
	const tbit256& entropy,			// We pass a ref here so we don't stamp critical info all over memory,
	int numEntropyBits,
	Bip39::Dictionary::Language
);

// Returns the full compliment of bits (ENT and CS) directly from the words.
// Expects the number of words to be 12, 15, 18, 21, or 24.
bool GetFullBits
(
	tbit512& fullBits,
	int& numFullBits,
	const tList<tStringItem>& words,
	Bip39::Dictionary::Language
);

// Gets the raw bitpattern for an arbitrary number of words. Only fails if a word can't be found
// or you submit more than 46 words. 0 words returns true with numRawBits set to 0.
bool GetRawBits
(
	tbit512& rawBits,
	int& numRawBits,
	const tList<tStringItem>& words,
	Bip39::Dictionary::Language
);

// Splits ENT+CS into ENT and CS.
bool SplitFullBits
(
	tbit256& entropy,	int& numENTBits,
	uint32& checksum,	int& numCSBits,
	const tbit512& fullBits,
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

// Depending on what the entropy will be used for, it may still be out of range. This function
// checks if it is good for Secp256k1. It is _extremely_ unlikely it will be out of range as the
// period of the curve is really large... not quite 2^256, but not relatively that far off.
bool IsValidSecp256k1Range(const tbit256& entropy);

// Returns true if the number of words is 12, 15, 18, 21, or 24.
bool IsValidNumWords(int numWords);

// Returns ValidateResult::OK if the CS is valid for the supplied word list.
enum class ValidateResult { Valid, InvalidWordCount, InvalidWords, InvalidSecp256k1Range, InvalidBip39Checksum, NumValidateResults };
ValidateResult ValidateMnemonic(const tList<tStringItem>& words, Bip39::Dictionary::Language, bool checkSecp256k1Range);

// Overwrite the entropyBits in memory many times to help make it more secure.
void ClearEntropy(tbit256& entropyBits);

// Overwrite the (full) bits in memory many times to help make it more secure.
void ClearBits(tbit512& bits);

}
