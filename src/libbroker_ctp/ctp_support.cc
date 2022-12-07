#include <string>

#include "ctp_support.h"
#include "broker/broker.h"
#include "boost/algorithm/string.hpp"

namespace co {

    // 流控判断
    bool is_flow_control(int ret_code) {
        return ((ret_code == -2) || (ret_code == -3));
    }

    string CtpToUTF8(const char* str) {
        string text;
        if (str) {
            try {
                std::string s = str;
                text = x::GBKToUTF8(s);
            } catch (std::exception & e) {
                text = str;
            }
        }
        return text;
    }

    string CtpApiError(int rc) {
        stringstream ss;
        string ret;
        switch (rc) {
        case 0:
            ret = "0-ok";
            break;
        case -1:
            ret = "-1-network error";
            break;
        case -2:
        case -3:
            ss << rc << "-flow control error";
            ret = ss.str();
            break;
        default:
            ss << rc << "-unknown error";
            ret = ss.str();
            break;
        }
        return ret;
    }

    string CtpError(int code, const char* msg) {
        stringstream ss;
        ss << code << "-" << CtpToUTF8(msg);
        return ss.str();
    }

    string CtpError(TThostFtdcErrorIDType code, TThostFtdcErrorMsgType msg) {
        stringstream ss;
        ss << code << "-" << CtpToUTF8(msg);
        return ss.str();
    }

    int64_t CtpTimestamp(int64_t date, TThostFtdcTimeType time) {
        string s_time = time;  // 15:00:01
        boost::algorithm::replace_all(s_time, ":", "");
        int64_t i_time = atoll(s_time.c_str()) * 1000;
        int64_t Timestamp = date * 1000000000LL + i_time % 1000000000LL;
        if (date < 19700101 || date > 30000101 || i_time < 0 || i_time > 235959999) {
            xthrow() << "illegal ctp Timestamp: date = " << date << ", time = " << time;
        }
        return Timestamp;
    }

    int64_t ctp_time2std(TThostFtdcTimeType v) {
        string s = v;  // 15:00:01
        boost::algorithm::replace_all(s, ":", "");
        int64_t stamp = atoi(s.c_str()) * 1000;
        return stamp;
    }

    int64_t ctp_market2std(TThostFtdcExchangeIDType v) {
        int64_t std_market = 0;
        string s = "." + string(v);
        if (s == co::kSuffixCFFEX) {
            std_market = co::kMarketCFFEX;
        } else if (s == co::kSuffixSHFE) {
            std_market = co::kMarketSHFE;
        } else if (s == co::kSuffixDCE) {
            std_market = co::kMarketDCE;
        } else if (s == co::kSuffixCZCE) {
            std_market = co::kMarketCZCE;
        } else if (s == co::kSuffixINE) {
            std_market = co::kMarketINE;
        } else {
            __error << "unknown ctp_market: " << v;
        }
        return std_market;
    }

    string market2ctp(int64_t v) {
        string ret;
        switch (v) {
        case co::kMarketCFFEX:
            ret = co::kSuffixCFFEX.substr(1);
            break;
        case co::kMarketSHFE:
            ret = co::kSuffixSHFE.substr(1);
            break;
        case co::kMarketDCE:
            ret = co::kSuffixDCE.substr(1);
            break;
        case co::kMarketCZCE:
            ret = co::kSuffixCZCE.substr(1);
            break;
        case co::kMarketINE:
            ret = co::kSuffixINE.substr(1);
            break;
        default:
            xthrow() << "unknown market: " << v;
            break;
        }
        return ret;
    }

    int64_t ctp_bs_flag2std(TThostFtdcDirectionType v) {
        int64_t std_bs_flag = 0;
        switch (v) {
        case THOST_FTDC_D_Buy:  // 买
            std_bs_flag = kBsFlagBuy;
            break;
        case THOST_FTDC_D_Sell:  // 卖
            std_bs_flag = kBsFlagSell;
            break;
        default:
            xthrow() << "unknown ctp_bs_flag: " << v;
            break;
        }
        return std_bs_flag;
    }

    TThostFtdcDirectionType bs_flag2ctp(int64_t v) {
        TThostFtdcDirectionType ctp_bs_flag = '\0';
        switch (v) {
        case kBsFlagBuy:
            ctp_bs_flag = THOST_FTDC_D_Buy;
            break;
        case kBsFlagSell:
            ctp_bs_flag = THOST_FTDC_D_Sell;
            break;
        default:
            xthrow() << "unknown bs_flag: " << v;
            break;
        }
        return ctp_bs_flag;
    }

    int64_t ctp_ls_flag2std(TThostFtdcPosiDirectionType v) {
        int64_t std_bs_flag = 0;
        switch (v) {
            // case THOST_FTDC_PD_Net:  // 净
            //    break;
        case THOST_FTDC_PD_Long:  // 多头
            std_bs_flag = kBsFlagBuy;
            break;
        case THOST_FTDC_PD_Short:  // 空头
            std_bs_flag = kBsFlagSell;
            break;
        default:
            xthrow() << "unknown ctp_ls_flag: " << v;
            break;
        }
        return std_bs_flag;
    }

    int64_t ctp_hedge_flag2std(TThostFtdcHedgeFlagType v) {
        int64_t std_hedge_flag = 0;
        switch (v) {
        case THOST_FTDC_CIDT_Speculation:
            std_hedge_flag = kHedgeFlagSpeculate;
            break;
        case THOST_FTDC_CIDT_Arbitrage:
            std_hedge_flag = kHedgeFlagArbitrage;
            break;
        case THOST_FTDC_CIDT_Hedge:
            std_hedge_flag = kHedgeFlagHedge;
            break;
        default:
            xthrow() << "unknown ctp_hedge_flag: " << v;
            break;
        }
        return std_hedge_flag;
    }

    TThostFtdcHedgeFlagType hedge_flag2ctp(int64_t v) {
        TThostFtdcHedgeFlagType ctp_hedge_flag = 0;
        switch (v) {
        case kHedgeFlagSpeculate:
            ctp_hedge_flag = THOST_FTDC_CIDT_Speculation;
            break;
        case kHedgeFlagArbitrage:
            ctp_hedge_flag = THOST_FTDC_CIDT_Arbitrage;
            break;
        case kHedgeFlagHedge:
            ctp_hedge_flag = THOST_FTDC_CIDT_Hedge;
            break;
        default:
            xthrow() << "unknown hedge_flag: " << v;
            break;
        }
        return ctp_hedge_flag;
    }

    int64_t ctp_oc_flag2std(TThostFtdcOffsetFlagType v) {
        int32_t std_oc_flag = 0;
        switch (v) {
        case THOST_FTDC_OF_Open:  /// 开仓
            std_oc_flag = kOcFlagOpen;
            break;
        case THOST_FTDC_OF_Close:  /// 平仓
            std_oc_flag = kOcFlagClose;
            break;
        case THOST_FTDC_OF_ForceClose:  /// 强平
            std_oc_flag = kOcFlagForceClose;
            break;
        case THOST_FTDC_OF_CloseToday:  /// 平今
            std_oc_flag = kOcFlagCloseToday;
            break;
        case THOST_FTDC_OF_CloseYesterday:  /// 平昨
            std_oc_flag = kOcFlagCloseYesterday;
            break;
        case THOST_FTDC_OF_ForceOff:  /// 强减
            std_oc_flag = kOcFlagForceOff;
            break;
        case THOST_FTDC_OF_LocalForceClose:  /// 本地强平
            std_oc_flag = kOcFlagLocalForceClose;
            break;
        default:
            xthrow() << "unknown ctp_oc_flag: " << v;
            break;
        }
        return std_oc_flag;
    }

    TThostFtdcOffsetFlagType oc_flag2ctp(int64_t v) {
        TThostFtdcOffsetFlagType ctp_oc_flag = '\0';
        switch (v) {
        case kOcFlagOpen:  /// 开仓
            ctp_oc_flag = THOST_FTDC_OF_Open;
            break;
        case kOcFlagClose:  /// 平仓
            ctp_oc_flag = THOST_FTDC_OF_Close;
            break;
        case kOcFlagForceClose:  /// 强平
            ctp_oc_flag = THOST_FTDC_OF_ForceClose;
            break;
        case kOcFlagCloseToday :  /// 平今
            ctp_oc_flag = THOST_FTDC_OF_CloseToday;
            break;
        case kOcFlagCloseYesterday:  /// 平昨
            ctp_oc_flag = THOST_FTDC_OF_CloseYesterday;
            break;
        case kOcFlagForceOff:  /// 强减
            ctp_oc_flag = THOST_FTDC_OF_ForceOff;
            break;
        case kOcFlagLocalForceClose:  /// 本地强平
            ctp_oc_flag = THOST_FTDC_OF_LocalForceClose;
            break;
        default:
            xthrow() << "unknown oc_flag: " << v;
            break;
        }
        return ctp_oc_flag;
    }

    int64_t ctp_order_state2std(TThostFtdcOrderStatusType order_state, TThostFtdcOrderSubmitStatusType submit_state) {
        // 委托状态: 0: 未报, 1: 待报, (2: 已报), 3: 已报待撤, 4: 部成待撤, (5: 部撤), (6: 已撤), (7: 部成), (8: 已成), (9: 废单)
        int64_t std_order_state = 0;
        switch (order_state) {
        case THOST_FTDC_OST_AllTraded:  /// 全部成交
            std_order_state = kOrderFullyKnocked;
            break;
        case THOST_FTDC_OST_PartTradedQueueing:  /// 部分成交还在队列中
            std_order_state = kOrderPartlyKnocked;
            break;
        case THOST_FTDC_OST_PartTradedNotQueueing:  /// 部分成交不在队列中, 即“部成部撤”
            std_order_state = kOrderPartlyCanceled;
            break;
        case THOST_FTDC_OST_NoTradeQueueing:  /// 未成交还在队列中
            std_order_state = kOrderCreated;
            break;
        case THOST_FTDC_OST_NoTradeNotQueueing:  /// 未成交不在队列中, 非法申报
            std_order_state = kOrderFailed;
            break;
        case THOST_FTDC_OST_Canceled:  /// 撤单
            if (submit_state == THOST_FTDC_OSS_InsertRejected || submit_state == THOST_FTDC_OSS_ModifyRejected) {  // 报单已经被拒绝, 改单已经被拒绝
                std_order_state = kOrderFailed;
            } else {
                std_order_state = kOrderFullyCanceled;
            }
            break;
        case THOST_FTDC_OST_Unknown:  /// 未知
            std_order_state = kOrderCreated;
            break;
        case THOST_FTDC_OST_NotTouched:  /// 尚未触发
            std_order_state = kOrderCreated;
            break;
        case THOST_FTDC_OST_Touched:  /// 已触发
            std_order_state = kOrderCreated;
            break;
        default:
            xthrow() << "unknown ctp_order_state: " << order_state;
            break;
        }
        return std_order_state;
    }

    TThostFtdcOrderPriceTypeType order_price_type2ctp(int64_t v) {
        TThostFtdcOrderPriceTypeType ret = '\0';
        switch (v) {
        case 0:
            ret = THOST_FTDC_OPT_LimitPrice;  /// 默认: 限价
            break;
        case 1:
            ret = THOST_FTDC_OPT_AnyPrice;  /// 任意价
            break;
        case 2:
            ret = THOST_FTDC_OPT_LimitPrice;  /// 限价
            break;
        case 3:
            ret = THOST_FTDC_OPT_BestPrice;  /// 最优价
            break;
        case 4:
            ret = THOST_FTDC_OPT_LastPrice;  /// 最新价
            break;
        case 5:
            ret = THOST_FTDC_OPT_LastPricePlusOneTicks;  /// 最新价浮动上浮1个ticks
            break;
        case 6:
            ret = THOST_FTDC_OPT_LastPricePlusTwoTicks;  /// 最新价浮动上浮2个ticks
            break;
        case 7:
            ret = THOST_FTDC_OPT_LastPricePlusThreeTicks;  /// 最新价浮动上浮3个ticks
            break;
        case 8:
            ret = THOST_FTDC_OPT_AskPrice1;  /// 卖一价
            break;
        case 9:
            ret = THOST_FTDC_OPT_AskPrice1PlusOneTicks;  /// 卖一价浮动上浮1个ticks
            break;
        case 10:
            ret = THOST_FTDC_OPT_AskPrice1PlusTwoTicks;  /// 卖一价浮动上浮2个ticks
            break;
        case 11:
            ret = THOST_FTDC_OPT_AskPrice1PlusThreeTicks;  /// 卖一价浮动上浮3个ticks
            break;
        case 12:
            ret = THOST_FTDC_OPT_BidPrice1;  /// 买一价
            break;
        case 13:
            ret = THOST_FTDC_OPT_BidPrice1PlusOneTicks;  /// 买一价浮动上浮1个ticks
            break;
        case 14:
            ret = THOST_FTDC_OPT_BidPrice1PlusTwoTicks;  /// 买一价浮动上浮2个ticks
            break;
        case 15:
            ret = THOST_FTDC_OPT_BidPrice1PlusThreeTicks;  /// 买一价浮动上浮3个ticks
            break;
        case 16:
            ret = THOST_FTDC_OPT_FiveLevelPrice;  // 五档价
            break;
        default:
            xthrow() << "unknown order_price_type: " << v;
            break;
        }
        return ret;
    }

    int64_t ctp_order_price_type2std(TThostFtdcOrderPriceTypeType v) {
        int64_t ret = 0;
        switch (v) {
        case THOST_FTDC_OPT_AnyPrice:
            ret = 1;  /// 任意价
            break;
        case THOST_FTDC_OPT_LimitPrice:
            ret = 2;  /// 限价
            break;
        case THOST_FTDC_OPT_BestPrice:
            ret = 3;  /// 最优价
            break;
        case THOST_FTDC_OPT_LastPrice:
            ret = 4;  /// 最新价
            break;
        case THOST_FTDC_OPT_LastPricePlusOneTicks:
            ret = 5;  /// 最新价浮动上浮1个ticks
            break;
        case THOST_FTDC_OPT_LastPricePlusTwoTicks:
            ret = 6;  /// 最新价浮动上浮2个ticks
            break;
        case THOST_FTDC_OPT_LastPricePlusThreeTicks:
            ret = 7;  /// 最新价浮动上浮3个ticks
            break;
        case THOST_FTDC_OPT_AskPrice1:
            ret = 8;  /// 卖一价
            break;
        case THOST_FTDC_OPT_AskPrice1PlusOneTicks:
            ret = 9;  /// 卖一价浮动上浮1个ticks
            break;
        case THOST_FTDC_OPT_AskPrice1PlusTwoTicks:
            ret = 10;  /// 卖一价浮动上浮2个ticks
            break;
        case THOST_FTDC_OPT_AskPrice1PlusThreeTicks:
            ret = 11;  /// 卖一价浮动上浮3个ticks
            break;
        case THOST_FTDC_OPT_BidPrice1:
            ret = 12;  /// 买一价
            break;
        case THOST_FTDC_OPT_BidPrice1PlusOneTicks:
            ret = 13;  /// 买一价浮动上浮1个ticks
            break;
        case THOST_FTDC_OPT_BidPrice1PlusTwoTicks:
            ret = 14;  /// 买一价浮动上浮2个ticks
            break;
        case THOST_FTDC_OPT_BidPrice1PlusThreeTicks:
            ret = 15;  /// 买一价浮动上浮3个ticks
            break;
        case THOST_FTDC_OPT_FiveLevelPrice:
            ret = 16;  // 五档价
            break;
        default:
            xthrow() << "unknown order_price_type: " << v;
            break;
        }
        return ret;
    }

    TThostFtdcTimeConditionType order_time_condition2ctp(string v) {
        TThostFtdcTimeConditionType ret;
        if (v.empty() || v == "General_Order") {  // 当日有效
            ret = THOST_FTDC_TC_GFD;
        } else if (v == "Automatically_Withdraw") {
            ret = THOST_FTDC_TC_IOC;
        } else {
            xthrow() << "unknown order_time_condition: " << v;
        }
        return ret;
    }

    double ctp_equity(CThostFtdcTradingAccountField *p) {
        /*
        ==========================================
        上次结算准备金: 　          1006015.23
        - 上次信用额度: 　　                0.00
        - 上次质押金额: 　　                0.00
        + 质押金额: 　　　　                0.00
        - 今日出金: 　　　　                0.00
        + 今日入金: 　　　　                0.00
        ------------------------------------------
        = 静态权益: 　　　　          1006015.23
        + 平仓盈亏: 　　　　                0.00
        + 持仓盈亏: 　　　　                0.00
        + 权利金: 　　　　　                0.00
        - 手续费: 　　　　　                0.00
        ------------------------------------------
        = 动态权益: 　　　　          1006015.23；
        - 占用保证金: 　　　                0.00
        - 冻结保证金: 　　　           432128.00
        - 冻结手续费: 　　　               27.01
        - 交割保证金: 　　　                0.00
        - 冻结权利金: 　　　                0.00
        + 信用金额: 　　　　                0.00
        ------------------------------------------
        = 可用资金: 　　　　           573860.22
        ==========================================

        ==========================================
        保底资金: 　　　　                0.00
        可取资金: 　　　　           401702.16
        ==========================================
        */
        double static_equity = p->PreBalance
            - p->PreCredit
            - p->PreMortgage
            + p->Mortgage
            - p->Withdraw
            + p->Deposit;
        double equity = static_equity
            + p->CloseProfit
            + p->PositionProfit
            - p->Commission;
        return equity;
    }

    // SR2109 SR109, 只转SR2109
    void DeleteCzceCode(string& code) {
        int _num = 0;
        int index = -1;
        for (int _i = 0; _i < code.length(); _i++) {
            char temp = code.at(_i);
            if (temp >= '0' && temp <= '9') {
                _num++;
                if (index < 0) {
                    index = _i;
                }
            }
        }
        if (_num == 4) {
            code.erase(index, 1);
        }
    }

    void InsertCzceCode(string& code) {
        int _num = 0;
        int index = -1;
        for (int _i = 0; _i < code.length(); _i++) {
            char temp = code.at(_i);
            if (temp >= '0' && temp <= '9') {
                _num++;
                if (index < 0) {
                    index = _i;
                }
            }
        }
        if (_num == 3) {
            code.insert(index, "2");
        }
    }

    bool IsMonday(int64_t date) {
        int years = date / 10000;
        int left = date % 10000;
        int months = left / 100;
        int days = left % 100;
        int week_day = -1;
        if (1 == months || 2 == months) {
            months += 12;
            years--;
        }
        week_day = (days + 1 + 2 * months + 3 * (months + 1) / 5 + years + years / 4 - years / 100 + years / 400) % 7;
        if (week_day == 1) {
            return true;
        } else {
            return false;
        }
    }

}  // namespace co
