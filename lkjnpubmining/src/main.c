#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

// --- Part 1: 256-Bit Unsigned Integer Arithmetic ---

// Represents a 256-bit unsigned integer as four 64-bit limbs (little-endian).
typedef struct {
    uint64_t limbs[4];
} uint256_t;

// Function declarations for uint256_t operations
void uint256_from_hex(uint256_t* n, const char* hex);
void uint256_to_hex(const uint256_t* n, char* out);
void uint256_clear(uint256_t* n);
bool uint256_is_zero(const uint256_t* n);
int uint256_cmp(const uint256_t* a, const uint256_t* b);
void uint256_add(uint256_t* res, const uint256_t* a, const uint256_t* b);
void uint256_sub(uint256_t* res, const uint256_t* a, const uint256_t* b);
void uint256_mul(uint256_t* res, const uint256_t* a, const uint256_t* b);
void uint256_div_mod(uint256_t* q, uint256_t* r, const uint256_t* a, const uint256_t* b);
void uint256_mod_add(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod);
void uint256_mod_sub(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod);
void uint256_mod_mul(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod);
void uint256_mod_inverse(uint256_t* res, const uint256_t* a, const uint256_t* mod);
int uint256_bit_length(const uint256_t* n);
bool uint256_get_bit(const uint256_t* n, int bit);
void uint256_to_bytes(uint8_t out[32], const uint256_t* n);
void uint256_from_bytes(uint256_t* n, const uint8_t in[32]);


// --- Part 2: secp256k1 Elliptic Curve Cryptography ---

// secp256k1 curve parameters
static uint256_t P, N, Gx, Gy;

// Point representations
typedef struct {
    uint256_t x;
    uint256_t y;
    bool is_infinity;
} AffinePoint;

typedef struct {
    uint256_t X;
    uint256_t Y;
    uint256_t Z;
} JacobianPoint;

// Function declarations for ECC operations
void jacobian_to_affine(AffinePoint* res, const JacobianPoint* p);
void point_double_jacobian(JacobianPoint* res, const JacobianPoint* p);
void point_add_mixed(JacobianPoint* res, const JacobianPoint* p, const AffinePoint* q);
void scalar_multiply(AffinePoint* res, const uint256_t* scalar, const AffinePoint* p);

// --- Part 3: Bech32 Encoding ---

// Function declarations for Bech32
int bech32_encode(char* output, const char* hrp, const uint8_t* data, size_t data_len);

// --- Part 4: PRNG (xorshift128+) ---

typedef struct {
    uint64_t s[2];
} xorshift128p_state;

void xorshift128p_seed(xorshift128p_state* state);
uint64_t xorshift128p_next(xorshift128p_state* state);
void generate_random_bytes(xorshift128p_state* state, uint8_t* out, size_t len);

// --- Implementation Details ---

// --- 256-Bit Arithmetic Implementation ---

void uint256_from_hex(uint256_t* n, const char* hex) {
    uint256_clear(n);
    if (hex == NULL) return;
    size_t len = strlen(hex);
    if (len == 0) return;

    // Limbs are little-endian, hex is big-endian.
    // So we parse from the end of the string.
    for (int i = 0; i < 4; ++i) { // For each limb
        long long hex_pos = (long long)len - (i + 1) * 16;
        int num_chars = 16;
        if (hex_pos < 0) {
            num_chars += hex_pos;
            hex_pos = 0;
        }
        if (num_chars <= 0) break;
        
        char buf[17] = {0};
        strncpy(buf, hex + hex_pos, num_chars);
        n->limbs[i] = strtoull(buf, NULL, 16);
    }
}

void uint256_to_hex(const uint256_t* n, char* out) {
    sprintf(out, "%016llx%016llx%016llx%016llx",
        (unsigned long long)n->limbs[3],
        (unsigned long long)n->limbs[2],
        (unsigned long long)n->limbs[1],
        (unsigned long long)n->limbs[0]);
}

void uint256_clear(uint256_t* n) {
    memset(n->limbs, 0, sizeof(n->limbs));
}

bool uint256_is_zero(const uint256_t* n) {
    return (n->limbs[0] == 0 && n->limbs[1] == 0 && n->limbs[2] == 0 && n->limbs[3] == 0);
}

int uint256_cmp(const uint256_t* a, const uint256_t* b) {
    for (int i = 3; i >= 0; --i) {
        if (a->limbs[i] > b->limbs[i]) return 1;
        if (a->limbs[i] < b->limbs[i]) return -1;
    }
    return 0;
}

void uint256_add(uint256_t* res, const uint256_t* a, const uint256_t* b) {
    unsigned __int128 carry = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned __int128 sum = (unsigned __int128)a->limbs[i] + b->limbs[i] + carry;
        res->limbs[i] = (uint64_t)sum;
        carry = sum >> 64;
    }
}

void uint256_sub(uint256_t* res, const uint256_t* a, const uint256_t* b) {
    unsigned __int128 borrow = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned __int128 diff = (unsigned __int128)a->limbs[i] - b->limbs[i] - borrow;
        res->limbs[i] = (uint64_t)diff;
        borrow = (diff >> 127); // Check for borrow
    }
}

void uint256_mul(uint256_t* res, const uint256_t* a, const uint256_t* b) {
    uint64_t p[8] = {0}; // 512-bit intermediate product
    for (int i = 0; i < 4; ++i) {
        unsigned __int128 carry = 0;
        for (int j = 0; j < 4; ++j) {
            unsigned __int128 prod = (unsigned __int128)a->limbs[i] * b->limbs[j] + p[i + j] + carry;
            p[i + j] = (uint64_t)prod;
            carry = prod >> 64;
        }
        p[i + 4] += carry;
    }
    for (int i = 0; i < 4; ++i) res->limbs[i] = p[i];
}

int uint256_bit_length(const uint256_t* n) {
    for (int i = 3; i >= 0; --i) {
        if (n->limbs[i]!= 0) {
            int len = 64;
            uint64_t val = n->limbs[i];
            while (len > 0 && (val & (1ULL << (len - 1))) == 0) {
                len--;
            }
            return i * 64 + len;
        }
    }
    return 0;
}

bool uint256_get_bit(const uint256_t* n, int bit) {
    if (bit < 0 || bit >= 256) return false;
    return (n->limbs[bit / 64] >> (bit % 64)) & 1;
}

void uint256_lshift(uint256_t* n, int shift) {
    if (shift == 0) return;
    int limb_shift = shift / 64;
    int bit_shift = shift % 64;

    if (limb_shift > 0) {
        for (int i = 3; i >= limb_shift; --i) {
            n->limbs[i] = n->limbs[i - limb_shift];
        }
        for (int i = 0; i < limb_shift; ++i) {
            n->limbs[i] = 0;
        }
    }

    if (bit_shift > 0) {
        for (int i = 3; i > 0; --i) {
            n->limbs[i] = (n->limbs[i] << bit_shift) | (n->limbs[i - 1] >> (64 - bit_shift));
        }
        n->limbs[0] <<= bit_shift;
    }
}

void uint256_rshift(uint256_t* n, int shift) {
    if (shift == 0) return;
    int limb_shift = shift / 64;
    int bit_shift = shift % 64;

    if (limb_shift > 0) {
        for (int i = 0; i < 4 - limb_shift; ++i) {
            n->limbs[i] = n->limbs[i + limb_shift];
        }
        for (int i = 4 - limb_shift; i < 4; ++i) {
            n->limbs[i] = 0;
        }
    }
    
    if (bit_shift > 0) {
        for (int i = 0; i < 3; ++i) {
            n->limbs[i] = (n->limbs[i] >> bit_shift) | (n->limbs[i + 1] << (64 - bit_shift));
        }
        n->limbs[3] >>= bit_shift;
    }
}

void uint256_div_mod(uint256_t* q, uint256_t* r, const uint256_t* a, const uint256_t* b) {
    uint256_clear(q);
    *r = *a;
    if (uint256_is_zero(b)) {
        // Division by zero, handle error (e.g., set q and r to max)
        return;
    }
    if (uint256_cmp(a, b) < 0) {
        return;
    }
    
    int a_len = uint256_bit_length(a);
    int b_len = uint256_bit_length(b);
    int diff = a_len - b_len;
    
    uint256_t temp_b = *b;
    uint256_lshift(&temp_b, diff);

    for (int i = diff; i >= 0; --i) {
        if (uint256_cmp(r, &temp_b) >= 0) {
            uint256_sub(r, r, &temp_b);
            uint256_t one;
            uint256_clear(&one);
            one.limbs[0] = 1;
            uint256_lshift(&one, i);
            uint256_add(q, q, &one);
        }
        uint256_rshift(&temp_b, 1);
    }
}

void uint256_mod_add(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod) {
    uint256_add(res, a, b);
    if (uint256_cmp(res, mod) >= 0) {
        uint256_sub(res, res, mod);
    }
}

void uint256_mod_sub(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod) {
    if (uint256_cmp(a, b) < 0) {
        uint256_t temp;
        uint256_add(&temp, a, mod);
        uint256_sub(res, &temp, b);
    } else {
        uint256_sub(res, a, b);
    }
}

void uint256_mod_mul(uint256_t* res, const uint256_t* a, const uint256_t* b, const uint256_t* mod) {
    // Slower but correct Russian Peasant Multiplication for modular multiplication
    uint256_t temp_a = *a;
    uint256_t temp_b = *b;
    uint256_clear(res);
    
    // Ensure temp_a is less than mod
    if (uint256_cmp(&temp_a, mod) >= 0) {
        uint256_t dummy_q;
        uint256_div_mod(&dummy_q, &temp_a, &temp_a, mod);
    }

    for(int i = 0; i < 256; ++i) {
        if (uint256_get_bit(&temp_b, i)) {
            uint256_mod_add(res, res, &temp_a, mod);
        }
        uint256_mod_add(&temp_a, &temp_a, &temp_a, mod);
    }
}

void uint256_mod_inverse(uint256_t* res, const uint256_t* a, const uint256_t* mod) {
    // Extended Euclidean Algorithm
    uint256_t t, new_t, r, new_r, q, temp;
    uint256_clear(&t);
    uint256_clear(&new_t); new_t.limbs[0] = 1;
    r = *mod;
    new_r = *a;

    while (!uint256_is_zero(&new_r)) {
        uint256_t remainder;
        uint256_div_mod(&q, &remainder, &r, &new_r);
        
        r = new_r;
        new_r = remainder;

        uint256_mod_mul(&temp, &q, &t, mod);
        uint256_mod_sub(&temp, &new_t, &temp, mod);
        new_t = t;
        t = temp;
    }

    const uint256_t one = {.limbs={1,0,0,0}};
    if (uint256_cmp(&r, &one) > 0) {
        uint256_clear(res); // No inverse
    } else {
        *res = new_t;
    }
}

void uint256_to_bytes(uint8_t out[32], const uint256_t* n) {
    for (int i = 0; i < 4; ++i) { // for each limb
        uint64_t limb = n->limbs[3 - i]; // start with most significant limb
        for (int j = 0; j < 8; ++j) { // for each byte in the limb
            out[i * 8 + j] = (limb >> (56 - j * 8)) & 0xFF;
        }
    }
}

void uint256_from_bytes(uint256_t* n, const uint8_t in[32]) {
    uint256_clear(n);
    for (int i = 0; i < 4; ++i) { // for each limb
        for (int j = 0; j < 8; ++j) { // for each byte
            n->limbs[3 - i] |= (uint64_t)in[i * 8 + j] << (56 - j * 8);
        }
    }
}


// --- secp256k1 Implementation ---

void init_secp256k1_params() {
    uint256_from_hex(&P, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
    uint256_from_hex(&N, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
    uint256_from_hex(&Gx, "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
    uint256_from_hex(&Gy, "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");
}

void jacobian_to_affine(AffinePoint* res, const JacobianPoint* p) {
    if (uint256_is_zero(&p->Z)) {
        res->is_infinity = true;
        return;
    }
    res->is_infinity = false;
    uint256_t z_inv, z_inv2, z_inv3;
    uint256_mod_inverse(&z_inv, &p->Z, &P);
    uint256_mod_mul(&z_inv2, &z_inv, &z_inv, &P);
    uint256_mod_mul(&z_inv3, &z_inv2, &z_inv, &P);
    uint256_mod_mul(&res->x, &p->X, &z_inv2, &P);
    uint256_mod_mul(&res->y, &p->Y, &z_inv3, &P);
}

void point_double_jacobian(JacobianPoint* res, const JacobianPoint* p) {
    if (uint256_is_zero(&p->Y) || uint256_is_zero(&p->Z)) {
        uint256_clear(&res->X); uint256_clear(&res->Y); uint256_clear(&res->Z);
        return;
    }
    uint256_t S, M, T, Y2;
    uint256_mod_mul(&Y2, &p->Y, &p->Y, &P);
    uint256_mod_mul(&S, &p->X, &Y2, &P);
    uint256_mod_add(&S, &S, &S, &P);
    uint256_mod_add(&S, &S, &S, &P);

    uint256_t X2;
    uint256_mod_mul(&X2, &p->X, &p->X, &P);
    uint256_mod_add(&M, &X2, &X2, &P);
    uint256_mod_add(&M, &M, &X2, &P); // M = 3*X^2 (a=0)

    uint256_mod_mul(&T, &M, &M, &P);
    uint256_t S2;
    uint256_mod_add(&S2, &S, &S, &P);
    uint256_mod_sub(&res->X, &T, &S2, &P);

    uint256_mod_sub(&T, &S, &res->X, &P);
    uint256_mod_mul(&T, &M, &T, &P);
    uint256_t Y4;
    uint256_mod_mul(&Y4, &Y2, &Y2, &P);
    uint256_mod_add(&Y4, &Y4, &Y4, &P);
    uint256_mod_add(&Y4, &Y4, &Y4, &P);
    uint256_mod_add(&Y4, &Y4, &Y4, &P);
    uint256_mod_sub(&res->Y, &T, &Y4, &P);

    uint256_mod_add(&res->Z, &p->Y, &p->Y, &P);
    uint256_mod_mul(&res->Z, &res->Z, &p->Z, &P);
}

void point_add_mixed(JacobianPoint* res, const JacobianPoint* p, const AffinePoint* q) {
    if (q->is_infinity) {
        *res = *p;
        return;
    }
    if (uint256_is_zero(&p->Z)) {
        res->X = q->x; res->Y = q->y; uint256_clear(&res->Z); res->Z.limbs[0] = 1;
        return;
    }
    uint256_t Z1Z1, U2, S2, H, H2, H3, R, R2, U1H2;
    uint256_mod_mul(&Z1Z1, &p->Z, &p->Z, &P);
    uint256_mod_mul(&U2, &q->x, &Z1Z1, &P);
    uint256_mod_mul(&S2, &q->y, &p->Z, &P);
    uint256_mod_mul(&S2, &S2, &Z1Z1, &P);
    uint256_mod_sub(&H, &U2, &p->X, &P);
    uint256_mod_sub(&R, &S2, &p->Y, &P);

    if (uint256_is_zero(&H)) {
        if (uint256_is_zero(&R)) {
            point_double_jacobian(res, p);
        } else { // Point at infinity
            uint256_clear(&res->X); uint256_clear(&res->Y); uint256_clear(&res->Z);
        }
        return;
    }
    
    uint256_mod_mul(&H2, &H, &H, &P);
    uint256_mod_mul(&H3, &H2, &H, &P);
    uint256_mod_mul(&U1H2, &p->X, &H2, &P);
    uint256_mod_mul(&R2, &R, &R, &P);
    uint256_t temp;
    uint256_mod_add(&temp, &U1H2, &U1H2, &P);
    uint256_mod_add(&temp, &temp, &H3, &P);
    uint256_mod_sub(&res->X, &R2, &temp, &P);

    uint256_mod_sub(&temp, &U1H2, &res->X, &P);
    uint256_mod_mul(&temp, &R, &temp, &P);
    uint256_mod_mul(&R2, &p->Y, &H3, &P);
    uint256_mod_sub(&res->Y, &temp, &R2, &P);

    uint256_mod_mul(&res->Z, &p->Z, &H, &P);
}

void scalar_multiply(AffinePoint* res, const uint256_t* scalar, const AffinePoint* p) {
    JacobianPoint R;
    uint256_clear(&R.X); uint256_clear(&R.Y); uint256_clear(&R.Z); // Infinity point
    
    int bit_len = uint256_bit_length(scalar);
    for (int i = bit_len - 1; i >= 0; --i) {
        point_double_jacobian(&R, &R);
        if (uint256_get_bit(scalar, i)) {
            point_add_mixed(&R, &R, p);
        }
    }
    jacobian_to_affine(res, &R);
}

// --- Bech32 Implementation ---

static const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const uint32_t GEN[] = {0x3b6a57b2, 0x26508e6d, 0x1ea119fa, 0x3d4233dd, 0x2a1462b3};

uint32_t bech32_polymod(const uint8_t* values, size_t len) {
    uint32_t chk = 1;
    for (size_t i = 0; i < len; ++i) {
        uint8_t top = chk >> 25;
        chk = (chk & 0x1ffffff) << 5 ^ values[i];
        for (int j = 0; j < 5; ++j) {
            if ((top >> j) & 1) {
                chk ^= GEN[j];
            }
        }
    }
    return chk;
}

int convert_bits(uint8_t* out, size_t* outlen, int outbits, const uint8_t* in, size_t inlen, int inbits, int pad) {
    uint32_t val = 0;
    int bits = 0;
    uint32_t maxv = (1 << outbits) - 1;
    size_t pos = 0;
    for (size_t i = 0; i < inlen; ++i) {
        val = (val << inbits) | in[i];
        bits += inbits;
        while (bits >= outbits) {
            bits -= outbits;
            out[pos++] = (val >> bits) & maxv;
        }
    }
    if (pad) {
        if (bits) {
            out[pos++] = (val << (outbits - bits)) & maxv;
        }
    } else if (bits >= inbits || ((val << (outbits - bits)) & maxv)) {
        return 0;
    }
    *outlen = pos;
    return 1;
}

int bech32_encode(char* output, const char* hrp, const uint8_t* data, size_t data_len) {
    size_t hrp_len = strlen(hrp);
    size_t data5_len_max = (data_len * 8 + 4) / 5;
    uint8_t data5[data5_len_max];
    size_t data5_len;
    if (!convert_bits(data5, &data5_len, 5, data, data_len, 8, 1)) return 0;

    uint8_t polymod_data[hrp_len * 2 + 1 + data5_len + 6];
    size_t polymod_data_len = 0;

    for (size_t i = 0; i < hrp_len; ++i) polymod_data[polymod_data_len++] = hrp[i] >> 5;
    polymod_data[polymod_data_len++] = 0;
    for (size_t i = 0; i < hrp_len; ++i) polymod_data[polymod_data_len++] = hrp[i] & 0x1f;
    
    memcpy(polymod_data + polymod_data_len, data5, data5_len);
    polymod_data_len += data5_len;
    memset(polymod_data + polymod_data_len, 0, 6); // Placeholder for checksum
    
    uint32_t checksum = bech32_polymod(polymod_data, polymod_data_len + 6) ^ 1;
    
    strcpy(output, hrp);
    output[hrp_len] = '1';
    char* p = output + hrp_len + 1;
    
    for (size_t i = 0; i < data5_len; ++i) {
        *p++ = CHARSET[data5[i]];
    }
    for (int i = 0; i < 6; ++i) {
        *p++ = CHARSET[(checksum >> (5 * (5 - i))) & 0x1f];
    }
    *p = '\0';
    return 1;
}

// --- PRNG Implementation ---

void xorshift128p_seed(xorshift128p_state* state) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    uint64_t s1 = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
    // Simple splitmix64 to initialize state
    uint64_t z = (s1 += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    state->s[0] = z ^ (z >> 31);
    z = (s1 += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    state->s[1] = z ^ (z >> 31);
}

uint64_t xorshift128p_next(xorshift128p_state* state) {
    uint64_t s1 = state->s[0];
    const uint64_t s0 = state->s[1];
    const uint64_t result = s0 + s1;
    state->s[0] = s0;
    s1 ^= s1 << 23;
    state->s[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
    return result;
}

void generate_random_bytes(xorshift128p_state* state, uint8_t* out, size_t len) {
    for (size_t i = 0; i < len; i += 8) {
        uint64_t r = xorshift128p_next(state);
        size_t copy_len = (len - i < 8) ? len - i : 8;
        memcpy(out + i, &r, copy_len);
    }
}

// --- Main Application ---

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <prefix>\n", argv[0]);
        return 1;
    }
    const char* prefix = argv[1];
    char npub_prefix[128] = "npub1";
    strcat(npub_prefix, prefix);
    size_t full_prefix_len = strlen(npub_prefix);

    init_secp256k1_params();
    
    xorshift128p_state rng_state;
    xorshift128p_seed(&rng_state);

    uint64_t count = 0;
    struct timespec start_time, current_time;
    timespec_get(&start_time, TIME_UTC);

    printf("Searching for npub starting with '%s'...\n", npub_prefix);

    while (1) {
        uint8_t private_key_bytes[32];
        generate_random_bytes(&rng_state, private_key_bytes, 32);

        uint256_t private_key;
        uint256_from_bytes(&private_key, private_key_bytes);
        
        // Ensure private key is in valid range [1, N-1]
        if (uint256_is_zero(&private_key) || uint256_cmp(&private_key, &N) >= 0) {
            continue;
        }

        AffinePoint G = {Gx, Gy, false};
        AffinePoint public_key;
        scalar_multiply(&public_key, &private_key, &G);

        uint8_t public_key_bytes[32];
        uint256_to_bytes(public_key_bytes, &public_key.x);

        char npub[128];
        bech32_encode(npub, "npub", public_key_bytes, 32);

        if (strncmp(npub, npub_prefix, full_prefix_len) == 0) {
            printf("\n--- Found! ---\n");
            
            char nsec[128];
            bech32_encode(nsec, "nsec", private_key_bytes, 32);
            
            printf("nsec: %s\n", nsec);
            printf("npub: %s\n", npub);
            
            timespec_get(&current_time, TIME_UTC);
            double elapsed = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
            printf("Total keys processed: %llu\n", (unsigned long long)count);
            if (elapsed > 0) {
                printf("Time elapsed: %.2f seconds\n", elapsed);
                printf("Keys per second: %.0f\n", count / elapsed);
            }
            break;
        }

        count++;
        if (count % 100000 == 0) {
            timespec_get(&current_time, TIME_UTC);
            double elapsed = (current_time.tv_sec - start_time.tv_sec) + (current_time.tv_nsec - start_time.tv_nsec) / 1e9;
            if (elapsed > 2.0) {
                printf("\rProcessed %llu keys... (%.0f keys/sec)", (unsigned long long)count, count / elapsed);
                fflush(stdout);
            }
        }
    }

    return 0;
}