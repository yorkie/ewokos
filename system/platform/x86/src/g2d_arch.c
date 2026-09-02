#include <stdint.h>
#include <string.h>
#include <ewoksys/ewokdef.h>

#ifdef ARCH_BOOST
#include <emmintrin.h>
#endif

/* scalar per-pixel blend, same math as the C reference: effective alpha
   (src_a * alpha) >> 8 is applied by the callers, then the /255 blend. */
static inline uint32_t g2d_blend_argb_scalar(uint32_t dst_color, uint8_t a,
		uint8_t r, uint8_t g, uint8_t b) {
	uint32_t oa = (dst_color >> 24) & 0xff;
	uint32_t dr = (dst_color >> 16) & 0xff;
	uint32_t dg = (dst_color >> 8) & 0xff;
	uint32_t db = dst_color & 0xff;
	uint32_t inv_a = 255 - a;

	oa = oa + (255 - oa) * a / 255;
	dr = (r * a + dr * inv_a) / 255;
	dg = (g * a + dg * inv_a) / 255;
	db = (b * a + db * inv_a) / 255;
	return (oa << 24) | (dr << 16) | (dg << 8) | db;
}

#ifdef ARCH_BOOST
/* scalar blend of one pixel against a constant color, bit-identical math
   to x86_blend_4px (used for the head/tail alignment pixels) */
static inline uint32_t x86_fill_blend_px(uint32_t dst, uint32_t color, uint32_t a) {
	uint32_t inv_a = 255 - a;
	uint32_t db = dst & 0xff;
	uint32_t dg = (dst >> 8) & 0xff;
	uint32_t dr = (dst >> 16) & 0xff;
	uint32_t da = (dst >> 24) & 0xff;
	uint32_t div255;

	div255 = ((color & 0xff) * a + db * inv_a);
	db = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (((color >> 8) & 0xff) * a + dg * inv_a);
	dg = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (((color >> 16) & 0xff) * a + dr * inv_a);
	dr = (div255 + 1 + (div255 >> 8)) >> 8;
	div255 = (255 - da) * a;
	da = da + ((div255 + 1 + (div255 >> 8)) >> 8);
	return (da << 24) | (dr << 16) | (dg << 8) | db;
}

/* div255 for eight unsigned 16-bit lanes, same rounding as the scalar
   helper: (v + 1 + (v >> 8)) >> 8 (max input 65025, no lane overflow) */
static inline __m128i x86_div255_epu16(__m128i v) {
	__m128i t = _mm_add_epi16(v, _mm_set1_epi16(1));
	t = _mm_add_epi16(t, _mm_srli_epi16(v, 8));
	return _mm_srli_epi16(t, 8);
}

/* Source-over blend of 4 pixels against a constant color. Lanes hold
   [b,g,r,a,b,g,r,a] after the byte unpack; the alpha lane is replaced
   with bg_a + div255((255-bg_a)*a) via alpha_mask. dst must be 16-byte
   aligned. */
static inline void x86_blend_4px(uint32_t* dst, __m128i cpa16, __m128i a16,
		__m128i inv_a16, __m128i alpha_mask, __m128i full16, __m128i zero) {
	__m128i bg = _mm_load_si128((const __m128i*)dst);
	__m128i lo = _mm_unpacklo_epi8(bg, zero);
	__m128i hi = _mm_unpackhi_epi8(bg, zero);

	__m128i blend_lo = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(lo, inv_a16)));
	__m128i blend_hi = x86_div255_epu16(_mm_add_epi16(cpa16, _mm_mullo_epi16(hi, inv_a16)));

	__m128i oa_lo = _mm_add_epi16(lo, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, lo), a16)));
	__m128i oa_hi = _mm_add_epi16(hi, x86_div255_epu16(_mm_mullo_epi16(_mm_sub_epi16(full16, hi), a16)));

	lo = _mm_or_si128(_mm_and_si128(alpha_mask, oa_lo), _mm_andnot_si128(alpha_mask, blend_lo));
	hi = _mm_or_si128(_mm_and_si128(alpha_mask, oa_hi), _mm_andnot_si128(alpha_mask, blend_hi));

	_mm_store_si128((__m128i*)dst, _mm_packus_epi16(lo, hi));
}

/* alpha fill: simd rows of 4 pixels with a scalar head for 16-byte
   realignment and a scalar tail; the fill color is constant so the
   per-channel color*a constants are set up once per call. */
int32_t arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint32_t a;

	if(argb == NULL)
		return 0;
	a = (color >> 24) & 0xff;
	if(a == 0)
		return 0;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return 0;
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;

	{
		uint32_t inv_a = 255 - a;
		uint32_t cb = color & 0xff;
		uint32_t cg = (color >> 8) & 0xff;
		uint32_t cr = (color >> 16) & 0xff;

		/* per-lane color*a constants for the [b,g,r,a,b,g,r,a] lane
		   layout; the alpha lanes are don't-care (masked away), so
		   they hold 0 */
		__m128i cpa16 = _mm_set_epi32((int)(cr * a), (int)((cg * a) << 16 | (cb * a)),
				(int)(cr * a), (int)((cg * a) << 16 | (cb * a)));
		__m128i a16 = _mm_set1_epi16((short)a);
		__m128i inv_a16 = _mm_set1_epi16((short)inv_a);
		__m128i alpha_mask = _mm_set_epi32((int)0xFFFF0000, 0, (int)0xFFFF0000, 0);
		__m128i full16 = _mm_set1_epi16(255);
		__m128i zero = _mm_setzero_si128();

		for(int32_t row = y; row < y + h; row++) {
			uint32_t* dp = argb + row * argb_w + x;
			int32_t i = 0;

			/* scalar head realigns the row start to 16 bytes */
			while(i < w && (((uintptr_t)(dp + i)) & 0x0F) != 0) {
				dp[i] = x86_fill_blend_px(dp[i], color, a);
				i++;
			}

			for(; i + 4 <= w; i += 4)
				x86_blend_4px(dp + i, cpa16, a16, inv_a16, alpha_mask, full16, zero);

			/* scalar tail, at most 3 pixels per row */
			for(; i < w; i++)
				dp[i] = x86_fill_blend_px(dp[i], color, a);
		}
	}
	return 0;
}
#endif /* ARCH_BOOST */

int32_t arch_g2d_init(void) {
    return 0;
}

int32_t arch_g2d_fill(uint32_t* argb, ewokos_addr_t argb_phy, uint8_t contig, int32_t argb_w, int32_t argb_h,
			int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	(void)argb_phy;
	(void)contig;
	if(argb == NULL || argb_w <= 0 || argb_h <= 0)
		return -1;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;
	if(w <= 0 || h <= 0)
		return 0;
	for(int32_t row = 0; row < h; row++) {
		uint32_t *dst = argb + (y + row) * argb_w + x;
		for(int32_t col = 0; col < w; col++)
			dst[col] = color;
	}
	return 0;
}

int32_t arch_g2d_blt(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	(void)src_phy; (void)src_contig; (void)dst_phy; (void)dst_contig;
	if(argb_src == NULL || argb_dst == NULL || src_w <= 0 || src_h <= 0 ||
			dst_w <= 0 || dst_h <= 0 || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return -1;
	for(int32_t y = 0; y < dh; y++) {
		/* Map both destination endpoints to the corresponding source
		 * endpoints.  The previous y*sh/dh mapping skipped the last source
		 * row/column when downscaling (for example 560x320 -> 320x240). */
		int32_t src_y = sy + (dh == 1 ? 0 :
				(int32_t)(((int64_t)y * (sh - 1) + (dh - 1) / 2) /
						(dh - 1)));
		int32_t dst_y = dy + y;
		if(src_y < 0 || src_y >= src_h || dst_y < 0 || dst_y >= dst_h)
			continue;
		for(int32_t x = 0; x < dw; x++) {
			int32_t src_x = sx + (dw == 1 ? 0 :
					(int32_t)(((int64_t)x * (sw - 1) + (dw - 1) / 2) /
							(dw - 1)));
			int32_t dst_x = dx + x;
			if(src_x >= 0 && src_x < src_w && dst_x >= 0 && dst_x < dst_w)
				argb_dst[dst_y * dst_w + dst_x] = argb_src[src_y * src_w + src_x];
		}
	}
	return 0;
}

int32_t arch_g2d_blt_alpha(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			int32_t sx, int32_t sy, int32_t sw, int32_t sh,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h,
			int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	(void)src_phy; (void)src_contig; (void)dst_phy; (void)dst_contig;
	if(argb_src == NULL || argb_dst == NULL || src_w <= 0 || src_h <= 0 ||
			dst_w <= 0 || dst_h <= 0 || sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0)
		return -1;
	for(int32_t y = 0; y < dh; y++) {
		int32_t src_y = sy + (dh == 1 ? 0 :
				(int32_t)(((int64_t)y * (sh - 1) + (dh - 1) / 2) /
						(dh - 1)));
		int32_t dst_y = dy + y;
		if(src_y < 0 || src_y >= src_h || dst_y < 0 || dst_y >= dst_h)
			continue;
		for(int32_t x = 0; x < dw; x++) {
			int32_t src_x = sx + (dw == 1 ? 0 :
					(int32_t)(((int64_t)x * (sw - 1) + (dw - 1) / 2) /
							(dw - 1)));
			int32_t dst_x = dx + x;
			if(src_x < 0 || src_x >= src_w || dst_x < 0 || dst_x >= dst_w)
				continue;
			uint32_t src = argb_src[src_y * src_w + src_x];
			uint8_t a = (uint8_t)((((src >> 24) & 0xff) * alpha) / 255);
			argb_dst[dst_y * dst_w + dst_x] = g2d_blend_argb_scalar(
					argb_dst[dst_y * dst_w + dst_x], a,
					(uint8_t)(src >> 16), (uint8_t)(src >> 8), (uint8_t)src);
		}
	}
	return 0;
}

int32_t arch_g2d_scale_to(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h) {
	return arch_g2d_blt(argb_src, src_phy, src_contig, src_w, src_h,
			0, 0, src_w, src_h, argb_dst, dst_phy, dst_contig, dst_w, dst_h,
			0, 0, dst_w, dst_h);
}

#define G2D_FP_BITS 14
#define G2D_FP_ONE  (1 << G2D_FP_BITS)
#define G2D_FP_HALF (1 << (G2D_FP_BITS - 1))
static const int16_t g2d_sin_table[91] = {
	0,286,572,857,1143,1428,1713,1997,2280,2563,2845,3126,3406,3686,3964,4240,
	4516,4790,5063,5334,5604,5872,6138,6402,6664,6924,7182,7438,7692,7943,8192,8438,
	8682,8923,9162,9397,9630,9860,10087,10311,10531,10749,10963,11174,11381,11585,
	11786,11982,12176,12365,12551,12733,12911,13085,13255,13421,13583,13741,13894,
	14044,14189,14330,14466,14598,14726,14849,14968,15082,15191,15296,15396,15491,
	15582,15668,15749,15826,15897,15964,16026,16083,16135,16182,16225,16262,16294,
	16322,16344,16362,16374,16382,16384
};
static int32_t g2d_norm_degree(int32_t degree) {
	return ((degree % 360) + 360) % 360;
}
static int32_t g2d_sin_fp(int32_t degree) {
	degree = g2d_norm_degree(degree);
	if(degree <= 90) return g2d_sin_table[degree];
	if(degree <= 180) return g2d_sin_table[180 - degree];
	if(degree <= 270) return -g2d_sin_table[degree - 180];
	return -g2d_sin_table[360 - degree];
}
static int32_t g2d_cos_fp(int32_t degree) { return g2d_sin_fp(degree + 90); }

/* smallest size able to hold src_w x src_h rotated clockwise by degree
   (any angle). exact swap/keep for multiples of 90, rotated bounding
   box otherwise. */
int32_t arch_g2d_rotated_size(int32_t src_w, int32_t src_h, int32_t degree,
			int32_t* dst_w, int32_t* dst_h) {
	if(dst_w == NULL || dst_h == NULL || src_w <= 0 || src_h <= 0)
		return -1;
	degree = g2d_norm_degree(degree);
	if(degree == 90 || degree == 270) {
		*dst_w = src_h; *dst_h = src_w;
	}
	else if(degree == 0 || degree == 180) {
		*dst_w = src_w; *dst_h = src_h;
	}
	else {
		int32_t c = g2d_cos_fp(degree), s = g2d_sin_fp(degree);
		if(c < 0) c = -c;
		if(s < 0) s = -s;
		*dst_w = (int32_t)(((int64_t)src_w * c + (int64_t)src_h * s + G2D_FP_ONE - 1) >> G2D_FP_BITS);
		*dst_h = (int32_t)(((int64_t)src_w * s + (int64_t)src_h * c + G2D_FP_ONE - 1) >> G2D_FP_BITS);
	}
	return 0;
}

/* rotate the whole source surface clockwise by degree (any angle).
   dst must be at least the size given by arch_g2d_rotated_size(); for
   angles other than 0/90/180/270 pixels outside the rotated content
   become transparent.
   in-place (argb_src == argb_dst) is only valid for 0/180. */
int32_t arch_g2d_rotate(uint32_t* argb_src, ewokos_addr_t src_phy, uint8_t src_contig, int32_t src_w, int32_t src_h,
			uint32_t* argb_dst, ewokos_addr_t dst_phy, uint8_t dst_contig, int32_t dst_w, int32_t dst_h, int32_t degree) {
	(void)src_phy; (void)src_contig; (void)dst_phy; (void)dst_contig;
	int32_t bw, bh;
	if(argb_src == NULL || argb_dst == NULL ||
			src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
		return -1;
	degree = g2d_norm_degree(degree);
	if(arch_g2d_rotated_size(src_w, src_h, degree, &bw, &bh) != 0 ||
			dst_w < bw || dst_h < bh)
		return -1;
	if(argb_src == argb_dst && degree != 0 && degree != 180)
		return -1;
	if(degree == 0) {
		if(argb_src != argb_dst)
			memcpy(argb_dst, argb_src, (size_t)src_w * src_h * sizeof(uint32_t));
		return 0;
	}
	if(degree == 180 && argb_src == argb_dst) {
		uint32_t *lo = argb_src, *hi = argb_src + (size_t)src_w * src_h - 1;
		while(lo < hi) { uint32_t t = *lo; *lo++ = *hi; *hi-- = t; }
		return 0;
	}
	if(degree == 90) {
		for(int32_t y = 0; y < src_w; y++)
			for(int32_t x = 0; x < src_h; x++)
				argb_dst[y * dst_w + x] = argb_src[(src_h - 1 - x) * src_w + y];
		return 0;
	}
	if(degree == 180) {
		for(int32_t y = 0; y < src_h; y++)
			for(int32_t x = 0; x < src_w; x++)
				argb_dst[y * dst_w + x] = argb_src[(src_h - 1 - y) * src_w + src_w - 1 - x];
		return 0;
	}
	if(degree == 270) {
		for(int32_t y = 0; y < src_w; y++)
			for(int32_t x = 0; x < src_h; x++)
				argb_dst[y * dst_w + x] = argb_src[x * src_w + src_w - 1 - y];
		return 0;
	}
	memset(argb_dst, 0, (size_t)dst_w * dst_h * sizeof(uint32_t));
	int32_t c = g2d_cos_fp(degree), s = g2d_sin_fp(degree);
	int32_t scx = (src_w - 1) << (G2D_FP_BITS - 1);
	int32_t scy = (src_h - 1) << (G2D_FP_BITS - 1);
	int32_t dcx = (bw - 1) << (G2D_FP_BITS - 1);
	int32_t dcy = (bh - 1) << (G2D_FP_BITS - 1);
	for(int32_t y = 0; y < bh; y++) {
		int64_t fy = (int64_t)y * G2D_FP_ONE - dcy;
		for(int32_t x = 0; x < bw; x++) {
			int64_t fx = (int64_t)x * G2D_FP_ONE - dcx;
			int64_t sxf = ((fx * c + fy * s) >> G2D_FP_BITS) + scx;
			int64_t syf = ((fy * c - fx * s) >> G2D_FP_BITS) + scy;
			int32_t sx = (int32_t)((sxf + G2D_FP_HALF) >> G2D_FP_BITS);
			int32_t sy = (int32_t)((syf + G2D_FP_HALF) >> G2D_FP_BITS);
			if(sx >= 0 && sx < src_w && sy >= 0 && sy < src_h)
				argb_dst[y * dst_w + x] = argb_src[sy * src_w + sx];
		}
	}
	return 0;
}

#ifndef ARCH_BOOST
/* cpu back end for alpha fills: blend a solid color over a sub-rect,
   clipped to the buffer bounds, exact per-pixel access with the same
   blend math as the simd paths. alpha == 0 or an empty/fully clipped
   rect is a no-op. */
int32_t arch_g2d_fill_alpha(uint32_t* argb, int32_t argb_w, int32_t argb_h,
		int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	uint8_t a;

	if(argb == NULL)
		return 0;
	a = (uint8_t)((color >> 24) & 0xff);
	if(a == 0)
		return 0;
	if(x < 0) { w += x; x = 0; }
	if(y < 0) { h += y; y = 0; }
	if(w <= 0 || h <= 0 || x >= argb_w || y >= argb_h)
		return 0;
	if(x + w > argb_w) w = argb_w - x;
	if(y + h > argb_h) h = argb_h - y;

	for(int32_t row = y; row < y + h; row++) {
		uint32_t* dp = argb + row * argb_w + x;
		for(int32_t col = 0; col < w; col++) {
			dp[col] = g2d_blend_argb_scalar(dp[col], a,
					(uint8_t)((color >> 16) & 0xff),
					(uint8_t)((color >> 8) & 0xff),
					(uint8_t)(color & 0xff));
		}
	}
	return 0;
}
#endif /* !ARCH_BOOST */
