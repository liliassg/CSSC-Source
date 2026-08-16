/*
 * cssc_math_accel.c — C runtime for the math.accel CSSC module.
 *
 * Ported from includecpp/core/cssl/cpp/include/cssc_math_accel.cpp so the
 * v6-native backend does not depend on the interpreter-side Python/pyd
 * accelerator. Every function is exported with C linkage as `cssc_ma_*`;
 * the dispatch table in cir_lower.py maps `math.accel::<name>` calls to
 * these symbols directly.
 *
 * All routines are pure — no allocation, no I/O, no globals. Deterministic
 * for identical inputs (splitmix hashing, no time-seeded RNG).
 */

#include <stdint.h>
#include <stddef.h>
#include <math.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
    #define CSSC_MA_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
    #define CSSC_MA_EXPORT __attribute__((visibility("default")))
#else
    #define CSSC_MA_EXPORT
#endif

/* ---------------------------------------------------------------------------
 * deterministic 32-bit / 64-bit mixers
 * ------------------------------------------------------------------------- */
static inline uint32_t ma_splitmix32(uint32_t z) {
    z += 0x9E3779B9u;
    z = (z ^ (z >> 16)) * 0x85EBCA6Bu;
    z = (z ^ (z >> 13)) * 0xC2B2AE35u;
    return z ^ (z >> 16);
}

static inline uint64_t ma_splitmix64(uint64_t z) {
    z += 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* ---------------------------------------------------------------------------
 * scalar helpers (fade / lerp / clamp / smoothstep / smootherstep)
 * ------------------------------------------------------------------------- */
static inline double ma_fade_inline(double t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

static inline double ma_lerp_inline(double a, double b, double t) {
    return a + (b - a) * t;
}

CSSC_MA_EXPORT double cssc_ma_fade(double t) { return ma_fade_inline(t); }

CSSC_MA_EXPORT double cssc_ma_lerpf(double a, double b, double t) {
    return ma_lerp_inline(a, b, t);
}

CSSC_MA_EXPORT double cssc_ma_clampf(double v, double lo, double hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

CSSC_MA_EXPORT double cssc_ma_smoothstep(double e0, double e1, double x) {
    if (e1 == e0) return 0.0;
    double t = (x - e0) / (e1 - e0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return t * t * (3.0 - 2.0 * t);
}

CSSC_MA_EXPORT double cssc_ma_smootherstep(double e0, double e1, double x) {
    if (e1 == e0) return 0.0;
    double t = (x - e0) / (e1 - e0);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return ma_fade_inline(t);
}

CSSC_MA_EXPORT double cssc_ma_floorf(double v) { return floor(v); }
CSSC_MA_EXPORT int64_t cssc_ma_floori(double v) { return (int64_t)floor(v); }
CSSC_MA_EXPORT double cssc_ma_ceilf(double v)  { return ceil(v); }
CSSC_MA_EXPORT double cssc_ma_roundf(double v) { return round(v); }
CSSC_MA_EXPORT double cssc_ma_absf(double v)   { return fabs(v); }
CSSC_MA_EXPORT double cssc_ma_sqrtf(double v)  { return sqrt(v); }
CSSC_MA_EXPORT double cssc_ma_sinf(double v)   { return sin(v); }
CSSC_MA_EXPORT double cssc_ma_cosf(double v)   { return cos(v); }
CSSC_MA_EXPORT double cssc_ma_tanf(double v)   { return tan(v); }
CSSC_MA_EXPORT double cssc_ma_atan2f(double y, double x) { return atan2(y, x); }
CSSC_MA_EXPORT double cssc_ma_powf(double a, double b)   { return pow(a, b); }
CSSC_MA_EXPORT double cssc_ma_expf(double v)   { return exp(v); }
CSSC_MA_EXPORT double cssc_ma_logf(double v)   { return log(v); }
CSSC_MA_EXPORT double cssc_ma_minf(double a, double b) { return a < b ? a : b; }
CSSC_MA_EXPORT double cssc_ma_maxf(double a, double b) { return a > b ? a : b; }

/* ---------------------------------------------------------------------------
 * hashes
 * ------------------------------------------------------------------------- */
CSSC_MA_EXPORT int64_t cssc_ma_hash2(int64_t a, int64_t b) {
    uint64_t h = 0xCBF29CE484222325ULL;
    h ^= (uint64_t)a; h *= 0x100000001B3ULL;
    h ^= (uint64_t)b; h *= 0x100000001B3ULL;
    return (int64_t)(ma_splitmix64(h) & 0x7FFFFFFFFFFFFFFFULL);
}

CSSC_MA_EXPORT int64_t cssc_ma_hash3(int64_t a, int64_t b, int64_t c) {
    uint64_t h = 0xCBF29CE484222325ULL;
    h ^= (uint64_t)a; h *= 0x100000001B3ULL;
    h ^= (uint64_t)b; h *= 0x100000001B3ULL;
    h ^= (uint64_t)c; h *= 0x100000001B3ULL;
    return (int64_t)(ma_splitmix64(h) & 0x7FFFFFFFFFFFFFFFULL);
}

CSSC_MA_EXPORT int64_t cssc_ma_hash4(int64_t a, int64_t b, int64_t c, int64_t d) {
    uint64_t h = 0xCBF29CE484222325ULL;
    h ^= (uint64_t)a; h *= 0x100000001B3ULL;
    h ^= (uint64_t)b; h *= 0x100000001B3ULL;
    h ^= (uint64_t)c; h *= 0x100000001B3ULL;
    h ^= (uint64_t)d; h *= 0x100000001B3ULL;
    return (int64_t)(ma_splitmix64(h) & 0x7FFFFFFFFFFFFFFFULL);
}

CSSC_MA_EXPORT double cssc_ma_hashUnit(int64_t x, int64_t y) {
    uint64_t h = ma_splitmix64((uint64_t)x * 0x9E3779B97F4A7C15ULL
                                ^ (uint64_t)y * 0xBF58476D1CE4E5B9ULL);
    return (double)(h & 0x1FFFFFFFFFFFFFULL) / 9007199254740992.0;
}

CSSC_MA_EXPORT double cssc_ma_hashUnit3(int64_t x, int64_t y, int64_t seed) {
    uint64_t h = ma_splitmix64((uint64_t)x * 0x9E3779B97F4A7C15ULL
                                ^ (uint64_t)y * 0xBF58476D1CE4E5B9ULL
                                ^ (uint64_t)seed * 0x94D049BB133111EBULL);
    return (double)(h & 0x1FFFFFFFFFFFFFULL) / 9007199254740992.0;
}

/* ---------------------------------------------------------------------------
 * value-noise 2D + fBm sum
 * ------------------------------------------------------------------------- */
CSSC_MA_EXPORT double cssc_ma_valueNoise2(double x, double y, int64_t seed) {
    int64_t x0 = (int64_t)floor(x);
    int64_t y0 = (int64_t)floor(y);
    double fx = x - (double)x0;
    double fy = y - (double)y0;

    double v00 = cssc_ma_hashUnit3(x0,     y0,     seed);
    double v10 = cssc_ma_hashUnit3(x0 + 1, y0,     seed);
    double v01 = cssc_ma_hashUnit3(x0,     y0 + 1, seed);
    double v11 = cssc_ma_hashUnit3(x0 + 1, y0 + 1, seed);

    double u = ma_fade_inline(fx);
    double v = ma_fade_inline(fy);
    double a = ma_lerp_inline(v00, v10, u);
    double b = ma_lerp_inline(v01, v11, u);
    return ma_lerp_inline(a, b, v);
}

CSSC_MA_EXPORT double cssc_ma_fbm2(double x, double y, int64_t octaves,
                                     double lacunarity, double gain,
                                     int64_t seed) {
    if (octaves < 1) octaves = 1;
    if (octaves > 16) octaves = 16;
    double sum = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double norm = 0.0;
    for (int64_t o = 0; o < octaves; ++o) {
        sum += cssc_ma_valueNoise2(x * freq, y * freq, seed + o * 1013) * amp;
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return norm > 0.0 ? sum / norm : 0.0;
}

/* ---------------------------------------------------------------------------
 * Perlin-style gradient noise (2D) — 8-direction gradients, quintic fade
 * ------------------------------------------------------------------------- */
static inline double ma_grad_dot(int64_t hx, int64_t hy, int64_t seed,
                                    double dx, double dy) {
    uint64_t h = ma_splitmix64((uint64_t)hx * 0x9E3779B97F4A7C15ULL
                                ^ (uint64_t)hy * 0xBF58476D1CE4E5B9ULL
                                ^ (uint64_t)seed);
    switch ((int)(h & 7)) {
        case 0: return  dx +  dy;
        case 1: return -dx +  dy;
        case 2: return  dx -  dy;
        case 3: return -dx -  dy;
        case 4: return  dx;
        case 5: return -dx;
        case 6: return  dy;
        default: return -dy;
    }
}

CSSC_MA_EXPORT double cssc_ma_perlin2(double x, double y, int64_t seed) {
    int64_t x0 = (int64_t)floor(x);
    int64_t y0 = (int64_t)floor(y);
    double fx = x - (double)x0;
    double fy = y - (double)y0;

    double n00 = ma_grad_dot(x0,     y0,     seed, fx,       fy);
    double n10 = ma_grad_dot(x0 + 1, y0,     seed, fx - 1.0, fy);
    double n01 = ma_grad_dot(x0,     y0 + 1, seed, fx,       fy - 1.0);
    double n11 = ma_grad_dot(x0 + 1, y0 + 1, seed, fx - 1.0, fy - 1.0);

    double u = ma_fade_inline(fx);
    double v = ma_fade_inline(fy);
    double a = ma_lerp_inline(n00, n10, u);
    double b = ma_lerp_inline(n01, n11, u);
    double n = ma_lerp_inline(a, b, v);
    return 0.5 * (n + 1.0);
}

CSSC_MA_EXPORT double cssc_ma_perlinFbm2(double x, double y, int64_t octaves,
                                            double lacunarity, double gain,
                                            int64_t seed) {
    if (octaves < 1) octaves = 1;
    if (octaves > 16) octaves = 16;
    double sum = 0.0;
    double amp = 1.0;
    double freq = 1.0;
    double norm = 0.0;
    for (int64_t o = 0; o < octaves; ++o) {
        sum += cssc_ma_perlin2(x * freq, y * freq, seed + o * 1013) * amp;
        norm += amp;
        amp *= gain;
        freq *= lacunarity;
    }
    return norm > 0.0 ? sum / norm : 0.0;
}

/* ---------------------------------------------------------------------------
 * worldgen classifier — matches the Python-side tile-id policy
 *   0 grass, 1 rock/tree (solid), 2 dirt/desert, 3 water (solid)
 * ------------------------------------------------------------------------- */
CSSC_MA_EXPORT int64_t cssc_ma_worldgen_tile_at(double x, double y,
                                                  double el_scale, int64_t el_oct,
                                                  double mo_scale, int64_t mo_oct,
                                                  double de_scale, int64_t de_oct,
                                                  int64_t seed) {
    double elev   = cssc_ma_fbm2(x * el_scale, y * el_scale, el_oct, 2.0, 0.5,
                                   seed + 1000);
    double moist  = cssc_ma_fbm2(x * mo_scale, y * mo_scale, mo_oct, 2.0, 0.5,
                                   seed + 2000);
    double detail = cssc_ma_fbm2(x * de_scale, y * de_scale, de_oct, 2.0, 0.5,
                                   seed + 3000);
    int64_t tile = 0;
    if (moist  < 0.44) tile = 2;
    if (detail > 0.66) tile = 1;
    if (elev   < 0.42) tile = 3;
    return tile;
}

CSSC_MA_EXPORT int64_t cssc_ma_worldgen_is_solid_at(double x, double y,
                                                      double el_scale, int64_t el_oct,
                                                      double mo_scale, int64_t mo_oct,
                                                      double de_scale, int64_t de_oct,
                                                      int64_t seed) {
    int64_t t = cssc_ma_worldgen_tile_at(x, y, el_scale, el_oct, mo_scale, mo_oct,
                                          de_scale, de_oct, seed);
    return (t == 1 || t == 3) ? 1 : 0;
}

/* ---------------------------------------------------------------------------
 * availability marker — v6-native always ships the runtime, so this is 1.
 * (The interpreter side returns 0 if the pyd is missing; here the .o is
 * linked unconditionally, so functionality is guaranteed.)
 * ------------------------------------------------------------------------- */
CSSC_MA_EXPORT int64_t cssc_ma_available(void) { return 1; }
