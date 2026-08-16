/* ==========================================================================
 *  cssc_fmt_f64.h -- SHORTEST ROUND-TRIP f64 formatting (Steele & White /
 *  Burger & Dybvig "Dragon4", shortest mode) + Python-repr presentation.
 *
 *  WHY THIS EXISTS: the reference CSSC prints floats exactly like Python's
 *  repr() -- the SHORTEST decimal string that parses back to the same double,
 *  with a forced ".0" for integral values:
 *
 *      2.0            -> "2.0"          1.5 + 0.1  -> "1.6"   (not 1.6000000000000001)
 *      1.0 / 3.0      -> "0.3333333333333333"
 *      0.0001         -> "0.0001"       0.00001    -> "1e-05"
 *      1234567890123456.0 -> fixed      1.2345678901234567e+19 -> exponent
 *
 *  A naive "integer part + fixed fraction" printer can never match that, so we
 *  generate digits exactly. Exactness needs arbitrary precision: a double is
 *  m*2^e, and comparing it against decimal boundaries spans ~1080 bits. Hence
 *  the small fixed-size bignum below (no allocation, no libc -- this header is
 *  included by BOTH the host runtime and the freestanding CCOS kernel runtime,
 *  which is also why it lives in one file: the two must never drift apart).
 *
 *  Presentation rules (Python repr):
 *    * fixed notation when -4 <= exp10 <= 15, else exponent notation
 *    * fixed always carries at least one fractional digit ("2.0", "-1.0")
 *    * exponent form is d[.ddd]e{+|-}NN with at least two exponent digits and
 *      NO forced ".0" ("1e-05")
 * ========================================================================== */
#ifndef CSSC_FMT_F64_H
#define CSSC_FMT_F64_H

/* Enough limbs for the full double range: the scaling below can reach roughly
 * 2^1080, i.e. ~34 32-bit limbs; 40 leaves headroom for the *10 steps. */
#define CSSC_BIG_LIMBS 40

typedef struct { int n; unsigned int d[CSSC_BIG_LIMBS]; } cssc_big;   /* little-endian limbs */

static void cb_zero(cssc_big *b) { b->n = 0; }

static void cb_set_u64(cssc_big *b, unsigned long long v) {
    b->n = 0;
    while (v) { b->d[b->n++] = (unsigned int)(v & 0xFFFFFFFFu); v >>= 32; }
}

static void cb_copy(cssc_big *dst, const cssc_big *src) {
    dst->n = src->n;
    for (int i = 0; i < src->n; i++) dst->d[i] = src->d[i];
}

/* b *= m  (m fits in 32 bits) */
static void cb_mul_small(cssc_big *b, unsigned int m) {
    unsigned long long carry = 0;
    for (int i = 0; i < b->n; i++) {
        unsigned long long cur = (unsigned long long)b->d[i] * m + carry;
        b->d[i] = (unsigned int)(cur & 0xFFFFFFFFu);
        carry = cur >> 32;
    }
    while (carry && b->n < CSSC_BIG_LIMBS) {
        b->d[b->n++] = (unsigned int)(carry & 0xFFFFFFFFu);
        carry >>= 32;
    }
}

/* b <<= bits */
static void cb_shl(cssc_big *b, unsigned int bits) {
    if (b->n == 0) return;
    unsigned int limbs = bits >> 5, rem = bits & 31;
    if (rem) {
        unsigned int carry = 0;
        for (int i = 0; i < b->n; i++) {
            unsigned long long cur = ((unsigned long long)b->d[i] << rem) | carry;
            b->d[i] = (unsigned int)(cur & 0xFFFFFFFFu);
            carry = (unsigned int)(cur >> 32);
        }
        if (carry && b->n < CSSC_BIG_LIMBS) b->d[b->n++] = carry;
    }
    if (limbs) {
        for (int i = b->n - 1; i >= 0; i--)
            if ((unsigned)(i + limbs) < CSSC_BIG_LIMBS) b->d[i + limbs] = b->d[i];
        for (unsigned int i = 0; i < limbs; i++) b->d[i] = 0;
        b->n += limbs;
        if (b->n > CSSC_BIG_LIMBS) b->n = CSSC_BIG_LIMBS;
    }
}

static int cb_cmp(const cssc_big *a, const cssc_big *b) {
    if (a->n != b->n) return a->n < b->n ? -1 : 1;
    for (int i = a->n - 1; i >= 0; i--)
        if (a->d[i] != b->d[i]) return a->d[i] < b->d[i] ? -1 : 1;
    return 0;
}

/* a -= b   (requires a >= b) */
static void cb_sub(cssc_big *a, const cssc_big *b) {
    long long borrow = 0;
    for (int i = 0; i < a->n; i++) {
        long long cur = (long long)a->d[i] - borrow - (i < b->n ? (long long)b->d[i] : 0);
        if (cur < 0) { cur += 0x100000000LL; borrow = 1; } else borrow = 0;
        a->d[i] = (unsigned int)cur;
    }
    while (a->n > 0 && a->d[a->n - 1] == 0) a->n--;
}

/* r = a + b */
static void cb_add(cssc_big *r, const cssc_big *a, const cssc_big *b) {
    int n = a->n > b->n ? a->n : b->n;
    unsigned long long carry = 0;
    r->n = 0;
    for (int i = 0; i < n; i++) {
        unsigned long long cur = carry
            + (i < a->n ? a->d[i] : 0) + (i < b->n ? b->d[i] : 0);
        r->d[r->n++] = (unsigned int)(cur & 0xFFFFFFFFu);
        carry = cur >> 32;
    }
    if (carry && r->n < CSSC_BIG_LIMBS) r->d[r->n++] = (unsigned int)carry;
}

/* q = a / b (single digit 0..9 expected), a = a % b. Digit-by-digit compare/
 * subtract: the quotient is known to be < 10 here, so a small loop is exact
 * and far simpler than a general division. */
static unsigned int cb_divmod_digit(cssc_big *a, const cssc_big *b) {
    unsigned int q = 0;
    while (cb_cmp(a, b) >= 0) { cb_sub(a, b); q++; if (q > 20) break; }
    return q;
}

/* Generate the shortest round-tripping decimal digits of a FINITE, POSITIVE x.
 * Writes ASCII digits to `digits`, returns the count; *pexp10 receives the
 * base-10 exponent of the FIRST digit (value = 0.d1d2... * 10^(exp10+1)). */
static int cssc_f64_digits(double x, char *digits, int *pexp10) {
    union { double f; unsigned long long u; } bits;
    bits.f = x;
    unsigned long long mant = bits.u & 0xFFFFFFFFFFFFFULL;
    int biased = (int)((bits.u >> 52) & 0x7FF);
    int e2; unsigned long long m;
    int minimal;                      /* mantissa is a power of two boundary */
    if (biased == 0) { m = mant; e2 = -1074; minimal = 0; }        /* subnormal */
    else { m = mant | (1ULL << 52); e2 = biased - 1075; minimal = (mant == 0); }

    /* Burger & Dybvig scaled-value setup: value = R/S, with M+ / M- the half
     * gaps to the neighbouring doubles. `minimal` marks the smaller lower gap
     * at a binade boundary. */
    cssc_big R, S, Mp, Mm, tmp, tmp2;
    cb_set_u64(&R, m); cb_set_u64(&S, 1); cb_set_u64(&Mp, 1); cb_set_u64(&Mm, 1);
    if (e2 >= 0) {
        if (!minimal) { cb_shl(&R, e2 + 1); cb_set_u64(&S, 2); cb_shl(&Mp, e2); cb_shl(&Mm, e2); }
        else          { cb_shl(&R, e2 + 2); cb_set_u64(&S, 4); cb_shl(&Mp, e2 + 1); cb_shl(&Mm, e2); }
    } else {
        if (!minimal) { cb_shl(&R, 1); cb_set_u64(&S, 1); cb_shl(&S, 1 - e2); }
        else          { cb_shl(&R, 2); cb_set_u64(&S, 1); cb_shl(&S, 2 - e2); cb_shl(&Mp, 1); }
    }

    /* IEEE round-to-nearest-EVEN: at an exact midpoint the value rounds to the
     * even mantissa, so for an even mantissa the rounding interval is CLOSED
     * (use <= / >=) and for an odd one it is open (< / >). Getting this wrong
     * costs one digit of minimality on boundary values such as 1e+23. */
    int even = ((m & 1ULL) == 0);

    /* Scale so the first generated digit is the leading one. BOTH tests must use
     * the SAME "is high" predicate -- mixing >= here with <= below makes the two
     * halves oscillate forever on an exact boundary (R+M+ == S). */
    int k = 0;
    for (;;) {                                 /* while high(R+M+, S): S *= 10 */
        cb_add(&tmp, &R, &Mp);
        int c = cb_cmp(&tmp, &S);
        if (even ? (c >= 0) : (c > 0)) { cb_mul_small(&S, 10); k++; }
        else break;
    }
    for (;;) {                                 /* while !high((R+M+)*10, S): R *= 10 */
        cb_add(&tmp, &R, &Mp);
        cb_copy(&tmp2, &tmp); cb_mul_small(&tmp2, 10);
        int c2 = cb_cmp(&tmp2, &S);
        if (even ? (c2 < 0) : (c2 <= 0)) {
            cb_mul_small(&R, 10); cb_mul_small(&Mp, 10); cb_mul_small(&Mm, 10); k--;
        } else break;
    }

    int nd = 0;
    for (;;) {
        cb_mul_small(&R, 10); cb_mul_small(&Mp, 10); cb_mul_small(&Mm, 10);
        unsigned int d = cb_divmod_digit(&R, &S);
        int cl = cb_cmp(&R, &Mm);
        int low  = even ? (cl <= 0) : (cl < 0);         /* can round DOWN to here */
        cb_add(&tmp, &R, &Mp);
        int ch = cb_cmp(&tmp, &S);
        int high = even ? (ch >= 0) : (ch > 0);         /* can round UP to here   */
        if (low || high || nd >= 17) {
            if (low && !high) { /* keep d */ }
            else if (high && !low) d++;
            else {                                       /* both: pick the nearer */
                cb_copy(&tmp2, &R); cb_mul_small(&tmp2, 2);
                int ct = cb_cmp(&tmp2, &S);
                if (ct > 0) d++;
                /* An EXACT tie (remainder is precisely half) must round to the
                 * EVEN digit, not up: e.g. 1962311374373454.25 is equidistant
                 * from ...4.2 and ...4.3 and repr() yields the even ...4.2. */
                else if (ct == 0) { if (d & 1u) d++; }
            }
            digits[nd++] = (char)('0' + d);
            break;
        }
        digits[nd++] = (char)('0' + d);
    }
    *pexp10 = k - 1;                                     /* exponent of digit 1 */
    return nd;
}

/* Format `x` into `buf` exactly like Python's repr(). Returns the length. */
static long long cssc_fmt_f64_shortest(char *buf, double x) {
    union { double f; unsigned long long u; } bits; bits.f = x;
    int bi = 0;
    int neg = (int)(bits.u >> 63);
    unsigned long long expfield = (bits.u >> 52) & 0x7FF;
    unsigned long long frac = bits.u & 0xFFFFFFFFFFFFFULL;
    if (expfield == 0x7FF) {                       /* inf / nan */
        if (frac) { buf[0]='n';buf[1]='a';buf[2]='n'; return 3; }
        if (neg) { buf[bi++]='-'; }
        buf[bi++]='i'; buf[bi++]='n'; buf[bi++]='f'; return bi;
    }
    if (neg) buf[bi++] = '-';
    if (expfield == 0 && frac == 0) {              /* +-0.0 */
        buf[bi++]='0'; buf[bi++]='.'; buf[bi++]='0'; return bi;
    }
    double ax = neg ? -x : x;

    char dig[32]; int e10;
    int nd = cssc_f64_digits(ax, dig, &e10);

    if (e10 < -4 || e10 > 15) {                    /* exponent notation */
        buf[bi++] = dig[0];
        if (nd > 1) { buf[bi++] = '.'; for (int i = 1; i < nd; i++) buf[bi++] = dig[i]; }
        buf[bi++] = 'e';
        int ev = e10;
        if (ev < 0) { buf[bi++] = '-'; ev = -ev; } else buf[bi++] = '+';
        if (ev >= 100) { buf[bi++] = (char)('0' + ev / 100); ev %= 100; }
        buf[bi++] = (char)('0' + ev / 10);          /* always >= 2 digits */
        buf[bi++] = (char)('0' + ev % 10);
        return bi;
    }
    if (e10 >= 0) {                                /* fixed, >= 1 */
        for (int i = 0; i <= e10; i++) buf[bi++] = (i < nd) ? dig[i] : '0';
        buf[bi++] = '.';
        if (nd > e10 + 1) { for (int i = e10 + 1; i < nd; i++) buf[bi++] = dig[i]; }
        else buf[bi++] = '0';                       /* forced ".0" */
        return bi;
    }
    buf[bi++] = '0'; buf[bi++] = '.';              /* fixed, < 1 */
    for (int i = 0; i < -e10 - 1; i++) buf[bi++] = '0';
    for (int i = 0; i < nd; i++) buf[bi++] = dig[i];
    return bi;
}

#endif /* CSSC_FMT_F64_H */
