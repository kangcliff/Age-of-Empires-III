// Copyright(c) 2016 Cliff Kang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"),
// to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "DDTPlugIn.h"

void DDTReadBegin() {
	
	DDTHeader header;

	int32 bytesToRead;

	bytesToRead = sizeof(DDTHeader);
	*gResult = PSSDKRead(gFormatRecord->dataFork, &bytesToRead, &header);

	if (*gResult != noErr)
		return;

	if (header.Magic != DDT_MAGIC) {
		*gResult = formatCannotRead;
		return;
	}

	gPlugInData->InputTextureType = header.TextureType;

	gFormatRecord->imageMode = header.TextureType == DDT_GREYSCALE ? plugInModeGrayScale : plugInModeRGBColor;
	gFormatRecord->imageSize.h = header.Width;
	gFormatRecord->imageSize.v = header.Height;
	gFormatRecord->depth = 8;
	gFormatRecord->planes = header.TextureType == DDT_GREYSCALE ? 1 : 4;

}

void DDTReadContinue() {
	
	DDTImageEntry entry;

	PSBufferSuite1* sPSBufferSuite;

	int32 bytesToRead;

	bytesToRead = sizeof(DDTImageEntry);
	*gResult = PSSDKRead(gFormatRecord->dataFork, &bytesToRead, &entry);

	if (*gResult != noErr)
		goto DDT_BAD_ENTRY;

	*gResult = PSSDKSetFPos(gFormatRecord->dataFork, fsFromStart, entry.Offset);

	if (*gResult != noErr)
		goto DDT_BAD_ENTRY;

	gFormatRecord->loPlane = 0;
	gFormatRecord->hiPlane = gFormatRecord->planes - 1;
	gFormatRecord->colBytes = gFormatRecord->planes;
	gFormatRecord->planeBytes = 1;

	*gResult = gFormatRecord->sSPBasic->AcquireSuite(kPSBufferSuite,
		kPSBufferSuiteVersion1, (const void**)&sPSBufferSuite);

	if (*gResult != noErr)
		goto DDT_BAD_ENTRY;

	switch (gPlugInData->InputTextureType) {
	case DDT_BGRA:
		gFormatRecord->planeMap[0] = 2;
		gFormatRecord->planeMap[2] = 0;
	case DDT_GREYSCALE:
	{
		Ptr buffer;

		gFormatRecord->theRect.left = 0;
		gFormatRecord->theRect.right = gFormatRecord->imageSize.h;
		gFormatRecord->rowBytes = gFormatRecord->colBytes * gFormatRecord->imageSize.h;

		buffer = sPSBufferSuite->New(NULL, gFormatRecord->rowBytes);

		if (buffer == NULL) {
			*gResult = memFullErr;
			goto DDT_READ_ERROR;
		}

		gFormatRecord->data = buffer;
		bytesToRead = gFormatRecord->rowBytes;
		
		for (int32 r = 0; r < gFormatRecord->imageSize.v; ++r) {

			gFormatRecord->theRect.top = r;
			gFormatRecord->theRect.bottom = r + 1;

			*gResult = PSSDKRead(gFormatRecord->dataFork, &bytesToRead, buffer);

			if (*gResult != noErr)
				goto DDT_READ_ERROR;

			gFormatRecord->advanceState();
			gFormatRecord->progressProc(gFormatRecord->theRect.bottom, gFormatRecord->imageSize.v);

		}

		sPSBufferSuite->Dispose(&buffer);
	}	
		break;
	case DDT_DXT5:
		gFormatRecord->planeMap[0] = 3;
		gFormatRecord->planeMap[3] = 0;
	case DDT_DXT1:
	case DDT_DXT3:
	{
		const int8 bytesPerBlock = gPlugInData->InputTextureType == DDT_DXT1 ? 8 : 16;
		Ptr sourceBlock = sPSBufferSuite->New(NULL, bytesPerBlock);
		int32 blocks = 0;
		squish::u8 targetPixels[64];

		if (sourceBlock == NULL) {
			*gResult = memFullErr;
			goto DDT_READ_ERROR;
		}

		gFormatRecord->data = targetPixels;
		bytesToRead = bytesPerBlock;

		for (int32 y = 0; y < gFormatRecord->imageSize.v; y += 4) {
			for (int32 x = 0; x < gFormatRecord->imageSize.h; x += 4) {
				int32 offset = 0;

				gFormatRecord->theRect.left = x;
				gFormatRecord->theRect.right = min(x + 4, gFormatRecord->imageSize.h);
				gFormatRecord->theRect.top = y;
				gFormatRecord->theRect.bottom = min(y + 4, gFormatRecord->imageSize.v);
				gFormatRecord->rowBytes = gFormatRecord->colBytes * (gFormatRecord->theRect.right - gFormatRecord->theRect.left);

				*gResult = PSSDKRead(gFormatRecord->dataFork, &bytesToRead, sourceBlock);

				if (*gResult != noErr)
					goto DDT_READ_ERROR;
				
				squish::Decompress(targetPixels, sourceBlock,
					gPlugInData->InputTextureType == DDT_DXT1 ? squish::kDxt1 :
					gPlugInData->InputTextureType == DDT_DXT3 ? squish::kDxt3 : squish::kDxt5);

				if (x + 4 < gFormatRecord->imageSize.h &&
					y + 4 < gFormatRecord->imageSize32.v)
					goto ADVANCE;

				for (int32 py = 0; py < 4; ++py) {
					for (int32 px = 0; px < 4; ++px) {

						int32 sx = x + px;
						int32 sy = y + py;

						if (sx >= gFormatRecord->imageSize.h ||
							sy >= gFormatRecord->imageSize.v) {

							// move remaining pixels forward

							uint32* targetPixel = reinterpret_cast<uint32*>(targetPixels) + 4 * py + px - offset;

							for (int32 i = 4 * py + px; i < 15; ++i)
								*targetPixel++ = *(targetPixel + 1);
							
							++offset;
						}
					}
				}

			ADVANCE:

				gFormatRecord->advanceState();
				gFormatRecord->progressProc(++blocks,
					(gFormatRecord->imageSize.h * gFormatRecord->imageSize.v) >> 4);

			}
		}

		sPSBufferSuite->Dispose(&sourceBlock);
	}
		break;
	default:
		*gResult = formatCannotRead;
	}


DDT_READ_ERROR:

	*gResult = gFormatRecord->sSPBasic->ReleaseSuite(kPSBufferSuite,
		kPSBufferSuiteVersion1);

DDT_BAD_ENTRY:

	gFormatRecord->data = NULL;
}

void DDTFilterFile() {

	DDTHeader header;

	int32 bytesToRead = sizeof(DDTHeader);

	*gResult = PSSDKRead(gFormatRecord->dataFork, &bytesToRead, &header);

	if (*gResult != noErr)
		return;

	if (header.Magic != DDT_MAGIC)
		*gResult = formatCannotRead;

}
