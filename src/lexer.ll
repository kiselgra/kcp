%{

#include "token.h"

#include <iostream>
using std::cout, std::endl;

#define YY_DECL token yylex(void)

//#define LEX_DEBUG_OUT
#ifdef LEX_DEBUG_OUT
#define OUT(X) cout << X << endl
#else
#define OUT(X)
#endif

std::string lexer_current_filename;
static int col = 0;
static int last_line = 0;

#define YY_USER_ACTION \
  /* printf("matched token '%s' of len %d [state %d]\n", yytext, yyleng, yy_start); */ \
  if (last_line != yylineno) \
    last_line = yylineno, col = yyleng; \
  else \
    col += yyleng;

#define matched(X) return token(token::X, yytext, yylineno, col-yyleng, lexer_current_filename)

std::string string_accum = "";
int string_accum_start = 0;

std::string attribute_accum;
int attribute_accum_col_start = 0;
int attribute_accum_line_start = 0;
int attrib_nest = 0;

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
%s PP_INFO
%s PP_FILE
%s PP_REST
%s ATTRIB

%%

<<EOF>>										matched(eof);

<INITIAL>"\#"  { BEGIN(PP_INFO); }

<INITIAL>{WHITE_SPACE}						/*ignore*/

<INITIAL>"-"?{DIGIT}+"."{DIGIT}*("e""-"?{DIGIT}+)?			matched(floating);
<INITIAL>"-"?{DIGIT}*"."{DIGIT}+("e""-"?{DIGIT}+)?			matched(floating);
<INITIAL>{DIGIT}+    matched(integral);

<INITIAL>"//"								BEGIN(LINE_COMMENT);
<INITIAL>"/*"								BEGIN(COMMENT);

<INITIAL>"<<="  matched(left_left_equals);
<INITIAL>">>="  matched(right_right_equals);
<INITIAL>"..."  matched(ellipsis);

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
<INITIAL>"%"  matched(percent);
<INITIAL>"&"  matched(ampersand);
<INITIAL>"^"  matched(hat);
<INITIAL>"~"  matched(tilde);
<INITIAL>"."  matched(dot);

<INITIAL>"sizeof" matched(size_of);
<INITIAL>"void" matched(kw_void);
<INITIAL>"char" matched(kw_char);
<INITIAL>"short" matched(kw_short);
<INITIAL>"int" matched(kw_int);
<INITIAL>"long" matched(kw_long);
<INITIAL>"float" matched(kw_float);
<INITIAL>"double" matched(kw_double);
<INITIAL>"_Bool" matched(kw_bool);
<INITIAL>"_Complex" matched(kw_complex);
<INITIAL>"signed" matched(kw_signed);
<INITIAL>"unsigned" matched(kw_unsigned);
<INITIAL>"const" matched(kw_const);
<INITIAL>"volatile" matched(kw_volatile);
<INITIAL>"struct" matched(kw_struct);
<INITIAL>"union" matched(kw_union);
<INITIAL>"enum" matched(kw_enum);
<INITIAL>"static" matched(kw_static);
<INITIAL>"auto" matched(kw_auto);
<INITIAL>"extern" matched(kw_extern);
<INITIAL>"register" matched(kw_register);
<INITIAL>"typedef" matched(kw_typedef);
<INITIAL>"restrict" matched(kw_restrict);
<INITIAL>"__restrict" matched(kw_restrict);
<INITIAL>"__restrict__" matched(kw_restrict);

<INITIAL>"if" matched(kw_if);
<INITIAL>"else" matched(kw_else);
<INITIAL>"switch" matched(kw_switch);
<INITIAL>"return" matched(kw_return);
<INITIAL>"break" matched(kw_break);
<INITIAL>"continue" matched(kw_continue);
<INITIAL>"case" matched(kw_case);
<INITIAL>"default" matched(kw_default);
<INITIAL>"while" matched(kw_while);
<INITIAL>"do" matched(kw_do);
<INITIAL>"for" matched(kw_for);
<INITIAL>"goto" matched(kw_goto);

<INITIAL>"'"."'" return token::make_char(yytext, yylineno, col-yyleng, lexer_current_filename);
<INITIAL>"'\\"."'" return token::make_char(yytext, yylineno, col-yyleng, lexer_current_filename);

<INITIAL>\" { string_accum = ""; string_accum_start = col; BEGIN(STRING); }
<INITIAL>__attribute__{WHITE_SPACE}*  { attribute_accum = yytext; attribute_accum_col_start = col; attribute_accum_line_start = yylineno; attrib_nest = 0; BEGIN(ATTRIB); }
<INITIAL>__asm__{WHITE_SPACE}*  { attribute_accum = yytext; attribute_accum_col_start = col; attribute_accum_line_start = yylineno; attrib_nest = 0; BEGIN(ATTRIB); }

<INITIAL>{ALPHA}{ALNUM}*        matched(identifier);

<INITIAL>.									{ 	std::cerr << "unmatched char: " << (int)yytext[0] << "[" << yytext[0] << "]" << endl; }

<LINE_COMMENT>\n							BEGIN(INITIAL);
<LINE_COMMENT>.*							{	OUT("line comment: " << yytext); }

<COMMENT>"*/"       BEGIN(INITIAL);
<COMMENT>.*         { OUT("comment: " << yytext); }

<STRING>\\\" string_accum += '"';
<STRING>\\n string_accum += '\n';
<STRING>\\t string_accum += '\t';
<STRING>\\r string_accum += '\r';
<STRING>\" { BEGIN(INITIAL); return token::make_string(string_accum, yylineno, string_accum_start, lexer_current_filename); }
<STRING>[^\n"\\] string_accum += yytext;
<STRING>\n throw lexer_error(yylineno, col-yyleng, string_accum, "strings may not contain newlines.");

<PP_INFO>{WHITE_SPACE}+{DIGIT}+{WHITE_SPACE}+\"     { yylineno=atoi(yytext)-1; /* cout << "LINE is now " << yylineno << endl; */ BEGIN(PP_FILE); }
<PP_FILE>[^"]*                                      { /* cout << "PP-\"m: '" << yytext << "'" << endl; */ lexer_current_filename = yytext; }
<PP_FILE>\"                                         { /* cout << "PP-\"m: '" << yytext << "'" << endl; */ BEGIN(PP_REST); }
<PP_REST>[1234 \t]+	                                { /* cout << "PP-suffix: '" << yytext << "'" << endl; */ }
<PP_REST>\n							                { /* cout << "--> PP-done" << endl; */ BEGIN(INITIAL); }

<PP_INFO>.	                                        { throw lexer_error(yylineno, col-yyleng, yytext, "Unmatched character on preprocessor line information"); }
<PP_REST>.	                                        { std::cerr << "Unrecognized cpp character '" << yytext << "'" << endl; }

<ATTRIB>"("                  { attribute_accum+="("; attrib_nest++; }
<ATTRIB>")"                  { attribute_accum+=")"; attrib_nest--; if (attrib_nest==0) BEGIN(INITIAL);
                               return token::make_attribute(attribute_accum, attribute_accum_line_start, attribute_accum_col_start, lexer_current_filename); }
<ATTRIB>[^()]+               { attribute_accum+=yytext; }

%%


#include <string>
#include <vector>
#include <stdio.h>


std::vector<token> lex_input(const std::string &filename) {
  lexer_current_filename = filename;
  std::vector<token> tokens;
  yyin = fopen(filename.c_str(), "r");

  while (true) {
    token t = yylex();
    tokens.push_back(t);
    if (t.type == token::eof)
      return tokens;
  }
  return tokens;
}

