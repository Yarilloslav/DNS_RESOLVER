#include "dns_resolver.h"
#include <sstream>
#include <iostream>

DNSResolver::DNSResolver() : resolver_(io_service_), hwndOutput_(nullptr) {}

DNSResolver::~DNSResolver() {
    io_service_.stop();
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

void DNSResolver::setOutputHandle(HWND hwnd) {
    hwndOutput_ = hwnd;
}

const std::vector<std::wstring>& DNSResolver::getResults() const {
    return dnsResults_;
}

void DNSResolver::resolveHostAsync(const std::string& host, const std::string& service) {
    // Очистка старых результатов
    dnsResults_.clear();

    // Перезапуск io_service и потока
    if (io_thread_.joinable()) {
        io_service_.stop();
        io_thread_.join();
        io_service_.reset();
    }

    boost::asio::ip::tcp::resolver::query query(host, service);

    resolver_.async_resolve(query,
        [this, host](const boost::system::error_code& ec, auto results) {
            if (!ec) {
                for (const auto& entry : results) {
                    auto address = convertToWString(entry.endpoint().address().to_string());
                    dnsResults_.push_back(address);

                    if (hwndOutput_) {
                        auto resultStr = new std::wstring(address + L"\r\n");
                        SendMessage(hwndOutput_, WM_USER + 1, reinterpret_cast<WPARAM>(resultStr), 0);
                    }
                }
            }
            else {
                std::wcerr << L"Failed to resolve host: " << convertToWString(ec.message()) << std::endl;
            }
        });

    io_thread_ = std::thread([this]() { io_service_.run(); });
}


std::wstring DNSResolver::convertToWString(const std::string& str) {
    return std::wstring(str.begin(), str.end());
}
