#ifndef AOI_H
#define AOI_H

#include <cassert>
#include <cstdio>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class AOI {
 public:
  struct Unit;

  typedef std::unordered_set<Unit*> UnitSet;
  struct Unit {
    Unit(const int id_, const float x_, const float y_)
        : id(id_), x(x_), y(y_) {}

    ~Unit(){};

    void Subscribe(const Unit* other) {
      subscribe_set.insert(const_cast<Unit*>(other));
    }
    void UnSubscribe(const Unit* other) {
      subscribe_set.erase(const_cast<Unit*>(other));
    }

    const int id;
    float x;
    float y;
    UnitSet subscribe_set;
  };

 public:
  typedef std::function<void(int, int)> Callback;

  AOI(const float width, const float height, const float visible_range,
      Callback enter_callback, Callback leave_callback)
      : width_(width),
        height_(height),
        visible_range_(visible_range),
        enter_callback_(enter_callback),
        leave_callback_(leave_callback) {
    assert(width_ >= 0);
    assert(height_ >= 0);
    assert(visible_range >= 0);

    assert(nullptr != enter_callback);
    assert(nullptr != leave_callback);
  }

  virtual ~AOI(){};

  AOI(const AOI&) = delete;
  AOI(AOI&&) = delete;
  AOI& operator=(const AOI&) = delete;
  AOI& operator=(AOI&&) = delete;

  // Update when unit moved
  // id is a custom integer
  virtual void UpdateUnit(const int id, const float x, const float y) {
    (void)id;
    assert(x <= width_);
    assert(y <= width_);
  };

  // Add unit to AOI
  // id is a custom integer
  virtual void AddUnit(const int id, const float x, const float y) {
    assert(x <= width_ && y <= height_);
    assert(unit_map_.find(id) == unit_map_.end());

    Unit* unit = NewUnit(id, x, y);
    unit_map_.insert(std::pair(unit->id, unit));
  };

  // Remove unit from AOI
  // id is a custom integer
  virtual void RemoveUnit(int id) {
    Unit* unit = unit_map_[id];
    unit_map_.erase(id);
    DeleteUnit(unit);
  };

  // Find the ids of units in range near the given id, and exclude id itself
  std::unordered_set<int> FindNearbyUnit(int id, const float range) const {
    Unit* unit = get_unit(id);
    UnitSet unit_set = FindNearbyUnit(unit, range);
    std::unordered_set<int> id_set;
    for (const auto& unit : unit_set) {
      id_set.insert(unit->id);
    }
    return id_set;
  };

  // Find the ids of units in the subscribe set of given id
  std::unordered_set<int> GetSubScribeSet(int id) const {
    Unit* unit = get_unit(id);
    UnitSet subscribe_set = unit->subscribe_set;
    std::unordered_set<int> id_set;
    for (const auto& unit : subscribe_set) {
      id_set.insert(unit->id);
    }
    return id_set;
  };

  const float& get_width() const { return width_; }
  const float& get_height() const { return height_; }

 protected:
  virtual UnitSet FindNearbyUnit(Unit* unit, const float range) const = 0;
  virtual Unit* NewUnit(const int id, const float x, const float y) = 0;
  virtual void DeleteUnit(Unit* unit) = 0;

  Unit* get_unit(int id) const { return unit_map_[id]; }

  UnitSet Intersection(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) != other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  UnitSet Difference(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) == other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  void NotifyAll(Unit* unit, const UnitSet& enter_set,
                 const UnitSet& leave_set) const {
    NotifyEnter(unit, enter_set);
    NotifyLeave(unit, leave_set);
  }

  void NotifyEnter(Unit* unit, const UnitSet& enter_set) const {
    for (const auto& other : enter_set) {
      enter_callback_(other->id, unit->id);
      other->Subscribe(unit);
      enter_callback_(unit->id, other->id);
      unit->Subscribe(other);
    }
  }

  void NotifyLeave(Unit* unit, const UnitSet& leave_set) const {
    for (const auto& other : leave_set) {
      leave_callback_(other->id, unit->id);
      other->UnSubscribe(unit);
    }
  }

  const float width_;
  const float height_;
  const float visible_range_;
  mutable std::unordered_map<int, Unit*> unit_map_;

 private:
  Callback enter_callback_;
  Callback leave_callback_;
};

#endif  // AOI_H