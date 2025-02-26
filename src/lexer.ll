%{

#include "token.h"

#include <iostream>
using std::cout, std::endl;

#define YY_DECL token yylex(void)

#define LEX_DEBUG_OUT
#ifdef LEX_DEBUG_OUT
#define OUT(X) cout << X << endl
#else
#define OUT(X)
#endif

%}

%option noyywrap
%option yylineno

WHITE_SPACE [\n\r\ \t\b\012]
DIGIT [0-9]
ALPHA [a-zA-Z_]
ALNUM ({DIGIT}|{ALPHA})

%s COMMENT
%s LINE_COMMENT
%s EATIT


%%

<<EOF>>										return token(token::eof, "", 0);
<INITIAL>{WHITE_SPACE}						/*ignore*/

<INITIAL>"//"								BEGIN(LINE_COMMENT);
<INITIAL>"/*"								BEGIN(COMMENT);

<LINE_COMMENT>\n							BEGIN(INITIAL);
<LINE_COMMENT>.*							{	OUT("line comment: " << yytext); }

<COMMENT>"*/"       BEGIN(INITIAL);
<COMMENT>.*         OUT("comment: " << yytext);

<INITIAL>"-"?{DIGIT}+("."{DIGIT}*("e""-"?{DIGIT}+)?)?			{	OUT("number: " << yytext);		return token(token::number, yytext, yylineno);	}


<INITIAL>{ALPHA}{ALNUM}*        return token(token::identifier, yytext, yylineno);

<INITIAL>.									{ 	OUT("char: " << (int)yytext[0]) << "[" << yytext[0] << "]"; }





%%

#include <string>
#include <vector>
#include <stdio.h>


std::vector<token> lex_input(const std::string &filename) {
  std::vector<token> tokens;
  yyin = fopen(filename.c_str(), "r");

  while (true) {
    token t = yylex();
    if (t.type == token::eof)
      return tokens;
    tokens.emplace_back(t);
  }
}

