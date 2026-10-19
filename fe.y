/* fe.y - Compiler FRONT END: mini C-like language  ->  three-address code (TAC)
   Supports: x = expr;  if (c) s  else s   while (c) s   { ... }   return expr;
   Expressions: + - * / %  ( )  unary minus, comparisons in conditions.
   Output (stdout) is TAC that can be piped straight into the optimizer:   ./fe prog.src | ./v2        */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif
int yylex(void); void yyerror(const char *m); int yyparse(void);
extern int yylineno;
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *);

static int ntemp = 0, nlab = 0, nerr = 0, sp = 0, stk[256];
static char *outbuf; static size_t outlen = 0, outcap = 0;
static void push(int x) { stk[sp++] = x; }
static int pop(void) { return stk[--sp]; }

/* TAC is collected in a buffer and printed only if the whole program is error-free */
static void emit(const char *fmt, ...) {
    char line[256]; va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof line, fmt, ap); va_end(ap);
    size_t l = strlen(line) + 2; if (outlen + l > outcap) { outcap = (outcap + l) * 2; outbuf = realloc(outbuf, outcap); }
    outlen += sprintf(outbuf + outlen, "%s\n", line);
}
static int isnum(const char *s) { if (*s == '-') s++; if (!*s) return 0; for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0; return 1; }
static char *newtemp(void) { char *s = malloc(16); sprintf(s, "t%d", ++ntemp); return s; }
static char *bin(const char *op, const char *a, const char *b) { char *t = newtemp(); emit("%s = %s %s %s", t, a, op, b); return t; }
static char *neg(const char *a) {
    if (isnum(a)) { char *s = malloc(strlen(a) + 2); if (a[0] == '-') strcpy(s, a + 1); else sprintf(s, "-%s", a); return s; }
    char *t = newtemp(); emit("%s = -%s", t, a); return t; }
/* "if (a < b) ..." jumps over the body when the condition is FALSE, so we emit the inverse test */
static const char *inv(const char *r) {
    if (!strcmp(r, "<")) return ">=";  if (!strcmp(r, ">")) return "<=";
    if (!strcmp(r, "<=")) return ">";  if (!strcmp(r, ">=")) return "<";
    if (!strcmp(r, "==")) return "!="; return "=="; }
%}
%union { char *s; }
%token <s> ID NUM REL
%token IF ELSE WHILE RETURN
%type <s> expr cond
%nonassoc IFX
%nonassoc ELSE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS
%%
program : stmts ;
stmts   : /* empty */
        | stmts stmt
        ;
stmt    : ID '=' expr ';'               { emit("%s = %s", $1, $3); }
        | RETURN expr ';'               { emit("return %s", $2); }
        | '{' stmts '}'
        | ifpart stmt %prec IFX         { emit("L%d:", pop()); }
        | ifpart stmt elsepart stmt     { emit("L%d:", pop()); }
        | WHILE                         { int b = ++nlab; emit("L%d:", b); push(b); }
          '(' cond ')'                  { int e = ++nlab; emit("%s L%d", $4, e); push(e); }
          stmt                          { int e = pop(), b = pop(); emit("goto L%d", b); emit("L%d:", e); }
        | error ';'                     { yyerrok; }
        ;
ifpart  : IF '(' cond ')'               { int e = ++nlab; emit("%s L%d", $3, e); push(e); } ;
elsepart: ELSE                          { int le = pop(), end = ++nlab; emit("goto L%d", end); emit("L%d:", le); push(end); } ;
cond    : expr REL expr                 { char *b = malloc(160); sprintf(b, "if %s %s %s goto", $1, inv($2), $3); $$ = b; } ;
expr    : expr '+' expr                 { $$ = bin("+", $1, $3); }
        | expr '-' expr                 { $$ = bin("-", $1, $3); }
        | expr '*' expr                 { $$ = bin("*", $1, $3); }
        | expr '/' expr                 { $$ = bin("/", $1, $3); }
        | expr '%' expr                 { $$ = bin("%", $1, $3); }
        | '-' expr %prec UMINUS         { $$ = neg($2); }
        | '(' expr ')'                  { $$ = $2; }
        | NUM                           { $$ = $1; }
        | ID                            { $$ = $1; }
        ;
%%
void yyerror(const char *m) { fprintf(stderr, "line %d: %s\n", yylineno, m); nerr++; }

/* reads the source from a file, a pipe, or typed at the keyboard (finish with a line containing only: end) */
static char *read_input(FILE *f) {
    int tty = isatty(fileno(f));
    if (tty) fprintf(stderr, "Type your program (example:  x = (a + b) * 2;  while (i < 5) { i = i + 1; }  return x;)\nFinish with a line containing only: end   (or press Ctrl+D)\n> ");
    size_t cap = 4096, len = 0; char *buf = malloc(cap), line[512];
    while (fgets(line, sizeof line, f)) {
        char *p = line; while (isspace((unsigned char)*p)) p++;
        char *e = p + strlen(p); while (e > p && isspace((unsigned char)e[-1])) e--;
        if (e - p == 3 && !strncmp(p, "end", 3)) break;
        size_t l = strlen(line); if (len + l + 2 > cap) { cap *= 2; buf = realloc(buf, cap); }
        memcpy(buf + len, line, l); len += l;
        if (tty) fprintf(stderr, "> ");
    }
    buf[len++] = '\n'; buf[len] = 0; return buf;
}
int main(int argc, char **argv) {
    FILE *f = stdin;
    if (argc > 1 && strcmp(argv[1], "-")) { f = fopen(argv[1], "r"); if (!f) { perror(argv[1]); return 1; } }
    yy_scan_string(read_input(f)); yyparse();
    if (nerr) return 1;
    if (sp) { fprintf(stderr, "internal error: unbalanced labels\n"); return 1; }
    if (outbuf) fputs(outbuf, stdout);
    return 0;
}
