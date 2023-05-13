// UnitTestsBip39.cpp
//
// Validate the Bip39 and SHA functions with unit tests and test vectors from official sources where possible.
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

// This is only needed during dev to generate the C++ dictionary header files.
// #define DEV_GEN_WORDLIST
#include "Version.cmake.h"
#include <iostream>
#include <string>
#include <System/tCmdLine.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#include "Bip39/Bip39.h"


namespace UnitTestsBip39
{
	bool UnitTests();

	// SHA256 Test Data Sources
	// NIST_A  : https://www.nist.gov/itl/ssd/software-quality-group/nsrl-test-data
	// NIST_B  : https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA256.pdf
	// NIST_C  : https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA2_Additional.pdf
	// NIST_D  : https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing	(FIPS 180-4 ByteTestVector)
	// NIST_E  : https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing	(FIPS 180-4 BitTestVector)

	//
	// SHA256 string test vectors.
	//
	struct SHA256StringVector { const char* Message; const char* Digest; };
	SHA256StringVector SHA256StringVectors[] =
	{
		{ "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },																	// NIST_D
		{ "abc", "BA7816BF 8F01CFEA 414140DE 5DAE2223 B00361A3 96177A9C B410FF61 F20015AD" },														// NIST_A NIST_B
		{ "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248D6A61 D20638B8 E5C02693 0C3E6039 A33CE459 64FF2167 F6ECEDD4 19DB06C1" }	// NIST_A NIST_B
	};
	constexpr int NumSHA256StringVectors = sizeof(SHA256StringVectors)/sizeof(*SHA256StringVectors);

	//
	// SHA256 binary test vectors.
	//
	uint8 BinMsg01[]		= { 0xbd };
	const char* BinDig01	= "68325720 aabd7c82 f30f554b 313d0570 c95accbb 7dc4b5aa e11204c0 8ffe732b";						// NIST_C Vector 1

	uint8 BinMsg02[]		= { 0xc9, 0x8c, 0x8e, 0x55 };
	const char* BinDig02	= "7abc22c0 ae5af26c e93dbb94 433a0e0b 2e119d01 4f8e7f65 bd56c61c cccd9504";						// NIST_C Vector 2

	uint8 BinMsg03[]		= { 0xc2, 0x99, 0x20, 0x96, 0x82 };																	// NIST_D
	const char* BinDig03	= "f0887fe961c9cd3beab957e8222494abb969b1ce4c6557976df8b0f6d20e9166";

	uint8 BinMsg04[]		= { 0xe1, 0xdc, 0x72, 0x4d, 0x56, 0x21 };															// NIST_D
	const char* BinDig04	= "eca0a060b489636225b4fa64d267dabbe44273067ac679f20820bddc6b6a90ac";

	uint8 BinMsg05[]		= { 0x06, 0xe0, 0x76, 0xf5, 0xa4, 0x42, 0xd5 };														// NIST_D
	const char* BinDig05	= "3fd877e27450e6bbd5d74bb82f9870c64c66e109418baa8e6bbcff355e287926";

	uint8 BinMsg06[]		= { 0x57, 0x38, 0xc9, 0x29, 0xc4, 0xf4, 0xcc, 0xb6 };												// NIST_D
	const char* BinDig06	= "963bb88f27f512777aab6c8b1a02c70ec0ad651d428f870036e1917120fb48bf";

	uint8 BinMsg07[]		= { 0x33, 0x34, 0xc5, 0x80, 0x75, 0xd3, 0xf4, 0x13, 0x9e };											// NIST_D
	const char* BinDig07	= "078da3d77ed43bd3037a433fd0341855023793f9afd08b4b08ea1e5597ceef20";

	uint8 BinMsg08[]		= { 0x0a, 0x27, 0x84, 0x7c, 0xdc, 0x98, 0xbd, 0x6f, 0x62, 0x22, 0x0b, 0x04, 0x6e, 0xdd, 0x76, 0x2b };	// NIST_D
	const char* BinDig08	= "80c25ec1600587e7f28b18b1b18e3cdc89928e39cab3bc25e4d4a4c139bcedc4";

	struct SHA256BinaryVector { const uint8* Message; int NumBytes; const char* Digest; };
	SHA256BinaryVector SHA256BinaryVectors[] =
	{
		{ BinMsg01, sizeof(BinMsg01), BinDig01 },
		{ BinMsg02, sizeof(BinMsg02), BinDig02 },
		{ BinMsg03, sizeof(BinMsg03), BinDig03 },
		{ BinMsg04, sizeof(BinMsg04), BinDig04 },
		{ BinMsg05, sizeof(BinMsg05), BinDig05 },
		{ BinMsg06, sizeof(BinMsg06), BinDig06 },
		{ BinMsg07, sizeof(BinMsg07), BinDig07 },
		{ BinMsg08, sizeof(BinMsg08), BinDig08 }
	};
	constexpr int NumSHA256BinaryVectors = sizeof(SHA256BinaryVectors)/sizeof(*SHA256BinaryVectors);

	//
	// SHA256 binary byte-count test vectors.
	//
	struct SHA256BinaryByteCountVector { uint8 Byte; int Count; const char* Digest; };
	SHA256BinaryByteCountVector SHA256BinaryByteCountVectors[] =
	{
		{ 'a',	1000000,	"CDC76E5C 9914FB92 81A1C7E2 84D73E67 F1809A48 A497200E 046D39CC C7112CD0" },		// NIST_A
		{ 0x00,	55,			"02779466 cdec1638 11d07881 5c633f21 90141308 1449002f 24aa3e80 f0b88ef7" },		// NIST_C
		{ 0x00,	56,			"d4817aa5 497628e7 c77e6b60 6107042b bba31308 88c5f47a 375e6179 be789fbb" },		// NIST_C
		{ 0x00,	57,			"65a16cb7 861335d5 ace3c607 18b5052e 44660726 da4cd13b b745381b 235a1785" },		// NIST_C
		{ 0x00,	64,			"f5a5fd42 d16a2030 2798ef6e d309979b 43003d23 20d9f0e8 ea9831a9 2759fb4b" },		// NIST_C
		{ 0x00,	1000,		"541b3e9d aa09b20b f85fa273 e5cbd3e8 0185aa4e c298e765 db87742b 70138a53" },		// NIST_C
		{ 'A',	1000,		"c2e68682 3489ced2 017f6059 b8b23931 8b6364f6 dcd835d0 a519105a 1eadd6e4" },		// NIST_C
		{ 'U',	1005,		"f4d62dde c0f3dd90 ea1380fa 16a5ff8d c4c54b21 740650f2 4afc4120 903552b0" },		// NIST_C
		{ 0x00,	1000000,	"d29751f2 649b32ff 572b5e0a 9f541ea6 60a50f94 ff0beedf b0b692b9 24cc8025" },		// NIST_C

		// For reference. May restrict mem usage and we don't have hash streaming API for sha256 yet.
		{ 0x5a,	0x20000000,	"15a1868c 12cc5395 1e182344 277447cd 0979536b adcc512a d24c67e9 b2d4f3dd" },		// NIST_C
		{ 0x00,	0x41000000,	"461c19a9 3bd4344f 9215f5ec 64357090 342bc66b 15a14831 7d276e31 cbc20b53" },		// NIST_C
		{ 0x42,	0x6000003e,	"c23ce8a7 895f4b21 ec0daf37 920ac0a2 62a22004 5a03eb2d fed48ef9 b05aabea" }			// NIST_C
	};
	constexpr int NumSHA256BinaryByteCountVectors = sizeof(SHA256BinaryByteCountVectors)/sizeof(*SHA256BinaryByteCountVectors);

	//
	// BIP39 test vectors.
	//
	struct BIP39Vector { const char* Entropy; const char* Mnemonic; bool InRange; };
	BIP39Vector BIP39Vectors[] =
	{
		{ "00000000000000000000000000000000",									"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about", true },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",									"legal winner thank year wave sausage worth useful legal winner thank yellow", true },
		{ "80808080808080808080808080808080",									"letter advice cage absurd amount doctor acoustic avoid letter advice cage above", true },
		{ "ffffffffffffffffffffffffffffffff",									"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong", true },
		{ "000000000000000000000000000000000000000000000000",					"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent", true },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",					"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will", true },
		{ "808080808080808080808080808080808080808080808080",					"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always", true },
		{ "ffffffffffffffffffffffffffffffffffffffffffffffff",					"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when", true },
		{ "0000000000000000000000000000000000000000000000000000000000000000",	"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art", true },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",	"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title", true },
		{ "8080808080808080808080808080808080808080808080808080808080808080",	"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless", true },
		{ "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",	"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote", false },

		// These are for testing secp256k1 range detection.
		{ "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364140",	"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo word priority hover one trouble parent target virus rug snack brass agree alpha", true },
		{ "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141",	"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo word priority hover one trouble parent target virus rug snack brass agree cheap", false },

		{ "9e885d952ad362caeb4efe34a8e91bd2",									"ozone drill grab fiber curtain grace pudding thank cruise elder eight picnic", true },
		{ "6610b25967cdcca9d59875f5cb50b0ea75433311869e930b",					"gravity machine north sort system female filter attitude volume fold club stay feature office ecology stable narrow fog", true },
		{ "68a79eaca2324873eacc50cb9c6eca8cc68ea5d936f98787c60c7ebc74e6ce7c",	"hamster diagram private dutch cause delay private meat slide toddler razor book happy fancy gospel tennis maple dilemma loan word shrug inflict delay length", true },
		{ "c0ba5a8e914111210f2bd131f3d5e08d",									"scheme spot photo card baby mountain device kick cradle pact join borrow", true },
		{ "6d9be1ee6ebd27a258115aad99b7317b9c8d28b6d76431c3",					"horn tenant knee talent sponsor spell gate clip pulse soap slush warm silver nephew swap uncle crack brave", true },
		{ "9f6a2878b2520799a44ef18bc7df394e7061a224d2c33cd015b157d746869863",	"panda eyebrow bullet gorilla call smoke muffin taste mesh discover soft ostrich alcohol speed nation flash devote level hobby quick inner drive ghost inside", true },
		{ "23db8160a31d3e0dca3688ed941adbf3",									"cat swing flag economy stadium alone churn speed unique patch report train", true },
		{ "8197a4a47f0425faeaa69deebc05ca29c0a5b5cc76ceacc0",					"light rule cinnamon wrap drastic word pride squirrel upgrade then income fatal apart sustain crack supply proud access", true },
		{ "066dca1a2bb7e8a1db2832148ce9933eea0f3ac9548d793112d9a95c9407efad",	"all hour make first leader extend hole alien behind guard gospel lava path output census museum junior mass reopen famous sing advance salt reform", true },
		{ "f30f8c1da665478f49b001d94c5fc452",									"vessel ladder alter error federal sibling chat ability sun glass valve picture", true },
		{ "c10ec20dc3cd9f652c7fac2f1230f7a3c828389a14392f05",					"scissors invite lock maple supreme raw rapid void congress muscle digital elegant little brisk hair mango congress clump", true },
		{ "f585c11aec520db57dd353c69554b21a89b20fb0650966fa0a9d6f74fd989d8f",	"void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold", true },
	};
	constexpr int NumBIP39Vectors = sizeof(BIP39Vectors)/sizeof(*BIP39Vectors);

	//
	// BIP39 mnemonic validation tests.
	//
	struct BIP39MnemonicVector { const char* Mnemonic; Bip39::ValidateResult Result; };
	BIP39MnemonicVector BIP39MnemonicVectors[] =
	{
		{
			"void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold",
			Bip39::ValidateResult::Valid
		},

		{
			"come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold",
			Bip39::ValidateResult::InvalidWordCount
		},

		{
			"zzzz come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold",
			Bip39::ValidateResult::InvalidWords
		},

		// Invalid range.
		{
			"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo word priority hover one trouble parent target virus rug snack brass agree cheap",
			Bip39::ValidateResult::InvalidSecp256k1Range
		},

		// Invalid chaecksum.
		{
			"ozone drill grab fiber curtain grace pudding thank cruise elder eight piano",
			Bip39::ValidateResult::InvalidBip39Checksum
		}
	};
	constexpr int NumBIP39MnemonicVectors = sizeof(BIP39MnemonicVectors)/sizeof(*BIP39MnemonicVectors);

	bool WordsMatch(const tList<tStringItem>& a, const tList<tStringItem>& b);
	void PrintWords(const tList<tStringItem>&);

	bool TestSHA256StringVectors();
	bool TestSHA256BinaryVectors();
	bool TestSHA256RepeatedByteVectors();
	bool TestBIP39VectorsGeneration();
	bool TestBIP39VectorsValidation();
	bool TestBIP39Dictionary();
}


bool UnitTestsBip39::TestSHA256StringVectors()
{
	for (int t = 0; t < NumSHA256StringVectors; t++)
	{
		const char* message	= SHA256StringVectors[t].Message;
		const char* digest	= SHA256StringVectors[t].Digest;
		tuint256 computed	= tHash::tHashStringSHA256(message);
		tuint256 correct	(digest, 16);							// The 16 is the base.
		tPrintf
		(
			"String Message [%s]\n"
			"   Computed %0_64|256X\n"
			"   Correct  %0_64|256X\n",
			message, computed, correct
		);
		bool pass = (computed == correct);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
			return false;
	}
	return true;
}


bool UnitTestsBip39::TestSHA256BinaryVectors()
{
	for (int t = 0; t < NumSHA256BinaryVectors; t++)
	{
		const uint8* message	= SHA256BinaryVectors[t].Message;
		int length				= SHA256BinaryVectors[t].NumBytes;
		const char* digest		= SHA256BinaryVectors[t].Digest;
		tuint256 computed		= tHash::tHashDataSHA256(message, length);
		tuint256 correct		(digest, 16);							// The 16 is the base.
		tPrintf("Binary Message [ ");
		for (int b = 0; b < length; b++)
			tPrintf("%02X ", message[b]);
		tPrintf("]\n");
		tPrintf
		(
			"   Computed %0_64|256X\n"
			"   Correct  %0_64|256X\n",
			computed, correct
		);
		bool pass = (computed == correct);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
			return false;
	}
	return true;
}


bool UnitTestsBip39::TestSHA256RepeatedByteVectors()
{
	for (int t = 0; t < NumSHA256BinaryByteCountVectors; t++)
	{
		int count				= SHA256BinaryByteCountVectors[t].Count;
		if (count > 1000000)	continue;												// Limit memory use.
		uint8 byte				= SHA256BinaryByteCountVectors[t].Byte;
		const char* digest		= SHA256BinaryByteCountVectors[t].Digest;

		uint8* message = new uint8[count];
		tStd::tMemset(message, byte, count);
		tuint256 computed		= tHash::tHashDataSHA256(message, count);
		delete[] message;
		tuint256 correct		(digest, 16);											// The 16 is the base.

		tPrintf
		(
			"Binary Message [%d Bytes of 0x%02X]\n"
			"   Computed %0_64|256X\n"
			"   Correct  %0_64|256X\n",
			count, byte, computed, correct
		);
		bool pass = (computed == correct);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
			return false;
	}

	return true;
}


bool UnitTestsBip39::TestBIP39VectorsGeneration()
{
	tbit256 entropyBits;
	for (int t = 0; t < NumBIP39Vectors; t++)
	{
		const char* entropy		= BIP39Vectors[t].Entropy;
		const char* mnemonic	= BIP39Vectors[t].Mnemonic;
		bool entropyInRange		= BIP39Vectors[t].InRange;
		entropyBits.Set(entropy);

		bool inRange = Bip39::IsValidSecp256k1Range(entropyBits);

		tPrintf("Entropy [%064s]   Secp256k1 Range [%B]\n", entropy, entropyInRange);
		tPrintf("   Uint [%064|256x]   Secp256k1 Range [%B]\n", entropyBits, inRange);

		int numBits = tStd::tStrlen(entropy)*4;
		int numWords = Bip39::GetNumWords(numBits);
		tPrintf("   NumBits %d. NumWords %d\n", numBits, numWords);
		tList<tStringItem> words;

		// Self tests must be done in engligh as the test vectors are in that language only.
		Bip39::ComputeWordsFromEntropy(words, entropyBits, numBits, Bip39::Dictionary::Language::English);
		tAssert(numWords == words.GetNumItems());
		char generatedWords[1024];
		char* curr = generatedWords;
		for (tStringItem* word = words.First(); word; word = word->Next())
		{
			int numWritten = tsPrintf(curr, "%s ", word->Chars());
			curr += numWritten;
		}
		*(curr-1) = '\0';
		tPrintf("   GenWords [%s]\n", generatedWords);
		tPrintf("   Mnemonic [%s]\n", mnemonic);

		bool pass = (tStd::tStrcmp(generatedWords, mnemonic) == 0) && (entropyInRange == inRange);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
		{
			Bip39::ClearEntropy(entropyBits);
			return false;
		}
	}
	Bip39::ClearEntropy(entropyBits);
	return true;
}


bool UnitTestsBip39::TestBIP39VectorsValidation()
{
	for (int t = 0; t < NumBIP39MnemonicVectors; t++)
	{
		const char* mnemonic					= BIP39MnemonicVectors[t].Mnemonic;
		Bip39::ValidateResult expectedResult	= BIP39MnemonicVectors[t].Result;
		const char* expectedResultStr			= Bip39::GetValidateResultString(expectedResult);

		tPrintf("Words [%s]\n", mnemonic);
		tPrintf("   Expected: [%s]\n", expectedResultStr);

		tList<tStringItem> words;
		tStd::tExplode(words, tString(mnemonic), ' ');
		Bip39::ValidateResult receivedResult	= Bip39::ValidateMnemonic(words, Bip39::Dictionary::Language::English, true);
		const char* receivedResultStr			= Bip39::GetValidateResultString(receivedResult);
		tPrintf("   Received: [%s]\n", receivedResultStr);

		bool pass = (expectedResult == receivedResult);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
			return false;
	}
	return true;
}


bool UnitTestsBip39::WordsMatch(const tList<tStringItem>& a, const tList<tStringItem>& b)
{
	if (a.GetNumItems() != b.GetNumItems())
		return false;

	tStringItem* bword = b.First();
	for (tStringItem* aword = a.First(); aword; aword = aword->Next(), bword = bword->Next())
	{
		if (*aword != *bword)
			return false;
	}

	return true;
}


void UnitTestsBip39::PrintWords(const tList<tStringItem>& words)
{
	for (tStringItem* word = words.First(); word; word = word->Next())
		tPrintf("%s ", word->Chars());

	tPrintf("\n");
}


bool UnitTestsBip39::TestBIP39Dictionary()
{
	tList<tStringItem> words;
	Bip39::Dictionary::GetMatchingWords(words, "act", Bip39::Dictionary::Language::English);
	tPrintf("Result: "); PrintWords(words);

	tList<tStringItem> expected;
	expected.Append(new tStringItem("act"));
	expected.Append(new tStringItem("action"));
	expected.Append(new tStringItem("actor"));
	expected.Append(new tStringItem("actress"));
	expected.Append(new tStringItem("actual"));
	tPrintf("Expect: "); PrintWords(expected);

	if (!WordsMatch(words, expected))
		return false;

	tString fullWord;

	fullWord = Bip39::Dictionary::GetFullWord("act", Bip39::Dictionary::Language::English);
	tPrintf("Prefix:act FullWord:%s Expect:act\n", fullWord.Chars());
	if (fullWord != "act")
		return false;

	fullWord = Bip39::Dictionary::GetFullWord("abovTYPO", Bip39::Dictionary::Language::English);
	tPrintf("Prefix:abovTYPO FullWord:%s Expect:above\n", fullWord.Chars());
	if (fullWord != "above")
		return false;

	fullWord = Bip39::Dictionary::GetFullWord("ZZZZTYPO", Bip39::Dictionary::Language::English);
	tPrintf("Prefix:ZZZZTYPO FullWord:%s Expect:EMPTY\n", fullWord.Chars());
	if (!fullWord.IsEmpty())
		return false;

	return true;
}


bool UnitTestsBip39::UnitTests()
{
	tPrintf("Performing Unit Tests\n");

	tPrintf("Testing SHA256 String Vectors\n");
	if (!TestSHA256StringVectors())
		return false;

	tPrintf("Testing SHA256 Binary Vectors\n");
	if (!TestSHA256BinaryVectors())
		return false;

	tPrintf("Testing SHA256 Binary Repeated Byte Vectors\n");
	if (!TestSHA256RepeatedByteVectors())
		return false;

	tPrintf("Testing BIP39 Vectors Generation\n");
	if (!TestBIP39VectorsGeneration())
		return false;

	tPrintf("Testing BIP39 Vectors Validation\n");
	if (!TestBIP39VectorsValidation())
		return false;

	tPrintf("Testing BIP39 Dictionary\n");
	if (!TestBIP39Dictionary())
		return false;

	return true;
}


int main(int argc, char** argv)
{
	tSystem::tSetChannels(tSystem::tChannel_Systems);
	tPrintf("unittestsbip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);

	#ifdef DEV_GEN_WORDLIST
	Bip39::Dictionary::DevGenerateWordListHeaders();
	return 0;
	#endif

	bool pass = UnitTestsBip39::UnitTests();
	tPrintf("\nUnit Tests Result: %s\n", pass ? "PASS" : "FAIL");

	return pass ? 0 : 1;
}
