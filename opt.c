#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "opt.h"

static Opt_Error error(Opt_Error_Kind kind, void *payload) {
	return (Opt_Error) {
		.kind = kind,
		.payload = payload,
	};
}

static Opt_Error error_simple(Opt_Error_Kind kind) {
	return error(kind, NULL);
}

static Opt_Value value_none() {
	return (Opt_Value) {
		.kind = OPT_VALUE_NONE,
	};
}

static Opt_Error value_read(Opt_Value *value, const char *base) {
	switch (value->kind) {
		case OPT_VALUE_STRING:
			value->vstring = base;
			break;

		case OPT_VALUE_INT:
			value->vint = atoi(base); // XXX
			break;

		case OPT_VALUE_BOOL:
			value->vbool = !strcmp(base, "t") || !strcmp(base, "T") || !strcmp(base, "true"); // XXX
			break;

		default:
			return error(OPT_ERROR_UNKNOWN_VALUE, (void *)base);
	}
	return error_simple(OPT_ERROR_NONE);
}

static Opt_Match match_simple(const char *simple) {
	return (Opt_Match) {
		.kind = OPT_MATCH_SIMPLE,
		.simple = simple,
	};
}

static Opt_Match match_option(size_t opt, Opt_Value value) {
	return (Opt_Match) {
		.kind = OPT_MATCH_OPTION,
		.option = {
			.opt = opt,
			.value = value,
		},
	};
}

void opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len) {
	result->program = NULL;
	result->matches = matches;
	result->matches_len = 0;
	result->matches_size = matches_len;

	assert(matches != NULL);
	assert(matches_len != 0);
}

Opt_Error opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len) {
	parser->opts = opts;
	parser->opts_len = opts_len;

	assert(opts != NULL);
	assert(opts_len != 0);

	for (size_t opt = 0; opt < opts_len; ++opt) {
		Opt_Info *opti = &opts[opt];
		assert(opti->short_name != NULL || opti->long_name != NULL);
		if (opti->short_name != NULL && opti->short_len == 0) {
			opti->short_len = strlen(opti->short_name);
		}

		if (opti->long_name != NULL && opti->long_len == 0) {
			opti->long_len = strlen(opti->long_name);
		}
	}

	return error_simple(OPT_ERROR_NONE);
}

static Opt_Error result_push(Opt_Result *result, Opt_Match match) {
	if (result->matches_len + 1 >= result->matches_size) {
		return error_simple(OPT_ERROR_MATCHES_FULL);
	}

	result->matches[result->matches_len++] = match;
	return error_simple(OPT_ERROR_NONE);
}

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc) {
	for (int arg = 1; arg < argc; ++arg) {
		const char *argi = argv[arg];

		if (argi[0] == '-') {
			Opt_Match match = { 0 };
			bool found = false;

			if (argi[1] == '-') {
				// long
				const char *base = &argi[2];
				for (size_t opt = 0; opt < parser->opts_len; ++opt) {
					Opt_Info *info = &parser->opts[opt];
					Opt_Value value = { 0 };

					if (!strncmp(base, info->long_name, info->long_len)) {
						if (info->value_kind != OPT_VALUE_NONE) {
							const char *base_value = NULL;
							if (base[info->long_len] == '=') {
								base_value = &base[info->long_len + 1];
							} else {
								if (arg + 1 >= argc) assert(false);
								else {
									base_value = argv[++arg];
									assert(strcmp(base_value, "--"));
								}
							}

							value.kind = info->value_kind;
							Opt_Error error = value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else if (base[info->long_len] == '\0') value = value_none();
						else return error(OPT_ERROR_MALFORMED_ARG, (void *)argi);

						match = match_option(opt, value);
						found = true;
						break;
					}
				}
			} else {
				// short
				const char *base = &argi[1];
				for (size_t opt = 0; opt < parser->opts_len; ++opt) {
					Opt_Info *info = &parser->opts[opt];
					Opt_Value value = { 0 };

					if (!strncmp(base, info->short_name, info->short_len)) {
						if (info->value_kind != OPT_VALUE_NONE) {
							const char *base_value = NULL;
							if (base[info->short_len] == '=') {
								base_value = &base[info->short_len + 1];
							} else {
								if (arg + 1 >= argc) assert(false);
								else {
									base_value = argv[++arg];
									assert(strcmp(base_value, "--"));
								}
							}

							value.kind = info->value_kind;
							Opt_Error error = value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else if (base[info->short_len] == '\0') value = value_none();
						else return error(OPT_ERROR_MALFORMED_ARG, (void *)argi);

						match = match_option(opt, value);
						found = true;
						break;
					}
				}
			}

			if (found) {
				Opt_Error error = result_push(result, match);
				if (error.kind != OPT_ERROR_NONE) return error;
			} else return error(OPT_ERROR_UNKNOWN_OPTION, (void *)argi);
		} else {
			Opt_Error error = result_push(result, match_simple(argi));
			if (error.kind != OPT_ERROR_NONE) return error;
		}
	}

	result->program = argv[0];
	return error_simple(OPT_ERROR_NONE);
}
