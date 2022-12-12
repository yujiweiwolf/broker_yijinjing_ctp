#include <iostream>
#include <sstream>
#include <string>
#include <regex>
#include <filesystem>

#include <x/x.h>

#include "broker_options.h"

namespace yijinjing {

    namespace fs = std::filesystem;

    shared_ptr<BrokerOptions> BrokerOptions::Load(const string& filename) {
        shared_ptr<BrokerOptions> opt = std::make_shared<BrokerOptions>();
        opt->Init(filename);
        return opt;
    }

    void BrokerOptions::Init(const std::string& filename) {
        string _filename = filename.empty() ? "broker.ini" : filename;
        x::INI ini = x::FindIni(_filename);
//        wal_ = ini.get<std::string>("broker.wal");
//        enable_upload_ = ini.get<bool>("broker.enable_upload", true);
//        trade_gateway_ = ini.get<string>("broker.trade_gateway");
//        upstream_address_ = ini.get<string>("broker.upstream_address", ""); // @TODO 待删除
//        request_timeout_ms_ = ini.get<int64_t>("broker.request_timeout_ms", 0);
//        if (request_timeout_ms_ <= 0) {
//            request_timeout_ms_ = 10000;
//        }
//        enable_stock_short_selling_ = x::ToLower(ini.get<string>("broker.enable_stock_short_selling", "false")) == "true" ? true : false;
//        enable_query_only_ = x::ToLower(ini.get<string>("broker.enable_query_only", "false")) == "true" ? true : false;

        query_asset_interval_ms_ = ini.get<int64_t>("broker.query_asset_interval_ms", 0);
        query_position_interval_ms_ = ini.get<int64_t>("broker.query_position_interval_ms", 0);
        query_knock_interval_ms_ = ini.get<int64_t>("broker.query_knock_interval_ms", 0);
        idle_sleep_ns_ = ini.get<int64_t>("broker.idle_sleep_ns", 100000); // 100us
        cpu_affinity_ = ini.get<int64_t>("broker.cpu_affinity", -1);
        fund_id_ = ini.get<string>("broker.fund_id");
        broker_dir_ = ini.get<string>("broker.broker_dir");
        broker_name_ = ini.get<string>("broker.broker_name");
        strategy_dir_ = ini.get<string>("broker.strategy_dir");
        log_opt_ = x::LoggingOptions::Load(_filename);
        try {
            x::InitializeLogging(*log_opt_);
        } catch (std::exception& e) {
            std::cerr << "init logging error: " << e.what();
        }
    }

    string BrokerOptions::ToString() {
        std::regex re("cluster_token=[^&a-zA-Z0-9]*");
        string gw = trade_gateway_;
        std::regex_replace(gw, re, "*");
        stringstream ss;
        ss << "broker:" << std::endl
//            << "  trade_gateway: " << gw << std::endl
//            << "  wal: " << wal_ << std::endl
//            << "  upstream_address: " << upstream_address_ << std::endl
//            << "  request_timeout_ms: " << request_timeout_ms_ << "ms" << std::endl
            << "  idle_sleep_ns: " << idle_sleep_ns_ << "ns" << std::endl
            << "  cpu_affinity: " << cpu_affinity_ << std::endl
            << "  fund_id: " << fund_id_ << std::endl
            << "  broker_dir: " << broker_dir_ << std::endl
            << "  broker_name: " << broker_name_ << std::endl
            << "  strategy_dir: " << strategy_dir_ << std::endl
//            << "  enable_upload: " << (enable_upload_ ? "true" : "false") << std::endl
//            << "  enable_stock_short_selling: " << (enable_stock_short_selling_ ? "true" : "false") << std::endl
//            << "  enable_query_only: " << (enable_query_only_ ? "true" : "false") << std::endl
            << "  query_asset_interval_ms: " << query_asset_interval_ms_ << "ms" << std::endl
            << "  query_position_interval_ms: " << query_position_interval_ms_ << "ms" << std::endl
            << "  query_knock_interval_ms: " << query_knock_interval_ms_ << "ms" << std::endl
            << log_opt_->ToString() << std::endl;
        return ss.str();
    }

}
