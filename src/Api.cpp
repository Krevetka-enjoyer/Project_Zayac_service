#include "Api.h"

Ispolnitel::Ispolnitel(const components::ComponentConfig& config,const components::ComponentContext& context): 
    server::handlers::HttpHandlerJsonBase{config, context},
    my_rabbit_{context.FindComponent<RabbitSend>()},
    http_client_{context.FindComponent<components::HttpClient>().GetHttpClient()}
  {}
formats::json::Value Ispolnitel::HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const
  {
    if (!request_json.HasMember("message")) 
    {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return formats::json::FromString(R"("{"error": "missing required field "message""}")");
    }
    my_rabbit_.Publish(request_json["message"].As<std::string>(),request_json["users"].As<std::string>());
    return {};
  }

Users_Informator::Users_Informator(const components::ComponentConfig& config,const components::ComponentContext& context):
  server::handlers::HttpHandlerJsonBase{config, context},my_rabbit_{context.FindComponent<RabbitSend>()},
  http_client_{context.FindComponent<components::HttpClient>().GetHttpClient()}
  {
  }

formats::json::Value Users_Informator::HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const 
  {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    if (request.GetMethod() == userver::server::http::HttpMethod::kGet) 
    {
      auto req = http_client_.CreateRequest()
                     ->get()
                     ->retry(1)
                     ->http_version(clients::http::HttpVersion::k11)
                     ->timeout(std::chrono::seconds{2})
                     ->url("http://guest:guest@"+my_rabbit_.host+':'+my_rabbit_.port+"/api/users");
      auto res = req->perform();
      if (res->IsOk()) 
      {
        formats::json::Value json = formats::json::FromString(std::move(*res).body());
        std::string names;
        for (auto i=json.begin();i!=json.end();++i)
        {
          if (!names.empty())
            names+=',';
          names+=(*i)["name"].As<std::string>();
        }
        builder["names"]=names;
        return builder.ExtractValue();;
      }
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return formats::json::FromString(R"("{"Error": "No Users""}")");
    }
    else if (request_json.HasMember("user"))
    {
      auto request = http_client_.CreateRequest()
                     ->put()
                     ->data("{\"password\":\""+request_json["pass"].As<std::string>()+"\",\"tags\":\"administrator\"}")
                     ->retry(1)
                     ->http_version(clients::http::HttpVersion::k11)
                     ->timeout(std::chrono::seconds{2})
                     ->url("http://guest:guest@"+my_rabbit_.host+':'+my_rabbit_.port+"/api/users/"+request_json["user"].As<std::string>());
      auto res = request->perform();
      auto request2 = http_client_.CreateRequest()
                     ->put()
                     ->data("{\"configure\":\".*\",\"write\":\".*\",\"read\":\".*\"}")
                     ->retry(1)
                     ->http_version(clients::http::HttpVersion::k11)
                     ->timeout(std::chrono::seconds{2})
                     ->url("http://guest:guest@"+my_rabbit_.host+':'+my_rabbit_.port+"/api/permissions/%2F/"+request_json["user"].As<std::string>());
      res = request2->perform();
      builder["messages"] = "good";
      my_rabbit_.AddQueue(request_json["user"].As<std::string>());
      return builder.ExtractValue();
    }
    else
    {
      request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
      return formats::json::FromString(R"("{"error": "missing required field "user""}")");
    }
  }

User_Informator::User_Informator(const components::ComponentConfig& config,const components::ComponentContext& context):
  server::handlers::HttpHandlerJsonBase{config, context},
  pg_cluster_(context.FindComponent<components::Postgres>("commands_db").GetCluster())
  {}

formats::json::Value User_Informator::HandleRequestJsonThrow(const server::http::HttpRequest& request,const formats::json::Value& request_json,
                                          server::request::RequestContext&) const
  {
    formats::json::ValueBuilder builder{formats::json::Type::kObject};
    storages::postgres::ResultSet res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave, kSelectValue, request.GetPathArg("name"));
    if (res.IsEmpty()) 
      builder["Responses"] = "No responses";
    else
    {
        for (auto row : res) 
        {
          auto [index, cmd, resp, cTime, rTime] = row.As<int64_t, std::string, std::string, storages::postgres::TimePointTz, storages::postgres::TimePointTz>();
          builder[std::to_string(index)]["Command:"] = cmd;
          builder[std::to_string(index)]["Command_Time:"] = utils::datetime::Timestring(cTime.GetUnderlying());
          builder[std::to_string(index)]["Response:"] = resp;
          builder[std::to_string(index)]["Response_Time:"] = utils::datetime::Timestring(rTime.GetUnderlying());
        }
    }
    return builder.ExtractValue();
  }
