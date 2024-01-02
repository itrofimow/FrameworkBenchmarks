#include "handler.hpp"

#include <userver/components/component_context.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/postgres/postgres.hpp>

#include <boost/container/small_vector.hpp>

namespace userver::storages::postgres::io::traits {

// Hijack userver's PG traits to work with small_vector
template <typename T, std::size_t Size>
struct IsCompatibleContainer<boost::container::small_vector<T, Size>>
    : std::true_type {};

}  // namespace userver::storages::postgres::io::traits

namespace userver_techempower::updates {

namespace {

constexpr const char* kUpdateQueryStr{R"(
UPDATE World w SET
  randomNumber = new_numbers.randomNumber
FROM ( SELECT
  UNNEST($1) as id,
  UNNEST($2) as randomNumber
) new_numbers
WHERE w.id = new_numbers.id
)"};

constexpr std::size_t kBestConcurrencyWildGuess = 256;

constexpr std::chrono::milliseconds kQueryQueueTimeout{7500};

}  // namespace

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
    : userver::server::handlers::HttpHandlerJsonBase{config, context},
      pg_{context.FindComponent<userver::components::Postgres>("hello-world-db")
              .GetCluster()},
      query_arg_name_{"queries"},
      update_query_{db_helpers::CreateNonLoggingQuery(kUpdateQueryStr)},
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
  boost::container::small_vector<int, 20> ids(queries);
  boost::container::small_vector<int, 20> values(queries);
  for (auto& id : ids) {
    id = db_helpers::GenerateRandomId();
  }
  // we have to sort ids to not deadlock in update
  std::sort(ids.begin(), ids.end(),
            [](const auto& lhs, const auto& rhs) { return lhs < rhs; });

  boost::container::small_vector<db_helpers::WorldTableRow, 20> result(queries);

  {
    const auto lock = semaphore_.Acquire();

    {
      auto query_queue = pg_->CreateQueryQueue(db_helpers::kClusterHostType,
                                               kQueryQueueTimeout);
      for (const auto& id : ids) {
        query_queue.Push(kQueryQueueTimeout, db_helpers::kSelectRowQuery, id);
      }
      const auto db_result = query_queue.Collect(kQueryQueueTimeout);
      for (std::size_t i = 0; i < db_result.size(); ++i) {
        result[i] = db_result[i].AsSingleRow<db_helpers::WorldTableRow>(
            userver::storages::postgres::kRowTag);
      }
    }

    for (auto& value : values) {
      value = db_helpers::GenerateRandomValue();
    }
    pg_->Execute(db_helpers::kClusterHostType, update_query_, ids, values);
  }

  return userver::formats::json::ValueBuilder{result}.ExtractValue();
}

}  // namespace userver_techempower::updates
