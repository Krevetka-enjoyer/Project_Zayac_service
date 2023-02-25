#include <userver/utest/using_namespace_userver.hpp>

#include <string_view>
#include <vector>
#include <fstream>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "RabComp.h"
#include "Api.h"
#include "lock.h"

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/rabbitmq.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/secdist/provider_component.hpp>


namespace userver::components {

template <>
inline constexpr bool kHasValidate<RabbitConsumer> = true;

template <>
inline constexpr bool kHasValidate<RabbitSend> =
    true;
}  

int main(int argc, char* argv[]) {
  const auto components_list =
      userver::components::MinimalServerComponentList()
          .Append<RabbitSend>()
          .Append<RabbitConsumer>()
          .Append<Ispolnitel>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::Secdist>()
          .Append<userver::components::DefaultSecdistProvider>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<components::HttpClient>()
          .Append<components::Postgres>("commands_db")
          .Append<Users_Informator>()
          .Append<PgWorker>()
          .Append<User_Informator>();

  return userver::utils::DaemonMain(argc, argv, components_list);
}
