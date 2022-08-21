#ifndef OPT
#define OPT

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
	OPT_ERROR_NONE,
	OPT_ERROR_MATCHES_FULL,
	OPT_ERROR_MALFORMED_ARG,
	OPT_ERROR_UNKNOWN_OPTION,
	OPT_ERROR_UNKNOWN_VALUE,
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

typedef struct {
	const char *long_name;
	size_t long_len;
	const char *short_name;
	size_t short_len;
//	const char *description;
//	enum {
//		OPT_MUST = 1 << 1,
//		OPT_SINGLE = 1 << 2,
//		OPT_MANY = 1 << 3,
//	} flags;
	Opt_Value_Kind value_kind;
	bool required;
} Opt_Info;

typedef struct {
	Opt_Info *opts;
	size_t opts_len;
} Opt_Parser;

typedef enum {
	OPT_MATCH_SIMPLE,
	OPT_MATCH_OPTION,
} Opt_Match_Kind;

typedef struct {
	Opt_Match_Kind kind;
	union {
		const char *simple;
		struct {
			size_t opt;
			Opt_Value value;
		} option;
	};
} Opt_Match;

typedef struct {
	const char *program;
	Opt_Match *matches;
	size_t matches_len;
	size_t matches_size;
} Opt_Result;

void opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len);

Opt_Error opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len);

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc);

#endif
