/* fe.l - Lex scanner for the mini source language (front end) */
%option noyywrap yylineno nounput noinput
%{
#include <string.h>
#include "y.tab.h"
%}
%%
[ \t\r\n]+                      ;
"//"[^\n]*                      ;                       /* comment */
"if"                            return IF;
"else"                          return ELSE;
"while"                         return WHILE;
"return"                        return RETURN;
[0-9]+                          { yylval.s = strdup(yytext); return NUM; }
[A-Za-z_][A-Za-z0-9_]*          { yylval.s = strdup(yytext); return ID; }
"<="|">="|"=="|"!="|"<"|">"     { yylval.s = strdup(yytext); return REL; }
"+"|"-"|"*"|"/"|"%"|"="|"("|")"|"{"|"}"|";"   return yytext[0];
.                               { fprintf(stderr, "line %d: illegal character '%s'\n", yylineno, yytext); }
%%
