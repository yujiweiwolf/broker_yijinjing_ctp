#include <regex>
#include "../libbroker_ctp/libbroker_ctp.h"
using namespace co;
using namespace std;

string fund_id = "8100429";
// 测试环境中, 价格不正常, 返回废单; 数量不正确，返回报单出错

void order_sh(std::shared_ptr<co::Broker> broker) {
    co::fbs::TradeOrderMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.trade_type = co::kTradeTypeSpot;
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    {
        std::unique_ptr<co::fbs::TradeOrderT> _order = std::make_unique<co::fbs::TradeOrderT>();;
        _order->id = x::UUID();
        _order->timestamp = x::RawDateTime();
        _order->fund_id = fund_id;

        _order->market = kMarketCFFEX;
        _order->code = "IF2209.CFFEX";
        _order->bs_flag = kBsFlagBuy;
        // _order->oc_flag = kOcFlagOpen;
        _order->price = 4067;
        _order->volume = 1;


        /*_order->market = kMarketCZCE;
        _order->code = "TA2209.CZCE";
        _order->bs_flag = kBsFlagSell;
        _order->price = 6666;
        _order->volume = 1;*/

        //// 4673
        //_order->market = kMarketSHFE;
        //_order->code = "rb2209.SHFE";
        //_order->bs_flag = kBsFlagSell;
        //// _order->oc_flag = kOcFlagCloseYesterday;
        //_order->price = 3800;
        //_order->volume = 2;

        /*_order->market = kMarketDCE;
        _order->code = "i2209.DCE";
        _order->bs_flag = kBsFlagBuy;
        _order->price = 743;
        _order->volume = 2;*/

        /*_order->market = kMarketINE;
        _order->code = "sc2206.INE";
        _order->bs_flag = kBsFlagBuy;
        _order->price = 717.9 + 2;
        _order->volume = 2;*/

        _order->price_type = kQOrderTypeLimit;
        msg.items.emplace_back(std::move(_order));
    }
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::TradeOrderMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->SendTradeOrder(raw);
    //broker->SendTradeOrder(raw);
}

void order_sz(std::shared_ptr<co::Broker> broker) {
    co::fbs::TradeOrderMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.trade_type = co::kTradeTypeSpot;
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    {
        std::unique_ptr<co::fbs::TradeOrderT> _order = std::make_unique<co::fbs::TradeOrderT>();;
        _order->id = x::UUID();
        _order->timestamp = x::RawDateTime();
        _order->fund_id = fund_id;

        /*_order->market = kMarketDCE;
        _order->code = "i2109.DCE";
        _order->price = 1195;*/

        //_order->market = kMarketCZCE;
        //_order->code = "SR2109.CZCE";
        //_order->price = 5620.8;

        /*_order->market = kMarketCFFEX;
        _order->code = "IF2205.CFFEX";
        _order->price = 3900;*/

        _order->market = kMarketSHFE;
        _order->code = "rb2205.SHFE";
        _order->price = 5000;

        _order->bs_flag = kBsFlagSell;
        _order->volume = 1;

        _order->price_type = kQOrderTypeLimit;
        msg.items.emplace_back(std::move(_order));
    }
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::TradeOrderMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->SendTradeOrder(raw);
    // broker->SendTradeOrder(raw);
}

void withdraw_sh(std::shared_ptr<co::Broker> broker) {
    co::fbs::TradeWithdrawMessageT msg;
    msg.id = x::UUID();
    msg.trade_type = co::kTradeTypeSpot;
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    char temp[64] = "";;
    printf("please input order_no\n");
    scanf("%s", temp);
    string _order_no = temp;
    msg.order_no = temp;
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::TradeWithdrawMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->SendTradeWithdraw(raw);
}

// BUY IC2212.CFFEX 1 5606.6
// BUY y2211.DCE 1 10770
// SELL sc2212.INE 1 674.0
// BUY MA2211.CZCE 1 2778
// SELL ag2212.SHFE 1 4466
void order(std::shared_ptr<co::Broker> broker) {
    co::fbs::TradeOrderMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.trade_type = co::kTradeTypeSpot;
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    {
        std::unique_ptr<co::fbs::TradeOrderT> _order = std::make_unique<co::fbs::TradeOrderT>();;
        _order->id = x::UUID();
        _order->fund_id = fund_id;
        getchar();
        cout << "please input : BUY 600000.SH 100 9.9\n";
        std::string input;
        getline(std::cin, input);
        std::cout << "your input is: # " << input << " #" << std::endl;
        std::smatch result;
        if (regex_match(input, result, std::regex("^(BUY|SELL) ([a-zA-Z]{1,2})([0-9]{4})\.([A-Z]{1,5}) ([0-9]{1,10}) ([.0-9]{1,10})$")))
        {
            string command = result[1].str();
            string instrument = result[2].str() + result[3].str();
            string market = result[4].str();
            string volume = result[5].str();
            string price = result[6].str();
            if (command == "BUY") {
                _order->bs_flag = kBsFlagBuy;
            } else if (command == "SELL") {
                _order->bs_flag = kBsFlagSell;
            }

            if (market == "SH") {
                _order->market = kMarketSH;
                _order->code = instrument + ".SH";
            } else if (market == "SZ") {
                _order->market = kMarketSZ;
                _order->code = instrument + ".SZ";
            } else if (market == "CFFEX") {
                _order->market = kMarketCFFEX;
                _order->code = instrument + ".CFFEX";
            } else if (market == "SHFE") {
                _order->market = kMarketSHFE;
                _order->code = instrument + ".SHFE";
            } else if (market == "DCE") {
                _order->market = kMarketDCE;
                _order->code = instrument + ".DCE";
            } else if (market == "CZCE") {
                _order->market = kMarketCZCE;
                _order->code = instrument + ".CZCE";
            } else if (market == "INE") {
                _order->market = kMarketINE;
                _order->code = instrument + ".INE";
            }
            _order->volume = atoll(volume.c_str());
            _order->price = atof(price.c_str());
        }
        _order->price_type = kQOrderTypeLimit;
        _order->timestamp = x::RawDateTime();
        msg.items.emplace_back(std::move(_order));
    }
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::TradeOrderMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->SendTradeOrder(raw);
}

void query_asset(std::shared_ptr<co::Broker> broker) {
    co::fbs::GetTradeAssetMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::GetTradeAssetMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->QueryTradeAsset(raw);
}

void query_position(std::shared_ptr<co::Broker> broker) {
    co::fbs::GetTradePositionMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::GetTradePositionMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->QueryTradePosition(raw);
}

void query_order(std::shared_ptr<co::Broker> broker) {
}

void query_knock(std::shared_ptr<co::Broker> broker) {
    co::fbs::GetTradeKnockMessageT msg;
    msg.items.clear();
    msg.id = x::UUID();
    msg.fund_id = fund_id;
    msg.timestamp = x::RawDateTime();
    //char temp[64] = "";;
    //printf("please input cursor\n");
    //scanf_s("%s", temp, 50);
    // msg.cursor = "112636379.0000000058090295018e";
    flatbuffers::FlatBufferBuilder fbb;
    fbb.Finish(co::fbs::GetTradeKnockMessage::Pack(fbb, &msg));
    std::string raw((const char*)fbb.GetBufferPointer(), fbb.GetSize());
    broker->QueryTradeKnock(raw);
}

int main() {
    BrokerOptionsPtr options = Config::Instance()->options();
    shared_ptr<CTPBroker> broker = make_shared<CTPBroker>();
    co::BrokerServer server;
    server.Init(options, broker);
    server.Start();

    string usage("\nTYPE  'q' to quit program\n");
    usage += "      '1' to order_shfe\n";
    usage += "      '2' to order_sz\n";
    usage += "      '3' to withdraw_shfe\n";
    usage += "      '4' to order\n";
    usage += "      '5' to query asset\n";
    usage += "      '6' to query position\n";
    usage += "      '7' to query order\n";
    usage += "      '8' to query knock\n";
    cerr << (usage);

    char c;
    while ((c = getchar()) != 'q') {
        switch (c) {
        case '1': {
            order_sh(broker);
            break;
        }
        case '2': {
            order_sz(broker);
            break;
        }
        case '3': {
            withdraw_sh(broker);
            break;
        }
        case '4': {
            order(broker);
            break;
        }
        case '5': {
            query_asset(broker);
            break;
        }
        case '6': {
            query_position(broker);
            break;
        }
        case '8': {
            query_knock(broker);
            break;
        }
        default:
            break;
        }
    }
    std::cout << "Finished!" << endl;
    return 0;
}
