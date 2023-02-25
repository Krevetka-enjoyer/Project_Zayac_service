#include "lock.h"

PgWorker::PgWorker(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : userver::storages::postgres::DistLockComponentBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("commands_db").GetCluster()),
      config_(context.FindComponent<components::DynamicConfig>().GetSource())  
{
  using storages::postgres::ClusterHostType;
  constexpr auto kCreateTable = R"~(CREATE TABLE IF NOT EXISTS distlocks
  (
    key             TEXT PRIMARY KEY,
    owner           TEXT,
    expiration_time TIMESTAMPTZ
  )
  )~";
  pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTable);
  AutostartDistLock();
}

PgWorker::~PgWorker() { StopDistLock(); }

std::string GetTimeFromConfig(const userver::dynamic_config::DocsMap& docs_map) {
    return docs_map.Get("REARM_DB_TIME").As<std::string>();
}
constexpr userver::dynamic_config::Key<GetTimeFromConfig> GetTime{};

std::chrono::duration<int,std::ratio<60>> StrToChrono(const std::string& str)
{
  int i=str.find(' ');
  int num=std::stoi(str.substr(0,i));
  std::string type=str.substr(i+1);
  std::transform(type.begin(), type.end(), type.begin(), tolower);
  if (type=="minute")
    return std::chrono::duration<int,std::ratio<60>>(std::chrono::minutes(num));
  else if (type=="hour")
    return std::chrono::duration<int,std::ratio<60>>(std::chrono::hours(num));
  else if (type=="day")
    return std::chrono::duration<int,std::ratio<60>>(std::chrono::hours(24*num));
  else
    throw std::runtime_error("Bad type of time");
}

void PgWorker::DoWork() {
  while (!engine::current_task::ShouldCancel())
  {
    const auto runtime_config = config_.GetSnapshot();
    std::string t=runtime_config[GetTime];
    pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,Clear, t);
    if (engine::current_task::ShouldCancel()) break;
    engine::SleepFor(StrToChrono("1 minute"));
  }
}

