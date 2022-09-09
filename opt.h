#ifndef OPT
#define OPT

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
	OPT_VALUE_NONE,
	OPT_VALUE_STRING,
	OPT_VALUE_INT,
	OPT_VALUE_FLOAT,
	OPT_VALUE_BOOL,
} Opt_Value_Kind;

typedef struct {
	Opt_Value_Kind kind;
	union {
		const char *vstring;
		int64_t vint;
		double vfloat;
		bool vbool;
	};
} Opt_Value;

typedef enum {
	OPT_ERROR_NONE,
	OPT_ERROR_STOPPED,
	OPT_ERROR_UNKNOWN_OPTION,
	OPT_ERROR_DUPLICATE_OPTION,
	OPT_ERROR_MISSING_VALUE,
	OPT_ERROR_INVALID_VALUE,
} Opt_Error_Kind;

typedef struct {
	Opt_Error_Kind kind;
	union {
		const char *unknown_opt;
		struct {
			size_t opt;
			Opt_Value value;
		} duplicate;
		struct {
			size_t opt;
			Opt_Value_Kind expected_value;
		} missing;
		struct {
			Opt_Value_Kind expected_value;
			const char *base;
		} invalid;
	};
} Opt_Error;

typedef enum {
	OPT_INFO_NONE = 0,
	OPT_INFO_MATCH_NONE = 1 << 0,
	OPT_INFO_MATCH_FIRST = 1 << 1,
	OPT_INFO_MATCH_LAST = 1 << 2,
	OPT_INFO_MATCH_MISSING = 1 << 3, // NOTE: Easy to mistake with OPT_MATCH_INFO
	OPT_INFO_STOP_DUPLICATE = 1 << 4,
	OPT_INFO_STOP_PARSER = 1 << 5,
	//OPT_INFO_STACK,
} Opt_Info_Flag;

typedef struct {
	const char *long_name;
	size_t long_len;
	const char *short_name;
	size_t short_len;
	const char *desc;
	Opt_Value_Kind value_kind;
	const char *value_name;
	Opt_Info_Flag flags;
	size_t _seen;
	size_t _match;
} Opt_Info;

typedef struct {
	Opt_Info *opts;
	size_t opts_len;
} Opt_Parser;

typedef struct {
	const char *name;
	const char **args;
	size_t args_len;
	size_t line_max;
} Opt_Usage;

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
	size_t simple;
	size_t option;
	size_t missing;
} Opt_Result;

typedef void (*Opt_Result_Simple_F)(const char *simple);

typedef void (*Opt_Result_Option_F)(Opt_Value value, bool missing);

Opt_Error opt_value_read(Opt_Value *value, const char *base);

void opt_value_print(Opt_Value value, FILE *file);

void opt_info_init(Opt_Info *info, const char *long_name, const char *short_name, const char *desc, Opt_Value_Kind value_kind, const char *value_name, Opt_Info_Flag flags);

void opt_info_usage(Opt_Info *opts, size_t opts_len, Opt_Usage *usage, FILE *file);

void opt_info_help(Opt_Info *opts, size_t opts_len, const char *head_note, const char *foot_note, Opt_Usage *usage, FILE *file);

void opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len);

void opt_result_sort(Opt_Result *result, bool sort_opt);

void opt_result_iter(Opt_Result *result, Opt_Result_Simple_F simple_f, Opt_Result_Option_F *opt_fs);

void opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len);

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc);

#endif
