#include <stdlib.h>
#include <stdio.h>

#include "opt.h"

static const char *errs[] = {
	"OPT_ERROR_NONE",
	"OPT_ERROR_MATCHES_FULL",
	"OPT_ERROR_MALFORMED_ARG",
	"OPT_ERROR_UNKNOWN_OPTION",
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

void check(Opt_Error error) {
	if (error.kind != OPT_ERROR_NONE) {
		printf("%s\n", errs[error.kind]);
		exit(1);
	}
}

int main(int argc, const char **argv) {
	Opt_Error error;
	Opt_Match matches[10];

	Opt_Result result;
	opt_result_init(&result, matches, 10);

 	Opt_Info opts[3] = {
		(Opt_Info) {
			.long_name = "verbose",
			.short_name = "v",
			.value_kind = OPT_VALUE_NONE,
		},
		(Opt_Info) {
			.short_name = "o",
			.value_kind = OPT_VALUE_STRING,
		},
		(Opt_Info) {
			.long_name = "must-write",
			.value_kind = OPT_VALUE_NONE,
		},
	};

	Opt_Parser parser;
	error = opt_parser_init(&parser, opts, 3);
	check(error);

	error = opt_parser_run(&parser, &result, argv, argc);
	check(error);

	printf("program: %s\n", result.program);

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

	return 0;
}
