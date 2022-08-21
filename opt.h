#ifndef OPT
#define OPT

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	OPT_ERROR_NONE,
	// Developer
	OPT_ERROR_FULL_MATCHES,
	OPT_ERROR_INVALID_MATCHES,
	OPT_ERROR_INVALID_OPTS,
	OPT_ERROR_MISSING_NAME,
	// User
	OPT_ERROR_MISSING_VALUE,
	OPT_ERROR_UNKNOWN_OPTION,
	OPT_ERROR_UNKNOWN_VALUE,
	OPT_ERROR_INVALID_VALUE,
} Opt_Error_Kind;

typedef struct {
	Opt_Error_Kind kind;
	void *payload;
} Opt_Error;

typedef enum {
	OPT_VALUE_NONE,
	OPT_VALUE_STRING,
	OPT_VALUE_INT,
	OPT_VALUE_BOOL,
} Opt_Value_Kind;

typedef struct {
	Opt_Value_Kind kind;
	union {
		const char *vstring;
		int64_t vint;
		bool vbool;
	};
} Opt_Value;

typedef enum {
	OPT_INFO_COLLAPSE = 1 << 1,
	OPT_INFO_KEEP_LAST = 1 << 2,
	OPT_INFO_REPORT_MISSING = 1 << 3,
} Opt_Info_Flag;

typedef struct {
	const char *long_name;
	size_t long_len;
	const char *short_name;
	size_t short_len;
	const char *desc;
	Opt_Value_Kind value_kind;
	Opt_Info_Flag flags;
	size_t _seen;
	size_t _match;
} Opt_Info;

typedef struct {
	Opt_Info *opts;
	size_t opts_len;
} Opt_Parser;

typedef enum {
	OPT_MATCH_SIMPLE,
	OPT_MATCH_OPTION,
	OPT_MATCH_MISSING,
} Opt_Match_Kind;

typedef struct {
	Opt_Match_Kind kind;
	union {
		const char *simple;
		struct {
			size_t opt;
			Opt_Value value;
		} option;
		size_t missing_opt;
	};
} Opt_Match;

typedef struct {
	const char *bin_name;
	Opt_Match *matches;
	size_t matches_len;
	size_t matches_size;
} Opt_Result;

Opt_Error opt_info_init(Opt_Info *info, const char *long_name, const char *short_name, const char *desc, Opt_Value_Kind value_kind, Opt_Info_Flag flags);

Opt_Error opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len);

void opt_result_sort(Opt_Result *result);

Opt_Error opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len);

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc);

#endif
