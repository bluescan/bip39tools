#pragma once
#define set(verStr) namespace Version { extern int Major, Minor, Revision; struct Parser { Parser(const char*);  }; static Parser parser(#verStr); }

set("TACENTBIP39_VERSION" "0.1.1")

#undef set

