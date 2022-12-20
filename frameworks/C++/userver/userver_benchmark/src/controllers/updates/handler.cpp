#include "handler.hpp"

#include "../../common/db_helpers.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/mysql.hpp>

#include <boost/container/small_vector.hpp>

namespace userver_techempower::updates {

namespace {

constexpr std::string_view kUpdateQueryStr{R"(
INSERT INTO World(id, randomNumber) VALUES(?, ?)
ON DUPLICATE KEY UPDATE randomNumber = VALUES(randomNumber)
)"};

}

Handler::Handler(const userver::components::ComponentConfig &config,
                 const userver::components::ComponentContext &context)
        : userver::server::handlers::HttpHandlerJsonBase{config, context},
          mysql_{context.FindComponent<userver::components::MySQL>("hello-world-db")
                         .GetCluster()},
          query_arg_name_{"queries"},
          update_query_{kUpdateQueryStr} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
        const userver::server::http::HttpRequest &request,
        const userver::formats::json::Value &,
        userver::server::request::RequestContext &) const {
  const auto queries_count =
          db_helpers::ParseParamFromQuery(request, query_arg_name_);

  std::vector<int> random_ids(queries_count);
  std::generate(random_ids.begin(), random_ids.end(),
                db_helpers::GenerateRandomId);
  std::sort(random_ids.begin(), random_ids.end());

  boost::container::small_vector<db_helpers::WorldTableRow, 500> result{};
  for (auto id: random_ids) {
    result.push_back(mysql_->Execute(db_helpers::kClusterHostType,
                                     db_helpers::kSelectRowQuery, id)
                             .AsSingleRow<db_helpers::WorldTableRow>());
  }

  auto result_builder = userver::formats::json::ValueBuilder{result};

  for (auto& row : result) {
    row.random_number = db_helpers::GenerateRandomValue();
  }
  mysql_->InsertMany(update_query_, result);

  return result_builder.ExtractValue();
}

}  // namespace userver_techempower::updates
