// BipMain.cpp
//
// Generate a valid BIP-39 mnemonic phrase with dice.
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

// These two should not be defined when compiling. They are for testing/dev purposes only.
// #define DEV_AUTO_GENERATE
// #define DEV_GEN_WORDLIST
#include "Version.cmake.h"
#include <iostream>
#include <string>
#include <System/tCommand.h>
#include <Foundation/tBitField.h>
#include <System/tPrint.h>
#include <System/tFile.h>
#ifdef DEV_AUTO_GENERATE
#include <Math/tRandom.h>
#include <System/tTime.h>
#endif
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


tCommand::tOption ConciseOutput	("Concise output.",	'c',	"concise");
tCommand::tOption NormalOutput	("Normal output.",	'n',	"normal");
tCommand::tOption VerboseOutput	("Verbose output.",	'v',	"verbose");


namespace Bip
{
	bool SelfTest();
	void QueryUserSetLanguage();
	int QueryUserNumWords();
	int GetNumEntropyBits(int numWords);
	int GetNumEntropyWords(int numBits);

	enum class Method { Auto, Simple, Parallel, Extractor };
	const char* MedthodNames[] = { "Auto", "Simple", "Parallel", "Extractor" };
	Method QueryUserMethod();

	void QueryUserEntropyBits_Simple();
	void QueryUserEntropyBits_Parallel();
	void QueryUserEntropyBits_Extractor();
	#ifdef DEV_AUTO_GENERATE
	void QueryUserEntropyBits_DevGen();
	#endif
	bool QueryUserSave(const char* words[24], int numWords);	// Returns true if a file was saved.

	#ifdef DEV_GEN_WORDLIST
	void GenerateWordListHeaders();
	#endif
	int InputInt();				// Returns -1 if couldn't read an integer >= 0.
	void ComputeWordsFromEntropy(const char* words[], int numWords);

	const char* Languages[]		= { "english", "czech", "portuguese", "italian", "french", "spanish", "japanese", "korean", "chinese_simplified", "chinese_traditional" };
	enum class Language       	  {  English,   Czech,   Portuguese,   Italian,   French,   Spanish,   Japanese,   Korean,   Chinese_Simplified,   Chinese_Traditional  };
	const int NumLanguages		= sizeof(Languages)/sizeof(*Languages);
	Language CurrentLanguage	= Language::English;
	typedef const char* WordListType[2048];
	WordListType* WordList = nullptr;

	uint64 ChConc = tSystem::tChannel_Verbosity0;
	uint64 ChNorm = tSystem::tChannel_Verbosity1;
	uint64 ChVerb = tSystem::tChannel_Verbosity2;

	int NumMnemonicWords		= 0;
	int NumEntropyBitsNeeded	= 0;
	int NumEntropyBitsGenerated	= 0;
	int RollCount				= 1;
	tbit256 Entropy;
};


namespace Test
{
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
	struct BIP39Vector { const char* Entropy; const char* Mnemonic; };
	BIP39Vector BIP39Vectors[] =
	{
		{ "00000000000000000000000000000000",									"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about" },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",									"legal winner thank year wave sausage worth useful legal winner thank yellow" },
		{ "80808080808080808080808080808080",									"letter advice cage absurd amount doctor acoustic avoid letter advice cage above" },
		{ "ffffffffffffffffffffffffffffffff",									"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong" },
		{ "000000000000000000000000000000000000000000000000",					"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon agent" },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",					"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal will" },
		{ "808080808080808080808080808080808080808080808080",					"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter always" },
		{ "ffffffffffffffffffffffffffffffffffffffffffffffff",					"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo when" },
		{ "0000000000000000000000000000000000000000000000000000000000000000",	"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon art" },
		{ "7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f7f",	"legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth useful legal winner thank year wave sausage worth title" },
		{ "8080808080808080808080808080808080808080808080808080808080808080",	"letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic avoid letter advice cage absurd amount doctor acoustic bless" },
		{ "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",	"zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo vote" },
		{ "9e885d952ad362caeb4efe34a8e91bd2",									"ozone drill grab fiber curtain grace pudding thank cruise elder eight picnic" },
		{ "6610b25967cdcca9d59875f5cb50b0ea75433311869e930b",					"gravity machine north sort system female filter attitude volume fold club stay feature office ecology stable narrow fog" },
		{ "68a79eaca2324873eacc50cb9c6eca8cc68ea5d936f98787c60c7ebc74e6ce7c",	"hamster diagram private dutch cause delay private meat slide toddler razor book happy fancy gospel tennis maple dilemma loan word shrug inflict delay length" },
		{ "c0ba5a8e914111210f2bd131f3d5e08d",									"scheme spot photo card baby mountain device kick cradle pact join borrow" },
		{ "6d9be1ee6ebd27a258115aad99b7317b9c8d28b6d76431c3",					"horn tenant knee talent sponsor spell gate clip pulse soap slush warm silver nephew swap uncle crack brave" },
		{ "9f6a2878b2520799a44ef18bc7df394e7061a224d2c33cd015b157d746869863",	"panda eyebrow bullet gorilla call smoke muffin taste mesh discover soft ostrich alcohol speed nation flash devote level hobby quick inner drive ghost inside" },
		{ "23db8160a31d3e0dca3688ed941adbf3",									"cat swing flag economy stadium alone churn speed unique patch report train" },
		{ "8197a4a47f0425faeaa69deebc05ca29c0a5b5cc76ceacc0",					"light rule cinnamon wrap drastic word pride squirrel upgrade then income fatal apart sustain crack supply proud access" },
		{ "066dca1a2bb7e8a1db2832148ce9933eea0f3ac9548d793112d9a95c9407efad",	"all hour make first leader extend hole alien behind guard gospel lava path output census museum junior mass reopen famous sing advance salt reform" },
		{ "f30f8c1da665478f49b001d94c5fc452",									"vessel ladder alter error federal sibling chat ability sun glass valve picture" },
		{ "c10ec20dc3cd9f652c7fac2f1230f7a3c828389a14392f05",					"scissors invite lock maple supreme raw rapid void congress muscle digital elegant little brisk hair mango congress clump" },
		{ "f585c11aec520db57dd353c69554b21a89b20fb0650966fa0a9d6f74fd989d8f",	"void come effort suffer camp survey warrior heavy shoot primary clutch crush open amazing screen patrol group space point ten exist slush involve unfold" }
	};
	constexpr int NumBIP39Vectors = sizeof(BIP39Vectors)/sizeof(*BIP39Vectors);

	bool TestSHA256StringVectors();
	bool TestSHA256BinaryVectors();
	bool TestSHA256RepeatedByteVectors();
	bool TestBIP39Vectors();
}


bool Test::TestSHA256StringVectors()
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


bool Test::TestSHA256BinaryVectors()
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


bool Test::TestSHA256RepeatedByteVectors()
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


bool Test::TestBIP39Vectors()
{
	for (int t = 0; t < NumBIP39Vectors; t++)
	{
		const char* entropy		= BIP39Vectors[t].Entropy;
		const char* mnemonic	= BIP39Vectors[t].Mnemonic;
		tPrintf("Entropy [%064s]\n", entropy);

		Bip::Entropy.Set(entropy, 16);
		tPrintf("   Uint [%064|256x]\n", Bip::Entropy);

		int numBits = tStd::tStrlen(entropy)*4;
		int numWords = Bip::GetNumEntropyWords(numBits);
		tPrintf("   NumBits %d. NumWords %d\n", numBits, numWords);

		Bip::NumEntropyBitsGenerated = Bip::NumEntropyBitsNeeded = numBits;

		const char* words[24];
		Bip::ComputeWordsFromEntropy(words, numWords);
		char generatedWords[1024];
		char* curr = generatedWords;
		for (int w = 0; w < numWords; w++)
		{
			int numWritten = tsPrintf(curr, "%s ", words[w]);
			curr += numWritten;
		}
		*(curr-1) = '\0';
		tPrintf("   GenWords [%s]\n", generatedWords);
		tPrintf("   Mnemonic [%s]\n", mnemonic);

		bool pass = (tStd::tStrcmp(generatedWords, mnemonic) == 0);
		tPrintf("   Result:  %s\n\n", pass ? "Pass" : "Fail");
		if (!pass)
			return false;
	}
	return true;
}


bool Bip::SelfTest()
{
	tPrintf("Performing Self-Tests\n");

	tPrintf("Testing SHA256 String Vectors\n");
	if (!Test::TestSHA256StringVectors())
		return false;

	tPrintf("Testing SHA256 Binary Vectors\n");
	if (!Test::TestSHA256BinaryVectors())
		return false;

	tPrintf("Testing SHA256 Binary Repeated Byte Vectors\n");
	if (!Test::TestSHA256RepeatedByteVectors())
		return false;

	tPrintf("Testing BIP39 Vectors\n");
	// Self tests must be done in engligh as the test vectors are in that language only.
	CurrentLanguage = Language::English;
	WordList = &Bip39WordList_english;
	if (!Test::TestBIP39Vectors())
		return false;

	return true;
}


int Bip::InputInt()
{
	int val = -1;
	char str[128];
	std::cin.getline(str, 128);
	int numRead = sscanf(str, "%d", &val);
	if (numRead == 1)
		return val;

	return -1;
}


#ifdef DEV_GEN_WORDLIST
void Bip::GenerateWordListHeaders()
{
	for (int lang = 0; lang < NumLanguages; lang++)
	{
		const char* language = Languages[lang]; 
		tString srcFile;
		tsPrintf(srcFile, "../Reference/WordLists/%s.txt", language);

		tString words;
		tSystem::tLoadFile(srcFile, words, '_');
		words.Replace('\n', '_');

		tString dstFile;
		tsPrintf(dstFile, "../Src/WordList_%s.h", language);
		tFileHandle file = tSystem::tOpenFile(dstFile, "wb");
		if (!file)
		{
			tPrintf("Error generating header for language %s\n", language);
			continue;
		}

		// Prologue.
		tfPrintf
		(
			file,
			"// Generated by dice2bip39\n"
			"#pragma once\n"
			"const char* Bip39WordList_%s[] =\n"
			"{\n",
			language
		);

		int num = 0;
		do
		{
			tString word = words.ExtractLeft('_');
			tfPrintf(file, "\t\"%s\"", word.Chars());
			if (num == 2047)
				tfPrintf(file, "\n");
			else
				tfPrintf(file, ",\n");
			tPrintf("Word %d [%s]\n", ++num, word.Chars());
		}
		while (!words.IsEmpty());
		tAssert(num == 2048);

		// Epilogue.
		tfPrintf(file, "};\n");
		tSystem::tCloseFile(file);
	}
}
#endif


void Bip::QueryUserSetLanguage()
{
	const char* langOpts = "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]: ";

	tPrintf("What language for the mnemonic phrase?\n");
	for (int l = 0; l < NumLanguages; l++)
		tPrintf("%d=%s\n", l, Languages[l]);

	int lang = -1;
	do
	{
		tPrintf("Language: %s", langOpts);
		lang = InputInt();
		if (lang == -1)
		{
			lang = 0;
			break;
		}
	}
	while ((lang < 0) || (lang > 9));

	tPrintf("Language Set To %s\n", Languages[lang]);
	CurrentLanguage = Language(lang);
	switch (CurrentLanguage)
	{
		case Language::English:				WordList = &Bip39WordList_english;				break;
		case Language::Czech:				WordList = &Bip39WordList_czech;				break;
		case Language::Portuguese:			WordList = &Bip39WordList_portuguese;			break;
		case Language::Italian:				WordList = &Bip39WordList_italian;				break;
		case Language::French:				WordList = &Bip39WordList_french;				break;
		case Language::Spanish:				WordList = &Bip39WordList_spanish;				break;
		case Language::Japanese:			WordList = &Bip39WordList_japanese;				break;
		case Language::Korean:				WordList = &Bip39WordList_korean;				break;
		case Language::Chinese_Simplified:	WordList = &Bip39WordList_chinese_simplified;	break;
		case Language::Chinese_Traditional:	WordList = &Bip39WordList_chinese_traditional;	break;
	}

	if (CurrentLanguage >= Language::French)
	{
		tPrintf
		(
			ChVerb | ChNorm,
			"You have chosen a language that has special characters that do not always\n"
			"display correctly in bash, cmd, or powershell. Make sure to use a utf-8 font\n"
			"such as NSimSun or MS Gothic. In Windows command you will need to run\n"
			"\"chcp 65001\" before running this software. In PowerShell you will need to run\n"
			"\"[Console]::OutputEncoding = [System.Text.Encoding]::UTF8\" before running.\n"
			"\n"
			"After completing your dice rolls you will be given the option to save your\n"
			"mnemonic phrase to a file. Many text editore read utf-8 very well, like VSCode.\n"
			"Made sure to wipe the file afterwards and only run this SW on an air-gapped\n"
			"machine.\n"
		);
	}
}


int Bip::QueryUserNumWords()
{
	tPrintf(ChNorm | ChVerb, "How many words do you want in your mnemonic phrase?\n");
	tPrintf(ChNorm | ChVerb, "Valid answers are 0, 12, 15, 18, 21, and 24. Enter 0 for self-test.\n");
	tPrintf(ChNorm | ChVerb, "Most recovery wallets will accept 12 or 24. Press Enter after answering.\n");

	const char* wordOpts = "[0, 12, 15, 18, 21, 24]: ";

	int numWords = -1;
	do
	{
		tPrintf("Number of Words %s", wordOpts);
		numWords = InputInt();
	}
	while ((numWords != 0) && (numWords != 12) && (numWords != 15) && (numWords != 18) && (numWords != 21) && (numWords != 24));
	return numWords;
}


Bip::Method Bip::QueryUserMethod()
{
	#ifdef DEV_AUTO_GENERATE
	const char* methodOpts = "[0, 1, 2, 3]: ";
	#else
	const char* methodOpts = "[1, 2, 3]: ";
	#endif

	tPrintf(ChVerb | ChNorm, "What method should be used to generate your phrase?\n\n");
	#ifdef DEV_AUTO_GENERATE
	tPrintf(ChVerb | ChNorm, "0) DevAuto\n   Do not use. For development only.\n\n");
	#endif

	tPrintf
	(
		ChVerb | ChNorm,
		"1) Simple\n"
		"   If you have one Casino-quality 6-sided die that is evenly balanced and has\n"
		"   no bias, this method generates a maximum of 2 bits per roll. Rolls of 5 or 6\n"
		"   are discarded. Expect to roll the die approx 171 times for a 24-word phrase.\n"
		"\n"
		"2) Parallel\n"
		"   If you have two Casino-quality 6-sided dice that are evenly balanced and\n"
		"   have no bias, this tool generates a maximum of 5 bits for each roll of two\n"
		"   dice. This is because you can treat the two rolls as a two-digit base-36\n"
		"   number. From 36, 32 is the next lower power of two so each double-roll\n"
		"   generates 5 bits. When rolling the 2 dice, enter the leftmost one first.\n"
		"   Expect approximately 58 double-rolls for a 24-word phrase.\n"
		"\n"
		"3) Extractor\n"
		"   If you have a low-quality die or a suspected biased die use this bias-\n"
		"   removing method. For the extremely paranoid, this 3rd method will also work\n"
		"   with a good balanced die, removing any and all bias. The method is based on\n"
		"   a Von Neumann extractor. You roll the the SAME die twice in a row. If roll 1\n"
		"   is lower than roll 2, a 0 is generated. If roll 1 is larger than roll 2, a 1\n"
		"   is generated. If equal, re-roll. You can expect approximately 597 individual\n"
		"   rolls to generate a 24-word mnemonic.\n"
	);

	int method = -1;
	do
	{
		#ifdef DEV_AUTO_GENERATE
		tPrintf("Method: 0=Auto 1=Simple 2=Parallel 3=Extractor %s", methodOpts);
		#else
		tPrintf("Method: 1=Simple 2=Parallel 3=Extractor %s", methodOpts);
		#endif

		method = InputInt();
	}
	#ifdef DEV_AUTO_GENERATE
	while ((method != 0) && (method != 1) && (method != 2) && (method != 3));
	#else
	while ((method != 1) && (method != 2) && (method != 3));
	#endif

	return Method(method);
}


void Bip::QueryUserEntropyBits_Simple()
{
	int roll = 0;
	do
	{
		tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
		roll = InputInt();
		RollCount++;
	}
	while ((roll < 1) || (roll > 4));

	tAssert((NumEntropyBitsNeeded - NumEntropyBitsGenerated) >= 2);
	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;
	switch (roll)
	{
		case 1:				// 00
			Entropy.SetBit(bitIndex-0, false);
			Entropy.SetBit(bitIndex-1, false);
			break;
		case 2:				// 01
			Entropy.SetBit(bitIndex-0, false);
			Entropy.SetBit(bitIndex-1, true);
			break;
		case 3:				// 10
			Entropy.SetBit(bitIndex-0, true);
			Entropy.SetBit(bitIndex-1, false);
			break;
		case 4:				// 11
			Entropy.SetBit(bitIndex-0, true);
			Entropy.SetBit(bitIndex-1, true);
			break;
	}
	NumEntropyBitsGenerated += 2;
}


void Bip::QueryUserEntropyBits_Parallel()
{
	uint32 base36 = 0;
	do
	{
		int roll1 = 0;
		do
		{
			tPrintf("Roll#%03d Left die  [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll1 = InputInt();
		}
		while ((roll1 < 1) || (roll1 > 6));

		int roll2 = 0;
		do
		{
			tPrintf("Roll#%03d Right die [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll2 = InputInt();
		}
		while ((roll2 < 1) || (roll2 > 6));
		RollCount++;		
		roll1--;
		roll2--;

		base36 = (roll1*6) + roll2;
		tPrintf(ChVerb, "Base36 Number: %d\n", base36);
	}
	while (base36 >= 32);

	// Since the number of bits required may not be divisible by 5, make sure we don't go over.
	int bitCount = NumEntropyBitsNeeded-NumEntropyBitsGenerated;
	int bitIndex = bitCount - 1;

	if (bitCount > 5) bitCount = 5;
	for (int b = 0; b < bitCount; b++)
	{
		bool bit = (base36 & (1 << b)) ? true : false;
		Entropy.SetBit(bitIndex-b, bit);
	}

	NumEntropyBitsGenerated += bitCount;
}


void Bip::QueryUserEntropyBits_Extractor()
{
	int roll1 = 0;
	int roll2 = 0;

	do 
	{
		do
		{
			tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll1 = InputInt();
			RollCount++;
		}
		while ((roll1 < 1) || (roll1 > 6));

		do
		{
			tPrintf("Roll#%03d [1, 2, 3, 4, 5, 6]: ", RollCount);
			roll2 = InputInt();
			RollCount++;
		}
		while ((roll2 < 1) || (roll2 > 6));
	}
	while (roll1 == roll2);
	tAssert(roll1 != roll2);
	bool bit = (roll1 < roll2) ? false : true;
	tPrintf(ChVerb, "Generated a %s\n", bit ? "1" : "0");

	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;
	Entropy.SetBit(bitIndex-0, bit);

	NumEntropyBitsGenerated += 1;
}


#ifdef DEV_AUTO_GENERATE
void Bip::QueryUserEntropyBits_DevGen()
{
	tMath::tRandom::DefaultGenerator.SetSeed( uint64(tSystem::tGetHardwareTimerCount()) );
	tAssert((NumEntropyBitsNeeded - NumEntropyBitsGenerated) >= 32);
	int bitIndex = NumEntropyBitsNeeded-NumEntropyBitsGenerated-1;

	uint32 randBits = tMath::tRandom::tGetBits();
	for (int b = 0; b < 32; b++)
	{
		bool bit = (randBits & (1 << b)) ? true : false;
		Entropy.SetBit(bitIndex-b, bit);
	}

	NumEntropyBitsGenerated += 32;
}
#endif


int Bip::GetNumEntropyBits(int numWords)
{
	switch (numWords)
	{
		case 12:	return 128;
		case 15:	return 160;
		case 18:	return 192;
		case 21:	return 224;
		case 24:	return 256;
	}
	return 0;
}


int Bip::GetNumEntropyWords(int numBits)
{
	switch (numBits)
	{
		case 128:	return 12;
		case 160:	return 15;
		case 192:	return 18;
		case 224:	return 21;
		case 256:	return 24;
	}
	return 0;
}


void Bip::ComputeWordsFromEntropy(const char* words[], int numWords)
{
	// From BIP-39
	//
	// First, an initial entropy of ENT bits is generated. A checksum is generated by taking the first
	// ENT / 32 bits of its SHA256 hash. This checksum is appended to the end of the initial entropy.
	// Next, these concatenated bits are split into groups of 11 bits, each encoding a number from
	// 0-2047, serving as an index into a wordlist. Finally, we convert these numbers into words and
	// use the joined words as a mnemonic sentence.
	//
	// Compute the SHA-256 hash of the entropy.
	uint8 entropyByteArray[32];
	int numEntropyBytes = Bip::NumEntropyBitsGenerated/8;
	for (int b = 0; b < numEntropyBytes; b++)
		entropyByteArray[b] = Bip::Entropy.GetByte(numEntropyBytes - b - 1);
	tuint256 sha256 = tHash::tHashDataSHA256(entropyByteArray, numEntropyBytes);
	tPrintf(Bip::ChVerb | Bip::ChNorm, "SHA256: %0_64|256X\n", sha256);

	// How many of the first bits do we need?
	int numHashBitsNeeded = Bip::NumEntropyBitsNeeded / 32;
	uint8 firstBits = sha256.GetByte(31);
	firstBits >>= (8-numHashBitsNeeded);
	tPrintf(Bip::ChVerb, "The first %d bits of the sha are: %08b\n", numHashBitsNeeded, firstBits);

	// We now need to store the entropy and the first bits of the sha in a single variable. We make one
	// big enough for the 24-word case: 264 bits. Just for efficiency, we'll use 288, since internally
	// the fixed int class uses 32-bit ints to store their value (and 288 is divisible by 32).
	// Actually, we'll just use 512, as the tPrintf supports that size.
	tuint512 entropyAndChecksum;
	entropyAndChecksum.MakeZero();
	for (int r = 0; r < Bip::Entropy.GetNumElements(); r++)
		entropyAndChecksum.RawElement(r) = Bip::Entropy.GetElement(r);

	entropyAndChecksum <<= numHashBitsNeeded;
	entropyAndChecksum |= firstBits;
	tPrintf(Bip::ChVerb, "EntropyAndChecksum\n");
	tPrintf(Bip::ChVerb, "%0_512|512b\n", entropyAndChecksum);

	// Next we make an array for our word indices. We will be filling it in backwards to
	// avoid extra shift operations. We just shift by 11 each time.
	int wordIndices[24];
	for (int w = 0; w < numWords; w++)
	{
		int wordIndex = entropyAndChecksum & tuint512(0x000007FF);
		tAssert((wordIndex >= 0) && (wordIndex < 2048));
		wordIndices[w] = wordIndex;
		entropyAndChecksum >>= 11;
	}

	// And finally we put the words on the list in the correct order.
	for (int w = 0; w < numWords; w++)
	{
		int wordIndex = wordIndices[numWords - w - 1];
		words[w] = (*WordList)[wordIndex];
	}

	// Before exiting let's clear the entropy variables.
	// @todo Make sure this can't get optimized away.
	for (int w = 0; w < numWords; w++)
		wordIndices[w] = -1;
	for (int b = 0; b < 32; b++)
		entropyByteArray[b] = 0;
	entropyAndChecksum.MakeZero();
}


bool Bip::QueryUserSave(const char* words[24], int numWords)
{
	bool savedFile = false;

	// Should give option to save if language that doesn't dislay correctly in console chosen.
	if (CurrentLanguage >= Language::French)
	{
		const char* wordSaveFile = "WordListResult.txt";
		tPrintf
		(
			Bip::ChVerb | Bip::ChNorm,
			"Since you chose a language that has special characters, do you want\n"
			"to save it as \"%s\"\n", wordSaveFile
		);
		tPrintf("Save to %s? 0=No 1=Yes [0, 1]: ", wordSaveFile);
		int doSave = InputInt();
		if (doSave == 1)
		{
			tPrintf("Saving words.\n");
			tFileHandle file = tSystem::tOpenFile(wordSaveFile, "wb");
			for (int w = 0; w < numWords; w++)
				tfPrintf(file, "Word %02d: %s\n", w+1, words[w]);
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


int main(int argc, char** argv)
{
	tCommand::tParse(argc, argv);
	tSystem::tSetChannels(tSystem::tChannel_Systems | Bip::ChNorm);
	if (VerboseOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | Bip::ChVerb);
	else if (NormalOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | Bip::ChNorm);
	else if (ConciseOutput)
		tSystem::tSetChannels(tSystem::tChannel_Systems | Bip::ChConc);

	if (ConciseOutput)
		tPrintf("dice2bip39 V%d.%d.%d\n", Version::Major, Version::Minor, Version::Revision);
	else
		tCommand::tPrintUsage(nullptr, "This program generates a valid BIP-39 passphrase using dice.", Version::Major, Version::Minor, Version::Revision);

	#ifdef DEV_GEN_WORDLIST
	Bip::GenerateWordListHeaders();
	return 0;
	#endif

ChooseLanguage:
	Bip::QueryUserSetLanguage();

	int numWords = Bip::QueryUserNumWords();
	if (numWords == 0)
	{
		tSystem::tSetChannels(tSystem::tChannel_Systems);
		bool pass = Bip::SelfTest();
		tPrintf("Seft-Test Result: %s\n", pass ? "PASS" : "FAIL");
	}
	else
	{
		tPrintf("A %d-word mnemonic will be created.\n", numWords);

		Bip::NumEntropyBitsNeeded = Bip::GetNumEntropyBits(numWords);
		tPrintf(Bip::ChVerb, "Your %d-word mnemonic phrase will contain %d bits of entropy.\n", numWords, Bip::NumEntropyBitsNeeded);

		Bip::Method method = Bip::QueryUserMethod();
		tPrintf("Using %s method.\n", Bip::MedthodNames[ int(method) ]);

		Bip::NumEntropyBitsGenerated = 0;
		Bip::Entropy.Clear();

		while (Bip::NumEntropyBitsGenerated < Bip::NumEntropyBitsNeeded)
		{
			switch (method)
			{
				#ifdef DEV_AUTO_GENERATE
				case Bip::Method::Auto:
					Bip::QueryUserEntropyBits_DevGen();
					break;
				#endif
				case Bip::Method::Simple:
					Bip::QueryUserEntropyBits_Simple();
					break;
				case Bip::Method::Parallel:
					Bip::QueryUserEntropyBits_Parallel();
					break;
				case Bip::Method::Extractor:
					Bip::QueryUserEntropyBits_Extractor();
					break;
			}
			tPrintf("Progress: %d of %d bits.\n", Bip::NumEntropyBitsGenerated, Bip::NumEntropyBitsNeeded);
			tPrintf(Bip::ChVerb, "Entropy: %0_256|256b\n", Bip::Entropy);
		}

		tAssert(Bip::NumEntropyBitsGenerated == Bip::NumEntropyBitsNeeded);

		const char* words[24];
		Bip::ComputeWordsFromEntropy(words, numWords);

		// Tell the user the words.
		tPrintf("\n");
		for (int w = 0; w < numWords; w++)
			tPrintf("Word %02d: %s\n", w+1, words[w]);
		tPrintf("\n");

		bool savedFile = Bip::QueryUserSave(words, numWords);
		if (savedFile)
			tPrintf("You saved results to a file. If you go again and save it will be overwritten.\n");
	}

	// Go again?
	const char* againOpts = "[0, 1]: ";
	int again = -1;
	do
	{
		tPrintf("Go Again? 0=No 1=Yes %s", againOpts);
		again = Bip::InputInt();
	}
	while ((again != 0) && (again != 1));

	if (again == 1)
		goto ChooseLanguage;

	// Before exiting let's clear the entropy variables.
	// @todo Make sure this can't get optimized away.
	tPrintf(Bip::ChVerb, "Erasing Memory\n");
	Bip::Entropy.Clear();	
	return 0;
}
