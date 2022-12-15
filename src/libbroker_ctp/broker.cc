#include "broker.h"
#include <boost/filesystem.hpp>

namespace yijinjing {
    Broker::Broker() {
    }

    Broker::~Broker() {
    }

    void Broker::StartWork(yijinjing::BrokerOptionsPtr options) {
        std::string trading_day = std::to_string(x::RawDate());
        std::string broker_dir = options->broker_dir();
        std::string broker_name_ = options->broker_name();
        std::string strategy_dir_ = options->strategy_dir();
        std::string fund_id = options->fund_id();
        string key = fund_id + "_" + trading_day;
        std::string broker_file = "broker_" + fund_id + "_" + trading_day;
        writer_ = yijinjing::JournalWriter::create(broker_dir, broker_file, broker_name_);

        std::set<std::string> all_strategy;
        if (boost::filesystem::exists(strategy_dir_)) {
            boost::filesystem::path p(strategy_dir_);
            for (auto &file: boost::filesystem::directory_iterator(p)) {
                const string filename = file.path().filename().string();
                auto it = filename.find(key);
                if (it != filename.npos) {
                    string strategy_name = filename.substr(0, it - 1);
                    all_strategy.insert(strategy_name);
                }
            }
            for (auto &it: all_strategy) {
                AddReadStrategyFile(strategy_dir_, it, broker_name_);
            }
        } else {
            __error << "strategy_dir: " << strategy_dir_ << " not exit.";
        }

        OnInit();
        thread_ = std::make_shared<std::thread>(std::bind(&Broker::Read, this));
        thread_->detach();
    }

    void Broker::Run() {
        while (true) {
            x::Sleep(1000);
        }
    }

    void Broker::SetWriteBrokerFile(const string &dir, const string &file, const string &client) {
        // writer_ = yijinjing::JournalWriter::create(dir, file, client);
    }

    void Broker::AddReadStrategyFile(const string &dir, const string &file, const string &client) {
        if (!reader_) {
            reader_ = yijinjing::JournalReader::create(dir, file, yijinjing::TIME_FROM_FIRST, client);
        } else {
            reader_->addJournal(dir, file);
        }
        reader_->seekTimeJournalByName(file, yijinjing::TIME_TO_LAST);
    }

    void Broker::Read() {
        while (true) {
            yijinjing::FramePtr frame = reader_->getNextFrame();
            if (frame.get() != nullptr) {
                ProcessMessage(frame);
            } else {
                // 空闲时查询
            }
        }
    }

    void Broker::ProcessMessage(yijinjing::FramePtr frame) {
        short msg_type = frame->getMsgType();
        void *data = frame->getData();
        int64_t msg_time = frame->getNano();
        int len = frame->getDataLength();
        FH_TYPE_LASTFG last_flag = frame->getLastFlag();
        switch (msg_type) {
            case TRADE_ORDER_REQ: {
                TradeOrderMessage *msg = (TradeOrderMessage *) data;
                yijinjing::TradeOrder *order = &msg->item;
                std::cout << " code: " << order->code
                          << ", price: " << order->price
                          << ", volume: " << order->volume
                          << ", bs_flag: " << order->bs_flag << std::endl;
                if (fund_id_.compare(msg->fund_id) == 0) {
                    OnTradeOrder(msg);
                }
                break;
            }
            case TRADE_WITHDRAQ_REQ: {
                TradeWithdrawMessage *msg = (TradeWithdrawMessage *) data;
                std::cout << "Withdraw, order_no: " << msg->order_no;
                if (fund_id_.compare(msg->fund_id) == 0) {
                    OnTradeWithdraw(msg);
                }
                break;
            }
            default:
                break;
        }
    }
}