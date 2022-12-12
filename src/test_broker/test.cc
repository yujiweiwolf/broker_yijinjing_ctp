#include <regex>
#include "../libbroker_ctp/ctp_broker.h"
using namespace yijinjing;
using namespace std;

string fund_id = "1000858";
string strategy_dir = "/home/work/sys/develop/broker_yijinjing_ctp/strategy";

// BUY IC2212.CFFEX 1 5606.6
// BUY y2211.DCE 1 10770
// SELL sc2212.INE 1 674.0
// BUY MA2211.CZCE 1 2778
// SELL ag2212.SHFE 1 4466
void order(JournalWriterPtr writer) {
    void* buffer = writer->GetFrame();
    int64_t nano = getNanoTime();
    Frame frame(buffer);
    frame.setNano(nano);

    TradeOrderMessage* msg = (TradeOrderMessage*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
    string uuid = x::UUID();
    strcpy(msg->id, uuid.c_str());
    strcpy(msg->fund_id, fund_id.c_str());
    msg->timestamp = x::RawDateTime();
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
            msg->item.bs_flag = kBsFlagBuy;
        } else if (command == "SELL") {
            msg->item.bs_flag = kBsFlagSell;
        }
        string code;
        if (market == "SH") {
            msg->item.market = kMarketSH;
            code = instrument + ".SH";
        } else if (market == "SZ") {
            msg->item.market = kMarketSZ;
            code = instrument + ".SZ";
        } else if (market == "CFFEX") {
            msg->item.market = kMarketCFFEX;
            code = instrument + ".CFFEX";
        } else if (market == "SHFE") {
            msg->item.market = kMarketSHFE;
            code = instrument + ".SHFE";
        } else if (market == "DCE") {
            msg->item.market = kMarketDCE;
            code = instrument + ".DCE";
        } else if (market == "CZCE") {
            msg->item.market = kMarketCZCE;
            code = instrument + ".CZCE";
        } else if (market == "INE") {
            msg->item.market = kMarketINE;
            code = instrument + ".INE";
        }
        strcpy(msg->item.code, code.c_str());
        msg->item.volume = atoll(volume.c_str());
        msg->item.price = atof(price.c_str());
    }
    msg->item.price_type = kQOrderTypeLimit;
    msg->item.timestamp = x::RawDateTime();
    writer->passFrame(frame, sizeof(TradeOrderMessage), TRADE_ORDER_REQ, 0);
}

void withdraw(JournalWriterPtr writer) {
    void* buffer = writer->GetFrame();
    int64_t nano = getNanoTime();
    Frame frame(buffer);
    frame.setNano(nano);

    TradeWithdrawMessage* msg = (TradeWithdrawMessage*)((char*)buffer + BASIC_FRAME_HEADER_LENGTH);
    string uuid = x::UUID();
    strcpy(msg->id, uuid.c_str());
    strcpy(msg->fund_id, fund_id.c_str());
    printf("please input order_no\n");
    string order_no;
    std::cin >> order_no;
    strcpy(msg->order_no, order_no.c_str());
    writer->passFrame(frame, sizeof(TradeWithdrawMessage), TRADE_WITHDRAQ_REQ, 0);
}

void query_asset(std::shared_ptr<yijinjing::Broker> broker) {
}

void query_position(std::shared_ptr<yijinjing::Broker> broker) {
}

void query_knock(std::shared_ptr<yijinjing::Broker> broker) {
}

int main() {
    std::string file_name = string("jump") + "_" + fund_id + "_" + std::to_string(x::RawDate());
    JournalWriterPtr writer = yijinjing::JournalWriter::create(strategy_dir.c_str(), file_name.c_str(), "test_strategy");

    BrokerOptionsPtr options = Config::Instance()->options();
    shared_ptr<yijinjing::CTPBroker> broker = make_shared<yijinjing::CTPBroker>();
    broker->StartWork(options);
    string usage("\nTYPE  'q' to quit program\n");
    usage += "      '1' to order\n";
    usage += "      '2' to withdraw\n";

    usage += "      '5' to query asset\n";
    usage += "      '6' to query position\n";
    usage += "      '7' to query order\n";
    usage += "      '8' to query knock\n";
    cerr << (usage);

    char c;
    while ((c = getchar()) != 'q') {
        switch (c) {
        case '1': {
            order(writer);
            break;
        }
        case '2': {
            withdraw(writer);
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
