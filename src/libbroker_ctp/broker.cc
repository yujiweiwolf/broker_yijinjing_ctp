#include "broker.h"

Broker::Broker(string fund_id) : fund_id_(fund_id){
}

Broker::~Broker() {
}

void Broker::StartWork() {
    OnInit();
    thread_ = std::make_shared<std::thread>(std::bind(&Broker::Run, this));
    thread_->detach();
}

void Broker::SetWriteBrokerFile(const string& dir, const string& file, const string& client) {
    writer_ = yijinjing::JournalWriter::create(dir, file, client);
}

void Broker::AddReadStrategyFile(const string& dir, const string& file, const string& client) {
    if (!reader_) {
        reader_ = yijinjing::JournalReader::create(dir, file, yijinjing::TIME_FROM_FIRST, client);
    } else {
        reader_->addJournal(dir, file);
    }
    reader_->seekTimeJournalByName(file, yijinjing::TIME_TO_LAST);
}

void Broker::Run() {
    while (true) {
        yijinjing::FramePtr frame = reader_->getNextFrame();
        ProcessMessage(frame);
    }
}

void Broker::ProcessMessage(yijinjing::FramePtr frame) {
    if (frame.get() != nullptr) {
        short msg_type = frame->getMsgType();
        void* data = frame->getData();
        int64_t msg_time = frame->getNano();
        int len = frame->getDataLength();
        FH_TYPE_LASTFG last_flag = frame->getLastFlag();
        switch(msg_type) {
            case TRADE_ORDER_REQ: {
                TradeOrderMessage* msg = (TradeOrderMessage*) data;
                yijinjing::TradeOrder* order = &msg->item;
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
                TradeWithdrawMessage* msg = (TradeWithdrawMessage*) data;
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