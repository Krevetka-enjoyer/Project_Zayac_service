#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <string_view>
#include <vector>
#include <fstream>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdexcept>

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

class RabbitSend final : public components::RabbitMQ {
 public:
  static constexpr std::string_view kName{"rabbit_sender"};

  RabbitSend(const components::ComponentConfig& config,const components::ComponentContext& context);

  int64_t InsertInCommands(const std::string& key,const std::string& mes);
  
  ~RabbitSend () override;

  void Publish(const std::string& message,const std::string& routing_key);
  void AddQueue(std::string&& name);
  
  std::string port="15672";
  std::string host="rabbitmq";
  private:
  
  //storages::secdist::SecdistConfig sec_config;
  const userver::urabbitmq::Exchange exchange{"MainExchange"};
  userver::urabbitmq::Queue RespQueue{"response"};
  std::list<userver::urabbitmq::Queue> queues;
  std::shared_ptr<userver::urabbitmq::Client> client_;
  storages::postgres::ClusterPtr pg_cluster_;
  const storages::postgres::Query InsInCmdQue{"INSERT INTO commands (users,cmd) "
                                                "VALUES ($1, $2)"
                                                "RETURNING index",
                                                storages::postgres::Query::Name{"InsertInCommands"},
  };
};

class RabbitConsumer final
    : public userver::urabbitmq::ConsumerComponentBase {
 public:
  static constexpr std::string_view kName{"rabbit-consumer"};

  RabbitConsumer(const components::ComponentConfig& config,
                   const components::ComponentContext& context);

 protected:
  bool InsertInResponses(const std::string& name,int index,const std::string& resp);
  
  void Process(std::string message) override;

 private:
 storages::postgres::ClusterPtr pg_cluster_;
 const storages::postgres::Query InsInResQue{"INSERT INTO responses (index,resp,name) "
                                                "VALUES ($1, $2, $3) "
                                                "ON CONFLICT DO NOTHING",
                                                storages::postgres::Query::Name{"InsertInResponses"},
  };
};
