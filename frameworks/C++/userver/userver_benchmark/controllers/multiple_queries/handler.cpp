#include "handler.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/postgres/postgres.hpp>

#include <boost/container/small_vector.hpp>

namespace userver_techempower::multiple_queries {

namespace {

constexpr std::size_t kBestConcurrencyWildGuess = 256;

constexpr std::chrono::milliseconds kQueryQueueTimeout{7500};

}  // namespace

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerJsonBase{config, context},
      pg_{context
              .FindComponent<userver::components::Postgres>(
                  db_helpers::kDbComponentName)
              .GetCluster()},
      query_arg_name_{"queries"},
      semaphore_{kBestConcurrencyWildGuess} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value&,
    userver::server::request::RequestContext&) const {
  const auto queries =
      db_helpers::ParseParamFromQuery(request, query_arg_name_);

  return GetResponse(queries);
}

userver::formats::json::Value Handler::GetResponse(int queries) const {
  boost::container::small_vector<db_helpers::WorldTableRow, 20> result(queries);
  for (auto& value : result) {
    value.id = db_helpers::GenerateRandomId();
  }

  {
    const auto lock = semaphore_.Acquire();

    auto query_queue =
        pg_->CreateQueryQueue(db_helpers::kClusterHostType, kQueryQueueTimeout);
    for (const auto& value : result) {
      query_queue.Push(kQueryQueueTimeout, db_helpers::kSelectRowQuery,
                       value.id);
    }
    const auto db_result = query_queue.Collect(kQueryQueueTimeout);
    for (std::size_t i = 0; i < db_result.size(); ++i) {
      result[i].random_number = db_result[i]
                                    .AsSingleRow<db_helpers::WorldTableRow>(
                                        userver::storages::postgres::kRowTag)
                                    .random_number;
    }
  }

  return userver::formats::json::ValueBuilder{result}.ExtractValue();
}

}  // namespace userver_techempower::multiple_queries
