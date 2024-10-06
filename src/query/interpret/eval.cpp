// Copyright 2021 Memgraph Ltd.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.txt; by using this file, you agree to be bound by the terms of the Business Source
// License, and you may not use this file except in compliance with the Business Source License.
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0, included in the file
// licenses/APL.txt.

#include "query/interpret/eval.hpp"

namespace query {

int64_t EvaluateInt(ExpressionEvaluator *evaluator, Expression *expr, const std::string &what) {
  TypedValue value = expr->Accept(*evaluator);
  try {
    return value.ValueInt();
  } catch (TypedValueException &e) {
    throw QueryRuntimeException(what + " must be an int");
  }
}

std::optional<size_t> EvaluateMemoryLimit(ExpressionEvaluator *eval, Expression *memory_limit, size_t memory_scale) {
  if (!memory_limit) return std::nullopt;
  auto limit_value = memory_limit->Accept(*eval);
  if (!limit_value.IsInt() || limit_value.ValueInt() <= 0)
    throw QueryRuntimeException("Memory limit must be a non-negative integer.");
  size_t limit = limit_value.ValueInt();
  if (std::numeric_limits<size_t>::max() / memory_scale < limit) throw QueryRuntimeException("Memory limit overflow.");
  return limit * memory_scale;
}

std::optional<utils::VTDateTime> EvaluateTemporalValue(ExpressionEvaluator *eval, Expression *temporal_v) {
  if (!temporal_v) return std::nullopt;

  auto temporal_value = temporal_v->Accept(*eval);

  if (temporal_value.IsInt()) {
    return utils::VTDateTime(temporal_value.ValueInt());
  }

  if(temporal_value.IsString()) {
    auto [date, time] = utils::ParseVTDateTimeParameters(temporal_value.ValueString());
    return utils::VTDateTime(date, time);
  }

  if(temporal_value.IsDate() ) {
    return utils::VTDateTime(temporal_value.ValueDate().MicrosecondsSinceEpoch());
  }

  if(temporal_value.IsLocalTime()) {
    return utils::VTDateTime(temporal_value.ValueLocalTime().MicrosecondsSinceEpoch());
  }

  if (temporal_value.IsLocalDateTime()) {
    return utils::VTDateTime(temporal_value.ValueLocalDateTime().MicrosecondsSinceEpoch());
  }

  if (temporal_value.IsVtDateTime()) {
    return utils::VTDateTime(temporal_value.ValueVtDateTime());
  }

  throw QueryRuntimeException("VT date must be an integer or a datetime");
}

}  // namespace query
