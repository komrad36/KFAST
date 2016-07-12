/*******************************************************************
*   KFAST.h
*   KFAST
*
*	Author: Kareem Omar
*	kareem.omar@uah.edu
*	https://github.com/komrad36
*
*	Last updated Jul 11, 2016
*******************************************************************/

#pragma once

#include <cstdint>
#include <immintrin.h>
#include <vector>

struct Keypoint {
	int32_t x;
	int32_t y;
	uint8_t score;

	Keypoint(const int32_t _x, const int32_t _y, const uint8_t _score) : x(_x), y(_y), score(_score) {}

};

// Yes, this function MUST be inlined.
// Even if your compiler thinks otherwise.
// 2000 -> 2600 microseconds without forced inlining.
template<const bool full, const bool nonmax_suppression>
#ifdef _MSC_VER
__forceinline
#else
__inline__
#endif
void processCols(int32_t& num_corners, const uint8_t* __restrict & ptr, int32_t& j,
	const int32_t* const __restrict offsets, const __m256i& ushft, const __m256i& t, const int32_t cols,
	const __m256i& consec, int32_t* const __restrict corners, uint8_t* const __restrict cur,
	std::vector<Keypoint>& keypoints, const int32_t i) {
	// ppt is an integer vector that now holds 32 of point p
	__m256i ppt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));

	// we subtract (and clamp) the threshold value from all 32 pixels
	// pmt represents p - t
	__m256i pmt = _mm256_xor_si256(_mm256_subs_epu8(ppt, t), ushft);

	// we add (and clamp) the threshold value to all 32 pixels
	// ppt represents p + t
	ppt = _mm256_xor_si256(_mm256_adds_epu8(ppt, t), ushft);

	// Rosten's point 9 for all 32 pixels in consideration
	__m256i p9 = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + *offsets)), ushft);

	// Rosten's point 5
	__m256i p5 = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + offsets[4])), ushft);

	// Rosten's point 1
	__m256i p1 = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + offsets[8])), ushft);

	// Rosten's point 13
	__m256i p13 = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + offsets[12])), ushft);

	// compare p's against p9's
	// compare p's against p5's
	// AND the result into ppt_accum
	__m256i ppt_accum = _mm256_and_si256(_mm256_cmpgt_epi8(p9, ppt), _mm256_cmpgt_epi8(p5, ppt));

	// compare p's against p9's
	// compare p's against p5's
	// AND the result into pmt_accum
	__m256i pmt_accum = _mm256_and_si256(_mm256_cmpgt_epi8(pmt, p9), _mm256_cmpgt_epi8(pmt, p5));

	// compare p's against p5's
	// compare p's against p1's
	// AND the result, and then OR that with ppt_accum into ppt_accum
	ppt_accum = _mm256_or_si256(ppt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(p5, ppt), _mm256_cmpgt_epi8(p1, ppt)));

	// compare p's against p5's
	// compare p's against p1's
	// AND the result, and then OR that with pmt_accum into pmt_accum
	pmt_accum = _mm256_or_si256(pmt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(pmt, p5), _mm256_cmpgt_epi8(pmt, p1)));

	// compare p's against p1's
	// compare p's against p13's
	// AND the result, and then OR that with ppt_accum into ppt_accum
	ppt_accum = _mm256_or_si256(ppt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(p1, ppt), _mm256_cmpgt_epi8(p13, ppt)));

	// compare p's against p1's
	// compare p's against p13's
	// AND the result, and then OR that with pmt_accum into pmt_accum
	pmt_accum = _mm256_or_si256(pmt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(pmt, p1), _mm256_cmpgt_epi8(pmt, p13)));

	// compare p's against p13's
	// compare p's against p9's
	// AND the result, and then OR that with ppt_accum into ppt_accum
	ppt_accum = _mm256_or_si256(ppt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(p13, ppt), _mm256_cmpgt_epi8(p9, ppt)));

	// compare p's against p13's
	// compare p's against p9's
	// AND the result, and then OR that with pmt_accum into pmt_accum
	pmt_accum = _mm256_or_si256(pmt_accum, _mm256_and_si256(_mm256_cmpgt_epi8(pmt, p13), _mm256_cmpgt_epi8(pmt, p9)));

	// 'full' is known by the template.
	// this and all following ternaries and ifs involving full
	// are optimized away, allowing efficient code generation for
	// the normal full 32 columns, or special handling for the last few columns if they
	// don't divide up evenly into 32
	uint32_t last_cols_mask;
	if (!full) last_cols_mask = (1 << (cols - j - 3)) - 1;

	// 'mask' now contains one bit for each element
	// which is SET if that element COULD be a corner based on the 2 consective cardinal point test
	// CAREFUL - returns signed int32_t but you NEED all 32 bits so cast immediately to unsigned
	uint32_t m = full ?
		static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(ppt_accum, pmt_accum))) :
		static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_or_si256(ppt_accum, pmt_accum))) & last_cols_mask;

	// if none of the elements can be corners, bail
	if (m == 0) return;

	// if none of the left half can be corners, retreat 16 pixels to the left and bail,
	// so that after the 'continue' the total change will be forward by 16 pixels
	if (full) {
		if ((m & 0xFFFF) == 0) {
			j -= 16;
			ptr -= 16;
			return;
		}
	}

	// profiling suggests it's not worth further bailout checks for 8, 4, 2, 1

	__m256i ppt_cnt = _mm256_setzero_si256();
	__m256i pmt_cnt = ppt_cnt;
	__m256i ppt_max = ppt_cnt;
	__m256i pmt_max = pmt_cnt;

	// for each of the 24 pixels in the circle (wrapping around extra 8 at the end)
	for (int32_t k = 0; k < 24; ++k) {
		// x is a vector of the kth member of the circle
		__m256i p = _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr + offsets[k])), ushft);

		ppt_accum = _mm256_cmpgt_epi8(p, ppt);
		pmt_accum = _mm256_cmpgt_epi8(pmt, p);

		// add 1 to the chain count for all salient pixels,
		// then AND with pmt_accum to destroy any exiting chain that was broken
		// at this offsets
		ppt_cnt = _mm256_and_si256(_mm256_sub_epi8(ppt_cnt, ppt_accum), ppt_accum);
		pmt_cnt = _mm256_and_si256(_mm256_sub_epi8(pmt_cnt, pmt_accum), pmt_accum);

		ppt_max = _mm256_max_epu8(ppt_max, ppt_cnt);
		pmt_max = _mm256_max_epu8(pmt_max, pmt_cnt);
	}

	// 'm' now contains one bit for whether each element
	// is a corner!
	m = full ?
		static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(_mm256_max_epu8(ppt_max, pmt_max), consec))) :
		static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(_mm256_max_epu8(ppt_max, pmt_max), consec))) & last_cols_mask;

	// for each of the 32 pixels considered in AVX vector,
	// and while there are still corners left in the mask
	for (int32_t k = 0; m; ++k, m >>= 1) {
		// if this one is a corner
		if (m & 1) {
			if (nonmax_suppression) {
				// add it!
				corners[num_corners++] = j + k;

				// --- BEGIN COMPUTE CORNER SCORE ---

				// inlining gives measurably better performance but g++
				// refuses to so I do, manually

				const uint8_t* ptrpk = ptr + k;

				// the actual offsets value of point p
				const int16_t p = static_cast<int16_t>(*ptrpk);

				int16_t ring[24];
				for (int n = 0; n < 24; ++n) ring[n] = p - static_cast<int16_t>(ptrpk[offsets[n]]);

				int16_t* ringp = ring;

				// points 0-15
				__m256i ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));

				// points 1-16
				__m256i ringv2 = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				__m256i minv = _mm256_min_epi16(ringv, ringv2);
				__m256i maxv = _mm256_max_epi16(ringv, ringv2);

				// points 2-17
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 3-18
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 4-19
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 5-20
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 6-21
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 7-22
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp++));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// points 8-23
				ringv = _mm256_loadu_si256(reinterpret_cast<__m256i*>(ringp));
				minv = _mm256_min_epi16(minv, ringv);
				maxv = _mm256_max_epi16(maxv, ringv);

				// minv now has the smallest of [0-8], [1-9], ..., [14-6], [15-7] (all 16 possible regions of 9 pixels)
				// maxv now has the largest of  [0-8], [1-9], ..., [14-6], [15-7] (all 16 possible regions of 9 pixels)

				// inside expression is just the negation of q1
				// to get expression of absolute deviation from center offsets, resulting in
				// the greatest deviation among:
				// [0-8], [1-9], ..., [14-6], [15-7] (all 16 possible regions of 9 pixels)
				
				maxv = _mm256_max_epi16(minv, _mm256_sub_epi16(_mm256_setzero_si256(), maxv));

				// The overall single max is now found through a horizontal reduction of 'maxv'.
				// This score represents the deviation of the most deviant region of 9 pixels.
				// The answer resides in an epi16 but it will be in
				// the range of a uint8_t for efficient removal from XMM
				// (should become vmovd eax)
				maxv = _mm256_max_epi16(maxv, _mm256_permute4x64_epi64(maxv, 0b1110));
				maxv = _mm256_max_epi16(maxv, _mm256_permute4x64_epi64(maxv, 0b11100101));
				maxv = _mm256_max_epi16(maxv, _mm256_shuffle_epi32(maxv, 0b11100101));
				maxv = _mm256_max_epi16(maxv, _mm256_shufflelo_epi16(maxv, 0b11100101));

				cur[j + k] = *reinterpret_cast<uint8_t*>(&maxv);

				// --- END COMPUTE CORNER SCORE ---

			}
			else {
				keypoints.emplace_back(j + k, i, 0);
			}
		}
	}
}

template <const bool nonmax_suppression>
void KFAST(const uint8_t* __restrict const data, const int32_t cols, const int32_t rows, const int32_t stride,
	std::vector<Keypoint>& keypoints, const uint8_t threshold) {
	keypoints.clear();
	keypoints.reserve(8500);

	// Rosten's circle pixels in the order 9, 8, 7, 6, 5, 4, 3, 2, 1, 16, 15, 14, 13, 12, 11, 10, then repeat 9, 8, 7, 6, 5, 4, 3, 2
	const int32_t offsets[24] = { 3 * stride, 3 * stride + 1, 2 * stride + 2, stride + 3, 3, -stride + 3, -2 * stride + 2,
		-3 * stride + 1, -3 * stride, -3 * stride - 1, -2 * stride - 2, -stride - 3, -3, stride - 3, 2 * stride - 2,
		3 * stride - 1, 3 * stride, 3 * stride + 1, 2 * stride + 2, stride + 3, 3, -stride + 3, -2 * stride + 2, -3 * stride + 1 };

	// no epu8 comparisons so in order to do them we must use epi8 comparisons and shift everything down by 128
	// add, xor, and sub 128 all do the same thing. Do it to both comparands before epi8 comparison to get
	// the equivalent unshifted epu8 comparison.
	const __m256i ushft = _mm256_set1_epi8(-128);

	// the threshold value repeated 32 times
	const __m256i t = _mm256_set1_epi8(threshold);

	// the value 8 repeated 32 times
	// will be used for comparing number of consecutive salient pixels - greater than 8 means corner!
	const __m256i consec = _mm256_set1_epi8(8);

	uint8_t* rawbuf;
	uint8_t* rowbuf[3];
	int32_t* cornerbuf[3];
	if (nonmax_suppression) {
		// allocate enough buffer for 3 rows of uin8_t and then 3 rows of int32_t
		rawbuf = reinterpret_cast<uint8_t*>(_mm_malloc(cols * 3 * (sizeof(int32_t) + sizeof(uint8_t)) + 4 * sizeof(int32_t), 4096));

		// each rowbuf entry is a pointer to a uint8_t row buffer
		rowbuf[0] = rawbuf;
		rowbuf[1] = rowbuf[0] + cols;
		rowbuf[2] = rowbuf[1] + cols;

		// each cornerbuf entry is a pointer to an int32_t row buffer
		// make sure it's aligned to a 4-byte boundary
		// and that there's space for an extra element to store num_corners
		cornerbuf[0] = reinterpret_cast<int32_t*>((reinterpret_cast<uintptr_t>(rowbuf[2] + cols) + 3) & ~3) + 1;
		cornerbuf[1] = cornerbuf[0] + cols + 1;
		cornerbuf[2] = cornerbuf[1] + cols + 1;

		// zero out the uint8_t row buffers
		memset(rowbuf[0], 0, cols * 3);
	}

	int32_t j;
	for (int32_t i = 3; i < rows - 2; ++i) {

		// ptr points to the first valid offsets in the row but HASN'T retrieved it yet
		const uint8_t* ptr = data + i*stride + 3;

		uint8_t* cur = nullptr;
		int32_t* corners = nullptr;
		int32_t num_corners;
		if (nonmax_suppression) {
			// cur points to which row buffer we're in - pattern goes 0, 1, 2, 0, 1, 2, 0, 1, 2, ...
			cur = rowbuf[i % 3];

			// corners does the same thing for the int32_t buffer
			corners = cornerbuf[i % 3];

			// zero out the current rowbuffer
			memset(cur, 0, cols);
			num_corners = 0;
		}

		if (i < rows - 3) {
			// for col (3) to (cols - 35)
			// jumping forward 32 cols at a time and also moving ptr forward 32 cols each time with it
			// these calls to processCols MUST be inlined for best performance, even if your compiler thinks otherwise
			for (j = 3; j < cols - 35; j += 32, ptr += 32) {
				processCols<true, nonmax_suppression>(num_corners, ptr, j, offsets, ushft, t,
					cols, consec, corners, cur, keypoints, i);
			}
			// handle last few columns
			processCols<false, nonmax_suppression>(num_corners, ptr, j, offsets, ushft, t,
				cols, consec, corners, cur, keypoints, i);
		}

		if (nonmax_suppression) {
			corners[-1] = num_corners;

			// bail if it's the first row
			if (i == 3) continue;

			// last buffered row
			const uint8_t* last = rowbuf[(i - 1) % 3];

			// last last buffered row
			const uint8_t* last2 = rowbuf[(i - 2) % 3];

			// set corners to previous buffered row
			corners = cornerbuf[(i - 1) % 3];

			// retrieve previous num_corners
			num_corners = corners[-1];
			// for each corner from the previous row
			for (int32_t k = 0; k < num_corners; ++k) {
				// corner was at col j
				j = corners[k];

				const uint8_t score = last[j];

				// if score is higher than score of all 8 surrounding pixels, add keypoint!
				if (static_cast<uint8_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(
					_mm256_xor_si256(_mm256_set1_epi8(score), ushft),
					_mm256_xor_si256(_mm256_setr_epi8(
						last[j - 1], last[j + 1],
						cur[j - 1], cur[j], cur[j + 1],
						last2[j - 1], last2[j], last2[j + 1],
						0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), ushft)))) == 0xFF) {
					keypoints.emplace_back(j, i - 1, score);
				}
			}
		}
	}

	if (nonmax_suppression) _mm_free(rawbuf);
}
