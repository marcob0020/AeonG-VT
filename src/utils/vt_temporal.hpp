//
// Copyright 2022 Memgraph Ltd.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.txt; by using this file, you agree to be bound by the terms of the Business Source
// License, and you may not use this file except in compliance with the Business Source License.
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0, included in the file
// licenses/APL.txt.
//
// Created by marcob0020 on 9/22/24.
//

#pragma once

#include "temporal.hpp"

namespace utils {

struct VTDateTimeParameters {
  DateParameters date;
  LocalTimeParameters time;

  bool operator==(const VTDateTimeParameters &) const = default;
};

VTDateTimeParameters ParseVTDateTimeParameters(std::string_view string);

class VTDateTime {
private:
    int64_t microseconds;
public:
    explicit VTDateTime(int64_t microseconds);
    explicit VTDateTime(const DateParameters &date_parameters, const LocalTimeParameters &local_time_parameters);
    explicit VTDateTime(const Date &date, const LocalTime &local_time);
    explicit VTDateTime(const LocalDateTime &local_date_time);

    explicit operator LocalDateTime() const;
    bool operator> (const VTDateTime &other) const;
    bool operator < (const VTDateTime &other) const;
    bool operator == (const VTDateTime &other) const;
    bool operator != (const VTDateTime &other) const;
    bool operator <= (const VTDateTime &other) const;
    bool operator >= (const VTDateTime &other) const;

    auto operator<=>(const VTDateTime &) const = default;

    friend std::ostream &operator<<(std::ostream &os, const VTDateTime &ldt) {
        os << LocalDateTime(ldt.microseconds);
        return os;
    }

    friend VTDateTime operator+(const VTDateTime &dt, const Duration &dur) {
        return VTDateTime(dt.microseconds + dur.microseconds);
    }

    friend VTDateTime operator+(const Duration &dur, const VTDateTime &dt) { return dt + dur; }

    friend VTDateTime operator-(const VTDateTime &dt, const Duration &dur) { return dt + (-dur); }

    friend Duration operator-(const VTDateTime &lhs, const VTDateTime &rhs) {
        return Duration(lhs.microseconds) - Duration(rhs.microseconds);
    }

    int64_t get_microseconds() const {
        return microseconds;
    }
};

struct VTDateTimeHash {
    size_t operator()(const VTDateTime &vt_date_time) const;
};

VTDateTime CurrentVTDateTime();

} // utils

