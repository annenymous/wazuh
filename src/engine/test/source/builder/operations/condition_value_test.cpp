#include <algorithm>
#include <gtest/gtest.h>
#include <rxcpp/rx.hpp>
#include <vector>

#include "asset_builder.hpp"
#include "registry.hpp"
#include "test_utils.hpp"

using condition_builder_t =
    AssetBuilder<observable<event>(observable<event>, value_t)>;

TEST(ConditionValueTest, Initializes) {
  ASSERT_NO_THROW(
      Registry::instance().builder<condition_builder_t>("condition.value"));
}

TEST(ConditionValueTest, Builds) {
  // Fake entry poing
  auto entry_point = rxcpp::observable<>::create<event>(handler);

  // Fake input json
  auto fake_jstring = R"(
      {
          "check": {
              "field": 1
          }
      }
  )";
  json::Document fake_j{fake_jstring};

  // Retreive builder
  auto _builder =
      Registry::instance().builder<condition_builder_t>("condition.value");

  // Build
  ASSERT_NO_THROW(auto _observable =
                      _builder(entry_point, fake_j.get(".check")));
}

TEST(ConditionValueTest, Operates) {
  // Fake entry poing
  rxcpp::observable<event> entry_point =
      rxcpp::observable<>::create<event>([](subscriber<event> o) {
        o.on_next(json::Document{R"(
      {
              "field": 1
      }
  )"});
        o.on_next(json::Document{R"(
      {
              "field": "1"
      }
  )"});
        o.on_next(json::Document{R"(
      {
              "otherfield": 1
      }
  )"});
      });

  // Fake input json
  auto fake_jstring = R"(
      {
          "check": {
              "field": 1
          }
      }
  )";
  json::Document fake_j{fake_jstring};

  // Retreive builder
  auto _builder =
      Registry::instance().builder<condition_builder_t>("condition.value");
  // Build
  auto _observable = _builder(entry_point, fake_j.get(".check"));
  // Fake subscriber
  vector<event> observed;
  auto on_next = [&observed](event j) { observed.push_back(j); };
  auto on_completed = []() {};
  auto subscriber = rxcpp::make_subscriber<event>(on_next, on_completed);

  // Operate
  ASSERT_NO_THROW(_observable.subscribe(subscriber));
  ASSERT_EQ(observed.size(), 1);
  for_each(begin(observed), end(observed),
           [](event j) { ASSERT_EQ(j.get(".field")->GetInt(), 1); });
}