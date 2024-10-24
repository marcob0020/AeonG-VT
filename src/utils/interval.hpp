//
// Created by marcob0020 on 10/22/24.
//

#ifndef INTERVAL_HPP
#define INTERVAL_HPP
#include "vt_temporal.hpp"

namespace utils {
template <typename T, typename container = std::vector<T>>
class interval {
private:
  container& _container_interval;
  bool fully_covered = false;
public:
  using Period = std::pair<utils::VTDateTime, utils::VTDateTime>;

  explicit interval(const container& container_interval) : _container_interval(container_interval) {}
  interval(): _container_interval() {
  };

  interval(const interval& other): _container_interval(other._container_interval) {}
  interval(interval&& other) noexcept: _container_interval(std::move(other._container_interval)) {}

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

  bool is_single(Period from_to) const {
    //TODO
  }



};
}

#endif //INTERVAL_HPP
