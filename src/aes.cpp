#include "pch.h"
#include "aes.h"
#include <xutility>

// https://ru.wikipedia.org/wiki/Advanced_Encryption_Standard
// https://csrc.nist.gov/csrc/media/publications/fips/197/final/documents/fips-197.pdf

namespace
{
	const int Nb = 4;  // Number of columns (32-bit words) comprising the State
	const int Nk = 4;  // 6 8 Number of 32-bit words comprising the Cipher Key
	const int Nr = 10; // 12 14  Number of rounds, which is a function of Nk and Nb

	uint8_t Sbox[] = {
		0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
		0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
		0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
		0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
		0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
		0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
		0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
		0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
		0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
		0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
		0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
		0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
		0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
		0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
		0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
		0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
	};

	uint8_t InvSbox[] = {
		0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
		0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
		0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
		0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
		0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
		0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
		0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
		0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
		0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
		0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
		0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
		0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
		0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
		0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
		0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
		0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
	};

	using Word = uint32_t;

	struct State
	{
		uint8_t s[4][Nb]; // 4 rows, Nb columns, in text S(r c) is  s[c][r]
		// s[0][0] s[1][0] s[2][0] s[3][0] 
		// s[0][1] s[1][1] s[2][1] s[3][1] 
		// s[0][2] s[1][2] s[2][2] s[3][2] 
		// s[0][3] s[1][3] s[2][3] s[3][3] 
		// in memory: s[0][0] s[0][1] s[0][2] ... s[1][0]... s[2][0].. 
		void from_bytes(const uint8_t* bytes16) { memcpy(&s[0][0], bytes16, 16); }
		void to_bytes(uint8_t* bytes16) const { memcpy(bytes16, &s[0][0], 16); }
		// columns form words
		Word& operator[](int n) { return (Word&)s[n]; }
	};

	// Addition (+) is XOR: {01010111} (+) {10000011} = {11010100} 
	// Multiplication
	//  {57} (.) {13} = {fe}
	/*
	{57} (.) {02} = xtime({57}) = {ae}
	{57} (.) {04} = xtime({ae}) = {47}
	{57} (.) {08} = xtime({47}) = {8e}
	{57} (.) {10} = xtime({8e}) = {07},

	{57} (.) {13} = {57} (.) ({01} (+) {02} (+) {10})
				  = {57} (.) {01} (+) {57} (.) {02} (+) {57} (.) {10}
				  = {57} (+) {ae} (+) {07}
				  = {fe}
	*/
	uint8_t xtime(uint8_t x)
	{
		return x & 0x80 ? (x << 1) ^ 0x1b : (x << 1);
	}
	uint8_t mul(uint8_t x, uint8_t y)
	{
		if (x < y)
			std::swap(x, y);
		uint8_t res = 0;
		uint8_t pow = x; // x**1, x**2 x**4 etc
		while (y) {
			if (y & 1)
				res ^= pow;
			y >>= 1;
			if (!y)
				break;
			pow = xtime(pow);
		}
		return res;
	}
	// The input and output for the AES algorithm each consist of sequences of 128 bits
	// The basic unit for processing in the AES algorithm is a byte, a sequence of eight bits treated as a single entity
	// Arrays of bytes will be represented in the following form: a0, a1, a2...


	void SubBytes(State& st)
	{
		for (auto& col : st.s)
			for (auto& el : col)
				el = Sbox[el];
	}

	void ShiftRows(State& st)
	{
		// s[0][0] s[1][0] s[2][0] s[3][0]   ->   s[0][0] s[1][0] s[2][0] s[3][0]
		// s[0][1] s[1][1] s[2][1] s[3][1]   ->   s[1][1] s[2][1] s[3][1] s[0][1]
		// s[0][2] s[1][2] s[2][2] s[3][2]   ->   s[2][2] s[3][2] s[0][2] s[1][2]
		// s[0][3] s[1][3] s[2][3] s[3][3]   ->   s[3][3] s[0][3] s[1][3] s[2][3]
		State old = st;
		st.s[0][1] = old.s[1][1];
		st.s[0][2] = old.s[2][2];
		st.s[0][3] = old.s[3][3];

		st.s[1][1] = old.s[2][1];
		st.s[1][2] = old.s[3][2];
		st.s[1][3] = old.s[0][3];

		st.s[2][1] = old.s[3][1];
		st.s[2][2] = old.s[0][2];
		st.s[2][3] = old.s[1][3];

		st.s[3][1] = old.s[0][1];
		st.s[3][2] = old.s[1][2];
		st.s[3][3] = old.s[2][3];
	}

	uint8_t mul0x2[256];
	uint8_t mul0x3[256];

	void InitEncTables()
	{
		for (int i = 0; i < 256; ++i)
		{
			mul0x2[i] = mul(i, 0x2);
			mul0x3[i] = mul(i, 0x3);
		}
	}

	void MixColumns(State& st)
	{
		// for each column:
		//  | s1' |   | 2 3 1 1 |   | s1 |
		//  | s2' | = | 1 2 3 1 | * | s2 |
		//  | s3' |   | 1 1 2 3 |   | s3 |
		//  | s4' |   | 3 1 1 2 |   | s4 |
		// XOR used instead of +
		for (auto& col : st.s)
		{
			uint8_t s1 = col[0];
			uint8_t s2 = col[1];
			uint8_t s3 = col[2];
			uint8_t s4 = col[3];
#if 0
			col[0] = mul(s1, 2) ^ mul(s2, 3) ^ s3 ^ s4;
			col[1] = s1 ^ mul(s2, 2) ^ mul(s3, 3) ^ s4;
			col[2] = s1 ^ s2 ^ mul(s3, 2) ^ mul(s4, 3);
			col[3] = mul(s1, 3) ^ s2 ^ s3 ^ mul(s4, 2);
#else
			col[0] = mul0x2[s1] ^ mul0x3[s2] ^ s3         ^ s4;
			col[1] = s1 ^ mul0x2[s2] ^ mul0x3[s3] ^ s4;
			col[2] = s1 ^ s2         ^ mul0x2[s3] ^ mul0x3[s4];
			col[3] = mul0x3[s1] ^ s2         ^ s3         ^ mul0x2[s4];
#endif
		}
	}

	void AddRoundKey(State& st, Word *w)
	{
		for (int i = 0; i < Nb; ++i)
			st[i] ^= w[i];
	}

	State Cipher(const State& in, Word w[Nb*(Nr + 1)])
	{// our AddRoundKey uses inverted w (reversed byte-order)
		State state = in;

		AddRoundKey(state, &w[0]);

		for (int round = 1; round < Nr; ++round)
		{
			SubBytes(state);
			ShiftRows(state);
			MixColumns(state);
			AddRoundKey(state, &w[round*Nb]);
		}

		SubBytes(state);
		ShiftRows(state);
		AddRoundKey(state, &w[Nr*Nb]);

		return state;
	}

	void InvShiftRows(State& st)
	{
		// s[0][0] s[1][0] s[2][0] s[3][0]   ->   s[0][0] s[1][0] s[2][0] s[3][0]
		// s[0][1] s[1][1] s[2][1] s[3][1]   ->   s[3][1] s[0][1] s[1][1] s[2][1]
		// s[0][2] s[1][2] s[2][2] s[3][2]   ->   s[2][2] s[3][2] s[0][2] s[1][2]
		// s[0][3] s[1][3] s[2][3] s[3][3]   ->   s[1][3] s[2][3] s[3][3] s[0][3]
		State old = st;
		st.s[0][1] = old.s[3][1];
		st.s[0][2] = old.s[2][2];
		st.s[0][3] = old.s[1][3];

		st.s[1][1] = old.s[0][1];
		st.s[1][2] = old.s[3][2];
		st.s[1][3] = old.s[2][3];

		st.s[2][1] = old.s[1][1];
		st.s[2][2] = old.s[0][2];
		st.s[2][3] = old.s[3][3];

		st.s[3][1] = old.s[2][1];
		st.s[3][2] = old.s[1][2];
		st.s[3][3] = old.s[0][3];
	}

	void InvSubBytes(State& st)
	{
		for (auto& col : st.s)
			for (auto& el : col)
				el = InvSbox[el];
	}

	uint8_t mul0x9[256];
	uint8_t mul0xb[256];
	uint8_t mul0xd[256];
	uint8_t mul0xe[256];
	void InvMixColumns(State& st)
	{
		// for each column:
		//  | s1' |   | 0e 0b 0d 09 |   | s1 |
		//  | s2' | = | 09 0e 0b 0d | * | s2 |
		//  | s3' |   | 0d 09 0e 0b |   | s3 |
		//  | s4' |   | 0b 0d 09 0e |   | s4 |
		// XOR used instead of +
		for (auto& col : st.s)
		{
			uint8_t s1 = col[0];
			uint8_t s2 = col[1];
			uint8_t s3 = col[2];
			uint8_t s4 = col[3];
#if 0
			col[0] = mul(s1, 0xe) ^ mul(s2, 0xb) ^ mul(s3, 0xd) ^ mul(s4, 0x9);
			col[1] = mul(s1, 0x9) ^ mul(s2, 0xe) ^ mul(s3, 0xb) ^ mul(s4, 0xd);
			col[2] = mul(s1, 0xd) ^ mul(s2, 0x9) ^ mul(s3, 0xe) ^ mul(s4, 0xb);
			col[3] = mul(s1, 0xb) ^ mul(s2, 0xd) ^ mul(s3, 0x9) ^ mul(s4, 0xe);
#else
			col[0] = mul0xe[s1] ^ mul0xb[s2] ^ mul0xd[s3] ^ mul0x9[s4];
			col[1] = mul0x9[s1] ^ mul0xe[s2] ^ mul0xb[s3] ^ mul0xd[s4];
			col[2] = mul0xd[s1] ^ mul0x9[s2] ^ mul0xe[s3] ^ mul0xb[s4];
			col[3] = mul0xb[s1] ^ mul0xd[s2] ^ mul0x9[s3] ^ mul0xe[s4];
#endif
		}
	}

	void InitDecTables()
	{
		for (int i = 0; i < 256; ++i)
		{
			mul0x9[i] = mul(i, 0x9);
			mul0xb[i] = mul(i, 0xb);
			mul0xd[i] = mul(i, 0xd);
			mul0xe[i] = mul(i, 0xe);
		}
	}

	State InvCipher(const State& in, Word w[Nb * (Nr + 1)])
	{
		State state = in;

		AddRoundKey(state, &w[Nr * Nb]);

		for (int round = Nr - 1; round > 0; --round)
		{
			InvShiftRows(state);
			InvSubBytes(state);
			AddRoundKey(state, &w[Nb * round]);
			InvMixColumns(state);
		}

		InvShiftRows(state);
		InvSubBytes(state);
		AddRoundKey(state, &w[0]);

		return state;
	}


	//Note that Nk = 4, 6, and 8 do not all have to be implemented;
	//they are all included in the conditional statement above for
	//conciseness.Specific implementation requirements for the
	//Cipher Key are presented in Sec. 6.1.

	Word DoWord(const uint8_t *bytes4)
	{
		return (bytes4[0] << 24) | (bytes4[1] << 16) | (bytes4[2] << 8) | bytes4[3];
	}

	// SubWord is a function that takes a four - byte input word and applies the S - box
	// (Sec. 5.1.1, Fig. 7) to each of the four bytes to produce an output word.
	Word SubWord(Word w)
	{
		uint8_t * ptr = (uint8_t *)&w;
		for (int i = 0; i < 4; ++i, ++ptr)
			*ptr = Sbox[*ptr];
		return w;
	}

	// The function RotWord() takes a word[a0, a1, a2, a3] as input, performs a cyclic permutation,
	// and returns the word[a1, a2, a3, a0]
	Word RotWord(Word w)
	{
		return (w >> 24) | (w << 8);
	}
	void KeyExpansion(const uint8_t key[4 * Nk], Word w[Nb*(Nr + 1)]) //, int Nk)
	{
		for (int i = 0; i < Nk; ++i)
			w[i] = DoWord(&key[4 * i]);

		static Word Rcon[] = { 0,
			0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000,
			0x20000000, 0x40000000, 0x80000000, 0x1b000000, 0x36000000,
		};

		for (int i = Nk; i < Nb * (Nr + 1); ++i) // Nk = 4
		{
			Word temp = w[i - 1];
			if (i % Nk == 0)
				temp = SubWord(RotWord(temp)) ^ Rcon[i / Nk];
			else if (Nk > 6 && i % Nk == 4)
				temp = SubWord(temp);

			w[i] = w[i - Nk] ^ temp;
		}
	}

	Word swap_bytes(Word w)
	{
		return (w << 24) | (w >> 24) | ((w & 0xff00) << 8) | ((w & 0xff0000) >> 8);
	}

} // namespace



Aes128::Aes128(const uint8_t *key16, const uint8_t* init_iv)
{
	static bool tables_ok = false;
	if (!tables_ok)
	{
		InitEncTables();
		InitDecTables();
		tables_ok = true;
	}

	Word w[Nb*(Nr + 1)];
	static_assert(sizeof(w) == sizeof(w1), "use only 128 bit keys or do revision");
	KeyExpansion(key16, w);

	for (int i = 0; i < std::size(w); ++i)
		w1[i] = swap_bytes(w[i]);

	reset_iv(init_iv);
}
void Aes128::reset_iv(const uint8_t* init_iv)
{
	if(init_iv)
		memcpy(iv, init_iv, sizeof(iv));
	else
		memset(iv, 0, sizeof(iv));
}

static void xor16(uint8_t* dst, const uint8_t* src1, const uint8_t* src2)
{
	for (int i = 0; i < 16; ++i)
		dst[i] = src1[i] ^ src2[i];
}
static void copy16(uint8_t* dst, const uint8_t* src)
{
	for (int i = 0; i < 16; ++i)
		dst[i] = src[i];
}

void Aes128::encrypt(const uint8_t* in_bytes16, uint8_t* out_bytes16)
{
	uint8_t mix[16];
	xor16(mix, in_bytes16, iv);
	(State&)* out_bytes16 = Cipher((const State&)mix, w1);
	copy16(iv, out_bytes16);
}

void Aes128::decrypt(const uint8_t* in_bytes16, uint8_t* out_bytes16)
{
	uint8_t res[16];
	copy16(res, in_bytes16);
	(State&)* out_bytes16 = InvCipher((const State&)*in_bytes16, w1);
	xor16(out_bytes16, out_bytes16, iv);
	copy16(iv, res);
}


int test_aes()
{
	Aes128 aes((uint8_t *) "some any key 16 bytes");

	uint8_t s1[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
//	uint8_t s2[100] = "";

	aes.encrypt(s1, s1);
	aes.encrypt(s1 + 16, s1 + 16);
	aes.encrypt(s1 + 32, s1 + 32);

	int deb = 0;
	aes.reset_iv();
	aes.decrypt(s1, s1);
	aes.decrypt(s1 + 16, s1 + 16);
	aes.decrypt(s1 + 32, s1 + 32);


	return 0;
}
