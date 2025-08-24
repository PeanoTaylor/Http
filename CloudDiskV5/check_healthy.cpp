#include <workflow/WFTaskFactory.h>
#include <workflow/WFFacilities.h>
#include <nlohmann/json.hpp>
#include <utility>
#include <iostream>

static std::pair<std::string,int>
discover_one_healthy(const std::string& consul_http, const std::string& service_name) {
    using json = nlohmann::json;
    std::string url = consul_http + "/v1/health/service/" + service_name + "?passing=true";

    std::pair<std::string,int> ep{"",-1};
    WFFacilities::WaitGroup wg(1);

    auto* task = WFTaskFactory::create_http_task(url, 3, 2,
        [&ep,&wg](WFHttpTask* t){
            if (t->get_state() != WFT_STATE_SUCCESS) { wg.done(); return; }
            protocol::HttpResponse* resp = t->get_resp();
            const void* body; size_t len; resp->get_parsed_body(&body,&len);
            try{
                auto arr = json::parse(std::string((const char*)body,len));
                if (!arr.is_array() || arr.empty()) { wg.done(); return; }
                // 取第一个健康实例（可按需改成随机/负载均衡）
                auto& item = arr[0];
                std::string ip = item["Service"].value("Address", "");
                int port = item["Service"].value("Port", -1);
                if (ip.empty() && item.contains("Node"))
                    ip = item["Node"].value("Address","");
                if (!ip.empty() && port>0) ep = {ip, port};
            } catch(...) {}
            wg.done();
        });
    task->get_req()->set_method("GET");
    task->start();
    wg.wait();
    return ep;
}
