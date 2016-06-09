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

#include "DDTSharpen.h"

void DDTSharpenImage(const void* src, void* dest, int32 width, int32 height, int32 planes, float sharpness) {

	const float sharpenFilter[][3] = {
		{0, -sharpness, 0},
		{ -sharpness, 1 + 4 * sharpness, -sharpness },
		{ 0, -sharpness, 0 }
	};

	const uint8 *pixels = static_cast<const uint8*>(src);
	uint8 *destPixels = static_cast<uint8*>(dest);

	for (int32 p = 0; p < planes; ++p) {
		for (int32 y = 0; y < height; ++y) {
			for (int32 x = 0; x < width; ++x) {
				int16 c = 0;

				if (x - 1 < 0 || x + 1 >= width ||
					y - 1 < 0 || y + 1 >= height) {
					destPixels[planes * (y * width + x) + p] = pixels[planes * (y * width + x) + p];
					continue;
				}

				for (int32 fy = 0; fy < 3; ++fy) {
					for (int32 fx = 0; fx < 3; ++fx)
						c += sharpenFilter[fy][fx] * pixels[planes * ((y + fy - 1) * width + (x + fx - 1)) + p];
				}

				destPixels[planes * (y * width + x) + p] = static_cast<uint8>(c > 255 ? 255 : c < 0 ? 0 : c);
			}
		}
	}
}