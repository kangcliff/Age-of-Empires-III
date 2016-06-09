#define plugInName "DDT"

#include "PIGeneral.h"

#define ResourceID 16000

resource 'PiPL' (ResourceID, plugInName " PiPL", purgeable)
{
    {
		Kind { ImageFormat },
		Name { "DDT" },
		Version { (latestFormatVersion << 16) | latestFormatSubVersion },

		#ifdef __PIMac__
			#if (defined(__x86_64__))
				CodeMacIntel64 { "PluginMain" },
			#endif
			#if (defined(__i386__))
				CodeMacIntel32 { "PluginMain" },
			#endif
		#else
			#if defined(_WIN64)
				CodeWin64X86 { "PluginMain" },
			#else
				CodeWin32X86 { "PluginMain" },
			#endif
		#endif

		SupportedModes
		{
			noBitmap, doesSupportGrayScale,
			noIndexedColor, doesSupportRGBColor,
			noCMYKColor, noHSLColor,
			noHSBColor, noMultichannel,
			noDuotone, noLABColor
		},

		EnableInfo { "in (PSHOP_ImageMode, GrayScaleMode, RGBMode) &&"
		"PSHOP_ImageDepth == 8 &&"
		"PSHOP_NumTrueChannels == 1 || PSHOP_NumTrueChannels == 4" },

		PlugInMaxSize { 2147483647, 2147483647 },

		FormatMaxSize { { 32767, 32767 } },

		FormatMaxChannels { {   0, 1, 0, 4, 0, 0, 
							   0, 0, 0, 0, 0, 0 } },
	
		FmtFileType { 'DDT ', '8BIM' },
		FilteredTypes { { '8B1F', '    ' } },
		ReadExtensions { { 'DDT ' } },
		WriteExtensions { { 'DDT ' } },
		FilteredExtensions { { 'DDT ' } },

		FormatFlags { fmtDoesNotSavesImageResources,
		              fmtCanRead, 
					  fmtCanWrite, 
					  fmtCanWriteIfRead, 
					  fmtCannotWriteTransparency,
					  fmtCannotCreateThumbnail },
		FormatICCFlags { 	iccCannotEmbedGray,
							iccCannotEmbedIndexed,
							iccCannotEmbedRGB,
							iccCannotEmbedCMYK }
	},
};

