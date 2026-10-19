/* tac.l - Lex scanner for three-address code (TAC) */
%option noyywrap yylineno nounput noinput
%{
#include <string.h>
#include "y.tab.h"
%}
%%
[ \t\r]+                        ;
;[^\n]*                         ;                      /* comment to end of line */
"goto"                          return GOTO;
"if"                            return IF;
"return"                        return RETURN;
[0-9]+                          { yylval.s = strdup(yytext); return NUM; }
[A-Za-z_][A-Za-z0-9_]*          { yylval.s = strdup(yytext); return ID; }
"<="|">="|"=="|"!="|"<"|">"     { yylval.s = strdup(yytext); return REL; }
"+"|"*"|"/"|"%"                 { yylval.s = strdup(yytext); return OP; }
"-"                             return '-';
"="                             return '=';
":"                             return ':';
\n                              return NL;
.                               { fprintf(stderr, "line %d: illegal character '%s'\n", yylineno, yytext); }
%%
