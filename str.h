/*
--------------------------------------------------------------------------------
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.
You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the
Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
Boston, MA  02110-1301, USA.
--------------------------------------------------------------------------------
*/

// Copyright (c) 2014-2017 John Seamons, ZL/KF6VO

#pragma once

#include "types.h"
#include "kiwi.h"


// kstr: Kiwi C-string package
//
// any kstr_cstr argument = kstr_t|C-string|NULL
// C-string: char array or string constant or NULL

typedef char kstr_t;

void kstr_init();
char *kstr_sp(kstr_t *s_kstr_cstr);     // return C-string pointer from kstr object
void kstr_free(kstr_t *s_kstr_cstr);    // only frees a kstr object, will not free a malloc()'d c-string unless kstr_wrap()'d
int kstr_len(kstr_t *s_kstr_cstr);      // return C-string length from kstr object
kstr_t *kstr_wrap(char *s_malloc);      // wrap a malloc()'d C-string in a kstr object so it is auto-freed later on
kstr_t *kstr_cat(kstr_t *s1_kstr_cstr, const kstr_t *s2_kstr_cstr);     // will kstr_free() s2_kstr_cstr argument


#define GET_CHARS(field, value) kiwi_get_chars(field, value, sizeof(field));
void kiwi_get_chars(char *field, char *value, size_t size);
#define SET_CHARS(field, value, fill) kiwi_set_chars(field, value, fill, sizeof(field));
void kiwi_set_chars(char *field, const char *value, const char fill, size_t size);
int kiwi_split(char *ocp, char **mbuf, const char *delims, char *argv[], int nargs);
void kiwi_str_unescape_quotes(char *str);
char *kiwi_str_encode(char *s);
char *kiwi_str_decode_inplace(char *src);
char *kiwi_str_decode_static(char *src);
int kiwi_str2enum(const char *s, const char *strs[], int len);
const char *kiwi_enum2str(int e, const char *strs[], int len);
void kiwi_chrrep(char *str, const char from, const char to);
bool kiwi_str_begins_with(char *s, const char *cs);
char *kiwi_skip_over(char *s, const char *skip);
char *kiwi_strncpy(char *dst, const char *src, size_t n);
char *kiwi_strncat(char *dst, const char *src, size_t n);
bool kiwi_sha256_strcmp(char *str, const char *key);
