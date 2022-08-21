#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opt.h"

static inline Opt_Error error(Opt_Error_Kind kind, void *payload) {
	return (Opt_Error) {
		.kind = kind,
		.payload = payload,
	};
}

static inline Opt_Error error_simple(Opt_Error_Kind kind) {
	return error(kind, NULL);
}

static inline Opt_Value value_none() {
	return (Opt_Value) {
		.kind = OPT_VALUE_NONE,
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
	if (end[0] != '\0') return error(OPT_ERROR_INVALID_VALUE, (void *)base);
	return error_simple(OPT_ERROR_NONE);
}

static inline Opt_Error bool_read(bool *vbool, const char *base) {
	if (!strcmp(base, "t") || !strcmp(base, "T") || !strcmp(base, "true")) *vbool = true;
	else if (!strcmp(base, "f") || !strcmp(base, "F") || !strcmp(base, "false")) *vbool = false;
	else return error(OPT_ERROR_INVALID_VALUE, (void *)base);

	return error_simple(OPT_ERROR_NONE);
}

Opt_Error opt_value_read(Opt_Value *value, const char *base) {
	switch (value->kind) {
		case OPT_VALUE_NONE:
			break;

		case OPT_VALUE_STRING:
			value->vstring = base;
			break;

		case OPT_VALUE_INT:
			if (base[0] == '\0') return error(OPT_ERROR_MISSING_VALUE, (void *)base);
			return int_read(&value->vint, base);

		case OPT_VALUE_BOOL:
			if (base[0] == '\0') return error(OPT_ERROR_MISSING_VALUE, (void *)base);
			return bool_read(&value->vbool, base);

		default:
			assert(true && "Unknown value kind");
	}
	return error_simple(OPT_ERROR_NONE);
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

		case OPT_VALUE_BOOL:
			printf("%s", value.vbool ? "true" : "false");
			break;

		default:
			assert(true && "Unknown value kind");
	}
}

void opt_info_init(Opt_Info *info, const char *long_name, const char *short_name, const char *desc, Opt_Value_Kind value_kind, Opt_Info_Flag flags) {
	info->long_name = long_name;
	info->long_len = long_name != NULL ? strlen(long_name) : 0;
	info->short_name = short_name;
	info->short_len = short_name != NULL ? strlen(short_name) : 0;
	info->desc = desc;
	info->value_kind = value_kind;
	info->flags = flags;
	info->_seen = 0;

	assert((short_name != NULL || long_name != NULL) && "No name given to option");
}

void opt_info_help(Opt_Info *opts, size_t opts_len, const char *usage, const char *note) {
}

void opt_result_init(Opt_Result *result, Opt_Match *matches, size_t matches_len) {
	result->bin_name = NULL;
	result->matches = matches;
	result->matches_len = 0;
	result->matches_size = matches_len;

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

void opt_result_sort(Opt_Result *result) {
	qsort(result->matches, result->matches_len, sizeof(Opt_Match), result_compare);
}

void opt_parser_init(Opt_Parser *parser, Opt_Info *opts, size_t opts_len) {
	parser->opts = opts;
	parser->opts_len = opts_len;

	//assert(opts != NULL && opts_len != 0);
}

static void result_push(Opt_Result *result, Opt_Match match) {
	assert((result->matches_len + 1 < result->matches_size) && "Too many matches");
	result->matches[result->matches_len++] = match;
}

Opt_Error opt_parser_run(Opt_Parser *parser, Opt_Result *result, const char **argv, const int argc) {
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
						if (info->_seen > 0 && (info->flags & OPT_INFO_KEEP_FIRST)) {
							ignore = true;
							break;
						}

						if (info->value_kind != OPT_VALUE_NONE) {
							const char *base_value = NULL;
							if (base[info->long_len] == '=') {
								base_value = &base[info->long_len + 1];
							} else if (arg + 1 < argc) base_value = argv[++arg];
							else return error(OPT_ERROR_MISSING_VALUE, (void *)argi);

							value.kind = info->value_kind;
							Opt_Error error = opt_value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else {
							if (base[info->long_len] != '\0') return error(OPT_ERROR_UNKNOWN_OPTION, (void *)argi);
							value = value_none();
						}

						match = match_option(opt, value);
						found = true;

						if (info->_seen > 0) {
							if (info->flags & OPT_INFO_KEEP_FIRST) {
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_KEEP_LAST) {
								memcpy(&result->matches[info->_match], &match, sizeof(Opt_Match));
								ignore = true;
								break;
							}
						}

						info->_match = result->matches_len;
						++info->_seen;
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
							else return error(OPT_ERROR_MISSING_VALUE, (void *)argi);

							value.kind = info->value_kind;
							Opt_Error error = opt_value_read(&value, base_value);
							if (error.kind != OPT_ERROR_NONE) return error;
						} else {
							if (base[info->short_len] != '\0') return error(OPT_ERROR_UNKNOWN_OPTION, (void *)argi);
							value = value_none();
						}

						match = match_option(opt, value);
						found = true;

						if (info->_seen > 0) {
							if (info->flags & OPT_INFO_KEEP_FIRST) {
								ignore = true;
								break;
							} else if (info->flags & OPT_INFO_KEEP_LAST) {
								memcpy(&result->matches[info->_match], &match, sizeof(Opt_Match));
								ignore = true;
								break;
							}
						}

						info->_match = result->matches_len;
						++info->_seen;
						break;
					}
				}
			}

			if (ignore) continue;
			if (!found) return error(OPT_ERROR_UNKNOWN_OPTION, (void *)argi);
		} else match = match_simple(argi);

		result_push(result, match);
	}

	for (size_t opt = 0; opt < parser->opts_len; ++opt) {
		Opt_Info *info = &parser->opts[opt];
		if ((info->flags & OPT_INFO_REPORT_MISSING) && info->_seen == 0) {
			result_push(result, match_missing(opt));
		}
	}

	result->bin_name = argv[0];
	return error_simple(OPT_ERROR_NONE);
}
