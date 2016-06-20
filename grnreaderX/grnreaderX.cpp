#include <cstdio>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include "Granny.h"
#include "maxscript.h"

const char * prog_title = "grnreaderX";

void __cdecl log(int a, int b, char* msg)
{
	MessageBoxA(GetActiveWindow(), msg, prog_title, MB_ICONWARNING | MB_OK);
}

t_Logger logger = { &log, &log };

void msg_printf(const char * format, ... )
{
	char * msg;
	const char * s = format;
	size_t length = 3;
	va_list ap;

	va_start(ap, format);
	do {
		length += strlen(s) - 2;
		s = va_arg(ap, const char*);
	} while (s);
	va_end(ap);

	msg = new char[length];

	va_start(ap, format);
	vsprintf_s(msg, length, format, ap);
	va_end(ap);
	MessageBoxA(GetActiveWindow(), msg, prog_title, MB_ICONERROR | MB_OK);

	delete[] msg;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {
	int files_count;
	char ** files = NULL;
	int data_only = 0;

	if (__argc > 1 && _stricmp(__argv[1], "-d") == 0)
		data_only = 1;

	if (__argc < 2 + data_only)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[4 * MAX_PATH] = "";
		char * p;
		size_t length;

		ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = GetActiveWindow();
		ofn.lpstrFilter = "Granny 2 Run-time File (*.gr2)\0*.gr2\0All files (*.*)\0*.*\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST;

		if (!GetOpenFileNameA(&ofn))
			return EXIT_FAILURE;

		files_count = 0;
		p = szFile;
		length = strlen(p) + 1;
		while (length > 1) {
			char * file = (char*)malloc(length);
			strcpy_s(file, length, p);
			files = (char **)realloc(files, 4 * ++files_count);
			files[files_count - 1] = file;
			p += length;
			length = strlen(p) + 1;
		}

		if (files_count > 1) {
			std::string dir = files[0];
			dir += '\\';
			const size_t dl = dir.length() + 1;
			for (int i = 1; i < files_count; ++i) {
				char * file;
				length = dl + strlen(files[i]);
				file = (char*)malloc(length);
				strcpy_s(file, length, dir.c_str());
				strcat_s(file, length, files[i]);
				free(files[i]);
				files[i] = file;
			}
			free(files[0]);
			--files_count;
			++files;
		}
	}
	else {
		files_count = __argc - (1 + data_only);
		files = __argv + (1 + data_only);
	}

	LoadStuff();
	(*GrannySetLogCallback)(&logger);

	const char* file = files[0], gx_script[] = "grnreaderX.ms";
	std::stringstream buf;
	std::string dir, script = file, filename;
	std::ofstream fs;

	// reset current directory
	dir.resize(MAX_PATH);
	GetModuleFileNameA(NULL, &dir[0], MAX_PATH);
	dir = dir.substr(0, dir.find_last_of('\\'));
	SetCurrentDirectoryA(dir.c_str());

	script = script.substr(0, script.find_last_of('.'));
	script += ".ms";

	if (!data_only) {
		if (!CopyFileA(gx_script, script.c_str(), FALSE)) {
			MessageBoxA(GetActiveWindow(), "Couldn't copy 'grnreaderX.ms'", prog_title, MB_ICONERROR | MB_OK);
			return EXIT_FAILURE;
		}
		buf << "\n\n";
	}

	for (int i = 0; i < files_count; ++i) {
		std::stringstream ss;
		std::string filespec = files[i];
		filespec = filespec.substr(filespec.find_last_of('\\') + 1, filespec.length());

		switch (OutputMAXScript(ss, files[i])) {
		case MS_OK:
		{
			if (!data_only)
				buf << "--- " << filespec << " ---\n"
				<< ss.str() << "\n\n";
			else
				buf << ss.str();
		}
			break;
		case MS_FILE_ERROR:
			msg_printf("Couldn't read file '%s' - maybe it's not the right name, or not in the right folder.",
				filespec.c_str());
			break;
		case MS_INFO_ERROR:
			msg_printf("Couldn't get GR2 FileInfo of '%s'. I have no idea why.",
				filespec.c_str());
			break;
		case MS_FD_ERROR:
			msg_printf("Couldn't convert FD Vertices of '%s'. Either the magic value mismatches or the data is corrupted.",
				filespec.c_str());
		}
	}

	if (!data_only)
		buf << "--- import to scene ---\ngx_importer.import()\n";

	fs.open(script, data_only ? 0 : std::ios::app);
	fs << buf.str();
	fs.close();

	return EXIT_SUCCESS;	
}
