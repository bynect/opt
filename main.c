#include <stdlib.h>
#include <stdio.h>

#include "opt.h"

#define LEN(x) (sizeof(x) / sizeof(*x))

static const char *errs[] = {
	"OPT_ERROR_NONE",
	"OPT_ERROR_MISSING_VALUE",
	"OPT_ERROR_UNKNOWN_OPTION",
	"OPT_ERROR_DUPLICATE_OPTION",
	"OPT_ERROR_INVALID_VALUE",
};

static void print_result(Opt_Result result) {
	printf("program: %s\n", result.bin_name);
	for (size_t i = 0; i < result.matches_len; ++i) {
		Opt_Match match = result.matches[i];
		if (match.kind == OPT_MATCH_SIMPLE) {
			printf("simple: %s\n", match.simple);
		} else if (match.kind == OPT_MATCH_OPTION) {
			printf("option: %zu = ", match.option.opt);
			opt_value_print(match.option.value);
			printf("\n");
		} else {
			printf("missing: %zu\n", match.missing_opt);
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

	Opt_Info opts[5];
	opt_info_init(&opts[0], "help", "h", "Show help information", OPT_VALUE_NONE, NULL, OPT_INFO_KEEP_FIRST);
	opt_info_init(&opts[1], "verbose", "v", "Set verbose output", OPT_VALUE_NONE, NULL, OPT_INFO_KEEP_FIRST);
	opt_info_init(&opts[2], "", "o", "Set output file path", OPT_VALUE_STRING, "FILE", OPT_INFO_REQUIRED);
	opt_info_init(&opts[3], "must-write", NULL, "Set must-write flag", OPT_VALUE_BOOL, NULL, OPT_INFO_KEEP_LAST);
	opt_info_init(&opts[4], "number", NULL, "Set number", OPT_VALUE_INT, NULL, OPT_INFO_NO_DUPLICATE);

	Opt_Parser parser;
	opt_parser_init(&parser, opts, LEN(opts));

	check(opt_parser_run(&parser, &result, argv, argc));

	printf("Raw result\n");
	print_result(result);

	opt_result_sort(&result, true);

	printf("\nSorted result\n");
	print_result(result);

	printf("\n");
	if (result.matches[0].kind == OPT_MATCH_OPTION && result.matches[0].option.opt == 0) {
		opt_info_help(opts, LEN(opts), "Usage: x [options]", NULL, NULL);

		printf("\n");
		opt_info_usage(opts, LEN(opts), result.bin_name);
	}

	return 0;
}
