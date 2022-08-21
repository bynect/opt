#include <stdlib.h>
#include <stdio.h>

#include "opt.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

static const char *errs[] = {
	"OPT_ERROR_NONE",
	"OPT_ERROR_MATCHES_FULL",
	"OPT_ERROR_MALFORMED_ARG",
	"OPT_ERROR_MALFORMED_VALUE",
	"OPT_ERROR_MISSING_NAME",
	"OPT_ERROR_MISSING_VALUE",
	"OPT_ERROR_INVALID_OPTS",
	"OPT_ERROR_UNKNOWN_OPTION",
	"OPT_ERROR_INVALID_MATCHES",
	"OPT_ERROR_UNKNOWN_VALUE",
};

static void print_value(Opt_Value value) {
	switch (value.kind) {
		case OPT_VALUE_NONE:
			printf("<<none>>");
			break;

		case OPT_VALUE_STRING:
			printf("%s", value.vstring);
			break;

		case OPT_VALUE_INT:
			printf("%ld", value.vint);
			break;

		case OPT_VALUE_BOOL:
			printf("%s", value.vbool ? "true" : "false");
			break;
	}
}

static void print_result(Opt_Result result) {
	printf("program: %s\n", result.bin_name);
	for (size_t i = 0; i < result.matches_len; ++i) {
		Opt_Match match = result.matches[i];
		if (match.kind == OPT_MATCH_SIMPLE) {
			printf("simple: %s\n", match.simple);
		} else {
			printf("option: %zu = ", match.option.opt);
			print_value(match.option.value);
			printf("\n");
		}
	}
}

static void check(Opt_Error error) {
	if (error.kind != OPT_ERROR_NONE) {
		printf("%s\n", errs[error.kind]);
		exit(1);
	}
}

int main(int argc, const char **argv) {
	Opt_Result result;
	Opt_Match matches[10];
	opt_result_init(&result, matches, LEN(matches));

 	Opt_Info opts[3];
	check(opt_info_init(&opts[0], "verbose", "v", "Set verbose output", OPT_VALUE_NONE, OPT_INFO_COLLAPSE));
	check(opt_info_init(&opts[1], "", "o", "Set output file path", OPT_VALUE_STRING, 0));
	check(opt_info_init(&opts[2], "must-write", NULL, "Set must-write flag", OPT_VALUE_BOOL, 0));

	Opt_Parser parser;
	check(opt_parser_init(&parser, opts, LEN(opts)));

	check(opt_parser_run(&parser, &result, argv, argc));


	printf("Raw result\n");
	print_result(result);

	opt_result_sort(&result);

	printf("\nSorted result\n");
	print_result(result);

	return 0;
}
