#include "RabComp.h"

RabbitSend::RabbitSend(const components::ComponentConfig& config,const components::ComponentContext& context): 
    components::RabbitMQ{config, context}, client_{GetClient()}, pg_cluster_(context.FindComponent<components::Postgres>("commands_db").GetCluster())
//sec_config(context.FindComponent<components::Secdist>().GetStorage())
  {
    using storages::postgres::ClusterHostType;
    constexpr auto kCreateTable = R"~(CREATE TABLE IF NOT EXISTS commands (
        date TIMESTAMPTZ DEFAULT Now(),
        users text,
        index bigserial PRIMARY KEY,
        cmd text
      )
    )~";
    pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTable);
    constexpr auto kCreateTable2 = R"~(CREATE TABLE IF NOT EXISTS responses (
        date TIMESTAMPTZ DEFAULT Now(),
        index bigserial,
        resp text,
        name VARCHAR,
        CONSTRAINT pisma FOREIGN KEY(index) REFERENCES commands(index)
      )
    )~";
    pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTable2);
    constexpr auto kCreateFunc = R"~(create or replace function fn_del_command() returns trigger as $psql$
      begin
        delete from commands where commands.index=old.index;
        return old;
      end;
    $psql$ language plpgsql;
    )~";
    pg_cluster_->Execute(ClusterHostType::kMaster, kCreateFunc);
     constexpr auto kDropOld = R"~(drop trigger if exists del_command on responses; )~";
    pg_cluster_->Execute(ClusterHostType::kMaster, kDropOld);
    constexpr auto kCreateTrigger = R"~(create trigger del_command after delete on responses
      for each row execute procedure fn_del_command(); 
    )~";
    pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTrigger);
    const auto setup_deadline =userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
    auto admin_channel = client_->GetAdminChannel(setup_deadline);
    admin_channel.DeclareExchange(exchange, userver::urabbitmq::Exchange::Type::kTopic, setup_deadline);
    admin_channel.DeclareQueue(RespQueue, setup_deadline);  
  }

int64_t RabbitSend::InsertInCommands(const std::string& key,const std::string& mes)
  {
    storages::postgres::Transaction transaction =
    pg_cluster_->Begin("trs_ist",storages::postgres::ClusterHostType::kMaster, {});
    auto res=transaction.Execute(InsInCmdQue,key,mes);
    if (res.RowsAffected()) {
      transaction.Commit();
      return res.AsSingleRow<int64_t>();
    }
    return -1;
  }
  
void RabbitSend::Publish(const std::string& message,const std::string& routing_key) {
    int num=InsertInCommands(routing_key,message);
    if (num>=0)
    {
      std::string mess=std::to_string(num)+";"+message;
      client_->PublishReliable(
        exchange, routing_key, mess,
        userver::urabbitmq::MessageType::kTransient,
        engine::Deadline::FromDuration(std::chrono::milliseconds{200}));
    }
    else
      throw std::runtime_error("WTF");
  }

void RabbitSend::AddQueue(std::string&& name)
  {
    queues.push_back(userver::urabbitmq::Queue(name+"'s_queue"));
    const auto setup_deadline =userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
    auto admin_channel = client_->GetAdminChannel(engine::Deadline::FromDuration(std::chrono::seconds{1}));
    admin_channel.DeclareQueue(queues.back(), setup_deadline);
    std::string x="#.";
    x+=name;
    x+=".#";
    admin_channel.BindQueue(exchange, queues.back(), x, setup_deadline);
  }

RabbitSend::~RabbitSend ()
  {
    auto admin_channel = client_->GetAdminChannel(
        engine::Deadline::FromDuration(std::chrono::seconds{1}));

    const auto teardown_deadline =userver::engine::Deadline::FromDuration(std::chrono::seconds{2});
    for (auto it=queues.begin();it!=queues.end();++it)
    {
      admin_channel.RemoveQueue(*it, teardown_deadline);
      queues.pop_front();
    }
    admin_channel.RemoveExchange(exchange, teardown_deadline);
  }

RabbitConsumer::RabbitConsumer(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
      : userver::urabbitmq::ConsumerComponentBase{config, context},
       pg_cluster_(context.FindComponent<components::Postgres>("commands_db").GetCluster())
       {}

bool RabbitConsumer::InsertInResponses(const std::string& name,int index,const std::string& resp)
  {
    storages::postgres::Transaction transaction =
    pg_cluster_->Begin("trs_ist",storages::postgres::ClusterHostType::kMaster, {});
    auto res = transaction.Execute(InsInResQue,index,resp,name);
    if (res.RowsAffected()) {
      transaction.Commit();
      return true;
    }
    return false;
  }

void RabbitConsumer::Process(std::string message)
  {
    int p=message.find(';');
  	std::string index=message.substr(0,p);
    message.erase(0,p+1);
    p=message.find(';');
    std::string name=message.substr(0,p);
    message.erase(0,p+1);  
    InsertInResponses(name,std::stoll(index),message);
  }
