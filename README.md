# tacentbip39
Generate a valid BIP-39 mnemonic using dice.

### Status
This software is now functional. Tested generation of 12, 15, 18, 21, and 24 word mnemonics against https://iancoleman.io/bip39/
The outstanding issues and todo's:
* Prints are too verbose. Add a command line flag to turn them on.
* There is a self test, but it only tests the SHA-256 implementation. Add more sha tests (see tacent unit-tests) as well as add the
  test vectors for BIP-39 from the Trezor site: https://github.com/trezor/python-mnemonic/blob/master/vectors.json
* Compile on Raspberry Pi/ARM in preparation for getting it going on a Pi Zero (no W).
* Print the roll number you are on for each method. We can see if the estimates below are correct.

### Introduction
This software is for generating a valid BIP-39 mnemonic of 12, 15, 18, 21 or 24 words in cases where a user
would rather generate their own entropy instead of relying on an unknown or otherwise opaque randomness source. This tool
uses physical dice for the source of randomness.

### Entropy Generation
Depending of the number and type of dice you have, different methods of generating random bits are used.
Note that 12 words represents 128 bits of entropy, 15 words -> 160 bits, 18 words -> 192 bits, 21 words -> 224 bits, and 24 words -> 256 bits of entropy.

* If you have one Casino-quality 6-sided die that is evenly balanced and has no bias, this tools generates a max of 2 bits
per roll. Since a roll of 5 or 6 means you need to discard and re-roll, you can expect to perform 128 + 43 = 171 rolls to
gernerate a 24 word mnemonic. The actual number is slightly higher (it's a limit) because even rerolls may not be successful.

* If you have two Casino-quality 6-sided dice that are evenly balanced and have no bias, this tool generates a max of 5 bits
for each roll of two dice. This is because you can treat the two rolls as a double digit base 36 number -- [0,5][0,5]. 32 is
the next lower power of two (2^5 = 32 so 5 bits), so each double-roll generates 5 bits. Only 4 combinations of the 36 would require
a re-roll (1 out of 9). In terms of number of 2-die rolls to generate a 24 word mnemonic, 256 / 5 = 52. Including one level of re-rolls, you
can expect roughly 52 + 52/9 = 58 double-rolls . It is slightly higher for the same reason as before. Note, the software does not
support 3 dice. It's too far from the next lower power of two. You could use one die and every two rolls generate the base-36 number.
That's only 116 rolls so is better than option 1, but you don't get to see the immediate progression after each roll, so the one die
option is being kept.

* If you have a low-quality die or a suspected biased die all is not lost. Indeed, for the extremely paranoid, this 3rd method will also
work with a good balanced die, removing any possible bias. The algorithm is based on the 2019 paper by
"Giulio Morina and Krzysztof Latuszynski". Look in the references directory of this repo. The algorithm, which I'm referring
to as 'Extractor' (Algorithm 1 in the paper), is based on a Von Neumann (1951) extractor. It is simple and provably removes skew/bias.
Roll the same die twice. If roll 1 is less than roll 2, generate a binary 0. If roll 1 is greater, generate a 1. If equal, re-roll.
All that is required is that the die yields all of the numbers, some of the time. The price you pay for removing the bias is rolling
the die more times. Each 2 rolls (of the _same_ biased die) yields only a single bit. With re-rolls approx 1/6 of the time when the
two rolls match (and no, it's not 1/36 because any number can match) you can expect 2*(256 + (256/6)) = 597 individual rolls to
generate a 24-word (256 bit) mnemonic.

### BIP-39 Considerations
The tricky part with BIP-39 is computing a valid checksum. The last word contains some bits that are the
checksum, and some that are part of the source entropy, so you really can't think of the checksum as 'the last word'.

BIP-39 is specified here https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki

The existence of the checksum (and specifically the method used to compute it) is, in my opinion, unfortunate.
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
on the side of caution. The hash must be valid. For a 24 word mnemonic sentence, a computer must be involved...
calculating a SHA256 hash without one is an exercise in insanity.

If a checksum is really that important, why not choose something much simpler than SHA-256. Something that you could compute easily on pen and paper (which can be burned afterward). The number of bits of the SHA-256 hash that are actually used in BIP-39 is (only) between 4 and 8 (inclusive, depending on total word count), so the fact that SHA-256 is
(currently) cryptographically secure is irrelevant.

One final note. I have perhaps been too much of a critic. Overall BIP-39 is completely functional and reliable. The fact that so many multiples of 32 (160, 192, 224, and 256) end up being divisible by 11 after adding the multiple divided by 32 is 'pretty neat'.

### Hardware Setup
This leads to the question of what to do if you want your own source of randomness. This software needs those entropy
bits to compute the checksum. I'm toying with the idea of running it on an air-gapped Raspberry Pi Zero (non-W). Something
like that. More to fill out in this section.
