// Copyright (c) 2016 Cliff Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//
//

#include "DDTMIPMap.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <cfloat>

bool DDTMIPMapGenerate(const void *src, int32 srcWidth, int32 srcHeight, int16 srcPlanes, Ptr* dest, int8 level,
	PSBufferSuite1* sPSBufferSuite) {

	const uint8* srcPixels = static_cast<const uint8*>(src);

	const int32 width = srcWidth >> level,
		height = srcHeight >> level;

	const int32 scale = 1 << level;

	const int32 samples = 32;

	*dest = sPSBufferSuite->New(NULL, srcPlanes * width * height);

	if (*dest == NULL)
		return false;

	Ptr buffer = sPSBufferSuite->New(NULL, sizeof(float) * srcPlanes * width * srcHeight);

	if (buffer == NULL) {
		sPSBufferSuite->Dispose(dest);
		return false;
	}

	Ptr pixel = sPSBufferSuite->New(NULL, sizeof(float) * srcPlanes);

	if (pixel == NULL) {
		sPSBufferSuite->Dispose(dest);
		sPSBufferSuite->Dispose(&buffer);
		return false;
	}

	for (int32 sy = 0; sy < srcHeight; ++sy) {
		for (int32 x = 0; x < width; ++x) {
			float w = 0.0f, totalWeight = 0.0f;

			for (int8 p = 0; p < srcPlanes; ++p)
				reinterpret_cast<float*>(buffer)[srcPlanes * (sy * width + x) + p] = 0.0f;

			for (int32 i = 0; i < scale; ++i) {
				int32 sx = scale * x + i;

				if (sx >= srcWidth)
					continue;

				for (int32 s = 1; s <= samples; ++s)
					w += 0.5f - fabsf(0.5f - (i + static_cast<float>(s) / (samples + 1)) / scale);

				w /= samples;

				for (int8 p = 0; p < srcPlanes; ++p)
					reinterpret_cast<float*>(buffer)[srcPlanes * (sy * width + x) + p] +=
					w * *(srcPixels + srcPlanes * (sy * srcWidth + sx) + p);

				totalWeight += w;
			}

			for (int8 p = 0; p < srcPlanes; ++p)
				reinterpret_cast<float*>(buffer)[srcPlanes * (sy * width + x) + p] /= totalWeight;
		}
	}

	for (int32 x = 0; x < width; ++x) {
		for (int32 y = 0; y < height; ++y) {
			float w = 0.0f, totalWeight = 0.0f;

			for (int8 p = 0; p < srcPlanes; ++p)
				reinterpret_cast<float*>(pixel)[p] = 0.0f;

			for (int32 i = 0; i < scale; ++i) {
				int32 sy = scale * y + i;

				if (sy >= srcHeight)
					continue;

				for (int32 s = 1; s <= samples; ++s)
					w += 0.5f - fabsf(0.5f - (i + static_cast<float>(s) / (samples + 1)) / scale);

				w /= samples;

				for (int8 p = 0; p < srcPlanes; ++p)
					reinterpret_cast<float*>(pixel)[p] += w * reinterpret_cast<float*>(buffer)[srcPlanes * (sy * width + x) + p];

				totalWeight += w;
			}

			for (int8 p = 0; p < srcPlanes; ++p)
				*(*dest + srcPlanes * (y * width + x) + p) = static_cast<uint8>(reinterpret_cast<float*>(pixel)[p] / totalWeight);
		}
	}

	sPSBufferSuite->Dispose(&buffer);
	sPSBufferSuite->Dispose(&pixel);

	return true;
}
