//
// Created by marcob0020 on 9/28/24.
//

#pragma once


#include "timespan.hpp"

namespace utils{
    enum class TemporalQueryType {
      NONE = 0,
      AS_OF,
      FROM_TO,
      BETWEEN_AND
    };

    inline std::ostream &operator<<(std::ostream &os, const TemporalQueryType &tqt) {
    switch (tqt) {
      case TemporalQueryType::NONE:
        os << "NONE";
      break;
      case TemporalQueryType::FROM_TO:
        os << "FROM_TO";
      break;
      case TemporalQueryType::AS_OF:
        os << "AS_OF";
      break;
      case TemporalQueryType::BETWEEN_AND:
        os << "BETWEEN_AND";
      break;
    }
    return os;
  }

    struct TemporalFilter {
      TemporalQueryType type;

      VTDateTime first;
      VTDateTime second;

      TemporalFilter():type(TemporalQueryType::NONE),first(VTDateTime::min()),second(VTDateTime::max()){}

      bool matches(const VTDateTime& t1, const VTDateTime& t2) const {
        switch (type) {
          case TemporalQueryType::NONE:
            return true;
          case TemporalQueryType::AS_OF:
            return t2 > first && first >= t1;
          case TemporalQueryType::FROM_TO:
            return t1 < second && t2 > first;
          case TemporalQueryType::BETWEEN_AND:
            return t1 <= second && t2 > first;
        }
        return false;
      }

      bool is_temporal() const {
        return type != TemporalQueryType::NONE;
      }

      VTDateTime get_first() const {
        switch (type) {
          case TemporalQueryType::NONE:
            return VTDateTime::min();
          default:
            return first;
        }
      }

      VTDateTime get_second() const {
        switch (type) {
          case TemporalQueryType::NONE:
            return VTDateTime::max();
          case TemporalQueryType::AS_OF:
            return utils::VTDateTime::next(first);
          default:
            return second;
        }
      }

      TimeSpan get_span() const {
        return {get_first(), get_second()};
      }

      TemporalFilter get_filtered_span(TimeSpan filter) const {
        const TimeSpan ts = get_span().intersect(filter);
        TemporalFilter ret = *this;
        ret.first = ts.first;
        ret.second = ts.second;

        return ret;
      }

      bool whole() const {
        return type != TemporalQueryType::AS_OF && first == VTDateTime::min() && second == VTDateTime::max();
      }

      friend std::ostream &operator<<(std::ostream &os, const TemporalFilter &ldt) {
        os << "Type: " << ldt.type;
        os << "{" << ldt.first << "," << ldt.second << "}";
        return os;
      }

      bool whole_or_none() const {
        return whole() || !is_temporal();
      }

    };


}


