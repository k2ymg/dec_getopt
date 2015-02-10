#include "../dec_getopt.h"

#ifndef WIN32
#define TCHAR char
#define _T(x) x
#define _tprintf printf
#define _ftprintf fprintf
#define _tcscmp strcmp
#define _tstoi atoi
#endif

static void print_usage()
{
	printf("Usage: test_getopt [options] <inputs...>\n");
	printf("  -h, --help       Print this text\n");
	printf("  -O<#>            Integer value\n");
	printf("  --double=<#>     Double value\n");
	printf("  -o<#>, --out=<#> String value\n");
	printf("  --verbose        print many log.\n");
	printf("  --quiet          do not print log\n");
	printf("  --threads=<#>    min, max, or integer number.\n");
	printf("\n");
	printf("example:\n");
	printf("test_getopt -O3 --quiet --threads=max foo.c bar.c -ohoge.exe\n");
}

#ifdef WIN32
int _tmain(int argc, TCHAR** argv)
#else
int main(int argc, char** argv)
#endif
{
	if(argc < 2){
		print_usage();
		return 0;
	}

	bool print_help = false;
	int integer_value = 0;
	double double_value = 1.0;
	const TCHAR* string_value = _T("");
	int log_level = 1;
	int threads = 10;

	std::function<bool(const TCHAR*)> log_level_option;
	log_level_option = [&log_level](const TCHAR* name) -> bool
	{
		if(_tcscmp(name, _T("verbose")) == 0){
			log_level = 2;
		}else if(_tcscmp(name, _T("quiet")) == 0){
			log_level = 0;
		}
		return true;
	};

	std::function<bool(const TCHAR*, const TCHAR*)> threads_option;
	threads_option = [&threads](const TCHAR* name, const TCHAR* value) -> bool
	{
		if(_tcscmp(value, _T("max")) == 0){
			threads = 99;
		}else if(_tcscmp(value, _T("min")) == 0){
			threads = 1;
		}else{
			int n = _tstoi(value);
			if(n < 0 || 99 < n){
				_ftprintf(stderr, _T("Invalid parameter: %s = %s\n"), name, value);
				return false;
			}
			threads = n;
		}
		return true;
	};

	bool succeeded = dec::getopt(argc, argv,
		"h|help", &print_help,
		"O", &integer_value,
		"double", &double_value,
		"o|out", &string_value,
		"verbose", log_level_option,
		"quiet", log_level_option,
		"threads", threads_option);
	if(!succeeded)
		return 0;

	if(print_help){
		print_usage();
		return 0;
	}

	_tprintf(_T(" integer value: %d\n"), integer_value);
	_tprintf(_T("  double value: %f\n"), double_value);
	_tprintf(_T("  string value: %s\n"), string_value);
	_tprintf(_T("     log level: %d\n"), log_level);
	_tprintf(_T("       threads: %d\n"), threads);
	_tprintf(_T("\n"));

	_tprintf(_T("remains: %d\n"), argc - 1);
	for(int i = 1; i < argc; i++)
		_tprintf(_T(" - %s\n"), argv[i]);

	return 0;
}
