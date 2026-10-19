/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 4 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#define NL_  24      /* max identifier length + 1 */
#define MAXI 512     /* max instructions */
#define MAXB 64      /* max basic blocks */
#define MAXV 128     /* max distinct variables */
#define MAXU 256     /* max (expression,holder) pairs for available-expressions */
enum { ASG, LAB, GTO, IFI, RET };

/* One TAC instruction.  ASG: dst = a op b | dst = -a (op "-", b empty) | dst = a (op empty) */
typedef struct { int k; char dst[NL_], op[NL_], a[NL_], b[NL_], tgt[NL_]; } Ins;
static Ins prog[MAXI];
static int nprog = 0, nerr = 0;
extern int yylineno;
int yylex(void);
void yyerror(const char *m);

/* optimization counters */
static struct { int fold, alg, lcse, gcse, br, unr, selfc, dead, gconst; } st;

static void cp(char *d, const char *s) { int i; for (i = 0; i < NL_ - 1 && s[i]; i++) d[i] = s[i]; d[i] = 0; }
static void add_ins(Ins x) { if (nprog >= MAXI) { fprintf(stderr, "program too long\n"); exit(1); } prog[nprog++] = x; }
static void add_label(const char *t)  { Ins x; memset(&x, 0, sizeof x); x.k = LAB; cp(x.tgt, t); add_ins(x); }
static void add_goto(const char *t)   { Ins x; memset(&x, 0, sizeof x); x.k = GTO; cp(x.tgt, t); add_ins(x); }
static void add_ret(const char *a)    { Ins x; memset(&x, 0, sizeof x); x.k = RET; cp(x.a, a); add_ins(x); }
static void add_if(const char *op, const char *a, const char *b, const char *t) {
    Ins x; memset(&x, 0, sizeof x); x.k = IFI; cp(x.op, op); cp(x.a, a); cp(x.b, b); cp(x.tgt, t); add_ins(x); }
static void add_assign(const char *d, const char *op, const char *a, const char *b) {
    Ins x; memset(&x, 0, sizeof x); x.k = ASG; cp(x.dst, d); cp(x.op, op); cp(x.a, a); cp(x.b, b); add_ins(x); }

static int isconst(const char *s) { if (*s == '-') s++; if (!*s) return 0; for (; *s; s++) if (!isdigit((unsigned char)*s)) return 0; return 1; }
static int istemp(const char *s) { if (s[0] != 't' || !s[1]) return 0; for (s++; *s; s++) if (!isdigit((unsigned char)*s)) return 0; return 1; }

static void istr(const Ins *i, char *o) {
    switch (i->k) {
    case LAB: sprintf(o, "%s:", i->tgt); break;
    case GTO: sprintf(o, "goto %s", i->tgt); break;
    case IFI: sprintf(o, "if %s %s %s goto %s", i->a, i->op, i->b, i->tgt); break;
    case RET: if (i->a[0]) sprintf(o, "return %s", i->a); else strcpy(o, "return"); break;
    default:  if (i->b[0]) sprintf(o, "%s = %s %s %s", i->dst, i->a, i->op, i->b);
              else if (i->op[0]) sprintf(o, "%s = -%s", i->dst, i->a);
              else sprintf(o, "%s = %s", i->dst, i->a);
    }
}

/* ---------------- basic blocks and CFG ---------------- */
typedef struct { int s, e, ns, succ[2]; } Blk;       /* instructions [s,e) ; successor block ids */

static int lab_blk(Ins *c, Blk *b, int nb, const char *t) {
    for (int j = 0; j < nb; j++) if (c[b[j].s].k == LAB && !strcmp(c[b[j].s].tgt, t)) return j;
    fprintf(stderr, "undefined label '%s'\n", t); exit(1);
}
static int build_blocks(Ins *c, int n, Blk *b) {
    char L[MAXI]; memset(L, 0, sizeof L);
    if (n == 0) return 0;
    L[0] = 1;
    for (int i = 0; i < n; i++) {                          /* leader rules */
        if (c[i].k == LAB) L[i] = 1;
        if ((c[i].k == GTO || c[i].k == IFI || c[i].k == RET) && i + 1 < n) L[i + 1] = 1;
    }
    int nb = 0;
    for (int i = 0; i < n; i++) if (L[i]) { if (nb >= MAXB) { fprintf(stderr, "too many blocks\n"); exit(1); } b[nb].s = i; if (nb) b[nb - 1].e = i; nb++; }
    b[nb - 1].e = n;
    for (int j = 0; j < nb; j++) {
        Ins *last = &c[b[j].e - 1]; int has_next = j + 1 < nb; b[j].ns = 0;
        if (last->k == GTO) b[j].succ[b[j].ns++] = lab_blk(c, b, nb, last->tgt);
        else if (last->k == IFI) { b[j].succ[b[j].ns++] = lab_blk(c, b, nb, last->tgt);
                                   if (has_next && b[j].succ[0] != j + 1) b[j].succ[b[j].ns++] = j + 1; }
        else if (last->k != RET && has_next) b[j].succ[b[j].ns++] = j + 1;
    }
    return nb;
}
static void show(Ins *c, int n, const char *title) {
    Blk b[MAXB]; int nb = build_blocks(c, n, b); char buf[128];
    printf("\n=== %s ===\n", title);
    for (int j = 0; j < nb; j++) {
        printf("B%d ->", j);
        if (!b[j].ns) printf(" exit");
        for (int k = 0; k < b[j].ns; k++) printf(" B%d", b[j].succ[k]);
        printf("\n");
        for (int i = b[j].s; i < b[j].e; i++) { istr(&c[i], buf); printf("    %s\n", buf); }
    }
}

/* ---------------- evaluation and algebraic rules ---------------- */
static long ev(const char *op, long x, long y, int *ok) {
    *ok = 1;
    switch (op[0]) {
    case '+': return x + y;  case '-': return x - y;  case '*': return x * y;
    case '/': if (!y) { *ok = 0; return 0; } return x / y;        /* C truncation */
    case '%': if (!y) { *ok = 0; return 0; } return x % y;
    case '<': return op[1] == '=' ? x <= y : x < y;
    case '>': return op[1] == '=' ? x >= y : x > y;
    case '=': return x == y;  case '!': return x != y;
    }
    return 0;
}
/* returns 0 = no rule, 1 = replaced by operand ra, 2 = replaced by (ro ra rb) */
static int simplify(const char *op, const char *a, const char *b, char *ro, char *ra, char *rb) {
    int za = !strcmp(a, "0"), zb = !strcmp(b, "0"), oa = !strcmp(a, "1"), ob = !strcmp(b, "1"), eq = !strcmp(a, b);
    if (!strcmp(op, "+"))      { if (zb) { cp(ra, a); return 1; } if (za) { cp(ra, b); return 1; } }
    else if (!strcmp(op, "-")) { if (zb) { cp(ra, a); return 1; } if (eq) { cp(ra, "0"); return 1; } }
    else if (!strcmp(op, "*")) {
        if (za || zb) { cp(ra, "0"); return 1; }
        if (ob) { cp(ra, a); return 1; }
        if (oa) { cp(ra, b); return 1; }
        if (!strcmp(b, "2")) { cp(ro, "+"); cp(ra, a); cp(rb, a); return 2; }      /* strength reduction */
        if (!strcmp(a, "2")) { cp(ro, "+"); cp(ra, b); cp(rb, b); return 2; }
    }
    else if (!strcmp(op, "/")) { if (ob) { cp(ra, a); return 1; } }
    else if (!strcmp(op, "%")) { if (ob) { cp(ra, "0"); return 1; } }
    else if (eq) {
        if (!strcmp(op, "==") || !strcmp(op, "<=") || !strcmp(op, ">=")) { cp(ra, "1"); return 1; }
        if (!strcmp(op, "!=") || !strcmp(op, "<")  || !strcmp(op, ">"))  { cp(ra, "0"); return 1; }
    }
    return 0;
}

/* ---------------- per-block tables: constants, copies, available expressions ---------------- */
typedef struct { char k[NL_], v[NL_]; int g; } KV;                 /* g = 1 if seeded from global analysis */
typedef struct { char op[NL_], a[NL_], b[NL_], h[NL_]; int g; } AV;  /* expression (op a b) held in variable h */
typedef struct { KV c[MAXV]; int nc; KV cp[MAXV]; int ncp; AV av[MAXV]; int nav; } Tab;

static KV *kvf(KV *a, int n, const char *k) { for (int i = 0; i < n; i++) if (!strcmp(a[i].k, k)) return &a[i]; return 0; }
static void kvdel(KV *a, int *n, const char *k) { for (int i = 0; i < *n; i++) if (!strcmp(a[i].k, k)) { a[i] = a[--*n]; return; } }
static void kvset(KV *a, int *n, const char *k, const char *v, int g) {
    KV *e = kvf(a, *n, k); if (!e) { if (*n >= MAXV) return; e = &a[(*n)++]; }
    cp(e->k, k); cp(e->v, v); e->g = g; }
static void mkkey(AV *k, const char *op, const char *a, const char *b) {   /* commutative ops: sorted operands */
    memset(k, 0, sizeof *k); cp(k->op, op);
    if ((!strcmp(op, "+") || !strcmp(op, "*") || !strcmp(op, "==") || !strcmp(op, "!=")) && strcmp(a, b) > 0) { cp(k->a, b); cp(k->b, a); }
    else { cp(k->a, a); cp(k->b, b); }
}
static AV *avf(Tab *t, const AV *k) {
    for (int i = 0; i < t->nav; i++) if (!strcmp(t->av[i].op, k->op) && !strcmp(t->av[i].a, k->a) && !strcmp(t->av[i].b, k->b)) return &t->av[i];
    return 0; }
static void avset(Tab *t, const AV *k, const char *h, int g) {
    if (avf(t, k) || t->nav >= MAXV) return; AV *e = &t->av[t->nav++]; *e = *k; cp(e->h, h); e->g = g; }

/* assignment to v invalidates every fact that mentions v */
static void kill(Tab *t, const char *v) {
    kvdel(t->c, &t->nc, v); kvdel(t->cp, &t->ncp, v);
    for (int i = 0; i < t->ncp;) if (!strcmp(t->cp[i].v, v)) t->cp[i] = t->cp[--t->ncp]; else i++;
    for (int i = 0; i < t->nav;)
        if (!strcmp(t->av[i].h, v) || !strcmp(t->av[i].a, v) || !strcmp(t->av[i].b, v)) t->av[i] = t->av[--t->nav]; else i++;
}
static const char *val(Tab *t, const char *s) {
    if (!*s || isconst(s)) return s;
    KV *e = kvf(t->c, t->nc, s);
    if (e) { if (e->g) st.gconst++; return e->v; }
    e = kvf(t->cp, t->ncp, s);
    return e ? e->v : s;
}

/* ---------------- the local optimizer: one forward pass over a block ---------------- */
static int opt_block(Ins *in, int cnt, Ins *out, Tab *t) {
    int no = 0;
    for (int i = 0; i < cnt; i++) {
        Ins x = in[i];
        if (x.k == LAB || x.k == GTO) { out[no++] = x; continue; }
        if (x.k == RET) { if (x.a[0]) { char a[NL_]; cp(a, val(t, x.a)); cp(x.a, a); } out[no++] = x; continue; }
        if (x.k == IFI) {
            char a[NL_], b[NL_]; cp(a, val(t, x.a)); cp(b, val(t, x.b));
            if (isconst(a) && isconst(b)) {                       /* branch folding */
                int ok; st.br++;
                if (ev(x.op, atol(a), atol(b), &ok)) { Ins g; memset(&g, 0, sizeof g); g.k = GTO; cp(g.tgt, x.tgt); out[no++] = g; }
            } else { cp(x.a, a); cp(x.b, b); out[no++] = x; }
            continue;
        }
        char a[NL_], b[NL_], op[NL_]; AV key; int haskey = 0;
        cp(a, val(t, x.a)); cp(b, val(t, x.b)); strcpy(op, x.op);            /* 1. propagate */
        if (!b[0] && op[0]) {                                                 /* unary minus */
            if (isconst(a)) { sprintf(a, "%ld", -atol(a)); op[0] = 0; st.fold++; }
        } else if (b[0]) {
            if (isconst(a) && isconst(b)) {                                   /* 2. constant folding */
                int ok; long r = ev(op, atol(a), atol(b), &ok);
                if (ok) { sprintf(a, "%ld", r); op[0] = 0; b[0] = 0; st.fold++; }
            }
            if (b[0]) {                                                       /* 3. algebraic simplification */
                char ro[NL_] = "", ra[NL_] = "", rb[NL_] = ""; int s = simplify(op, a, b, ro, ra, rb);
                if (s) { st.alg++; if (s == 2) { strcpy(op, ro); cp(a, ra); cp(b, rb); } else { cp(a, ra); op[0] = 0; b[0] = 0; } }
            }
            if (b[0]) {                                                       /* 4. common subexpression elimination */
                mkkey(&key, op, a, b); haskey = 1;
                AV *f = avf(t, &key);
                if (f) { if (f->g) st.gcse++; else st.lcse++; cp(a, f->h); op[0] = 0; b[0] = 0; haskey = 0; }
            }
        }
        if (!op[0] && !strcmp(a, x.dst)) { st.selfc++; continue; }            /* x = x is a no-op */
        kill(t, x.dst);                                                       /* 5. invalidate facts about dst */
        if (!op[0]) { if (isconst(a)) kvset(t->c, &t->nc, x.dst, a, 0); else kvset(t->cp, &t->ncp, x.dst, a, 0); }
        else if (haskey && strcmp(x.dst, a) && strcmp(x.dst, b)) avset(t, &key, x.dst, 0);   /* 6. record */
        Ins y; memset(&y, 0, sizeof y); y.k = ASG; cp(y.dst, x.dst); strcpy(y.op, op); cp(y.a, a); cp(y.b, b);
        out[no++] = y;
    }
    return no;
}

/* ---------------- unreachable block removal ---------------- */
static int drop_unreachable(Ins *c, int n) {
    Blk b[MAXB]; int nb = build_blocks(c, n, b); if (!nb) return n;
    int seen[MAXB] = {0}, stack[MAXB], sp = 0, cnt = 0; stack[sp++] = 0;
    while (sp) { int j = stack[--sp]; if (seen[j]) continue; seen[j] = 1; cnt++; for (int k = 0; k < b[j].ns; k++) stack[sp++] = b[j].succ[k]; }
    st.unr += nb - cnt;
    static Ins tmp[MAXI]; int m = 0;
    for (int j = 0; j < nb; j++) if (seen[j]) for (int i = b[j].s; i < b[j].e; i++) tmp[m++] = c[i];
    memcpy(c, tmp, m * sizeof(Ins)); return m;
}

/* ---------------- verification by execution ---------------- */
static char vn[MAXV][NL_]; static int nvn = 0;
static int vid(const char *s) {
    for (int i = 0; i < nvn; i++) if (!strcmp(vn[i], s)) return i;
    if (nvn >= MAXV) { fprintf(stderr, "too many variables\n"); exit(1); }
    cp(vn[nvn], s); return nvn++; }
static void reg_vars(Ins *c, int n) {
    for (int i = 0; i < n; i++) { if (c[i].dst[0]) vid(c[i].dst);
        if (c[i].a[0] && !isconst(c[i].a)) vid(c[i].a); if (c[i].b[0] && !isconst(c[i].b)) vid(c[i].b); } }
static long gv(const char *s, long *env) { return isconst(s) ? atol(s) : env[vid(s)]; }
static int findlab(Ins *c, int n, const char *t) { for (int i = 0; i < n; i++) if (c[i].k == LAB && !strcmp(c[i].tgt, t)) return i; return n; }
/* status: 0 returned, 1 fell off the end, 2 trap (divide by zero), 3 step limit */
static int run(Ins *c, int n, long *env, long *rv) {
    long steps = 0; int pc = 0; *rv = 0;
    while (pc < n) {
        if (++steps > 5000) return 3;
        Ins *i = &c[pc++]; int ok;
        switch (i->k) {
        case ASG: { long x = gv(i->a, env), r;
                    if (i->b[0]) { r = ev(i->op, x, gv(i->b, env), &ok); if (!ok) return 2; } else r = i->op[0] ? -x : x;
                    env[vid(i->dst)] = r; break; }
        case GTO: pc = findlab(c, n, i->tgt); break;
        case IFI: if (ev(i->op, gv(i->a, env), gv(i->b, env), &ok)) pc = findlab(c, n, i->tgt); break;
        case RET: *rv = i->a[0] ? gv(i->a, env) : 0; return 0;
        default: break;
        }
    }
    return 1;
}
/* strict = also compare every non-temporary variable (v1); otherwise only the returned value (v2) */
static void verify(Ins *o, int no, Ins *p, int np, int strict, int *good, int *tot) {
    reg_vars(o, no); reg_vars(p, np); *good = *tot = 0; srand(12345);
    for (int t = 0; t < 400; t++) {
        long e1[MAXV], e2[MAXV], r1, r2;
        for (int k = 0; k < nvn; k++) e1[k] = e2[k] = rand() % 16 - 6;
        int s1 = run(o, no, e1, &r1); if (s1 >= 2) continue;
        int s2 = run(p, np, e2, &r2); (*tot)++; if (s2 >= 2) continue;
        int same = s1 == s2 && (s1 != 0 || r1 == r2);
        if (same && (strict || s1 == 1)) for (int k = 0; k < nvn; k++) if (!istemp(vn[k]) && e1[k] != e2[k]) same = 0;
        *good += same;
    }
}
static int same_prog(Ins *a, int na, Ins *b, int nb) {
    if (na != nb) return 0; char x[128], y[128];
    for (int i = 0; i < na; i++) { istr(&a[i], x); istr(&b[i], y); if (strcmp(x, y)) return 0; } return 1; }
static void print_stats(void) {
    if (st.fold)   printf("  constant folding: %d\n", st.fold);
    if (st.alg)    printf("  algebraic simplification: %d\n", st.alg);
    if (st.lcse)   printf("  local CSE: %d\n", st.lcse);
    if (st.gcse)   printf("  global CSE: %d\n", st.gcse);
    if (st.gconst) printf("  global constants propagated: %d\n", st.gconst);
    if (st.br)     printf("  branch folded: %d\n", st.br);
    if (st.unr)    printf("  unreachable blocks removed: %d\n", st.unr);
    if (st.dead)   printf("  dead code removed: %d\n", st.dead);
    if (st.selfc)  printf("  self-copy removed: %d\n", st.selfc);
}

/* ---------------- input: file, pipe, or typed at the keyboard ---------------- */
typedef struct yy_buffer_state *YY_BUFFER_STATE;
YY_BUFFER_STATE yy_scan_string(const char *);
void yy_delete_buffer(YY_BUFFER_STATE);
int yyparse(void);

/* Reads TAC lines until EOF (Ctrl+D / Ctrl+Z) or a line containing only "end". */
static char *read_input(FILE *f) {
    int tty = isatty(fileno(f));
    if (tty) fprintf(stderr, "Enter three-address code, one instruction per line.\nExample:  t1 = a + b\nFinish with a line containing only: end   (or press Ctrl+D)\n> ");
    size_t cap = 4096, len = 0; char *buf = malloc(cap), line[512];
    while (fgets(line, sizeof line, f)) {
        char *p = line; while (isspace((unsigned char)*p)) p++;
        char *e = p + strlen(p); while (e > p && isspace((unsigned char)e[-1])) e--;
        if (e - p == 3 && !strncmp(p, "end", 3)) break;
        size_t l = strlen(line); if (len + l + 2 > cap) { cap *= 2; buf = realloc(buf, cap); }
        memcpy(buf + len, line, l); len += l;
        if (tty) fprintf(stderr, "> ");
    }
    buf[len++] = '\n'; buf[len] = 0;
    return buf;
}
/* common start-up for both versions: returns 1 on success */
static int load_program(char **file) {
    FILE *f = stdin;
    if (*file && strcmp(*file, "-")) { f = fopen(*file, "r"); if (!f) { perror(*file); return 0; } }
    else *file = "stdin";
    char *src = read_input(f); YY_BUFFER_STATE bs = yy_scan_string(src);
    yyparse(); yy_delete_buffer(bs); free(src);
    return !nerr && nprog;
}

/* clear all global state so another program can be entered in the same session */
static void reset_state(void) { nprog = 0; nerr = 0; memset(&st, 0, sizeof st); nvn = 0; }


#line 385 "y.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ID = 258,                      /* ID  */
    NUM = 259,                     /* NUM  */
    REL = 260,                     /* REL  */
    OP = 261,                      /* OP  */
    GOTO = 262,                    /* GOTO  */
    IF = 263,                      /* IF  */
    RETURN = 264,                  /* RETURN  */
    NL = 265                       /* NL  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define ID 258
#define NUM 259
#define REL 260
#define OP 261
#define GOTO 262
#define IF 263
#define RETURN 264
#define NL 265

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 317 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
 char *s; 

#line 461 "y.tab.c"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ID = 3,                         /* ID  */
  YYSYMBOL_NUM = 4,                        /* NUM  */
  YYSYMBOL_REL = 5,                        /* REL  */
  YYSYMBOL_OP = 6,                         /* OP  */
  YYSYMBOL_GOTO = 7,                       /* GOTO  */
  YYSYMBOL_IF = 8,                         /* IF  */
  YYSYMBOL_RETURN = 9,                     /* RETURN  */
  YYSYMBOL_NL = 10,                        /* NL  */
  YYSYMBOL_11_ = 11,                       /* ':'  */
  YYSYMBOL_12_ = 12,                       /* '='  */
  YYSYMBOL_13_ = 13,                       /* '-'  */
  YYSYMBOL_YYACCEPT = 14,                  /* $accept  */
  YYSYMBOL_prog = 15,                      /* prog  */
  YYSYMBOL_line = 16,                      /* line  */
  YYSYMBOL_opnd = 17,                      /* opnd  */
  YYSYMBOL_bop = 18                        /* bop  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   42

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  14
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  5
/* YYNRULES -- Number of rules.  */
#define YYNRULES  19
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  40

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   265


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,    13,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    11,     2,
       2,    12,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   322,   322,   323,   325,   326,   327,   328,   329,   330,
     331,   332,   333,   334,   336,   337,   338,   340,   341,   342
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ID", "NUM", "REL",
  "OP", "GOTO", "IF", "RETURN", "NL", "':'", "'='", "'-'", "$accept",
  "prog", "line", "opnd", "bop", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-8)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      -8,     1,    -8,     2,    20,     0,    11,     3,    -8,    -8,
      -8,     7,    16,    18,    -8,    -8,    22,    31,    -8,    25,
      -8,    30,    17,    -8,    -8,    11,    -8,    27,    -8,    -8,
      -8,    -8,    11,    14,    -8,    28,    36,    -8,    32,    -8
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     4,     3,
      13,     0,     0,     0,    14,    15,     0,     0,     8,     0,
       5,     0,     0,     6,    16,     0,     9,     0,    19,    17,
      10,    18,     0,     0,    11,     0,     0,    12,     0,     7
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
      -8,    -8,    -8,    -7,    -8
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,     1,     9,    17,    32
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      19,     2,     3,    13,     4,    22,    14,    15,     5,     6,
       7,     8,    10,    18,    14,    15,    16,    20,    33,    14,
      15,    36,    28,    29,    16,    35,    24,    30,    23,    21,
      31,    11,    12,    27,    24,    26,    25,    34,    37,    38,
       0,     0,    39
};

static const yytype_int8 yycheck[] =
{
       7,     0,     1,     3,     3,    12,     3,     4,     7,     8,
       9,    10,    10,    10,     3,     4,    13,    10,    25,     3,
       4,     7,     5,     6,    13,    32,     4,    10,    10,    13,
      13,    11,    12,     3,     4,    10,     5,    10,    10,     3,
      -1,    -1,    10
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    15,     0,     1,     3,     7,     8,     9,    10,    16,
      10,    11,    12,     3,     3,     4,    13,    17,    10,    17,
      10,    13,    17,    10,     4,     5,    10,     3,     5,     6,
      10,    13,    18,    17,    10,    17,     7,    10,     3,    10
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    14,    15,    15,    16,    16,    16,    16,    16,    16,
      16,    16,    16,    16,    17,    17,    17,    18,    18,    18
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     3,     3,     7,     2,     3,
       4,     5,     6,     2,     1,     1,     2,     1,     1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 5: /* line: ID ':' NL  */
#line 326 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_label((yyvsp[-2].s)); }
#line 1466 "y.tab.c"
    break;

  case 6: /* line: GOTO ID NL  */
#line 327 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_goto((yyvsp[-1].s)); }
#line 1472 "y.tab.c"
    break;

  case 7: /* line: IF opnd REL opnd GOTO ID NL  */
#line 328 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_if((yyvsp[-4].s), (yyvsp[-5].s), (yyvsp[-3].s), (yyvsp[-1].s)); }
#line 1478 "y.tab.c"
    break;

  case 8: /* line: RETURN NL  */
#line 329 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_ret(""); }
#line 1484 "y.tab.c"
    break;

  case 9: /* line: RETURN opnd NL  */
#line 330 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_ret((yyvsp[-1].s)); }
#line 1490 "y.tab.c"
    break;

  case 10: /* line: ID '=' opnd NL  */
#line 331 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_assign((yyvsp[-3].s), "", (yyvsp[-1].s), ""); }
#line 1496 "y.tab.c"
    break;

  case 11: /* line: ID '=' '-' ID NL  */
#line 332 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_assign((yyvsp[-4].s), "-", (yyvsp[-1].s), ""); }
#line 1502 "y.tab.c"
    break;

  case 12: /* line: ID '=' opnd bop opnd NL  */
#line 333 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { add_assign((yyvsp[-5].s), (yyvsp[-2].s), (yyvsp[-3].s), (yyvsp[-1].s)); }
#line 1508 "y.tab.c"
    break;

  case 13: /* line: error NL  */
#line 334 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { yyerrok; }
#line 1514 "y.tab.c"
    break;

  case 14: /* opnd: ID  */
#line 336 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { (yyval.s) = (yyvsp[0].s); }
#line 1520 "y.tab.c"
    break;

  case 15: /* opnd: NUM  */
#line 337 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { (yyval.s) = (yyvsp[0].s); }
#line 1526 "y.tab.c"
    break;

  case 16: /* opnd: '-' NUM  */
#line 338 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { char *t = malloc(strlen((yyvsp[0].s)) + 2); sprintf(t, "-%s", (yyvsp[0].s)); (yyval.s) = t; }
#line 1532 "y.tab.c"
    break;

  case 17: /* bop: OP  */
#line 340 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { (yyval.s) = (yyvsp[0].s); }
#line 1538 "y.tab.c"
    break;

  case 18: /* bop: '-'  */
#line 341 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { (yyval.s) = "-"; }
#line 1544 "y.tab.c"
    break;

  case 19: /* bop: REL  */
#line 342 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"
                                        { (yyval.s) = (yyvsp[0].s); }
#line 1550 "y.tab.c"
    break;


#line 1554 "y.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 344 "/mnt/c/Users/Prerak/Downloads/optimizer_website/optimizer_website/v1.y"

void yyerror(const char *m) { fprintf(stderr, "line %d: %s\n", yylineno, m); nerr++; }

/* ---------------- v1: remove dead temporaries (compiler-generated tN never read) ---------------- */
static int dead_temps(Ins *c, int n) {
    int ch = 1;
    while (ch) {
        ch = 0;
        for (int i = 0; i < n && !ch; i++) {
            if (c[i].k != ASG || !istemp(c[i].dst)) continue;
            int used = 0;
            for (int j = 0; j < n; j++) if (!strcmp(c[j].a, c[i].dst) || !strcmp(c[j].b, c[i].dst)) { used = 1; break; }
            if (!used) { for (int j = i; j < n - 1; j++) c[j] = c[j + 1]; n--; st.dead++; ch = 1; }
        }
    }
    return n;
}

static int run_once(char *file, int quiet) {
    if (!load_program(&file)) { fprintf(stderr, "no valid program entered\n"); return 1; }   /* file, or typed/piped stdin */

    static Ins orig[MAXI], flat[MAXI], out[MAXI]; static Tab t;
    int no = nprog, nf = 0; memcpy(orig, prog, no * sizeof(Ins));
    Blk b[MAXB]; int nb0 = build_blocks(prog, nprog, b);
    if (!quiet) show(orig, no, "BEFORE: basic blocks and CFG");

    for (int j = 0; j < nb0; j++) {                        /* local pass on every block, independently */
        memset(&t, 0, sizeof t);
        int m = opt_block(prog + b[j].s, b[j].e - b[j].s, out, &t);
        memcpy(flat + nf, out, m * sizeof(Ins)); nf += m;
    }
    nf = drop_unreachable(flat, nf);
    nf = dead_temps(flat, nf);

    Blk b1[MAXB]; int nb1 = build_blocks(flat, nf, b1); (void)b1;
    int good, tot; verify(orig, no, flat, nf, 1, &good, &tot);
    if (quiet) {
        printf("%-24s instr %3d->%-3d blocks %d->%-2d verified %d/%d\n", file, no, nf, nb0, nb1, good, tot);
        return good != tot;
    }
    show(flat, nf, "AFTER: optimized blocks and CFG");
    printf("\n=== STATISTICS ===\ninstructions: %d -> %d (%.1f%% fewer), blocks %d -> %d\n", no, nf, 100.0 * (no - nf) / no, nb0, nb1);
    print_stats();
    printf("verification: %d/%d random runs match the original\n", good, tot);
    return good != tot;
}

int main(int argc, char **argv) {
    int quiet = 0; char *file = 0;
    for (int i = 1; i < argc; i++) { if (!strcmp(argv[i], "-s")) quiet = 1; else file = argv[i]; }
    int interactive = !file && !quiet && isatty(fileno(stdin));
    if (!interactive) return run_once(file, quiet);          /* file argument or piped input: run once */
    printf("=== Basic-Block Optimizer: type your own three-address code ===\n");
    for (;;) {                                                /* interactive session: repeat until you quit */
        run_once(NULL, 0);
        printf("\nOptimize another program? (y/n): "); fflush(stdout);
        char ans[16];
        if (!fgets(ans, sizeof ans, stdin) || (ans[0] != 'y' && ans[0] != 'Y')) break;
        reset_state();
    }
    return 0;
}
