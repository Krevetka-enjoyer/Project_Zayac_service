#pragma once
#include <userver/utest/using_namespace_userver.hpp>
#include <string_view>
#include <vector>
#include <fstream>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "RabComp.h"

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

class Ispolnitel final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr const char* kName = "ispolnitel";

  Ispolnitel(const components::ComponentConfig& config,const components::ComponentContext& context);

  ~Ispolnitel() override = default;

  formats::json::Value HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const override;

 private:
  RabbitSend& my_rabbit_;
  clients::http::Client& http_client_;
};

class Users_Informator final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "users_informator";

  Users_Informator(const components::ComponentConfig& config,const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const override;
  RabbitSend& my_rabbit_;
  clients::http::Client& http_client_;
};

class User_Informator final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "user_informator";

  User_Informator(const components::ComponentConfig& config,const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const override ;
  storages::postgres::ClusterPtr pg_cluster_;
  const storages::postgres::Query kSelectValue{"SELECT index,cmd,resp,responses.date,commands.date FROM commands inner join responses using(index) where name=$1",
                                                storages::postgres::Query::Name{"select"},
  }; 
};
