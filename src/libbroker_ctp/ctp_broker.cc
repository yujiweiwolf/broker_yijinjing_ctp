#include <boost/algorithm/string.hpp>
#include "ctp_broker.h"


namespace yijinjing {
    CTPBroker::CTPBroker() : Broker(), CThostFtdcTraderSpi() {
        start_index_ = x::RawTime();
        // query_instruments_finish_.store(false);
        broker_id_ = Config::Instance()->ctp_broker_id();
        investor_id_ = Config::Instance()->ctp_investor_id();
    }

    CTPBroker::~CTPBroker() {
        if (ctp_api_) {
            ctp_api_->RegisterSpi(nullptr);
            ctp_api_->Release();
            ctp_api_ = nullptr;
        }
    }

    void CTPBroker::OnInit() {
        __info << "initialize CTPBroker ...";
        thread_ = std::make_shared<std::thread>(std::bind(&CTPBroker::RunCtp, this));
        thread_->detach();
        Wait();
        __info << "initialize CTPBroker successfully";
    }

    void CTPBroker::RunCtp() {
        bool disable_subscribe = Config::Instance()->disable_subscribe();
        ctp_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi("");
        ctp_api_->RegisterSpi(this);
        string addr = Config::Instance()->ctp_trade_front();
        ctp_api_->RegisterFront((char*)addr.c_str());
        if (!disable_subscribe) {
            ctp_api_->SubscribePublicTopic(THOST_TERT_RESTART);
            ctp_api_->SubscribePrivateTopic(THOST_TERT_RESTART);
        }
        ctp_api_->Init();
        ctp_api_->Join();
    }

    void CTPBroker::Wait() {
        while (state_ < kStartupStepGetInitPositionsOver) {
            x::Sleep(10);
        }
    }

    void CTPBroker::Start() {
        state_ = kStartupStepInit;
        string ctp_app_id = Config::Instance()->ctp_app_id();
        if (!ctp_app_id.empty()) {
            ReqAuthenticate();
        } else {
            ReqUserLogin();
        }
    }

    void CTPBroker::ReqAuthenticate() {
        __info << "authenticate ...";
        string app_id = Config::Instance()->ctp_app_id();
        string product_info = Config::Instance()->ctp_product_info();
        string auth_code = Config::Instance()->ctp_auth_code();
        CThostFtdcReqAuthenticateField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker_id_.c_str());
        strcpy(req.UserID, investor_id_.c_str());
        strcpy(req.AppID, app_id.c_str());
        strcpy(req.UserProductInfo, product_info.c_str());
        strcpy(req.AuthCode, auth_code.c_str());
        int rc = 0;
        while ((rc = ctp_api_->ReqAuthenticate(&req, GetRequestID())) != 0) {
            __warn << "ReqAuthenticate failed: " << CtpApiError(rc) << ", retring ...";
            x::Sleep(CTP_FLOW_CONTROL_MS);
        }
    }

    void CTPBroker::ReqUserLogin() {
        __info << "login ...";
        string pwd = Config::Instance()->ctp_password();
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker_id_.c_str());
        strcpy(req.UserID, investor_id_.c_str());
        strcpy(req.Password, pwd.c_str());
        int rc = 0;
        while ((rc = ctp_api_->ReqUserLogin(&req, GetRequestID())) != 0) {
            __warn << "ReqUserLogin failed: " << CtpApiError(rc) << ", retring ...";
            x::Sleep(CTP_FLOW_CONTROL_MS);
        }
    }

    void CTPBroker::ReqSettlementInfoConfirm() {
        __info << "confirm settlement info ...";
        CThostFtdcSettlementInfoConfirmField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker_id_.c_str());
        strcpy(req.InvestorID, investor_id_.c_str());
        int rc = ctp_api_->ReqSettlementInfoConfirm(&req, GetRequestID());
        if (rc != 0) {
            __error << "ReqSettlementInfoConfirm failed: " << CtpApiError(rc);
        }
    }

    void CTPBroker::ReqQryInstrument() {
        __info << "query all future contracts ...";
        PrepareQuery();
        CThostFtdcQryInstrumentField req;
        memset(&req, 0, sizeof(req));
        int rc = 0;
        while ((rc = ctp_api_->ReqQryInstrument(&req, GetRequestID())) != 0) {
            if (is_flow_control(rc)) {
                __warn << "ReqQryInstrument failed: " << CtpApiError(rc)
                       << ", retry in " << CTP_FLOW_CONTROL_MS << "ms ...";
                x::Sleep(CTP_FLOW_CONTROL_MS);
                continue;
            } else {
                __error << "ReqQryInstrument failed: " << CtpApiError(rc);
                break;
            }
        }
    }

    void CTPBroker::PrepareQuery() {
        int64_t elapsed_ms = x::Timestamp() - pre_query_timestamp_;
        int64_t Sleep_ms = CTP_FLOW_CONTROL_MS - elapsed_ms;
        if (Sleep_ms > 0) {
            __info << elapsed_ms << "ms elapsed after pre query, Sleep " << Sleep_ms << "ms for flow control ...";
            x::Sleep(Sleep_ms);
        }
        pre_query_timestamp_ = x::Timestamp();
    }

    string CTPBroker::GetContractName(const string code) {
        string name = "";
        auto it = all_instruments_.find(code);
        if (it != all_instruments_.end()) {
            name = it->second.first;
        }
        return name;
    }

    int CTPBroker::GetRequestID() {
        return ++start_index_;
    }

    void CTPBroker::OnQueryTradeAsset(QueryTradeAssetReq* req) {
        CThostFtdcQryTradingAccountField field;
        memset(&field, 0, sizeof(field));
        strcpy(field.BrokerID, broker_id_.c_str());
        strcpy(field.InvestorID, investor_id_.c_str());
        strcpy(field.CurrencyID, "CNY");  // 只查询人民币资金
        int ret = ctp_api_->ReqQryTradingAccount(&field, GetRequestID());
        if (ret != 0) {
            __error << "query asset error: " << ret;
        }
    }

    void CTPBroker::OnQueryTradePosition(QueryTradePositionReq* req) {
        CThostFtdcQryInvestorPositionField field;
        memset(&field, 0, sizeof(field));
        strcpy(field.BrokerID, broker_id_.c_str());
        strcpy(field.InvestorID, investor_id_.c_str());
        int ret = ctp_api_->ReqQryInvestorPosition(&field, GetRequestID());
        if (ret != 0) {
            __error << "query position error: " << ret;
        }
    }

    void CTPBroker::OnQueryTradeKnock(QueryTradeKnockReq* req) {
        CThostFtdcQryTradeField field;
        memset(&field, 0, sizeof(field));
        strcpy(field.BrokerID, broker_id_.c_str());
        strcpy(field.InvestorID, investor_id_.c_str());
        strncpy(field.TradeTimeStart, query_cursor_.data(), query_cursor_.length());
        int ret = ctp_api_->ReqQryTrade(&field, GetRequestID());
        if (ret != 0) {
            __error << "query knock error: " << ret;
        }
    }

    void CTPBroker::OnTradeOrder(TradeOrderMessage* req) {
        yijinjing::TradeOrder* order = &req->item;
        int64_t auto_oc_flag = order->oc_flag;
        __info << "order, code: " << order->code
               << ", bs_flag: " << order->bs_flag
               << ", oc_flag: " << order->oc_flag
               << ", volume: " << order->volume;
        // 自动开平
        __info << "auto_oc_flag: " << auto_oc_flag;
        string ctp_code = order->code;
        if (order->market == co::kMarketCZCE) {
            DeleteCzceCode(ctp_code);
        }
        CThostFtdcInputOrderField _req;
        memset(&_req, 0, sizeof(_req));
        strcpy(_req.BrokerID, broker_id_.c_str());
        strcpy(_req.InvestorID, investor_id_.c_str());
        strcpy(_req.InstrumentID, ctp_code.c_str());
        int _request_id = GetRequestID();
        sprintf(_req.OrderRef, "%d", _request_id);
        _req.Direction = bs_flag2ctp(order->bs_flag);  // 买卖方向
        _req.CombOffsetFlag[0] = oc_flag2ctp(auto_oc_flag);  // 组合开平标记, 单腿合约只设置第一个即可
        _req.CombHedgeFlag[0] = THOST_FTDC_CIDT_Speculation;  // 组合套保标记, 单腿合约只设置第一个即可
        _req.VolumeTotalOriginal = order->volume;  /// 数量
        // ------------------------
        _req.VolumeCondition = THOST_FTDC_VC_AV;  /// 成交量类型：任何数量
        _req.MinVolume = 1;  // 最小成交量
        _req.IsAutoSuspend = 0;  /// 自动挂起标志: 否
        _req.UserForceClose = 0;  /// 用户强评标志: 否
        _req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;  /// 强平原因: 非强平
        _req.IsSwapOrder = 0;  // 互换单标志
        // ----------------
        _req.OrderPriceType = order_price_type2ctp(order->price_type);  // 报单价格条件
        _req.LimitPrice = order->price;  /// 价格
        _req.TimeCondition = THOST_FTDC_TC_GFD;  // order_time_condition2ctp(order.order_type()); ///有效期类型
        _req.ContingentCondition = THOST_FTDC_CC_Immediately;  // 触发条件：立即
        {
            std::unique_lock<std::mutex> lock(mutex_);
            order_recode_.emplace(std::make_pair(_request_id, *req));
        }
        int _ret = ctp_api_->ReqOrderInsert(&_req, _request_id);
        __info << "ReqOrderInsert, request_id: " << _request_id;
        // 这句话应该从不会触发
        if (_ret != 0) {
            string error_msg = "order faild, ret: " + std::to_string(_ret) + ", " + CtpApiError(_ret);
            __error << error_msg;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                order_recode_.erase(_request_id);
            }

            {
                std::unique_lock<std::mutex> lock(write_mutex_);
                strncpy(req->error, error_msg.c_str(), error_msg.length());
                writer_->write_frame(req, sizeof(TradeOrderMessage), TRADE_ORDER_REP, 0);
            }
        }
    }

    void CTPBroker::OnTradeWithdraw(TradeWithdrawMessage* req) {
        CThostFtdcInputOrderActionField _req;
        memset(&_req, 0, sizeof(_req));
        strcpy(_req.BrokerID, broker_id_.c_str());
        strcpy(_req.InvestorID, investor_id_.c_str());
        _req.ActionFlag = THOST_FTDC_AF_Delete;  // 操作标志: 删除
        string order_no = req->order_no;
        vector<string> vec_info;
        boost::split(vec_info, order_no, boost::is_any_of("_"), boost::token_compress_on);
        string error_msg;
        if (vec_info.size() == 4) {
            int front_id = atoi(vec_info[0].c_str());
            int session_id = atoi(vec_info[1].c_str());
            _req.FrontID = front_id;
            _req.SessionID = session_id;
            strncpy(_req.OrderRef, vec_info[2].c_str(), vec_info[2].length());
            strncpy(_req.InstrumentID, vec_info[3].c_str(), vec_info[3].length());
            int _request_id = GetRequestID();
            // 撤单错误使用withdraw_msg_, 正确时使用req_msg_
            {
                std::unique_lock<std::mutex> lock(mutex_);
                withdraw_recode_.emplace(std::make_pair(_request_id, *req));
                withdraw_msg_.emplace(std::make_pair(order_no, *req));
            }
            __info << "ReqOrderAction, request_id: " << _request_id;
            int _ret = ctp_api_->ReqOrderAction(&_req, _request_id);
            if (_ret != 0) {
                error_msg = "withdraw faild, ret: " + std::to_string(_ret) + ", " + CtpApiError(_ret);
                std::unique_lock<std::mutex> lock(mutex_);
                withdraw_recode_.erase(_request_id);
                withdraw_msg_.erase(order_no);
            }
        } else {
            error_msg = "not valid order_no: " + order_no;
        }
        if (!error_msg.empty()) {
            std::unique_lock<std::mutex> lock(write_mutex_);
            strncpy(req->error, error_msg.c_str(), error_msg.length());
            writer_->write_frame(req, sizeof(TradeWithdrawMessage), TRADE_WITHDRAW_REP, 0);
        }
    }
    //------------------------------------------------------------------------------------------------------------------
    /// 当客户端与交易后台建立起通信连接时(还未登录前), 该方法被调用
    void CTPBroker::OnFrontConnected() {
        __info << "connect to CTP trade server ok";
        Start();
    }

    /// 当客户端与交易后台通信连接断开时, 该方法被调用. 当发生这个情况后,  API会自动重新连接, 客户端可不做处理.
    void CTPBroker::OnFrontDisconnected(int nReason) {
        stringstream ss;
        ss << "ret=" << nReason << ", msg=";
        switch (nReason) {
            case 0x1001:  //  0x1001 网络读失败
                ss << "read error";
                break;
            case 0x1002:  // 0x1002 网络写失败
                ss << "write error";
                break;
            case 0x2001:  // 0x2001 接收心跳超时
                ss << "recv heartbeat timeout";
                break;
            case 0x2002:  // 0x2002 发送心跳失败
                ss << "send heartbeat timeout";
                break;
            case 0x2003:  // 0x2003 收到错误报文
                ss << "recv broken data";
                break;
            default:
                ss << "unknown error";
                break;
        }
        __info << "connection is broken: " << ss.str();
        x::Sleep(2000);
    }

    void CTPBroker::OnRspUserLogout(CThostFtdcUserLogoutField* pUserLogout, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            __info << "logout ok";
        } else {
            __error << "logout failed: ret=" << pRspInfo->ErrorID << ", msg=" << CtpToUTF8(pRspInfo->ErrorMsg);
        }
    }

    /// 客户端认证响应//
    void CTPBroker::OnRspAuthenticate(CThostFtdcRspAuthenticateField* pRspAuthenticateField, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            __info << "authenticate ok";
            ReqUserLogin();
        } else {
            __error << "authenticate failed: " << CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
    }

    /// 登录请求响应//
    void CTPBroker::OnRspUserLogin(CThostFtdcRspUserLoginField* pRspUserLogin, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            date_ = atoi(ctp_api_->GetTradingDay());
            front_id_ = pRspUserLogin->FrontID;
            session_id_ = pRspUserLogin->SessionID;
            // order_ref_ = x::ToInt64(x::Trim(pRspUserLogin->MaxOrderRef));
            __info << "login ok: trading_day = " << date_ << ", front_id = " << front_id_ << ", session_id = " << session_id_ << ", max_order_ref = " << pRspUserLogin->MaxOrderRef;
            if (IsMonday(date_)) {
                pre_trading_day_ = x::PreDay(date_, 3);
                pre_trading_day_next_ = x::PreDay(date_, 2);
            } else {
                pre_trading_day_ = x::PreDay(date_);
                pre_trading_day_next_ = date_;
            }
            __info << "pre_trading_day: " << pre_trading_day_ << ", pre_trading_day_next: " << pre_trading_day_next_;
            if (date_ < 19700101 || date_ > 29991231) {
                __error << "illegal trading day: " << date_;
            } else {
                state_ = kStartupStepLoginOver;  // 登陆成功
                ReqSettlementInfoConfirm();
            }
        } else {
            __error << "login failed: " << CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
    }

    void CTPBroker::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField* pSettlementInfoConfirm, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            string date = pSettlementInfoConfirm ? pSettlementInfoConfirm->ConfirmDate : "";
            __info << "confirm settlement info ok: confirm_date = " << date;
        } else {
            __warn << "confirm settlement info failed: " << CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
        state_ = kStartupStepConfirmSettlementOver;
        ReqQryInstrument();
    }

    void CTPBroker::OnRspQryInstrument(CThostFtdcInstrumentField* p, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            if (p) {
                string ctp_code = p->InstrumentID;
                int64_t market = ctp_market2std(p->ExchangeID);
                if (market == co::kMarketCZCE) {
                    InsertCzceCode(ctp_code);
                }
                if (market) {
                    string suffix = GetMarketSuffix(market);
                    string code = ctp_code + suffix;
                    int _multiple = p->VolumeMultiple > 0 ? p->VolumeMultiple : 1;
                    all_instruments_.insert(make_pair(code, make_pair(x::GBKToUTF8(x::Trim(p->InstrumentName)), _multiple)));
                }
            }
            if (bIsLast) {
                __info << "query all future contracts ok: contracts = " << all_instruments_.size();
                state_ = kStartupStepGetInitPositionsOver;
            }
        } else {
            __error << "query all future contracts failed: " << CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
        }
    }

    void CTPBroker::OnRspQryTradingAccount(CThostFtdcTradingAccountField* pTradingAccount, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pTradingAccount) {
            std::unique_lock<std::mutex> lock(write_mutex_);
            void* buffer = writer_->GetFrame();
            int64_t nano = getNanoTime();
            Frame frame(buffer);
            frame.setNano(nano);

            yijinjing::TradeAsset* asset = (yijinjing::TradeAsset*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
            strcpy(asset->fund_id, fund_id_.c_str());
            asset->timestamp = x::RawDateTime();
            asset->trade_type = kTradeTypeFuture;
            asset->balance = 0;
            asset->usable = pTradingAccount->Available;
            asset->margin = pTradingAccount->CurrMargin;  // 保证金 = 占用保证金 + 冻结保证金//
            asset->equity = ctp_equity(pTradingAccount);
            asset->islast = bIsLast;
            writer_->passFrame(frame, sizeof(yijinjing::TradeAsset), REP_QRY_ACCOUT, 0);
        } else {
            if (pRspInfo && pRspInfo->ErrorID != 0) {
                __error << "query asset faild, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << x::GBKToUTF8(pRspInfo->ErrorMsg);
            }
        }
    }

    void CTPBroker::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField* pInvestorPosition, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        try {
                if (pInvestorPosition) {
//                    __info << "OnRspQryInvestorPosition, InstrumentID: " << pInvestorPosition->InstrumentID
//                           << ", PosiDirection: " << pInvestorPosition->PosiDirection
//                           << ", YdPosition: " << pInvestorPosition->YdPosition
//                           << ", Position: " << pInvestorPosition->Position
//                           << ", LongFrozen: " << pInvestorPosition->LongFrozen
//                           << ", ShortFrozen: " << pInvestorPosition->ShortFrozen;
                    string ctp_code = pInvestorPosition->InstrumentID;
                    int64_t market = ctp_market2std(pInvestorPosition->ExchangeID);
                    if (market == co::kMarketCZCE) {
                        InsertCzceCode(ctp_code);
                    }
                    string suffix = GetMarketSuffix(market);
                    string code = ctp_code + suffix;
                    // int64_t hedge_flag = ctp_hedge_flag2std(pInvestorPosition->HedgeFlag);
                    int64_t bs_flag = ctp_ls_flag2std(pInvestorPosition->PosiDirection);
                    auto it = all_pos_.find(code);
                    if (it == all_pos_.end()) {
                        std::unique_ptr<co::fbs::TradePositionT> item = std::make_unique<co::fbs::TradePositionT>();
                        item->timestamp = 0;
                        item->trade_type = kTradeTypeFuture;
                        item->fund_id = investor_id_;
                        item->market = market;
                        item->code = code;
                        auto itor = all_instruments_.find(code);
                        if (itor != all_instruments_.end()) {
                            item->name = itor->second.first;
                        }
                        all_pos_.insert(std::make_pair(code, std::move(item)));
                        it = all_pos_.find(code);
                    }
                    // YdPositionYdPositionYdPositionYdPosition YdPosition 表示昨日收盘时持仓数量(静态数值, 日间不随着开平而变化)//
                    // 当前的昨持仓 = 当前持仓数量 - 今开仓数量//
                    if (bs_flag == kBsFlagBuy) {
                        it->second->long_pre_volume = it->second->long_pre_volume + pInvestorPosition->YdPosition;
                        it->second->long_volume = it->second->long_volume + pInvestorPosition->Position;
                    } else if (bs_flag == kBsFlagSell) {
                        it->second->short_pre_volume = it->second->short_pre_volume + pInvestorPosition->YdPosition;
                        it->second->short_volume = it->second->short_volume + pInvestorPosition->Position;
                    }
                } else {
                    if (pRspInfo && pRspInfo->ErrorID != 0) {
                        __error << "query position faild, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << x::GBKToUTF8(pRspInfo->ErrorMsg);
                    }
                }
            if (bIsLast) {
                for (auto it = all_pos_.begin(); it != all_pos_.end(); ++it) {
                    __info << it->second->code
                           << ", long_volume: " << it->second->long_volume
                           << ", long_pre_volume: " << it->second->long_pre_volume
                           << ", short_volume: " << it->second->short_volume
                           << ", short_pre_volume: " << it->second->short_pre_volume;
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    void* buffer = writer_->GetFrame();
                    int64_t nano = getNanoTime();
                    Frame frame(buffer);
                    frame.setNano(nano);

                    // 待细化
                    yijinjing::TradePosition* pos = (yijinjing::TradePosition*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
                    strcpy(pos->fund_id, fund_id_.c_str());
                    strcpy(pos->code, it->second->code.c_str());
                    pos->timestamp = x::RawDateTime();
                    pos->trade_type = kTradeTypeFuture;
                    pos->buy_volume = it->second->long_volume;
                    pos->sell_volume = it->second->short_volume;
                    // pos->islast = bIsLast;
                    writer_->passFrame(frame, sizeof(yijinjing::TradePosition), REP_QRY_POSITION, 0);
                }
            }
        } catch (std::exception& e) {
            __error << "OnRspQryInvestorPosition: " << e.what();
        }
    }

    /// 请求查询成交响应//
    void CTPBroker::OnRspQryTrade(CThostFtdcTradeField* pTrade, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        try {
            if (pTrade) {
                string order_sys_id = x::Trim(pTrade->OrderSysID);
                string order_no;
                map<string, string>::iterator itr_order_no = order_nos_.find(order_sys_id);
                if (itr_order_no != order_nos_.end()) {
                    order_no = itr_order_no->second;
                }
                if (!order_no.empty()) {
                    string ctp_code = pTrade->InstrumentID;
                    int64_t market = ctp_market2std(pTrade->ExchangeID);
                    if (market == co::kMarketCZCE) {
                        InsertCzceCode(ctp_code);
                    }
                    string suffix = GetMarketSuffix(market);
                    string code = ctp_code + suffix;
                    string match_no = x::Trim(pTrade->TradingDay) + "_" + x::Trim(pTrade->TradeID);
                    double match_amount = pTrade->Price * pTrade->Volume;
                    string name;
                    auto it = all_instruments_.find(code);
                    if (it != all_instruments_.end()) {
                        name = it->second.first;
                        match_amount = match_amount * it->second.second;
                    }
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    void* buffer = writer_->GetFrame();
                    int64_t nano = getNanoTime();
                    Frame frame(buffer);
                    frame.setNano(nano);

                    // 待细化
                    yijinjing::TradeKnock* knock = (yijinjing::TradeKnock*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
                    knock->trade_type = kTradeTypeFuture;
                    strcpy(knock->fund_id, fund_id_.c_str());
                    strcpy(knock->code, code.c_str());
                    strcpy(knock->name, name.c_str());
                    if (strcmp(pTrade->TradeTime, "06:00:00") > 0 && strcmp(pTrade->TradeTime, "18:00:00") <= 0) {
                        knock->timestamp = CtpTimestamp(atoll(pTrade->TradingDay), pTrade->TradeTime);
                    } else if (strcmp(pTrade->TradeTime, "18:00:00") > 0 && strcmp(pTrade->TradeTime, "23:59:59") <= 0) {
                        knock->timestamp = CtpTimestamp(pre_trading_day_, pTrade->TradeTime);
                    } else {
                        knock->timestamp = CtpTimestamp(pre_trading_day_next_, pTrade->TradeTime);
                    }
                    strcpy(knock->order_no, order_no.c_str());
                    strcpy(knock->match_no, match_no.c_str());
                    knock->market = market;
                    knock->bs_flag = ctp_bs_flag2std(pTrade->Direction);
                    knock->oc_flag = ctp_oc_flag2std(pTrade->OffsetFlag);
                    knock->match_type = kMatchTypeOK;
                    knock->match_volume = pTrade->Volume;
                    knock->match_price = pTrade->Price;
                    knock->match_amount = match_amount;
                    writer_->passFrame(frame, sizeof(yijinjing::TradeKnock), REP_QRY_KNOCK, 0);
                } else {
                    __warn << "ignore knock because no order_no found of order_sys_id: " << order_sys_id;
                }
                query_cursor_ = pTrade->TradeTime;
            } else {
                if (pRspInfo && pRspInfo->ErrorID != 0) {
                    __error << "query knock faild, ErrorID: " << pRspInfo->ErrorID << ", ErrorMsg: " << x::GBKToUTF8(pRspInfo->ErrorMsg);
                }
            }
        } catch (std::exception& e) {
            __error << "OnRspQryTrade: " << e.what();
        }
    }

    /// 报单录入请求响应(CTP打回的废单会通过该函数返回)
    void CTPBroker::OnRspOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        if (pInputOrder) {
            __info << __FUNCTION__ << ", InstrumentID: " << pInputOrder->InstrumentID
                   << ", OrderRef: " << pInputOrder->OrderRef
                   << ", ExchangeID: " << pInputOrder->ExchangeID
                   << ", Direction: " << pInputOrder->Direction
                   << ", CombOffsetFlag: " << pInputOrder->CombOffsetFlag
                   << ", VolumeTotalOriginal: " << pInputOrder->VolumeTotalOriginal
                   << ", nRequestID: " << nRequestID;
        }

        try {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = order_recode_.find(nRequestID);
            if (it != order_recode_.end()) {
                string error = CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                TradeOrderMessage& req = it->second;
                strcpy(req.error, error.c_str());
                {
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    writer_->write_frame(&req, sizeof(TradeOrderMessage), TRADE_ORDER_REP, 0);
                }
                order_recode_.erase(it);
            } else {
                __error << "OnRspOrderInsert, not find nRequestID: " << nRequestID;
            }
        } catch (std::exception& e) {
            __error << "OnOrderTicketError: " << e.what();
        }
    }

    // 交易所打回的废单会通过该函数返回//
    void CTPBroker::OnErrRtnOrderInsert(CThostFtdcInputOrderField* pInputOrder, CThostFtdcRspInfoField* pRspInfo) {
        if (pInputOrder) {
            __info << __FUNCTION__ << ", InstrumentID: " << pInputOrder->InstrumentID
                   << ", OrderRef: " << pInputOrder->OrderRef
                   << ", Direction: " << pInputOrder->Direction
                   << ", ExchangeID: " << pInputOrder->ExchangeID
                   << ", CombOffsetFlag: " << pInputOrder->CombOffsetFlag
                   << ", nRequestID: " << pInputOrder->RequestID;
        }

        try {
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = order_recode_.find(pInputOrder->RequestID);
            if (it != order_recode_.end()) {
                string error = CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                TradeOrderMessage& req = it->second;
                strcpy(req.error, error.c_str());
                {
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    writer_->write_frame(&req, sizeof(TradeOrderMessage), TRADE_ORDER_REP, 0);
                }
                order_recode_.erase(it);
            } else {
                __error << "OnErrRtnOrderInsert, not find nRequestID: " << pInputOrder->RequestID;
            }
        } catch (std::exception& e) {
            __error << "OnErrRtnOrderInsert: " << e.what();
        }
    }


    /// 报单操作请求响应//
    void CTPBroker::OnRspOrderAction(CThostFtdcInputOrderActionField* pInputOrderAction, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        try {
            __info << __FUNCTION__ << ", nRequestID: " << nRequestID << ", ErrorId: " << pRspInfo->ErrorID;
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = withdraw_recode_.find(nRequestID);
            if (it != withdraw_recode_.end()) {
                string error = CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                TradeWithdrawMessage& req = it->second;
                strcpy(req.error, error.c_str());
                {
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    writer_->write_frame(&req, sizeof(TradeWithdrawMessage), TRADE_WITHDRAW_REP, 0);
                }
                withdraw_recode_.erase(it);
            } else {
                __error << "OnRspOrderAction, not find nRequestID: " << nRequestID;
            }
        } catch (std::exception& e) {
            __error << "OnRspOrderAction: " << e.what();
        }
    }

    void CTPBroker::OnErrRtnOrderAction(CThostFtdcOrderActionField* pOrderAction, CThostFtdcRspInfoField* pRspInfo) {
        try {
            __info << __FUNCTION__ << ", nRequestID: " << pOrderAction->RequestID << ", ErrorId: " << pRspInfo->ErrorID;
            std::unique_lock<std::mutex> lock(mutex_);
            auto it = withdraw_recode_.find(pOrderAction->RequestID);
            if (it != withdraw_recode_.end()) {
                string error = CtpError(pRspInfo->ErrorID, pRspInfo->ErrorMsg);
                TradeWithdrawMessage& req = it->second;
                strcpy(req.error, error.c_str());
                {
                    std::unique_lock<std::mutex> lock(write_mutex_);
                    writer_->write_frame(&req, sizeof(TradeWithdrawMessage), TRADE_WITHDRAW_REP, 0);
                }
                withdraw_recode_.erase(it);
            } else {
                __error << "OnErrRtnOrderAction, not find nRequestID: " << pOrderAction->RequestID;
            }
        } catch (std::exception& e) {
            __error << "OnErrRtnOrderAction: " << e.what();
        }
    }

    void CTPBroker::OnRtnOrder(CThostFtdcOrderField* pOrder) {
        if (pOrder) {
            __info << "OnRtnOrder, InstrumentID: " << pOrder->InstrumentID
                   << ", ExchangeID: " << pOrder->ExchangeID
                   << ", FrontID: " << pOrder->FrontID
                   << ", SessionID: " << pOrder->SessionID
                   << ", OrderRef: " << pOrder->OrderRef
                   << ", Direction: " << pOrder->Direction
                   << ", CombOffsetFlag: " << pOrder->CombOffsetFlag
                   << ", RequestID: " << pOrder->RequestID
                   << ", OrderSysID: " << pOrder->OrderSysID
                   << ", OrderStatus: " << pOrder->OrderStatus
                   << ", OrderSubmitStatus: " << pOrder->OrderSubmitStatus
                   << ", TradingDay: " << pOrder->TradingDay
                   << ", GTDDate: " << pOrder->GTDDate
                   << ", InsertDate: " << pOrder->InsertDate
                   << ", InsertTime: " << pOrder->InsertTime
                   << ", CancelTime: " << pOrder->CancelTime
                   << ", ActiveTime: " << pOrder->ActiveTime
                   << ", SuspendTime: " << pOrder->SuspendTime
                   << ", UpdateTime: " << pOrder->UpdateTime
                   << ", LimitPrice: " << pOrder->LimitPrice
                   << ", VolumeTraded: " << pOrder->VolumeTraded
                   << ", VolumeTotal: " << pOrder->VolumeTotal;
        }

        try {
            // 委托合同号: <前置编号>_<会话编号>_<报单引用>_<代码>//
            string order_sys_id = x::Trim(pOrder->OrderSysID);
            int64_t order_ref = atoi(pOrder->OrderRef);
            string ctp_code = pOrder->InstrumentID;
            stringstream ss;
            ss << pOrder->FrontID << "_" << pOrder->SessionID << "_" << order_ref << "_" << ctp_code;
            string order_no = ss.str();
            if (!order_sys_id.empty()) {
                order_nos_[order_sys_id] = order_no;
            }
            int64_t order_state = ctp_order_state2std(pOrder->OrderStatus, pOrder->OrderSubmitStatus);
            __info << "order_no: " << order_no << ", order_state: " << order_state;
            if (order_state == kOrderPartlyCanceled || order_state == kOrderFullyCanceled) {
                std::unique_lock<std::mutex> lock(mutex_);
                auto itor = withdraw_msg_.find(order_no);
                if (itor != withdraw_msg_.end()) {
                    {
                        std::unique_lock<std::mutex> lock(write_mutex_);
                        writer_->write_frame(&itor->second, sizeof(TradeWithdrawMessage), TRADE_WITHDRAW_REP, 0);
                    }
                    withdraw_msg_.erase(itor);
                    __info << "SendTradeWithdrawRep.";
                }
            } else {
                int _RequestID = atol(x::Trim(pOrder->OrderRef).c_str());
                std::unique_lock<std::mutex> lock(mutex_);
                auto it = order_recode_.find(_RequestID);
                if (it != order_recode_.end()) {
                    TradeOrderMessage& req = it->second;
                    strcpy(req.item.order_no, order_no.c_str());
                    {
                        std::unique_lock<std::mutex> lock(write_mutex_);
                        writer_->write_frame(&req, sizeof(TradeOrderMessage), TRADE_ORDER_REP, 0);
                    }
                    order_recode_.erase(it);
                }
            }
            // -----------------------------------------------------
            int64_t market = ctp_market2std(pOrder->ExchangeID);
            if (market == co::kMarketCZCE) {
                InsertCzceCode(ctp_code);
            }
            string suffix = GetMarketSuffix(market);
            string code = ctp_code + suffix;
            if (order_state == kOrderPartlyCanceled || order_state == kOrderFullyCanceled || order_state == kOrderFailed) {
                std::unique_lock<std::mutex> lock(write_mutex_);
                void* buffer = writer_->GetFrame();
                int64_t nano = getNanoTime();
                Frame frame(buffer);
                frame.setNano(nano);

                yijinjing::TradeKnock* knock = (yijinjing::TradeKnock*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
                strcpy(knock->fund_id, fund_id_.c_str());
                knock->trade_type = kTradeTypeFuture;
                // 废单只有InsertTime
                if (order_state == kOrderFailed) {
                    knock->timestamp = x::RawDateTime();
                } else {
                    if (strcmp(pOrder->CancelTime, "06:00:00") > 0 && strcmp(pOrder->CancelTime, "18:00:00") <= 0) {
                        knock->timestamp = CtpTimestamp(date_, pOrder->CancelTime);
                    } else if (strcmp(pOrder->CancelTime, "18:00:00") > 0 && strcmp(pOrder->CancelTime, "23:59:59") <= 0) {
                        knock->timestamp = CtpTimestamp(pre_trading_day_, pOrder->CancelTime);
                    } else {
                        knock->timestamp = CtpTimestamp(pre_trading_day_next_, pOrder->CancelTime);
                    }
                }
                strcpy(knock->fund_id, investor_id_.c_str());
                strcpy(knock->code, code.c_str());
                strcpy(knock->fund_id, investor_id_.c_str());
                string match_no = "_" + order_no;
                strcpy(knock->order_no, order_no.c_str());
                strcpy(knock->match_no, match_no.c_str());
                knock->market = market;
                auto it = all_instruments_.find(code);
                if (it != all_instruments_.end()) {
                    strcpy(knock->name, it->second.first.c_str());
                }
                knock->bs_flag = ctp_bs_flag2std(pOrder->Direction);
                knock->oc_flag = ctp_oc_flag2std(pOrder->CombOffsetFlag[0]);
                if (order_state == kOrderPartlyCanceled || order_state == kOrderFullyCanceled) {
                    knock->match_type = kMatchTypeWithdrawOK;
                    knock->match_volume = pOrder->VolumeTotalOriginal - pOrder->VolumeTraded;
                } else {
                    knock->match_type = kMatchTypeFailed;
                    knock->match_volume = pOrder->VolumeTotalOriginal;
                    if (order_state == kOrderFailed) {
                        string error = x::GBKToUTF8(x::Trim(pOrder->StatusMsg));
                        // strcpy(knock->error, error);
                    }
                }
                knock->match_price = 0;
                knock->match_amount = 0;
                writer_->passFrame(frame, sizeof(yijinjing::TradeKnock), TRADE_KNOCK, 0);
            }
        } catch (std::exception& e) {
            __error << "OnRtnOrder: " << e.what();
        }
    }


    /// 成交通知(测试发现, 委托状态更新比成交数据更快)
    void CTPBroker::OnRtnTrade(CThostFtdcTradeField* pTrade) {
        if (pTrade) {
            __info << "OnRtnTrade, InstrumentID: " << pTrade->InstrumentID
                   << ", OrderRef: " << pTrade->OrderRef
                   << ", Direction: " << pTrade->Direction
                   << ", CombOffsetFlag: " << pTrade->OffsetFlag
                   << ", TradeID: " << pTrade->TradeID
                   << ", OrderSysID: " << pTrade->OrderSysID
                   << ", Price: " << pTrade->Price
                   << ", Volume: " << pTrade->Volume
                   << ", TradeTime: " << pTrade->TradeTime
                   << ", TradeDate: " << pTrade->TradeDate
                   << ", TradingDay: " << pTrade->TradingDay;
        }

        try {
            // CTP推过来的成交数据中没有FrontId和SessionId, 无法生成委托合同号, 需要根据OrderSysId从映射表中查找//
            string order_sys_id = x::Trim(pTrade->OrderSysID);
            string match_no = x::Trim(pTrade->TradeID);
            map<string, string>::iterator itr_order_no = order_nos_.find(order_sys_id);
            if (itr_order_no != order_nos_.end()) {
                string order_no = itr_order_no->second;
                string ctp_code = pTrade->InstrumentID;
                int64_t market = ctp_market2std(pTrade->ExchangeID);
                if (market == co::kMarketCZCE) {
                    InsertCzceCode(ctp_code);
                }
                string suffix = GetMarketSuffix(market);
                string code = ctp_code + suffix;
                string match_no = x::Trim(pTrade->TradingDay) + "_" + x::Trim(pTrade->TradeID);
                double match_amount = pTrade->Price * pTrade->Volume;
                string name;
                auto it = all_instruments_.find(code);
                if (it != all_instruments_.end()) {
                    name = it->second.first;
                    match_amount = match_amount * it->second.second;
                }
                std::unique_lock<std::mutex> lock(write_mutex_);
                void* buffer = writer_->GetFrame();
                int64_t nano = getNanoTime();
                Frame frame(buffer);
                frame.setNano(nano);

                yijinjing::TradeKnock* knock = (yijinjing::TradeKnock*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
                strcpy(knock->fund_id, fund_id_.c_str());
                if (strcmp(pTrade->TradeTime, "06:00:00") > 0 && strcmp(pTrade->TradeTime, "18:00:00") <= 0) {
                    knock->timestamp = CtpTimestamp(date_, pTrade->TradeTime);
                } else if (strcmp(pTrade->TradeTime, "18:00:00") > 0 && strcmp(pTrade->TradeTime, "23:59:59") <= 0) {
                    knock->timestamp = CtpTimestamp(pre_trading_day_, pTrade->TradeTime);
                } else {
                    // 00:00:01 到 06:00:00
                    knock->timestamp = CtpTimestamp(pre_trading_day_next_, pTrade->TradeTime);
                }
                knock->trade_type = kTradeTypeFuture;
                knock->market = market;
                strcpy(knock->code, code.c_str());
                strcpy(knock->name, name.c_str());
                strcpy(knock->order_no, order_no.c_str());
                strcpy(knock->match_no, match_no.c_str());
                knock->bs_flag = ctp_bs_flag2std(pTrade->Direction);
                knock->oc_flag = ctp_oc_flag2std(pTrade->OffsetFlag);
                knock->match_type = kMatchTypeOK;
                knock->match_volume = pTrade->Volume;
                knock->match_price = pTrade->Price;
                knock->match_amount = match_amount;
                writer_->passFrame(frame, sizeof(yijinjing::TradeKnock), TRADE_KNOCK, 0);
            } else {
                __warn << "no order_no found of knock: order_sys_id = " << order_sys_id << ", match_no = " << match_no;
            }
        } catch (std::exception& e) {
            __error << "recv knock failed: " << e.what();
        }
    }

    /// 错误应答
    void CTPBroker::OnRspError(CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast) {
        __error << "OnRspError: ret=" << pRspInfo->ErrorID << ", msg=" << CtpToUTF8(pRspInfo->ErrorMsg);
        if (pRspInfo->ErrorID == 90) {  // <error id="NEED_RETRY" value="90" prompt="CTP：查询未就绪，请稍后重试" />//
            x::Sleep(CTP_FLOW_CONTROL_MS);
        }
        if (state_ == kStartupStepInit) {
            Start();
        } else if (state_ == kStartupStepLoginOver) {
            ReqSettlementInfoConfirm();
        } else if (state_ == kStartupStepConfirmSettlementOver) {
            ReqQryInstrument();
        } else if (state_ == kStartupStepGetContractsOver) {
            // ReqQryInvestorPosition();
        } else if (state_ == kStartupStepGetInitPositionsOver) { // 已启动完成之后，返回异步响应

        }
    }
}
