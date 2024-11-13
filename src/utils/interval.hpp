//
// Created by marcob0020 on 10/22/24.
//

#ifndef INTERVAL_HPP
#define INTERVAL_HPP
#include "vt_temporal.hpp"

namespace utils {
template <typename T, typename container = std::vector<T>>
class interval {
  using Period = std::pair<utils::VTDateTime, utils::VTDateTime>;
private:
  container& _container_interval;
  bool fully_covered = false;
  Period from_to;
public:

  explicit interval(const container& container_interval) : _container_interval(container_interval), from_to({VTDateTime::min(), VTDateTime::max()}) {}
  interval(): _container_interval(), from_to({VTDateTime::min(), VTDateTime::max()}) {}
  interval(const Period& interval) : _container_interval(), from_to(interval) {}


  interval(const interval<T, container>& other): _container_interval(other._container_interval), from_to(other.from_to) {}
  interval(interval&& other) noexcept: _container_interval(std::move(other._container_interval)), from_to(other.from_to) {}

  void add(Period from_to, const T& value) {
    //TODO
  }

  void remove(Period from_to) {
    //TODO
  }

  bool covered() const {
    return fully_covered;
  }

  bool covered(Period from_to) const {
    //TODO
  }

  T& get_single(Period from_to) const {
    //TODO
  }

  T& get_first(utils::VTDateTime from) const {
    //TODO
  }

  bool is_single(Period from_to) const {
    //TODO
  }

  interval<T, container> split(Period from_to) const {}



};
}

#endif //INTERVAL_HPP
