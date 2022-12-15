// Copyright 2020 Fancapital Inc.  All rights reserved.
#pragma once
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "journal/Timer.h"
#include "journal/JournalWriter.h"
#include "journal/JournalReader.h"
#include "common/datastruct.h"
#include "broker_options.h"

using yijinjing::JournalReaderPtr;
using yijinjing::JournalWriterPtr;
using namespace yijinjing;

namespace yijinjing {
    class Broker {
    public:
        Broker();

        virtual ~Broker();

        void StartWork(yijinjing::BrokerOptionsPtr options);

        void Run();

        void SetWriteBrokerFile(const string &dir, const string &file, const string &client);

        void AddReadStrategyFile(const string &dir, const string &file, const string &client);

        virtual void OnInit() {};

        virtual void OnQueryTradeAsset(QueryTradeAssetReq *req) {};

        virtual void OnQueryTradePosition(QueryTradePositionReq *req) {};

        virtual void OnQueryTradeKnock(QueryTradeKnockReq *req) {};

        virtual void OnTradeOrder(TradeOrderMessage *req) {};

        virtual void OnTradeWithdraw(TradeWithdrawMessage *req) {};
    private:
        void Read();

        void ProcessMessage(yijinjing::FramePtr frame);

    protected:
        JournalReaderPtr reader_;
        JournalWriterPtr writer_;
        std::shared_ptr<std::thread> thread_;
        std::mutex write_mutex_;
        std::string fund_id_;
    };
}




