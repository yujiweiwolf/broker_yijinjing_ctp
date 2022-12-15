#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <memory>

#include <x/x.h>

namespace yijinjing {

    using namespace std;

    /**
     * ��̨�ӿڷ���������ѡ��
     */
    class BrokerOptions {
    public:
        static shared_ptr<BrokerOptions> Load(const std::string& filename = "");

        std::string ToString();

        inline std::string trade_gateway() const {
            return trade_gateway_;
        }

        inline std::string upstream_address() const {
            return upstream_address_;
        }

        inline int64_t request_timeout_ms() const {
            return request_timeout_ms_;
        }

        inline int64_t query_asset_interval_ms() const {
            return query_asset_interval_ms_;
        }

        inline int64_t query_position_interval_ms() const {
            return query_position_interval_ms_;
        }

        inline int64_t query_knock_interval_ms() const {
            return query_knock_interval_ms_;
        }

        inline int64_t idle_sleep_ns() const {
            return idle_sleep_ns_;
        }

        inline int64_t cpu_affinity() const {
            return cpu_affinity_;
        }

        inline bool enable_stock_short_selling() const {
            return enable_stock_short_selling_;
        }

        inline bool enable_query_only() const {
            return enable_query_only_;
        }

        inline std::string wal() const {
            return wal_;
        }

        inline bool enable_upload() const {
            return enable_upload_;
        }
        inline string fund_id() const {
            return fund_id_;
        }
        inline string broker_dir() const {
            return broker_dir_;
        }
        inline string broker_name() const {
            return broker_name_;
        }
        inline string strategy_dir() const {
            return strategy_dir_;
        }

    protected:
        void Init(const string& filename);

    private:
        std::shared_ptr<x::LoggingOptions> log_opt_;
        std::string trade_gateway_;
        std::string upstream_address_;
        std::string wal_;

        bool enable_upload_ = true; // �Ƿ������ϴ���������

        int64_t request_timeout_ms_ = 5000; // ����ʱʱ��

        bool enable_stock_short_selling_ = false;
        bool enable_query_only_ = false; // �Ƿ�����ֻ��ѯģʽ�������ձ����ͳ�����ָ��

        int64_t query_asset_interval_ms_ = 0; // �ʽ��ѯʱ����
        int64_t query_position_interval_ms_ = 0; // �ֲֲ�ѯʱ����
        int64_t query_knock_interval_ms_ = 0; // �ɽ���ѯʱ����
        int64_t idle_sleep_ns_ = 100000; // �������п�תʱ����ʱ�䣨��λ�����룩
        int64_t cpu_affinity_ = -1; // CPU�˰�
        std::string fund_id_;
        std::string broker_dir_;
        std::string broker_name_;
        std::string strategy_dir_;
    };

    typedef std::shared_ptr<BrokerOptions> BrokerOptionsPtr;

}
