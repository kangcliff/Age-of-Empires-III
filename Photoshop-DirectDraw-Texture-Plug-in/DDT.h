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
#ifndef _DDT_H
#define _DDT_H

#include <PIFormat.h>
#include <FileUtilities.h>
#include <squish.h>

#define DDT_MAGIC 0x33535452

#define DDT_DIFFUSE 0
#define DDT_DIFFUSE2 1
#define DDT_BUMP 6
#define DDT_BUMP2 7
#define DDT_CUBE 8 // to-do, not yet supported

#define DDT_NONE 0
#define DDT_PLAYER 1
#define DDT_TRANSPARENT 4
#define DDT_BLEND 8

#define DDT_BGRA 1
#define DDT_DXT1 4
#define DDT_GREYSCALE 7
#define DDT_DXT3 8
#define DDT_DXT5 9

struct DDTHeader
{
	uint32 Magic;
	int8 TextureUsage;
	int8 TextureAlphaUsage;
	int8 TextureType;
	int8 ImageCount;
	uint32 Width;
	uint32 Height;
};

struct DDTImageEntry
{
	uint32 Offset;
	uint32 Size;
};

#endif