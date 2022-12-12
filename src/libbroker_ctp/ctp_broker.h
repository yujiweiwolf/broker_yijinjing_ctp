#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <memory>
#include <mutex>

#include "broker.h"
#include "ctp_support.h"
#include "config.h"

namespace yijinjing {
    class CTPBroker : public Broker, public CThostFtdcTraderSpi {
    public:
        CTPBroker();
        virtual ~CTPBroker();

        virtual void OnInit();
        virtual void OnQueryTradeAsset(QueryTradeAssetReq* req);
        virtual void OnQueryTradePosition(QueryTradePositionReq* req);
        virtual void OnQueryTradeKnock(QueryTradeKnockReq* req);
        virtual void OnTradeOrder(TradeOrderMessage* req);
        virtual void OnTradeWithdraw(TradeWithdrawMessage* req);

        /// 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
        virtual void OnFrontConnected();

        /// 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
        virtual void OnFrontDisconnected(int nReason);

        /// 客户端认证响应
        virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 登录请求响应
        virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 登出请求响应
        virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 投资者结算结果确认响应
        virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 请求查询交易所响应
        // virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

        /// 请求查询合约响应
        virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 请求查询资金账户响应
        virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 请求查询投资者持仓响应
        virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 请求查询报单响应
        virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

        /// 请求查询成交响应
        virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 报单录入请求响应
        virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 报单录入错误回报
        virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

        /// 报单操作请求响应
        virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        /// 报单操作错误回报
        virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

        /// 报单通知
        virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

        /// 成交通知
        virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

        /// 错误应答
        virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    private:
        void ReqAuthenticate();  // 客户端认证请求
        void ReqUserLogin();  // 登陆请求
        void ReqSettlementInfoConfirm();  // 请求确认结算单，确认后才可以进行交易
        void ReqQryInstrument();
        void RunCtp();
        void Wait();
        void Start();
        int GetRequestID();
        void PrepareQuery();
        string GetContractName(const string code);

    private:
        CThostFtdcTraderApi* ctp_api_ = nullptr;
        std::shared_ptr<std::thread> thread_;

        int state_ = 0;  // 接口状态，1-已登录，2-已确认结算单，3-已查询到合约信息
        string broker_id_;  // 经纪公司代码
        string investor_id_;  // 投资者代码
        int64_t date_ = 0;  // 当前交易日，从登陆成功响应中获取
        int64_t front_id_ = 0;  // 前置编号，从登陆成功响应中获取
        int64_t session_id_ = 0;  // 会话编号，从登陆成功响应中获取
        // int64_t order_ref_ = 0;  // 报单引用，从登陆成功响应中进行初始化，之后每次报单递增
        int64_t pre_trading_day_ = 0;  // 当前交易日的前一个交易日， 周一的前一日是周五,
        int64_t pre_trading_day_next_ = 0;  // 当前交易日的前一个交易日的下一天

        int start_index_ = 0;
        mutex mutex_;
        string query_cursor_;
        int64_t pre_query_timestamp_ = 0;  // 上次查询的时间戳，用于进行流控控制，CTP限制每秒只能查询一次
        map<string, string> order_nos_;  // CTP的OrderSysId到内部order_no的映射关系，用于在成交回报接收时查找对应的委托合同号
        std::unordered_map<std::string, std::pair<std::string, int>> all_instruments_;  // 保存合约名称与乘数
        std::unordered_map<std::string, std::unique_ptr<co::fbs::TradePositionT>> all_pos_;
        std::unordered_map<int, TradeOrderMessage> order_recode_;
        std::unordered_map<int, TradeWithdrawMessage> withdraw_recode_;
        std::unordered_map<std::string, TradeWithdrawMessage> withdraw_msg_;  // OnRtnOrder中的RequestID是0，导致必须要自己维护, key是order_no
    };
}
