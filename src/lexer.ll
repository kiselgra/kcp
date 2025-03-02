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

static int col = 0;
static int last_line = 0;

#define YY_USER_ACTION \
  printf("matched token '%s' of len %d\n", yytext, yyleng); \
  if (last_line != yylineno) \
    last_line = yylineno, col = yyleng; \
  else \
    col += yyleng;

#define matched(X) return token(token::X, yytext, yylineno, col-yyleng)

std::string string_accum = "";
int string_accum_start = 0;

%}

%option noyywrap
%option yylineno

WHITE_SPACE [\n\r\ \t\b\012]
DIGIT [0-9]
ALPHA [a-zA-Z_]
ALNUM ({DIGIT}|{ALPHA})

%s COMMENT
%s LINE_COMMENT
%s STRING


%%

<<EOF>>										matched(eof);
<INITIAL>{WHITE_SPACE}						/*ignore*/

<INITIAL>"-"?{DIGIT}+"."{DIGIT}*("e""-"?{DIGIT}+)?			matched(floating);
<INITIAL>"-"?{DIGIT}*"."{DIGIT}+("e""-"?{DIGIT}+)?			matched(floating);
<INITIAL>{DIGIT}+    matched(integral);

<INITIAL>"//"								BEGIN(LINE_COMMENT);
<INITIAL>"/*"								BEGIN(COMMENT);

<INITIAL>"<<="  matched(left_left_equals);
<INITIAL>">>="  matched(right_right_equals);

<INITIAL>"*="  matched(star_equals);
<INITIAL>"/="  matched(slash_equals);
<INITIAL>"%="  matched(percent_equals);
<INITIAL>"+="  matched(plus_equals);
<INITIAL>"-="  matched(minus_equals);
<INITIAL>"&="  matched(amp_equals);
<INITIAL>"^="  matched(hat_equals);
<INITIAL>"|="  matched(pipe_equals);
<INITIAL>"||"  matched(pipe_pipe);
<INITIAL>"&&"  matched(amp_amp);
<INITIAL>"=="  matched(equal_equal);
<INITIAL>"!="  matched(exclamation_equal);
<INITIAL>"<="  matched(left_equal);
<INITIAL>">="  matched(right_equal);
<INITIAL>"<<"  matched(left_left);
<INITIAL>">>"  matched(right_right);
<INITIAL>"++"  matched(plus_plus);
<INITIAL>"--"  matched(minus_minus);
<INITIAL>"->"  matched(arrow);

<INITIAL>"="  matched(equals);
<INITIAL>"+"  matched(plus);
<INITIAL>"*"  matched(star);
<INITIAL>"-"  matched(minus);
<INITIAL>"/"  matched(slash);
<INITIAL>"("  matched(paren_l);
<INITIAL>")"  matched(paren_r);
<INITIAL>"["  matched(bracket_l);
<INITIAL>"]"  matched(bracket_r);
<INITIAL>"{"  matched(brace_l);
<INITIAL>"}"  matched(brace_r);
<INITIAL>"<"  matched(left);
<INITIAL>">"  matched(right);
<INITIAL>","  matched(comma);
<INITIAL>";"  matched(semicolon);
<INITIAL>":"  matched(colon);
<INITIAL>"?"  matched(question);
<INITIAL>"!"  matched(exclamation);
<INITIAL>"|"  matched(pipe);
<INITIAL>"&"  matched(ampersand);
<INITIAL>"^"  matched(hat);
<INITIAL>"~"  matched(tilde);
<INITIAL>"."  matched(dot);

<INITIAL>"sizeof" matched(size_of);

<INITIAL>"'"."'" return token::make_char(yytext, yylineno, col-yyleng);
<INITIAL>"'\\"."'" return token::make_char(yytext, yylineno, col-yyleng);

<INITIAL>\" { string_accum = ""; string_accum_start = col; BEGIN(STRING); }

<INITIAL>{ALPHA}{ALNUM}*        matched(identifier);

<INITIAL>.									{ 	OUT("char: " << (int)yytext[0] << "[" << yytext[0] << "]"); }

<LINE_COMMENT>\n							BEGIN(INITIAL);
<LINE_COMMENT>.*							{	OUT("line comment: " << yytext); }

<COMMENT>"*/"       BEGIN(INITIAL);
<COMMENT>.*         OUT("comment: " << yytext);

<STRING>\\\" string_accum += yytext;
<STRING>\" { BEGIN(INITIAL); return token::make_string(yytext, yylineno, string_accum_start); }
<STRING>[^\n"\\] string_accum += yytext;
<STRING>\n std::cerr << "Lexer error on line " << yylineno << ": strings may not contain newlines." << std::endl;

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
  return tokens;
}

