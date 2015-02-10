#ifndef DEC_GETOPT_H
#define DEC_GETOPT_H

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#endif
#include <type_traits>
#include <functional>


namespace dec {

#ifdef WIN32
	typedef TCHAR Ch;
#else
	typedef char Ch;
	const auto& _ftprintf = fprintf;
	const auto& _tcstol = strtol;
	const auto& _tcstod = strtod;
	const auto& _tcsicmp = strcasecmp;
	template<typename T> T _T(T a) {return a;}
#endif

bool getopt_match(
	Ch* arg,
	const char* opt,
	const Ch*& out_name,
	const Ch*& out_value,
	bool& out_long_style
)
{
	if(arg[0] != '-')
		return false;
	arg++;

	bool long_style = arg[0] == '-';
	if(long_style)
		arg++;

	if(arg[0] == '\0')
		return false;

	out_name = arg;
	out_value = nullptr;
	out_long_style = long_style;

compare:
	if(long_style){
		for(size_t i = 0; ; i++){
			Ch c1 = arg[i];
			Ch c2 = (Ch)*opt++;
			if(c1 == '\0' || c1 == '='){
				if(c2 == '\0' || c2 == '|'){
					if(c1 == '='){
						arg[i] = '\0';
						out_value = &arg[i + 1];
					}
					return true;
				}
				goto skip_opt;
			}

			if(c2 == '|')
				goto compare;
			if(c2 == '\0')
				goto not_found;
			if(c1 != c2)
				goto skip_opt;
		}
	}else{
		Ch c = arg[0];
		if((c == opt[0]) && (opt[1] == '\0' || opt[1] == '|')){
			if(arg[1] != '\0'){
				arg[-1] = c;
				arg[0] = '\0';
				out_name = &arg[-1];
				out_value = &arg[1];
			}
			return true;
		}
	}

skip_opt:
	for(;;){
		Ch c = (Ch)*opt++;
		if(c == '|')
			goto compare;
		if(c == '\0')
			break;
	}

not_found:
	return false;
}

bool getopt_handle(
	int& argc,
	Ch** argv,
	const char* opt,
	bool bool_style,
	const Ch*& out_name,
	const Ch*& out_value)
{
	int i;
	const Ch* name;
	const Ch* value;
	bool long_style;

	const auto n = argc;
	for(i = 1; i < n; i++){
		Ch* arg = argv[i];
		if(arg == nullptr)
			continue;
		if(arg[0] == '-' && arg[1] == '-' && arg[2] == '\0')
			break;
		if(getopt_match(arg, opt, name, value, long_style))
			goto match;
	}

	out_name = nullptr;
	return true;

match:
	out_name = name;
	argv[i] = nullptr;

	if(value == nullptr && !bool_style){
		if(!long_style)
			goto param_need;

		if(i + 1 >= n)
			goto param_need;

		value = argv[i + 1];
		if(value == nullptr || value[0] == '-')
			goto param_need;

		argv[i + 1] = nullptr;
	}
	out_value = value;

	return true;

param_need:
	_ftprintf(stderr, _T("Need parameter : %s\n"), name);
	return false;
}

template<typename T>
bool getopt_emit(const Ch* name, const Ch* value, T* receiver)
{
	static_assert(sizeof(T) == 0, "Unsupported type");
	return false;
}

bool getopt_emit(const Ch* name, const Ch* value, bool* receiver)
{
	if(value != nullptr){
		if(_tcsicmp(value, _T("true")) == 0)
			*receiver = true;
		else if(_tcsicmp(value, _T("false")) == 0)
			*receiver = false;
		else{
			_ftprintf(stderr, _T("Cannot convert to boolean type : %s\n"), value);
			return false;
		}
	}

	*receiver = true;

	return true;
}

bool getopt_emit(const Ch* name, const Ch* value, int* receiver)
{
	Ch* endptr;

	errno = 0;
	*receiver = _tcstol(value, &endptr, 0);// 0=auto

	if(*endptr != '\0' || errno != 0){
		_ftprintf(stderr, _T("Cannot convert to integer type : %s\n"), value);
		return false;
	}

	return true;
}

bool getopt_emit(const Ch* name, const Ch* value, double* receiver)
{
	Ch* endptr;

	errno = 0;
	*receiver = _tcstod(value, &endptr);

	if(*endptr != '\0' || errno != 0){
		_ftprintf(stderr, _T("Cannot convert to floating-point type : %s\n"), value);
		return false;
	}

	return true;
}

bool getopt_emit(const Ch* name, const Ch* value, const Ch** receiver)
{
	*receiver = value;
	return true;
}

bool getopt_emit(const Ch* name, const Ch* value, std::function<bool()> receiver)
{
	return receiver();
}

bool getopt_emit(const Ch* name, const Ch* value, std::function<bool(const Ch*)> receiver)
{
	return receiver(name);
}

bool getopt_emit(const Ch* name, const Ch* value, std::function<bool(const Ch*, const Ch*)> receiver)
{
	return receiver(name, value);
}

bool getopt_imp(int& argc, Ch** argv)
{
	const auto n = argc;
	int i = 1;
	int d = 1;

	for(; i < n; i++){
		Ch* arg = argv[i];
		if(arg == nullptr)
			continue;

		if(arg[0] == '-'){
			if(arg[1] == '-' && arg[2] == '\0'){
				i++;
				goto simple;
			}else{
				_ftprintf(stderr, _T("Unrecognized option : %s\n"), arg);
				return false;
			}
		}

		argv[d++] = arg;
	}
	goto end;

simple:
	for(; i < n; i++){
		Ch* arg = argv[i];
		if(arg != nullptr)
			argv[d++] = arg;
	}

end:
	argc = d;
	return true;
}

template<typename R, typename... Args>
bool getopt_imp(int& argc, Ch** argv, const char* opt, R receiver, Args... args)
{
	const Ch* name;
	const Ch* value;

	const bool bool_style =
		std::is_same<R, bool*>() ||
		std::is_same<R, std::function<bool()> >() ||
		std::is_same<R, std::function<bool(const Ch*)> >();

	if(!getopt_handle(argc, argv, opt, bool_style, name, value))
		return false;

	if(name != nullptr){
		if(!getopt_emit(name, value, receiver))
			return false;
	}

	return getopt_imp(argc, argv, args...);
}

template<typename... Args>
bool getopt(int& argc, Ch** argv, Args... args)
{
	return getopt_imp(argc, argv, args...);
}

}

#endif
