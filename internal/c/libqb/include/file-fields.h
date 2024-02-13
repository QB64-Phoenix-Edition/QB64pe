#pragma once

#include <stdint.h>

#include "qbs.h"

void lrset_field(qbs *str);
void field_free(qbs *str);
void field_new(int32_t fileno);
void field_update(int32_t fileno);
void lrset_field(qbs *str);
void field_free(qbs *str);
void field_add(qbs *str, int64_t size);
void field_get(int32_t fileno, int64_t offset, int32_t passed);
void field_put(int32_t fileno, int64_t offset, int32_t passed);
