//
// Created by marcob0020 on 10/22/24.
//

#ifndef INTERVAL_HPP
#define INTERVAL_HPP
#include <forward_list>
#include <storage/v2/temporal_period.hpp>

#include "vt_temporal.hpp"

namespace utils {
template <typename T>
class interval_item {
  storage::TemporalPeriod _timespan;
  T _value;
public:
  interval_item(storage::TemporalPeriod timespan, T value): _timespan(timespan), _value(value) {}
  storage::TemporalPeriod timespan() const { return _timespan; }
  storage::TemporalPeriod& timespan() { return _timespan; }
  T& value()  { return _value; }
  T value() const { return _value; }
};

template<typename T>
class interval_item<bool> {
  storage::TemporalPeriod _timespan;
public:
  interval_item(storage::TemporalPeriod timespan, T value): _timespan(timespan) {}
  explicit interval_item(storage::TemporalPeriod timespan): _timespan(timespan) {}
  storage::TemporalPeriod timespan() const  { return _timespan; }
  storage::TemporalPeriod& timespan()   { return _timespan; }
  T value() const { return true; }
};

template <typename T>
class valued_timeline {
private:
  std::forward_list<interval_item<T>> _container_interval;

  storage::TemporalPeriod from_to;

public:
  using Iterator = typename decltype(_container_interval)::iterator;
  using ConstIterator = typename decltype(_container_interval)::const_iterator;
  using Item = interval_item<T>;
  using Container = std::forward_list<interval_item<T>>;
  using TimeSpan = storage::TemporalPeriod;

  explicit valued_timeline(const Container& container_interval) : _container_interval(container_interval), from_to({VTDateTime::min(), VTDateTime::max()}) {}
  valued_timeline(): _container_interval(), from_to({VTDateTime::min(), VTDateTime::max()}) {}
  explicit valued_timeline(const TimeSpan& interval) : _container_interval(), from_to(interval) {}


  valued_timeline(const valued_timeline& other): _container_interval(other._container_interval), from_to(other.from_to) {}
  valued_timeline(valued_timeline&& other) noexcept: _container_interval(std::move(other._container_interval)), from_to(other.from_to) {}

  void add(TimeSpan from_to, const T& value);

  void remove(TimeSpan from_to);

  bool covered() const {
    return !_container_interval.empty() && _container_interval.front().first <= from_to.first && _container_interval.front().second <= from_to.first;
  }

  bool covered(TimeSpan from_to) const ;

  T get_single(TimeSpan from_to) const ;

  T get_first(VTDateTime from) const ;

  bool is_single(TimeSpan from_to) const ;

  valued_timeline split(TimeSpan from_to) const;


  ConstIterator begin() const;
  ConstIterator end() const;

  Iterator begin();
  Iterator end();

private:
  std::optional<Iterator> seek(typename Iterator start, VTDateTime vt);
  std::optional<ConstIterator> seek(typename ConstIterator start, VTDateTime vt) const;

};


class timeline {
  private:
    std::forward_list<interval_item<bool>> _container_interval;

    storage::TemporalPeriod from_to;

  public:
    using Iterator = std::forward_list<interval_item<bool>>::iterator;
    using ConstIterator = std::forward_list<interval_item<bool>>::const_iterator;
    using Item = interval_item<bool>;
    using Container = std::forward_list<interval_item<bool>>;
    using TimeSpan = storage::TemporalPeriod;

    explicit timeline(const Container& container_interval) : _container_interval(container_interval), from_to({VTDateTime::min(), VTDateTime::max()}) {}
    timeline(): _container_interval(), from_to({VTDateTime::min(), VTDateTime::max()}) {}
    timeline(const TimeSpan& interval) : _container_interval(), from_to(interval) {}


    timeline(const timeline& other): _container_interval(other._container_interval), from_to(other.from_to) {}
    timeline(timeline&& other) noexcept: _container_interval(std::move(other._container_interval)), from_to(other.from_to) {}

    void add(TimeSpan from_to);

    void remove(TimeSpan from_to);

    bool covered() const ;

    bool covered(TimeSpan from_to) const ;

    bool get_single(TimeSpan from_to) const ;

    bool get_first(VTDateTime from) const ;

    bool is_single(TimeSpan from_to) const ;

    timeline split(TimeSpan from_to) const;


    Iterator begin();
    Iterator end();

    ConstIterator begin() const ;
    ConstIterator end() const ;

  private:
    std::optional<Iterator> seek(Iterator start, VTDateTime vt);
    std::optional<ConstIterator> seek(ConstIterator start, VTDateTime vt) const;

  };

}

#endif //INTERVAL_HPP
