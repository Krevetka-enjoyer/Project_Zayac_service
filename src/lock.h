#pragma once
#include <chrono>
#include <stdexcept>

#include <userver/engine/sleep.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/storages/postgres/dist_lock_component_base.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/components/component_context.hpp>

class PgWorker final: public userver::storages::postgres::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "pg-worker";

  PgWorker(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);
  ~PgWorker() override;

  void DoWork() final;
 private:
  storages::postgres::ClusterPtr pg_cluster_;
  dynamic_config::Source config_;
  const storages::postgres::Query Clear{"delete from responses where date<(now()- $1::interval)",
                                                storages::postgres::Query::Name{"Clear"},
  };
};
