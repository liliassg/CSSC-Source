

#include <stdint.h>
typedef uint32_t            u32;
typedef int32_t             s32;
typedef uint64_t            u64;
typedef int64_t             s64;

typedef union { double d; u64 u; } dbl_u;
typedef union { float  f; u32 u; } flt_u;

static u32 _u32_abs(int a) {
    return (a < 0) ? ((u32)0u - (u32)a) : (u32)a;
}
static u64 _u64_abs(s64 a) {
    return (a < 0) ? ((u64)0ULL - (u64)a) : (u64)a;
}

static u64 _round_to_even(u64 m, unsigned g, unsigned r, unsigned s) {
    if (g) {
        if (r | s) ++m;
        else if (m & 1ULL) ++m;
    }
    return m;
}

unsigned long long __umulsidi3(unsigned int a, unsigned int b) {
    u32 a_lo = a & 0xFFFFu, a_hi = a >> 16;
    u32 b_lo = b & 0xFFFFu, b_hi = b >> 16;
    u32 ll = a_lo * b_lo;
    u32 lh = a_lo * b_hi;
    u32 hl = a_hi * b_lo;
    u32 hh = a_hi * b_hi;
    u64 mid = (u64)lh + (u64)hl;
    return ((u64)hh << 32)
         + (mid << 16)
         + (u64)ll;
}

long long __mulsidi3(int a, int b) {
    int neg = ((a ^ b) < 0);
    u32 ua = _u32_abs(a);
    u32 ub = _u32_abs(b);
    u64 q = __umulsidi3(ua, ub);
    return neg ? -(s64)q : (s64)q;
}

long long __muldi3(long long a, long long b) {
    u64 ua = (u64)a, ub = (u64)b;
    u32 a_lo = (u32)ua,        a_hi = (u32)(ua >> 32);
    u32 b_lo = (u32)ub,        b_hi = (u32)(ub >> 32);
    u64 ll = __umulsidi3(a_lo, b_lo);
    u32 lh = a_lo * b_hi;
    u32 hl = a_hi * b_lo;
    return (long long)(ll + (((u64)(lh + hl)) << 32));
}

unsigned int __udivsi3(unsigned int a, unsigned int b) {
    if (b == 0) return 0;
    u32 q = 0, r = 0;
    for (int i = 31; i >= 0; --i) {
        r = (r << 1) | ((a >> i) & 1);
        if (r >= b) { r -= b; q |= (1u << i); }
    }
    return q;
}

unsigned int __umodsi3(unsigned int a, unsigned int b) {
    if (b == 0) return a;
    u32 r = 0;
    for (int i = 31; i >= 0; --i) {
        r = (r << 1) | ((a >> i) & 1);
        if (r >= b) r -= b;
    }
    return r;
}

int __divsi3(int a, int b) {
    int neg = ((a ^ b) < 0);
    u32 ua = _u32_abs(a);
    u32 ub = _u32_abs(b);
    u32 q = __udivsi3(ua, ub);
    return neg ? -(int)q : (int)q;
}

int __modsi3(int a, int b) {
    u32 ua = _u32_abs(a);
    u32 ub = _u32_abs(b);
    u32 r = __umodsi3(ua, ub);
    return (a < 0) ? -(int)r : (int)r;
}

unsigned long long __udivdi3(unsigned long long a, unsigned long long b) {
    if (b == 0) return 0;
    u64 q = 0, r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((a >> i) & 1ULL);
        if (r >= b) { r -= b; q |= (1ULL << i); }
    }
    return q;
}

unsigned long long __umoddi3(unsigned long long a, unsigned long long b) {
    if (b == 0) return a;
    u64 r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((a >> i) & 1ULL);
        if (r >= b) r -= b;
    }
    return r;
}

long long __divdi3(long long a, long long b) {
    int neg = ((a < 0) ^ (b < 0));
    u64 ua = _u64_abs(a);
    u64 ub = _u64_abs(b);
    u64 q = __udivdi3(ua, ub);
    return neg ? -(s64)q : (s64)q;
}

long long __moddi3(long long a, long long b) {
    u64 ua = _u64_abs(a);
    u64 ub = _u64_abs(b);
    u64 r = __umoddi3(ua, ub);
    return (a < 0) ? -(s64)r : (s64)r;
}

double __extendsfdf2(float a) {
    flt_u fa = { .f = a };
    u32 ux = fa.u;
    u32 sign = ux & 0x80000000u;
    u32 e32  = (ux >> 23) & 0xFF;
    u32 m32  = ux & 0x7FFFFFu;
    u64 sign64 = ((u64)sign) << 32;
    if (e32 == 0xFF) {
        u64 payload = (u64)m32 << 29;
        if (m32 != 0 && payload == 0) payload = 1ULL;
        dbl_u r = { .u = sign64 | (0x7FFULL << 52) | payload };
        return r.d;
    }
    if (e32 == 0) {
        if (m32 == 0) {
            dbl_u r = { .u = sign64 };
            return r.d;
        }

        int shift = 0;
        while (!(m32 & (1u << 23))) { m32 <<= 1; ++shift; }
        m32 &= 0x7FFFFFu;
        int e64 = (int)1 - 127 + 1023 - shift;
        dbl_u r = { .u = sign64 | ((u64)e64 << 52) | ((u64)m32 << 29) };
        return r.d;
    }
    int e64 = (int)e32 - 127 + 1023;
    dbl_u r = { .u = sign64 | ((u64)e64 << 52) | ((u64)m32 << 29) };
    return r.d;
}

float __truncdfsf2(double a) {
    dbl_u da = { .d = a };
    u64 ux = da.u;
    u32 sign = (u32)(ux >> 32) & 0x80000000u;
    int e64  = (int)((ux >> 52) & 0x7FF);
    u64 m64  = ux & 0x000FFFFFFFFFFFFFULL;
    if (e64 == 0x7FF) {

        u32 m32 = (u32)(m64 >> 29);
        if (m64 != 0 && m32 == 0) m32 = 1;
        flt_u r = { .u = sign | (0xFFu << 23) | m32 };
        return r.f;
    }
    int e32 = e64 - 1023 + 127;
    if (e32 >= 0xFF) {
        flt_u r = { .u = sign | (0xFFu << 23) };
        return r.f;
    }
    if (e32 <= 0) {
        flt_u r = { .u = sign };
        return r.f;
    }

    u32 m32 = (u32)(m64 >> 29);
    u32 dropped = (u32)(m64 & 0x1FFFFFFFu);
    unsigned g = (dropped >> 28) & 1;
    unsigned r_bit = (dropped >> 27) & 1;
    unsigned s_bit = (dropped & 0x07FFFFFFu) ? 1u : 0u;
    u64 rounded = _round_to_even((u64)m32, g, r_bit, s_bit);
    if (rounded >= 0x00800000ULL) {
        rounded >>= 1;
        ++e32;
        if (e32 >= 0xFF) {
            flt_u r = { .u = sign | (0xFFu << 23) };
            return r.f;
        }
    }
    flt_u r = { .u = sign | ((u32)e32 << 23) | (u32)(rounded & 0x7FFFFFu) };
    return r.f;
}

double __floatsidf(int a) {
    if (a == 0) { dbl_u r = { .u = 0 }; return r.d; }
    u32 sign = 0;
    u32 ua;
    if (a < 0) { sign = 1; ua = _u32_abs(a); } else { ua = (u32)a; }
    int hi = 31;
    while (!(ua & (1u << hi))) --hi;
    int exp = hi + 1023;
    u64 m = ((u64)ua) << (52 - hi);
    dbl_u r = { .u = ((u64)sign << 63) | ((u64)exp << 52)
                     | (m & 0x000FFFFFFFFFFFFFULL) };
    return r.d;
}

double __floatunsidf(unsigned int a) {
    if (a == 0) { dbl_u r = { .u = 0 }; return r.d; }
    int hi = 31;
    while (!(a & (1u << hi))) --hi;
    int exp = hi + 1023;
    u64 m = ((u64)a) << (52 - hi);
    dbl_u r = { .u = ((u64)exp << 52) | (m & 0x000FFFFFFFFFFFFFULL) };
    return r.d;
}

int __fixdfsi(double a) {
    dbl_u da = { .d = a };
    u64 ux = da.u;
    u32 sign = (u32)(ux >> 63);
    int exp = (int)((ux >> 52) & 0x7FF) - 1023;
    u64 m = (ux & 0x000FFFFFFFFFFFFFULL) | 0x0010000000000000ULL;
    if (exp < 0)  return 0;
    if (exp > 30) return sign ? (int)0x80000000 : 0x7FFFFFFF;
    u32 r = (exp >= 52) ? (u32)(m << (exp - 52))
                        : (u32)(m >> (52 - exp));
    return sign ? -(int)r : (int)r;
}

unsigned int __fixunsdfsi(double a) {
    dbl_u da = { .d = a };
    u64 ux = da.u;
    u32 sign = (u32)(ux >> 63);
    int exp = (int)((ux >> 52) & 0x7FF) - 1023;
    if (sign || exp < 0) return 0;
    if (exp > 31)        return 0xFFFFFFFF;
    u64 m = (ux & 0x000FFFFFFFFFFFFFULL) | 0x0010000000000000ULL;
    return (exp >= 52) ? (u32)(m << (exp - 52))
                       : (u32)(m >> (52 - exp));
}

static double _qnan(void) {
    dbl_u r = { .u = 0x7FF8000000000000ULL };
    return r.d;
}

static int _is_nan(u64 ux) {
    return ((ux >> 52) & 0x7FF) == 0x7FF
         && (ux & 0x000FFFFFFFFFFFFFULL) != 0;
}
static int _is_inf(u64 ux) {
    return ((ux >> 52) & 0x7FF) == 0x7FF
         && (ux & 0x000FFFFFFFFFFFFFULL) == 0;
}

double __adddf3(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    u64 ax = ua.u, bx = ub.u;
    u32 sa = (u32)(ax >> 63),       sb = (u32)(bx >> 63);
    int ea = (int)((ax >> 52) & 0x7FF), eb = (int)((bx >> 52) & 0x7FF);
    u64 ma = ax & 0x000FFFFFFFFFFFFFULL, mb = bx & 0x000FFFFFFFFFFFFFULL;
    if (_is_nan(ax) || _is_nan(bx)) return _qnan();
    if (ea == 0x7FF && eb == 0x7FF) {
        if (sa != sb) return _qnan();
        dbl_u r = { .u = ((u64)sa << 63) | (0x7FFULL << 52) };
        return r.d;
    }
    if (ea == 0x7FF) return a;
    if (eb == 0x7FF) return b;
    if (ea == 0) { if (ma == 0) return b; ea = 1; }
    else          ma |= 0x0010000000000000ULL;
    if (eb == 0) { if (mb == 0) return a; eb = 1; }
    else          mb |= 0x0010000000000000ULL;

    ma <<= 3; mb <<= 3;
    int e;
    u32 sticky_a = 0, sticky_b = 0;
    if (ea > eb) {
        int sh = ea - eb;

        if (sh > 63) {
            sticky_b = (mb != 0);
            mb = 0;
        } else {
            u64 lost = mb & ((sh < 64) ? ((1ULL << sh) - 1ULL) : ~0ULL);
            sticky_b = lost ? 1u : 0u;
            mb >>= sh;
        }
        e = ea;
    } else if (eb > ea) {
        int sh = eb - ea;
        if (sh > 63) {
            sticky_a = (ma != 0);
            ma = 0;
        } else {
            u64 lost = ma & ((sh < 64) ? ((1ULL << sh) - 1ULL) : ~0ULL);
            sticky_a = lost ? 1u : 0u;
            ma >>= sh;
        }
        e = eb;
    } else {
        e = ea;
    }
    ma |= sticky_a;
    mb |= sticky_b;

    u64 mr; u32 sr;
    if (sa == sb) {
        mr = ma + mb; sr = sa;
        if (mr & 0x0100000000000000ULL) {
            u64 lost = mr & 1ULL;
            mr = (mr >> 1) | lost;
            ++e;
        }
    } else if (ma >= mb) {
        mr = ma - mb; sr = sa;
    } else {
        mr = mb - ma; sr = sb;
    }
    if (mr == 0) {

        u32 sign_out = (sa == sb && sa == 1) ? 1u : 0u;
        dbl_u r = { .u = (u64)sign_out << 63 };
        return r.d;
    }
    while (!(mr & 0x0080000000000000ULL)) { mr <<= 1; --e; }
    unsigned g = (unsigned)((mr >> 2) & 1);
    unsigned r_bit = (unsigned)((mr >> 1) & 1);
    unsigned s_bit = (unsigned)(mr & 1);
    u64 m_round = (mr >> 3);
    m_round = _round_to_even(m_round, g, r_bit, s_bit);
    if (m_round & 0x0020000000000000ULL) {
        m_round >>= 1;
        ++e;
    }
    if (e <= 0)      { dbl_u r = { .u = (u64)sr << 63 }; return r.d; }
    if (e >= 0x7FF)  { dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) }; return r.d; }
    dbl_u r = { .u = ((u64)sr << 63) | ((u64)e << 52) |
                     (m_round & 0x000FFFFFFFFFFFFFULL) };
    return r.d;
}

double __subdf3(double a, double b) {
    dbl_u ub = { .d = b };
    ub.u ^= 0x8000000000000000ULL;
    return __adddf3(a, ub.d);
}

double __muldf3(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    u64 ax = ua.u, bx = ub.u;
    u32 sa = (u32)(ax >> 63), sb = (u32)(bx >> 63);
    int ea = (int)((ax >> 52) & 0x7FF), eb = (int)((bx >> 52) & 0x7FF);
    u64 ma = (ax & 0x000FFFFFFFFFFFFFULL);
    u64 mb = (bx & 0x000FFFFFFFFFFFFFULL);
    u32 sr = sa ^ sb;

    if (_is_nan(ax) || _is_nan(bx)) return _qnan();
    int a_is_inf = _is_inf(ax), b_is_inf = _is_inf(bx);
    int a_is_zero = (ea == 0 && ma == 0);
    int b_is_zero = (eb == 0 && mb == 0);

    if ((a_is_inf && b_is_zero) || (a_is_zero && b_is_inf)) return _qnan();
    if (a_is_inf || b_is_inf) {
        dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) };
        return r.d;
    }
    if (a_is_zero || b_is_zero) {
        dbl_u r = { .u = (u64)sr << 63 };
        return r.d;
    }

    if (ea == 0) { while (!(ma & 0x0010000000000000ULL)) { ma <<= 1; --ea; } ++ea; }
    else { ma |= 0x0010000000000000ULL; }
    if (eb == 0) { while (!(mb & 0x0010000000000000ULL)) { mb <<= 1; --eb; } ++eb; }
    else { mb |= 0x0010000000000000ULL; }

    u32 a_hi = (u32)(ma >> 32);
    u32 a_lo = (u32)ma;
    u32 b_hi = (u32)(mb >> 32);
    u32 b_lo = (u32)mb;
    u64 ll = __umulsidi3(a_lo, b_lo);
    u64 lh = __umulsidi3(a_lo, b_hi);
    u64 hl = __umulsidi3(a_hi, b_lo);
    u64 hh = __umulsidi3(a_hi, b_hi);
    u64 mid = lh + hl;
    u64 low = ll + (mid << 32);
    u64 carry_low = (low < ll) ? 1ULL : 0ULL;
    u64 high = hh + (mid >> 32) + carry_low;
    int e = ea + eb - 1023;
    u64 mr;
    unsigned g, r_bit, s_bit;
    if (high & (1ULL << 41)) {
        mr = (high << 11) | (low >> 53);
        g = (unsigned)((low >> 52) & 1);
        r_bit = (unsigned)((low >> 51) & 1);
        s_bit = (low & ((1ULL << 51) - 1ULL)) ? 1u : 0u;
        ++e;
    } else {
        mr = (high << 12) | (low >> 52);
        g = (unsigned)((low >> 51) & 1);
        r_bit = (unsigned)((low >> 50) & 1);
        s_bit = (low & ((1ULL << 50) - 1ULL)) ? 1u : 0u;
    }
    mr = _round_to_even(mr, g, r_bit, s_bit);
    if (mr & 0x0020000000000000ULL) {
        mr >>= 1;
        ++e;
    }
    if (e <= 0)     { dbl_u r = { .u = (u64)sr << 63 }; return r.d; }
    if (e >= 0x7FF) { dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) }; return r.d; }
    dbl_u r = { .u = ((u64)sr << 63) | ((u64)e << 52) |
                     (mr & 0x000FFFFFFFFFFFFFULL) };
    return r.d;
}

double __divdf3(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    u64 ax = ua.u, bx = ub.u;
    u32 sa = (u32)(ax >> 63), sb = (u32)(bx >> 63);
    int ea = (int)((ax >> 52) & 0x7FF), eb = (int)((bx >> 52) & 0x7FF);
    u64 ma = (ax & 0x000FFFFFFFFFFFFFULL);
    u64 mb = (bx & 0x000FFFFFFFFFFFFFULL);
    u32 sr = sa ^ sb;

    if (_is_nan(ax) || _is_nan(bx)) return _qnan();
    int a_is_inf = _is_inf(ax), b_is_inf = _is_inf(bx);
    int a_is_zero = (ea == 0 && ma == 0);
    int b_is_zero = (eb == 0 && mb == 0);
    if (a_is_inf && b_is_inf) return _qnan();
    if (a_is_zero && b_is_zero) return _qnan();
    if (b_is_zero) {
        dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) };
        return r.d;
    }
    if (a_is_zero) { dbl_u r = { .u = (u64)sr << 63 }; return r.d; }
    if (a_is_inf)  { dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) }; return r.d; }
    if (b_is_inf)  { dbl_u r = { .u = (u64)sr << 63 }; return r.d; }

    if (ea == 0) { while (!(ma & 0x0010000000000000ULL)) { ma <<= 1; --ea; } ++ea; }
    else         { ma |= 0x0010000000000000ULL; }
    if (eb == 0) { while (!(mb & 0x0010000000000000ULL)) { mb <<= 1; --eb; } ++eb; }
    else         { mb |= 0x0010000000000000ULL; }

    u64 q = 0;
    if (ma < mb) { ma <<= 1; --ea; }
    for (int i = 53; i >= 0; --i) {
        if (ma >= mb) { ma -= mb; q |= (1ULL << i); }
        ma <<= 1;
    }

    unsigned g = (ma >= mb) ? 1u : 0u;
    if (g) ma -= mb;
    unsigned s_bit = (ma != 0) ? 1u : 0u;
    unsigned r_bit_lo = (unsigned)(q & 1u);
    q >>= 1;
    q = _round_to_even(q, g, r_bit_lo, s_bit);
    int e = ea - eb + 1023;
    if (q & 0x0020000000000000ULL) {
        q >>= 1;
        ++e;
    }
    if (e <= 0)     { dbl_u r = { .u = (u64)sr << 63 }; return r.d; }
    if (e >= 0x7FF) { dbl_u r = { .u = ((u64)sr << 63) | (0x7FFULL << 52) }; return r.d; }
    dbl_u r = { .u = ((u64)sr << 63) | ((u64)e << 52) |
                     (q & 0x000FFFFFFFFFFFFFULL) };
    return r.d;
}

int __eqdf2(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    if (_is_nan(ua.u) || _is_nan(ub.u)) return 1;
    if (ua.u == ub.u) return 0;

    if (((ua.u | ub.u) & ~0x8000000000000000ULL) == 0) return 0;
    return 1;
}

int __nedf2(double a, double b) { return __eqdf2(a, b); }

int __ltdf2(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    if (_is_nan(ua.u) || _is_nan(ub.u)) return 1;

    if (((ua.u | ub.u) & ~0x8000000000000000ULL) == 0) return 0;
    u32 sa = (u32)(ua.u >> 63), sb = (u32)(ub.u >> 63);
    if (sa != sb)                  return sa ? -1 : 1;
    if (sa == 0)                   return (ua.u < ub.u) ? -1 :
                                          (ua.u > ub.u) ? 1 : 0;

    return (ua.u > ub.u) ? -1 : (ua.u < ub.u) ? 1 : 0;
}
int __ledf2(double a, double b) { return __ltdf2(a, b); }
int __gtdf2(double a, double b) { return __ltdf2(a, b); }
int __gedf2(double a, double b) { return __ltdf2(a, b); }

int __unorddf2(double a, double b) {
    dbl_u ua = { .d = a }, ub = { .d = b };
    return (_is_nan(ua.u) || _is_nan(ub.u)) ? 1 : 0;
}

double cssc_sqrt(double x) {
    dbl_u ux = { .d = x };

    if (_is_nan(ux.u)) return x;
    if ((ux.u >> 63) & 1) {
        dbl_u nan_u; nan_u.u = 0x7FF8000000000000ULL;
        return nan_u.d;
    }

    if ((ux.u & ~0x8000000000000000ULL) == 0) return 0.0;

    int exp_biased = (int)((ux.u >> 52) & 0x7FF);
    int e = exp_biased - 1023;
    dbl_u seed;
    seed.u = (uint64_t)((1023 + (e >> 1)) & 0x7FF) << 52;
    double y = seed.d;

    for (int i = 0; i < 8; ++i) {
        double q = x / y;
        y = 0.5 * (y + q);
    }
    return y;
}

static uint64_t _cssc_rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t _cssc_rng_next(void) {
    uint64_t x = _cssc_rng_state;
    if (x == 0) x = 0x9E3779B97F4A7C15ULL;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    _cssc_rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

double cssc_random(void) {
    uint64_t r = _cssc_rng_next() >> 11;
    return (double)r * (1.0 / (double)(1ULL << 53));
}

long long cssc_random_int(long long a, long long b) {
    if (b < a) return a;
    unsigned long long range = (unsigned long long)(b - a) + 1ULL;
    unsigned long long r = _cssc_rng_next();
    return a + (long long)(r % range);
}
