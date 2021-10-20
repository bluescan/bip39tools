# bip39tools
There are currently 5 tools in this mini-suite, as well as a clean implementation of BIP-0039.
* Bip39 and Bip39::Dictionary namespaces contain the core API.
* dice2bip39: Generate a valid BIP-39 mnemonic using dice.
* finalwordsbip39: Generate the list of final valid words if you already have the previous words.
* validatebip39: Validate an existing mnemonic seed phrase to make sure the checksum is correct.
* makecompliantbip39: Fixes a mnemonic phrase to have a valid checksum. Optionally clears the checksum. Does not modify the entropy that is already present.
* unittestsbip39: Units tests for the Bip39, dictionary, and SHA-256 APIs.

## Introduction
This C++ software is for generating and decomposing valid BIP-39 mnemonics of 12, 15, 18, 21 or 24 words. All currently
listed languages are supported.

* ***dice2bip39***
In cases where a user would rather generate their own entropy instead of relying on an unknown or otherwise
opaque randomness source. This tool uses physical dice for the source of randomness. Generation of 12, 15, 18, 21, and 24 word mnemonics has been tested against https://iancoleman.io/bip39/. There are 3 different methods of processing dice rolls, all of which are secure and easy to understand. The third method allows a loaded die to be used, but it takes many more rolls to eliminate the bias.

* ***finalwordsbip39***
This tool generate the list of _all_ possible final valid words if you already have the previous words of a seed phrase.
For example, perhaps you have a 12-word mnemonic and want to extend it to 24 words. Simply generate 11 more words, enter
the 23 words into finalwordsbip39, and it will give you all candidates for the 24th word. In order to choose a random
candidate, this tool also allows you to flip a coin a few times to choose a single final word.

* ***validatebip39***
Enter a seed phrase of 12, 15, 18, 21, or 24 words and this will tell you if it is valid or not. Valid means it checks if
the encoded checksum accurately matches the hash of the entropy.

* ***makecompliantbip39***
Takes words you enter, extracts the entropy, and sets the checksum bits to be either a) Bip-39 compliant, or b) All zeros. Finally it re-outputs a set of words that have the same entropy, but modified checksum bits.
Case (a) is useful if you have a mnemonic sentence that has an invalid checksum and you want to use it with a wallet that not only checks the CS, but also refuses to use your entropy unless it is Bip-39 compliant. Generally this mode takes an invalid mnemonic and makes it valid. This is the default behavior.
Case (b) is useful if you have a wallet that is not Bip-39 compliant and further requires the mnemonic sentence to also be non-compliant by checking that all the CS bits are cleared. The Helium (HNT) mobile Android and iOS wallets (as of Oct 19, 2021) are examples of this non-standard requirement. You can now use dice2bip39 to create your own trusted mnemonic and then use this tool to clear the checksum bits so it can be used/restored-from in the Helium wallet.

* ***unittestsbip39***
Units tests of the Bip39, the Dictionary, and the SHA-256 implementation. Well-known test vectors are used for both SHA-256 and BIP39 verification.

## Dice2Bip39 Entropy Generation
Depending of the number and type of dice you have, different methods of generating random bits are
available. 12 words gets you 128 bits of entropy, 15 words -> 160 bits, 18 words -> 192 bits, 21 words -> 224 bits,
and 24 words -> 256 bits of entropy.

* ***Method 1 - Simple :*** If you have one Casino-quality 6-sided die that is evenly balanced and has no bias, this method
generates a max of 2 bits per roll. A roll of 5 or 6 means you need to discard and re-roll (1/3 of the time). You can expect to
perform 128 + 43 = 171 rolls to gernerate a 24-word mnemonic. The actual number is slightly higher (it's a limit) because
even re-rolls may not be successful.

* ***Method 2 - Parallel :*** If you have two Casino-quality 6-sided dice that are evenly balanced and have no bias, this
method generates a max of 5 bits for each roll of two dice. This is because you can treat the two rolls as a double-digit
base 6 number -- [0,5][0,5] which is a value from 0 to 35. 32 is the next lower power-of-two (2^5 = 32 so 5 bits), so each
double-roll generates 5 bits. Only 4 combinations of the 36 states would require a re-roll (1 out of 9). In terms of number
of 2-die rolls to generate a 24-word mnemonic, 256 / 5 = 52. Including one level of re-rolls, you can expect roughly
52 + 52/9 = 58 double-rolls. It is slightly higher for the same reason as before. Note, the software does not support 3 dice.
It's too far from the next lower power-of-two. You can also use one die and every two rolls generate the 2-digit
base-6 number. That's only 116 rolls so is better than method 1, but you don't get to see the immediate progression results
each roll, so the simpler one-die option is being kept. When rolling two dice at the same time, enter the left-most die
first. Be consistent, you don't want to subconsciously order them smaller to larger or some such.

* ***Method 3 - Extractor :*** If you have a low-quality die or a suspected biased die all is not lost. Indeed, for the
extremely paranoid, this 3rd method will also work with a good balanced die, removing any possible bias. The algorithm is based
on the 2019 paper by "Giulio Morina and Krzysztof Latuszynski". Look in the references directory of this repo. The algorithm, which
I'm referring to as 'Extractor' (Algorithm 1 in the paper), is based on a Von Neumann (1951) extractor. It is simple and provably
removes skew/bias. Roll the same die twice. If roll 1 is less than roll 2, generate a binary 0. If roll 1 is greater, generate a 1.
If equal, re-roll. All that is required is that the die yields all of the numbers some of the time. The price you pay for removing
the bias is rolling the die more times. Each 2 rolls (of the same biased die) yields only a single bit. With re-rolls
approx 1/6 of the time (when the two rolls match, you can expect 2*(256 + (256/6)) = 597 individual rolls to generate a
24-word (256 bit) mnemonic.

## BIP-39 Considerations

BIP-39 is specified here https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki

The tricky part with BIP-39 is computing a valid checksum offline. The last word contains some bits that are the checksum and some
that are part of the source entropy -- the checksum is not really 'the last word'. That's why with other dice methods you can
roll for 23 words, and then there are multiple choices for the final word, one for each choice of the entropy bits for that word.
With a 24-word mnemonic, 8 of the 11 bits are for the checksum.

The existence of the checksum (and specifically the method used to compute it) is, I think, a little unfortunate.
If there were no checksum, it would be a trivial exercise to generate the mnemonic from any random source a
user desires without the sentence touching a computer at all. Specifically there are two paragraphs that I find
vague:

"Although using a mnemonic not generated by the algorithm described in "Generating the mnemonic" section is possible,
this is not advised and software must compute a checksum for the mnemonic sentence using a wordlist and issue a
warning if it is invalid."

I take it the 'warning' must be issued if the software encounters a mnemonic where the checksum does not match. This
can happen if the sentence was generated outside of the software in question. If it were saying 'compute the
checksum, and then validate it', then of course it will match, as it will be the same software that computes it both
times, so if there were a coding mistake, say, in generating the SHA256 hash, it would go unnoticed. That leads me to
believe the 'warning' is when an externally supplied word-phrase is being validated, but the fact that it's a 'warning'
and not an outright rejection leads me to believe a sentence without a matching checksum can still be used (which
would allow dice to be used without any computer involvement). Next paragraph:

"The described method also provides plausible deniability, because every passphrase generates a valid seed (and thus
a deterministic wallet) but only the correct one will make the desired wallet available."

So here it says it generates a valid seed, but NOT to make the wallet available. What is a wallet implementer to do?
I know what I'd do if someone was importing a mnemonic -- I'd ignore the checksum completely and allow the user to
access their funds. If they entered it wrong they'll know soon enough as all balances will be zero. However, not
everyone is me, so the only conclusion is that users generating their mnemonic phrase for the first time must err
on the side of caution. The hash must be valid. For a 24-word mnemonic sentence, a computer must be involved...
calculating a SHA256 hash without one is an exercise in futility.

If a checksum is really that important, why not choose something much simpler than SHA-256. The number of bits of
the SHA-256 hash that are actually used in BIP-39 is (only) between 4 and 8 (inclusive, depending on total word count),
so the fact that SHA-256 is (currently) cryptographically secure is irrelevant. Something that you could compute easily
on pen and paper (which can be burned afterward)... maybe even a 'check sum'.

Final note. I have perhaps been too much of a critic. BIP-39 is great and I'm glad it exists. The fact that so many
consecutive multiples of 32 (160, 192, 224, and 256) end up being divisible by 11 after adding the multiple divided
by 32 is 'pretty neat'.

## Unit Tests
A good number of SHA-256 vectors from NIST are tested. Test vectors for BIP-39 are from Trezor's GitHub site. The
sources of the test vectors are:
1. https://github.com/trezor/python-mnemonic/blob/master/vectors.json
2. https://www.nist.gov/itl/ssd/software-quality-group/nsrl-test-data
3. https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA256.pdf
4. https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA2_Additional.pdf
5. https://csrc.nist.gov/Projects/Cryptographic-Algorithm-Validation-Program/Secure-Hashing	(FIPS 180-4 ByteTestVector)

## Mnemonic Phrase Language
Supported languages for the generated word list are: English, Czech, Portuguese, Italian, French, Spanish, Japanese, Korean, Chinese_Simplified,
Chinese_Traditional. These are all the sanctioned lists available (10) at the time this tool was created.
For languages with special characters (French and up), this tool provides the option to save the word-list to a file so you
can read them in a good utf-8 text editor afterwards. While I don't recommend it, if you're running on a fully air-gapped
machine that will be either wiped after use, or never connected to a network again, it should be fine.

## Hardware Setup
What hardware should this be run on? This software (or any other dice tool for generating a 24-word phrase) needs those
entropy bits to compute the checksum. A good solution is using an air-gapped Raspberry Pi Zero (non-W).
These devices a) Don't cost an arm and a leg. and b) Have no wifi or Bluetooth. Dice2Bip39 has been compiled and passes
all self-tests on the following platforms:

* Windows 10 (x64)
* Ubuntu (x64)
* Raspberry Pi 3b (ARM32)
* Raspberry Pi Zero No W (ARM32)
* Raspberry Pi 4 (Running Ubuntu ARM64)

### Setup on Raspberry Pi Zero
The hardware you will need is:
1. A RaspBerry Pi Zero.
2. A Wired Keyboard.
3. A (powered) USB Hub and a converter to plug it into a micro-USB port.
4. A USB to Ethernet Dongle (I have one from IOGear that works fine).
5. A Mini HDMI to Regular HDMI Cable.
6. A micro-USB power supply.
7. A monitor that accepts HDMI in.

Most of the above hardware can be bought as a kit from somewhere like CanaKit. The procedure is as follows.
1. Connect all the hardware up. Do NOT plug in the ethernet cable yet.
2. Install RaspBerry Pi OS Lite 32-bit using the Raspberry Pi Imager. It will prep the micro-SD card for you. By default ssh will be disabled. Keep it disabled.
4. Login: pi Password: raspberry
5. passwd             (Change password to something better than raspberry)
6. PLUG IN ETHERNET CABLE
7. sudo apt-get update
8. sudo apt-get install git
9. sudo apt-get install cmake
10. I suggest only grabbing tagged branches that have been released in github. You can check for a later version under GitHub's releases link.
11. Git clone --depth 1 --branch v0.9.5 https://github.com/bluescan/bip39tools
12. If you just want the latest, use: git clone https://github.com/bluescan/bip39tools
13. cd bip39tools
14. mkdir build
15. cd build
16. cmake ..           (If you want faster compile times. you could use: cmake .. -DCMAKE_BUILD_TYPE=Debug)
17. UNPLUG ETHERNET CABLE
18. make

Thats it. All 4 tools have been created. To run the dice generator tool:
* ./dice2bip39

If you want it to autostart after logging in from a fresh reboot next time:
1. cd /home/pi
2. nano .bashrc
3. Add this line to the bottom: ./bip39tools/build/dice2bip39
4. Optional instead of 3 add to the bottom ./bip39tools/build/Bip39ToolsLauncher
5. Ctrl-X to exit (hit Y to save the file)
6. sudo shutdown -r now   (To restart and test it)

Do not add the program to rc.local. Command line input is not initialized that early in the boot sequence (plus, it's better to force a login). If you choose to add the Bip39ToolsLauncher to .bashrc it will allow you to choose which tool to run (in a loop). This is useful if you want to run either the last-word generator or simply validate existing words.

The default PiZero keyboard for me was GB not US. You may need to change the layout via 'sudo raspi-config' to US.

You now have an offline device to generate secure wallet seed phrases. Either destroy the SD card when you're done and keep the Pi for other
stuff, or never connect it to a network again and store it in a safe or something.

## Building
It's a cmake C++ project. Install cmake and Visual Studio Code. Open the bip39tools directory VS Code. Same instructions for Windows and Linux.

## Running Dice2Bip39
Type dice2bip39 from a command prompt or shell. The command-line options control the amount of ouput spew.
* ***dice2bip39 -c***
  For a concise level of output.
* ***dice2bip39 -n***
  For a normal amount of output (default).
* ***dice2bip39 -v***
  For more detailed information including binary prints of the entropy and hash.

## The Bip39 and Dictionary API
The API is completely stateless. Call any function in any order and it will work assuming the input is well-formed.

The ***Bip39::Dictionary*** namespace supports all 10 current lanugages. It has functions for listing candidate words given
a partial prefix of the full word, extracting the 11 bits of a word, determining the full unique word from a prefix,
getting the full word from the 11 entropy bits, etc.

See: ***Src/Bip39/Dictionary/Dictionary.h***


The ***Bip39*** namespace has everything else needed. It supports all BIP-0039 phrase sizes, can extract the entropy bits,
or the ENT+CS bits, create the full ENT+CS bits from supplied entropy (by performing the SHA hash), validate a
seed phrase, and can deal with phrase input and output in all supported lanugages.

See: ***Src/Bip39/Bip39.h***
