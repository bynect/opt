#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opt.h"

static inline Opt_Value value_none() {
	return (Opt_Value) {
		.kind = OPT_VALUE_NONE,
	};
}

static inline Opt_Error error_none() {
	return (Opt_Error) {
		.kind = OPT_ERROR_NONE,
	};
}

static inline Opt_Error error_unknown(const char *name) {
	return (Opt_Error) {
		.kind = OPT_ERROR_UNKNOWN_OPTION,
		.unknown_opt = name,
	};
}

static inline Opt_Error error_duplicate(size_t opt, Opt_Value value) {
	return (Opt_Error) {
		.kind = OPT_ERROR_DUPLICATE_OPTION,
		.duplicate = {
			.opt = opt,
			.value = value,
		},
	};
}

static inline Opt_Error error_missing(size_t opt, Opt_Value_Kind expected_value) {
	return (Opt_Error) {
		.kind = OPT_ERROR_MISSING_VALUE,
		.missing = {
			.opt = opt,
			.expected_value = expected_value,
		},
	};
}

static inline Opt_Error error_invalid(Opt_Value_Kind expected_value, const char *base) {
	return (Opt_Error) {
		.kind = OPT_ERROR_INVALID_VALUE,
		.invalid = {
			.expected_value = expected_value,
			.base = base,
		},
	};
}

static inline Opt_Match match_simple(const char *simple) {
	return (Opt_Match) {
		.kind = OPT_MATCH_SIMPLE,
		.simple = simple,
	};
}

static inline Opt_Match match_option(size_t opt, Opt_Value value) {
	return (Opt_Match) {
		.kind = OPT_MATCH_OPTION,
		.option = {
			.opt = opt,
			.value = value,
		},
	};
}

static inline Opt_Match match_missing(size_t opt) {
	return (Opt_Match) {
		.kind = OPT_MATCH_MISSING,
		.missing_opt = opt,
	};
}

static inline Opt_Error int_read(int64_t *vint, const char *base) {
	char *end = NULL;
	*vint = strtol(base, &end, 0);
	if (end[0] != '\0') return error_invalid(OPT_VALUE_INT, base);
	return error_none();
}

static inline Opt_Error float_read(double *vfloat, const char *base) {
	char *end = NULL;
	*vfloat = strtod(base, &end);
	if (end[0] != '\0') return error_invalid(OPT_VALUE_FLOAT, base);
	return error_none();
}

static inline Opt_Error bool_read(bool *vbool, const char *base) {
	if (!strcmp(base, "t") || !strcmp(base, "T") || !strcmp(base, "true")) *vbool = true;
	else if (!strcmp(base, "f") || !strcmp(base, "F") || !strcmp(base, "false")) *vbool = false;
	else return error_invalid(OPT_VALUE_BOOL, base);
	return error_none();
}

Opt_Error opt_value_read(Opt_Value *value, const char *base) {
	switch (value->kind) {
		case OPT_VALUE_NONE:
			break;

		case OPT_VALUE_STRING:
			value->vstring = base;
			break;

		case OPT_VALUE_INT:
			return int_read(&value->vint, base);

		case OPT_VALUE_FLOAT:
			return float_read(&value->vfloat, base);

		case OPT_VALUE_BOOL:
			return bool_read(&value->vbool, base);

		default:
			assert(true && "Unknown value kind");
	}
	return error_none();
}

void opt_value_print(Opt_Value value) {
	switch (value.kind) {
		case OPT_VALUE_NONE:
			printf("none");
			break;

		case OPT_VALUE_STRING:
			printf("'%s'", value.vstring);
			break;

		case OPT_VALUE_INT:
			printf("%ld", value.vint);
			break;

		case OPT_VALUE_FLOAT:
			printf("%lf", value.vfloat);
			break;

		case OPT_VALUE_BOOL:
			printf("%s", value.vbool ? "true" : "false");
			break;

		default:
			assert(true && "Unknown value kind");
	}
}

void opt_info_init(Opt_Info *info, const char *long_name, const char *short_name, const char *desc, Opt_Value_Kind value_kind, const char *value_name, Opt_Info_Flag flags) {
	info->long_name = long_name;
	info->long_len = long_name != NULL ? strlen(long_name) : 0;
	info->short_name = short_name;
	info->short_len = short_name != NULL ? strlen(short_name) : 0;
	info->desc = desc;
	info->value_kind = value_kind;
	info->value_name = value_name;
	info->flags = flags;
	info->_seen = 0;

	assert((short_name != NULL || long_name != NULL) && "No name given to option");

	if (flags & OPT_INFO_MATCH_FIRST) assert(!(flags & (OPT_INFO_MATCH_LAST | OPT_INFO_STOP_DUPLICATE)) && "Conflicting flags set");
	if (flags & OPT_INFO_MATCH_LAST) assert(!(flags & (OPT_INFO_MATCH_FIRST | OPT_INFO_STOP_DUPLICATE)) && "Conflicting flags set");
	if (flags & OPT_INFO_STOP_DUPLICATE) assert(!(flags & (OPT_INFO_MATCH_FIRST | OPT_INFO_MATCH_LAST)) && "Conflicting flags set");
}

void opt_info_usage(Opt_Info *opts, size_t opts_len, Opt_Usage *usage) {
	assert(usage != NULL && "Missing usage info");
	assert(usage->line_max != 0);

	size_t line_curr = printf("Usage: %s", usage->name);
	size_t line_pad = line_curr + 1;

	const char *value[5] = { "", "string", "int", "float", "bool" };

	for (size_t opt = 0; opt < opts_len; ++opt) {
		Opt_Info *info = &opts[opt];

		size_t span = strlen(info->value_name != NULL && info->value_name[0] != '\0' ? info->value_name :  value[info->value_kind]);

		bool optional = !(info->flags & OPT_MATCH_MISSING);
		span += (optional * 4);

		line_curr += printf(" ");

		if (info->short_len != 0) {
			span += info->long_len + 1;
			assert(span < usage->line_max && "Option is too long to fit");

			if (line_curr + span > usage->line_max) {
				printf("\n");
				for (size_t i = 0; i < line_pad; ++i) putchar(' ');
				line_curr = line_pad;
			}

			if (optional) printf("[ ");
			printf("-%s", info->short_name);

			if (info->value_kind != OPT_VALUE_NONE) {
				if (info->value_name != NULL && info->value_name[0] != '\0') printf(" %s", info->value_name);
				else printf(" %s", value[info->value_kind]);
			}

			if (optional) printf(" ]");
			line_curr += span;
		} else {
			span += info->long_len + 2;
			assert(span < usage->line_max && "Option is too long to fit");

			if (line_curr + span > usage->line_max) {
				printf("\n");
				for (size_t i = 0; i < line_pad; ++i) putchar(' ');
				line_curr = line_pad;
			}

			if (optional) printf("[ ");
			printf("--%s", info->long_name);

			if (info->value_kind != OPT_VALUE_NONE) {
				if (info->value_name != NULL && info->value_name[0] != '\0') printf(" %s", info->value_name);
				else printf(" %s", value[info->value_kind]);
			}

			if (optional) printf(" ]");
			line_curr += span;
		}
	}

	for (size_t arg = 0; arg < usage->args_len; ++arg) {
		size_t span = strlen(usage->args[arg]);
		assert(span < usage->line_max && "Option is too long to fit");

		line_curr += printf(" ");

		if (line_curr + span > usage->line_max) {
			printf("\n");
			for (size_t i = 0; i < line_pad; ++i) putchar(' ');
			line_curr = line_pad;
		}

		printf("%s", usage->args[arg]);
		line_curr += span;
	}

	printf("\n");
}

void opt_info_help(Opt_Info *opts, size_t opts_len, const char *head_note, const char *foot_note, Opt_Usage *usage) {
	if (usage != NULL) opt_info_usage(opts, opts_len, usage);

	printf("%s\n", (head_note == NULL || head_note[0] == '\0') ? "Options:" : head_note);

	for (size_t opt = 0; opt < opts_len; ++opt) {
		Opt_Info *info = &opts[opt];

		size_t padding = 20;
		padding -= printf("  ");

		if (info->long_len != 0) {
			padding -= printf("--%s", info->long_name);
			if (info->short_len != 0) padding -= printf(", ");
		}

		if (info->short_len != 0) padding -= printf("-%s", info->short_name);

		for (size_t i = 0; i < padding; ++i) putchar(' ');
		printf("%s\n", info->desc != NULL ? info->desc : "");
	}

	if (foot_note != NULL && foot_note[0] != '\0') printf("%s\n", foot_note);
}

void opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len) {
	result->bin_name = NULL;
	result->matches = matches;
	result->matches_len = 0;
	result->matches_size = matches_len;
	result->simple = 0;
	result->option = 0;
	result->missing = 0;

	assert((matches != NULL && matches_len != 0) && "Matches pool empty");
}

static int result_compare(const void *a, const void *b) {
	const Opt_Match *match_a = a;
	const Opt_Match *match_b = b;

	// -1 = less
	// 0 = equal
	// 1 = greater
	if (match_a->kind == OPT_MATCH_SIMPLE) {
		if (match_b->kind == OPT_MATCH_SIMPLE) return 0;
		return 1;
	}

	if (match_b->kind == OPT_MATCH_SIMPLE) return -1;

	assert(match_a->kind == OPT_MATCH_OPTION || match_a->kind == OPT_MATCH_MISSING);
	assert(match_b->kind == OPT_MATCH_OPTION || match_b->kind == OPT_MATCH_MISSING);

	size_t opt_a = match_a->kind == OPT_MATCH_OPTION ? match_a->option.opt : match_a->missing_opt;
	size_t opt_b = match_b->kind == OPT_MATCH_OPTION ? match_b->option.opt : match_b->missing_opt;
	return (opt_a > opt_b) - (opt_a < opt_b);
}

static int result_compare_simple(const void *a, const void *b) {
	const Opt_Match *match_a = a;
	const Opt_Match *match_b = b;

	// -1 = less
	// 0 = equal
	// 1 = greater
	if (match_a->kind == OPT_MATCH_SIMPLE) {
		if (match_b->kind == OPT_MATCH_SIMPLE) return 0;
		return 1;
	}

	if (match_b->kind == OPT_MATCH_SIMPLE) return -1;

	assert(match_a->kind == OPT_MATCH_OPTION || match_a->kind == OPT_MATCH_MISSING);
	assert(match_b->kind == OPT_MATCH_OPTION || match_b->kind == OPT_MATCH_MISSING);
	return 0;
}

void opt_result_sort(Opt_Result *result, bool sort_opt) {
	qsort(result->matches, result->matches_len, sizeof(Opt_Match), sort_opt ? result_compare : result_compare_simple);
}

void opt_result_iter(Opt_Result *result, Opt_Result_Simple_F simple_f, Opt_Result_Option_F *opt_fs) {
	assert(simple_f != NULL && opt_fs != NULL);
	for (size_t i = 0; i < result->matches_len; ++i) {
		Opt_Match *match = &result->matches[i];
		switch (match->kind) {
			case OPT_MATCH_SIMPLE:
				simple_f(match->simple);
				break;

			case OPT_MATCH_OPTION:
				opt_fs[match->option.opt](match->option.value, false);
				break;

			case OPT_MATCH_MISSING:
				opt_fs[match->missing_opt](value_none(), true);
				break;

			default:
				assert(false && "Unreachable");
		}
	}
}

static inline void result_push(Opt_Result *result, Opt_Match match) {
	assert((result->matches_len + 1 < result->matches_size) && "Too many matches");
	result->matches[result->matches_len++] = match;
}

void opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len) {
	parser->opts = opts;
	parser->opts_len = opts_len;

	//assert(opts != NULL && opts_len != 0);
}

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc) {
	result->bin_name = argv[0];
	bool no_opt = false;

	for (int arg = 1; arg < argc; ++arg) {
		const char *argi = argv[arg];
		Opt_Match match = { 0 };

		if (argi[0] == '-' && !no_opt) {
			bool found = false;
			bool ignore = false;

			if (argi[1] == '-') {
				if (argi[2] == '\0') {
					no_opt = true;
					continue;
				}

				// long
				const char *base = &argi[2];
				for (size_t opt = 0; opt < parser->opts_len; ++opt) {
					Opt_Info *info = &parser->opts[opt];
					Opt_Value value = { 0 };

					if (info->long_len == 0) continue;
					if (!strncmp(base, info->long_name, info->long_len)) {
						if (info->value_kind != OPT_VALUE_NONE) {
							const char *base_value = NULL;
							if (base[info->long_len] == '=') {
								base_value = &base[info->long_len + 1];
							} else if (arg + 1 < argc) base_value = argv[++arg];
							else return error_missing(opt, info->value_kind);

							value.kind = info->value_kind;
							if (base[0] == '\0' && value.kind != OPT_VALUE_STRING) return error_missing(opt, value.kind);

							Opt_Error error = opt_value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else {
							if (base[info->long_len] != '\0') return error_unknown(argi);
							value = value_none();
						}

						if (info->flags & OPT_INFO_MATCH_NONE) {
							ignore = true;
							break;
						}

						match = match_option(opt, value);
						found = true;

						if (info->flags & OPT_INFO_STOP_PARSER) {
							result_push(result, match);
							return error_none();
						}

						if (info->_seen++ > 0) {
							if (info->flags & OPT_INFO_MATCH_FIRST) {
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_MATCH_LAST) {
								memcpy(&result->matches[info->_match], &match, sizeof(Opt_Match));
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_STOP_DUPLICATE) {
								return error_duplicate(opt, value);
							}
						} else info->_match = result->matches_len;
						break;
					}
				}
			} else {
				// short
				const char *base = &argi[1];
				for (size_t opt = 0; opt < parser->opts_len; ++opt) {
					Opt_Info *info = &parser->opts[opt];
					Opt_Value value = { 0 };

					if (info->short_len == 0) continue;
					if (!strncmp(base, info->short_name, info->short_len)) {
						if (info->value_kind != OPT_VALUE_NONE) {
							const char *base_value = NULL;
							if (base[info->short_len] == '=') {
								base_value = &base[info->short_len + 1];
							} else if (arg + 1 < argc) base_value = argv[++arg];
							else return error_missing(opt, info->value_kind);

							value.kind = info->value_kind;
							if (base[0] == '\0' && value.kind != OPT_VALUE_STRING) return error_missing(opt, value.kind);

							Opt_Error error = opt_value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else {
							if (base[info->short_len] != '\0') return error_unknown(argi);
							value = value_none();
						}

						if (info->flags & OPT_INFO_MATCH_NONE) {
							ignore = true;
							break;
						}

						match = match_option(opt, value);
						found = true;

						if (info->flags & OPT_INFO_STOP_PARSER) {
							result_push(result, match);
							return error_none();
						}

						if (info->_seen++ > 0) {
							if (info->flags & OPT_INFO_MATCH_FIRST) {
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_MATCH_LAST) {
								memcpy(&result->matches[info->_match], &match, sizeof(Opt_Match));
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_STOP_DUPLICATE) {
								return error_duplicate(opt, value);
							}
						} else info->_match = result->matches_len;
						break;
					}
				}
			}

			if (ignore) continue;
			if (!found) return error_unknown(argi);
			++result->option;
		} else {
			match = match_simple(argi);
			++result->simple;
		}

		result_push(result, match);
	}

	for (size_t opt = 0; opt < parser->opts_len; ++opt) {
		Opt_Info *info = &parser->opts[opt];
		if (info->_seen == 0 && info->flags & OPT_MATCH_MISSING) {
			++result->missing;
			result_push(result, match_missing(opt));
		}
	}

	return error_none();
}
