#include "token.h"

std::string token::type_name(enum token::type t) {
	switch (t) {
	case eof:                return "EOF";
	case identifier:         return "ID";
	case number:             return "NUM";
	case star:               return "*";
	case slash:              return "/";
	case plus:               return "+";
	case minus:              return "-";
	case paren_l:            return "(";
	case paren_r:            return ")";
	case bracket_l:          return "[";
	case bracket_r:          return "]";
	case brace_l:            return "{";
	case brace_r:            return "}";
	case comma:              return ",";
	case semicolon:          return ";";
	case colon:              return ":";
	case question:           return "?";
	case exclamation:        return "!";
	case equals:             return "=";
	case star_equals:        return "*=";
	case slash_equals:       return "/=";
	case percent_equals:     return "%=";
	case plus_equals:        return "+=";
	case minus_equals:       return "-=";
	case left_left_equals:   return "<<=";
	case right_right_equals: return ">>=";
	case amp_equals:         return "&=";
	case hat_equals:         return "^=";
	case pipe_equals:        return "|=";
	default: return "UNKNOWN_TOKEN_TYPE";
	}
}
