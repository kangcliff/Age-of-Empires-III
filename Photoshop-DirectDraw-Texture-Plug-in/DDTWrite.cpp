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
#include "DDTPlugin.h"
#include "DDTMIPMap.h"
#include "DDTSharpen.h"

void DDTEstimateBegin() {

	int32 dataBytes = 0;

	for (int8 i = 0; i < gPlugInData->Options.ImageCount; ++i) {
		dataBytes += gFormatRecord->imageSize.h * gFormatRecord->imageSize.v >>
			(i << 1);
	}

	dataBytes *= gFormatRecord->planes;

	dataBytes >>= gPlugInData->Options.TextureType == DDT_BGRA || gPlugInData->Options.TextureType == DDT_GREYSCALE ? 0 :
		gPlugInData->Options.TextureType == DDT_DXT1 ? 3 : 2;

	dataBytes += sizeof(DDTHeader) +
		sizeof(DDTImageEntry) * gPlugInData->Options.ImageCount;

	gFormatRecord->minDataBytes = dataBytes;
	gFormatRecord->maxDataBytes = INT_MAX;

	gFormatRecord->data = NULL;
}

void DDTWriteBegin() {

	int32 bytesToWrite;
	
	uint32 magic = DDT_MAGIC;
	uint32 width = gFormatRecord->imageSize.h,
		height = gFormatRecord->imageSize.v;

	PSBufferSuite2* sPSBufferSuite;

	bytesToWrite = 4;
	PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, &magic);

	if (*gResult != noErr)
		return;

	bytesToWrite = sizeof(DDTPlugInData::DDTOptions) - sizeof(float);
	PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, &gPlugInData->Options);

	if (*gResult != noErr)
		return;

	bytesToWrite = 4;
	PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, &width);

	if (*gResult != noErr)
		return;

	PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, &height);

	if (*gResult != noErr)
		return;

	gFormatRecord->theRect.top = 0;
	gFormatRecord->theRect.left = 0;
	gFormatRecord->theRect.bottom = gFormatRecord->imageSize.v;
	gFormatRecord->theRect.right = gFormatRecord->imageSize.h;
	gFormatRecord->loPlane = 0;
	gFormatRecord->hiPlane = gFormatRecord->planes - 1;
	gFormatRecord->colBytes = gFormatRecord->planes;
	gFormatRecord->rowBytes = gFormatRecord->colBytes * gFormatRecord->imageSize.h;
	gFormatRecord->planeBytes = 1;

	switch (gPlugInData->Options.TextureType) {
	case DDT_BGRA:
		gFormatRecord->planeMap[0] = 2;
		gFormatRecord->planeMap[2] = 0;
		break;
	case DDT_DXT5:
		gFormatRecord->planeMap[0] = 3;
		gFormatRecord->planeMap[3] = 0;
	}

	*gResult = gFormatRecord->sSPBasic->AcquireSuite(kPSBufferSuite, kPSBufferSuiteVersion2,
		(const void**)&sPSBufferSuite);

	if (*gResult != noErr)
		return;

	gFormatRecord->data = 
		sPSBufferSuite->New(NULL, gFormatRecord->rowBytes * gFormatRecord->imageSize.v);

	if (gFormatRecord->data == NULL) {
		*gResult = memFullErr;
		return;
	}

	*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kPSBufferSuite, kPSBufferSuiteVersion);
}

void DDTWriteContinue() {

	PSBufferSuite1* sPSBufferSuite;

	Ptr buffer;

	int32 bytesToWrite;

	DDTImageEntry entry;

	int32 entryTableOffset;

	*gResult = gFormatRecord->sSPBasic->AcquireSuite(kPSBufferSuite,
		kPSBufferSuiteVersion1, (const void**)&sPSBufferSuite);

	if (*gResult != noErr)
		goto DDT_ACQUIRE_SUITE_ERROR;

#ifdef MSWindows
		entryTableOffset = SetFilePointer(reinterpret_cast<HANDLE>(gFormatRecord->dataFork),
			0, NULL, FILE_CURRENT);

		if (entryTableOffset == INVALID_SET_FILE_POINTER) {
			*gResult = writErr;
			goto DDT_WRITE_ERROR;
		}
#endif

	*gResult = PSSDKSetFPos(gFormatRecord->dataFork, fsFromStart,
		entryTableOffset +sizeof(DDTImageEntry) * gPlugInData->Options.ImageCount);

	if (*gResult != noErr)
		goto DDT_WRITE_ERROR;

	for (int8 i = 0; i < gPlugInData->Options.ImageCount; ++i) {

		int16 width = gFormatRecord->imageSize.h >> i,
			height = gFormatRecord->imageSize.v >> i;

#ifdef MSWindows
		entry.Offset = SetFilePointer(reinterpret_cast<HANDLE>(gFormatRecord->dataFork),
			0, NULL, FILE_CURRENT);

		if (entry.Offset == INVALID_SET_FILE_POINTER) {
			*gResult = writErr;
			goto DDT_WRITE_ERROR;
		}
#endif

		if (i == 0)
			buffer = static_cast<Ptr>(gFormatRecord->data);
		else {

			if (!DDTMIPMapGenerate(gFormatRecord->data,
				gFormatRecord->imageSize.h, gFormatRecord->imageSize.v,
				gFormatRecord->planes, &buffer, i, sPSBufferSuite)) {
				*gResult = memFullErr;
				goto DDT_WRITE_ERROR;
			}

			Ptr destBuffer = sPSBufferSuite->New(NULL, gFormatRecord->planes * width * height);

			if (destBuffer == NULL) {
				*gResult = memFullErr;
				goto DDT_WRITE_ERROR;
			}

			DDTSharpenImage(buffer, destBuffer, width, height, gFormatRecord->planes, gPlugInData->Options.MIPMapSharpness);
			sPSBufferSuite->Dispose(&buffer);
			buffer = destBuffer;

		}

		switch (gPlugInData->Options.TextureType) {
		case DDT_BGRA:
		case DDT_GREYSCALE:
		{
			bytesToWrite = gFormatRecord->colBytes * width;

			for (int32 r = 0; r < height; ++r) {

				PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, buffer);

				if (*gResult != noErr)
					goto DDT_WRITE_ERROR;
				
				buffer += bytesToWrite;

			}
		}
			break;
		case DDT_DXT1:
		case DDT_DXT3:
		case DDT_DXT5:
		{

			const int8 bytesPerBlock = gPlugInData->Options.TextureType == DDT_DXT1 ? 8 : 16;

			Ptr targetBlock = sPSBufferSuite->New(NULL, bytesPerBlock);

			squish::u8 targetPixels[64];

			if (targetBlock == NULL) {
				*gResult = memFullErr;
				goto DDT_WRITE_ERROR;
			}

			bytesToWrite = bytesPerBlock;

			for (int32 y = 0; y < height; y += 4)
			{
				for (int32 x = 0; x < width; x += 4)
				{
					uint32* targetPixel = reinterpret_cast<uint32*>(targetPixels);

					int32 mask = 0;

					for (int32 py = 0; py < 4; ++py)
					{
						for (int32 px = 0; px < 4; ++px)
						{
							int32 sx = x + px;
							int32 sy = y + py;

							if (sx < width && sy < height)
							{
								*targetPixel++ = *(reinterpret_cast<uint32*>(buffer)
									+ width * sy + sx);

								mask |= (1 << (4 * py + px));
							}
							else
								targetPixel++;
						}
					}

					squish::CompressMasked(targetPixels, mask, targetBlock,
						gPlugInData->Options.TextureType == DDT_DXT1 ? squish::kDxt1 :
						gPlugInData->Options.TextureType == DDT_DXT3 ? squish::kDxt3 : squish::kDxt5);

					PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite, targetBlock);

					if (*gResult != noErr)
						goto DDT_WRITE_ERROR;

				}
			}
		}
		default:
			*gResult = formatBadParameters;
		}

		if (i > 0)
			sPSBufferSuite->Dispose(&buffer);

#ifdef MSWindows
		entry.Size = SetFilePointer(reinterpret_cast<HANDLE>(gFormatRecord->dataFork),
			0, NULL, FILE_CURRENT);

		if (entry.Size == INVALID_SET_FILE_POINTER) {
			*gResult = writErr;
			goto DDT_WRITE_ERROR;
		}

		entry.Size -= entry.Offset;
#endif

		*gResult = PSSDKSetFPos(gFormatRecord->dataFork, fsFromStart,
			entryTableOffset + sizeof(DDTImageEntry) * i);

		if (*gResult != noErr)
			goto DDT_WRITE_ERROR;

		bytesToWrite = sizeof(DDTImageEntry);
		*gResult = PSSDKWrite(gFormatRecord->dataFork, &bytesToWrite,
			&entry);

		if (*gResult != noErr)
			goto DDT_WRITE_ERROR;

		*gResult = PSSDKSetFPos(gFormatRecord->dataFork, fsFromStart,
			entry.Offset + entry.Size);

		if (*gResult != noErr)
			goto DDT_WRITE_ERROR;

		gFormatRecord->advanceState();	
		gFormatRecord->progressProc(i + 1, gPlugInData->Options.ImageCount);
	}

DDT_WRITE_ERROR:

	sPSBufferSuite->Dispose((Ptr*)&gFormatRecord->data);

	*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kPSBufferSuite,
		kPSBufferSuiteVersion1);

DDT_ACQUIRE_SUITE_ERROR:

	gFormatRecord->theRect.top = 0;
	gFormatRecord->theRect.left = 0;
	gFormatRecord->theRect.bottom = 0;
	gFormatRecord->theRect.right = 0;

}