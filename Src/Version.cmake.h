#pragma once
#define set(verStr) namespace Version { extern int Major, Minor, Revision; struct Parser { Parser(const char*);  }; static Parser parser(#verStr); }

set("TACENTBIP39_VERSION" "0.8.5")

#undef set

