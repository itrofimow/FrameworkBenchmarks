#pragma once

#include <userver/formats/json/value.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/storages/mysql/cluster_host_type.hpp>

namespace userver_techempower::db_helpers {

constexpr int kMaxWorldRows = 10000;
const std::string kSelectRowQuery{
    "SELECT id, randomNumber FROM World WHERE id = ?"};
constexpr auto kClusterHostType =
    userver::storages::mysql::ClusterHostType::kMaster;
constexpr std::string_view kDbComponentName = "hello-world-db";

struct WorldTableRow final {
  std::uint32_t id;
  int random_number;
};

int GenerateRandomId();
int GenerateRandomValue();

userver::formats::json::Value Serialize(
    const WorldTableRow& value,
    userver::formats::serialize::To<userver::formats::json::Value>);

int ParseParamFromQuery(const userver::server::http::HttpRequest& request,
                        const std::string& name);

}  // namespace userver_techempower::db_helpers
