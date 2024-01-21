#include <iostream>

#include <easylogging++.h>

#include "order/order.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char *argv[]) {
    el::Loggers::configureFromGlobal("logging.conf");

    if (argc != 2) {
        std::cout << "Usage:: " << argv[0] << " path\n";
        return 1;
    }

    algos::order::Order order;
    config::InputTable parser = std::make_shared<CSVParser>(argv[1], '|', true);

    order.SetOption("table", std::move(parser));
    order.LoadData();
    unsigned long long time = order.Execute();
    std::cout << "Order deps num: " << order.GetValidODs().size() << ", time: " << time << '\n';
    return 0;
}
