#include <regex>
#include "../libbroker_ctp/ctp_broker.h"
using namespace yijinjing;
using namespace std;

int main() {
    yijinjing::BrokerOptionsPtr options = yijinjing::Config::Instance()->options();
    shared_ptr<yijinjing::CTPBroker> broker = make_shared<yijinjing::CTPBroker>();
    broker->StartWork(options);
    broker->Run();
    std::cout << "Finished!" << endl;
    return 0;
}
