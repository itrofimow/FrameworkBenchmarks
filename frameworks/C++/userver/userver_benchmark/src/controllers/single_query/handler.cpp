#include "handler.hpp"

#include "../../common/db_helpers.hpp"

#include <userver/components/component_context.hpp>
#include <userver/storages/mysql.hpp>

namespace userver_techempower::single_query {

Handler::Handler(const userver::components::ComponentConfig &config,
                 const userver::components::ComponentContext &context)
        : userver::server::handlers::HttpHandlerJsonBase{config, context},
          mysql_{context
                         .FindComponent<userver::components::MySQL>(
                                 db_helpers::kDbComponentName)
                         .GetCluster()} {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
        const userver::server::http::HttpRequest &,
        const userver::formats::json::Value &,
        userver::server::request::RequestContext &) const {
  const auto row =
          mysql_->Execute(db_helpers::kClusterHostType,
                          userver::engine::Deadline::FromDuration(std::chrono::milliseconds{1750}),
                          db_helpers::kSelectRowQuery,
                          db_helpers::GenerateRandomId())
                  .AsSingleRow<db_helpers::WorldTableRow>();

  return db_helpers::Serialize(row, {});
}

}  // namespace userver_techempower::single_query
