#pragma once
#include <iostream>
#include <fstream>
#include <string.h>
#include <math.h>
#include <algorithm>
#include "strategy/strategy.h"



// 一个reader, 读feeder 与 broker
// 一个writer, 写broker
namespace yijinjing {
    class TestStrategy : public Strategy {
    public:
        TestStrategy();
        virtual ~TestStrategy();

        virtual void OnInit();
        virtual void OnTick(QTickT* data);

        virtual void OnRspQueryAccout(yijinjing::QueryTradeAssetRep* rsp);
        virtual void OnRspQueryPosition(QueryTradePositionRep* rsp);
        virtual void OnRspQueryTrade(QueryTradeKnockRep* rsp);

        virtual void OnRspOrder(TradeOrderMessage* rsp);
        virtual void OnRspWithdraw(TradeWithdrawMessage* rsp);
        virtual void OnRtnTradeKnock(TradeKnock* rtn);
    };
}

