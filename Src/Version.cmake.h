#pragma once
#define set(verStr) namespace Version { extern int Major, Minor, Revision; struct Parser { Parser(const char*);  }; static Parser parser(#verStr); }

set("BIP39TOOLS_VERSION" "0.9.9")

#undef set
