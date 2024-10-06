//
// Created by marcob0020 on 9/28/24.
//

#pragma once

#include "utils/vt_temporal.hpp"

namespace query{
    enum class TemporalQueryType {
      NONE = 0,
      AS_OF,
      FROM_TO
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
    }
    return os;
  }

    struct TemporalFilter {
      TemporalQueryType type;

      utils::VTDateTime first;
      utils::VTDateTime second;

      TemporalFilter():type(TemporalQueryType::NONE),first(0),second(0){}

      bool matches(const utils::VTDateTime& t1, const utils::VTDateTime& t2) const {
        switch (type) {
          case TemporalQueryType::NONE:
            return true;
          case TemporalQueryType::AS_OF:
            return t2 > first && first >= t1;
          case TemporalQueryType::FROM_TO:
            return t1 < second && t2 > first;
        }
        return false;
      }

      bool is_temporal() const {
        return type != TemporalQueryType::NONE;
      }

      friend std::ostream &operator<<(std::ostream &os, const TemporalFilter &ldt) {
        os << "Type: " << ldt.type;
        os << "{" << ldt.first << "," << ldt.second << "}";
        return os;
      }
    };


}


