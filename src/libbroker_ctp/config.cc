#include "config.h"

namespace yijinjing {

    Config* Config::instance_ = 0;

    Config* Config::Instance() {
        if (instance_ == 0) {
            instance_ = new Config();
            instance_->Init();
        }
        return instance_;
    }

    void Config::Init() {
        string filename = x::FindFile("config.ini");
        x::INI ini = x::Ini(filename);
        options_ = BrokerOptions::Load(filename);

        //ctp_market_front_ = ini.get<string>("ctp.ctp_market_front");
        ctp_trade_front_ = ini.get<string>("ctp.ctp_trade_front");
        ctp_broker_id_ = ini.get<string>("ctp.ctp_broker_id");
        ctp_investor_id_ = ini.get<string>("ctp.ctp_investor_id");
        ctp_password_ = ini.get<string>("ctp.ctp_password");
        ctp_app_id_ = ini.get<string>("ctp.ctp_app_id");
        ctp_product_info_ = ini.get<string>("ctp.ctp_product_info");
        ctp_auth_code_ = ini.get<string>("ctp.ctp_auth_code");
        disable_subscribe_ = ini.get<string>("ctp.disable_subscribe") == "true" ? true : false;

        string s = ini.get<string>("risk.risk_forbid_closing_today");
        risk_forbid_closing_today_ = x::ToLower(s) == "true" ? true : false;
        risk_max_today_opening_volume_ = ini.get<int>("risk.risk_max_today_opening_volume");
        try {
            string log_level = ini.get<string>("log.level");
            // x::SetLogLevel(log_level);
        } catch (...) {
            // pass
        }
        stringstream ss;
        ss << "+-------------------- configuration begin --------------------+" << endl;
        ss << options_->ToString() << endl;
        ss << endl;
        ss << "ctp:" << endl
            << "  ctp_trade_front: " << ctp_trade_front_ << endl
            << "  ctp_broker_id: " << ctp_broker_id_ << endl
            << "  ctp_investor_id: " << ctp_investor_id_ << endl
            << "  ctp_password: " << string(ctp_password_.size(), '*') << endl
            << "  ctp_app_id: " << ctp_app_id_ << endl
            << "  ctp_product_info: " << ctp_product_info_ << endl
            << "  ctp_auth_code: " << ctp_auth_code_ << endl
            << "  disable_subscribe: " << (disable_subscribe_ ? "true" : "false") << endl
            << "risk:" << endl
            << "  risk_forbid_closing_today: " << (risk_forbid_closing_today_ ? "true" : "false") << endl
            << "  risk_max_today_opening_volume: " << risk_max_today_opening_volume_ << endl;
        ss << "+-------------------- configuration end   --------------------+";
        __info << endl << ss.str();
    }
}