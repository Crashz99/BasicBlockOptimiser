/* tac.y - Yacc grammar + local basic-block optimizer (Project Review 1)
   Optimizations per basic block: constant folding/propagation, algebraic simplification, common subexpression elimination.
   Also: branch folding, unreachable-block removal, dead-temporary removal, execution-based verification. */
%{
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

%}
%union { char *s; }
%token <s> ID NUM REL OP
%token GOTO IF RETURN NL
%type <s> opnd bop
%%
prog : /* empty */
     | prog line
     ;
line : NL
     | ID ':' NL                        { add_label($1); }
     | GOTO ID NL                       { add_goto($2); }
     | IF opnd REL opnd GOTO ID NL      { add_if($3, $2, $4, $6); }
     | RETURN NL                        { add_ret(""); }
     | RETURN opnd NL                   { add_ret($2); }
     | ID '=' opnd NL                   { add_assign($1, "", $3, ""); }
     | ID '=' '-' ID NL                 { add_assign($1, "-", $4, ""); }
     | ID '=' opnd bop opnd NL          { add_assign($1, $4, $3, $5); }
     | error NL                         { yyerrok; }
     ;
opnd : ID                               { $$ = $1; }
     | NUM                              { $$ = $1; }
     | '-' NUM                          { char *t = malloc(strlen($2) + 2); sprintf(t, "-%s", $2); $$ = t; }
     ;
bop  : OP                               { $$ = $1; }
     | '-'                              { $$ = "-"; }
     | REL                              { $$ = $1; }
     ;
%%
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
