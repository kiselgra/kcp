#include "token.h"

std::string token::type_name(enum token::type t) {
	switch (t) {
	case eof:        return "EOF";
	case identifier: return "ID";
	case number:     return "NUM";
	default: return "UNKNOWN_TOKEN_TYPE";
	}
}
