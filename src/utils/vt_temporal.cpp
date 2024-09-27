//
// Created by marcob0020 on 9/22/24.
//

#include "vt_temporal.hpp"

namespace utils {


    VTDateTimeParameters ParseVTDateTimeParameters(const std::string_view string) {
        std::pair<DateParameters, LocalTimeParameters> parameters = utils::ParseLocalDateTimeParameters(string);

        return VTDateTimeParameters{parameters.first, parameters.second};
    }

    VTDateTime::VTDateTime(const int64_t microseconds) {
        this->microseconds = microseconds;
    }

    VTDateTime::VTDateTime(const DateParameters &date_parameters, const LocalTimeParameters &local_time_parameters) {
        utils::LocalDateTime holder(date_parameters, local_time_parameters);
        this->microseconds = holder.MicrosecondsSinceEpoch();
    }

    VTDateTime::VTDateTime(const Date &date, const LocalTime &local_time) {
        utils::LocalDateTime holder(date, local_time);
        this->microseconds = holder.MicrosecondsSinceEpoch();
    }

    VTDateTime::VTDateTime(const LocalDateTime &local_date_time) {
        this->microseconds = local_date_time.MicrosecondsSinceEpoch();
    }

    VTDateTime::operator LocalDateTime() const {
        return LocalDateTime(this->microseconds);
    }

    bool VTDateTime::operator>(const VTDateTime &other) const {
        return this->microseconds > other.microseconds;
    }

    bool VTDateTime::operator<(const VTDateTime &other) const {
        return this->microseconds < other.microseconds;
    }

    bool VTDateTime::operator==(const VTDateTime &other) const {
        return this->microseconds == other.microseconds;
    }

    bool VTDateTime::operator!=(const VTDateTime &other) const {
        return this->microseconds != other.microseconds;
    }

    bool VTDateTime::operator<=(const VTDateTime &other) const {
        return this->microseconds <= other.microseconds;
    }

    bool VTDateTime::operator>=(const VTDateTime &other) const {
        return this->microseconds >= other.microseconds;
    }

    size_t VTDateTimeHash::operator()(const VTDateTime &vt_date_time) const {
        LocalDateTimeHash hasher;

        return hasher(static_cast<LocalDateTime>(vt_date_time));

    }

    VTDateTime CurrentVTDateTime() {
        namespace chrono = std::chrono;
        auto ts = chrono::time_point_cast<chrono::microseconds>(chrono::system_clock::now());
        return VTDateTime(ts.time_since_epoch().count());
    }
} // utils