/**
* functions.hpp
* Utility functions for the trading system
*
* @author Yicheng Sun
*/

#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <fstream>
#include "products.hpp"

using namespace std;
using namespace chrono;

// convert prices from fractional notations (string) to decimal notations (double)
double convertPrice(const string& priceStr)
{
    size_t dashPos = priceStr.find('-');
    if (dashPos == string::npos || dashPos + 3 > priceStr.length()) {
        throw invalid_argument("Invalid price format");
    }

    // extract the whole number and fractional parts
    string intpart = priceStr.substr(0, dashPos);
    string dec1 = priceStr.substr(dashPos + 1, 2);
    string dec2 = priceStr.substr(dashPos + 3, 1);

    // deal with special representation of 'z' character
    if (dec2 == "+") {
        dec2 == "4";
    }

    // calculate the decimal price
    double price = stod(intpart) + stod(dec1) * 1.0 / 32.0 + stod(dec2) * 1.0 / 256.0;
    return price;
}

// convert prices from decimal notations (double) to fractional noations (string)
string convertPrice(double price) {
    int intPart = floor(price);
    double fractionalPart = price - intPart;

    // convert the fractional part to ticks (out of 256)
    int totalTicks = floor(round(fractionalPart * 256));

    // calculate 'xy' (out of 32) and 'z' (remainder out of 256)
    int xy = totalTicks / 8;
    int z = totalTicks % 8;

    // construct the string representation
    string priceStr = to_string(intPart) + "-" + (xy < 10 ? "0" : "") + to_string(xy) + to_string(z);
    return priceStr;
}

// get time and format in milliseconds
string getTimeStamp() {
    auto now = chrono::system_clock::now();
    auto now_time_t = chrono::system_clock::to_time_t(now);
    tm now_tm = *localtime(&now_time_t);

    // get the milliseconds component
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // format the time and append milliseconds
    ostringstream oss;
    oss << put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << setfill('0') << setw(3) << milliseconds.count();

    return oss.str();
}

string getTimeStamp(chrono::system_clock::time_point _now) {
    auto now_time_t = chrono::system_clock::to_time_t(_now);
    tm now_tm = *localtime(&now_time_t);

    // get the milliseconds component
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(_now.time_since_epoch()) % 1000;

    // format the time and append milliseconds
    ostringstream oss;
    oss << put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << setfill('0') << setw(3) << milliseconds.count();

    return oss.str();
}

// join a vector of string with delimiter
string join(const vector<string>& strings, const string& delimiter) {
    string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += delimiter + strings[i];
    }

    return result;
}


// log messages and logger function
enum LogType { INFO, WARNING, ERROR };

void logger(LogType type, const string& message) {
    std::string logTypeStr;
    switch (type) {
        case INFO: logTypeStr = "INFO"; break;
        case WARNING: logTypeStr = "WARNING"; break;
        case ERROR: logTypeStr = "ERROR"; break;
    }

    cout << getTimeStamp() << " [" << logTypeStr << "] " << message << endl;
}

// get Productobject
template <typename T>
T getProductObject(const string& cusip) {
    if (cusip == "9128283H1") {
        return Bond("9128283H1", CUSIP, "US2Y", 0.01750, from_string("2025/12/30"));
    } else if (cusip == "9128283L2") {
        return Bond("9128283L2", CUSIP, "US3Y", 0.01875, from_string("2026/12/30"));
    } else if (cusip == "912828M80") {
        return Bond("912828M80", CUSIP, "US5Y", 0.02000, from_string("2028/12/30"));
    } else if (cusip == "9128283J7") {
        return Bond("9128283J7", CUSIP, "US7Y", 0.02125, from_string("2030/12/30"));
    } else if (cusip == "9128283F5") {
        return Bond("9128283F5", CUSIP, "US10Y", 0.02250, from_string("2033/12/30"));
    } else if (cusip == "912810TW8") {
        return Bond("912810TW8", CUSIP, "US20Y", 0.02500, from_string("2043/12/30"));
    } else if (cusip == "912810RZ3") {
        return Bond("912810RZ3", CUSIP, "US30Y", 0.02750, from_string("2053/12/30"));
    } else {
        throw invalid_argument("Unknown CUSIP: " + cusip);
    }
}

// define pv01 value for cusips
double getPV01(const string& _cusip) {
	double _pv01 = 0;
	if (_cusip == "9128283H1") _pv01 = 0.01948992;
	if (_cusip == "9128283L2") _pv01 = 0.02865304;
	if (_cusip == "912828M80") _pv01 = 0.04581119;
	if (_cusip == "9128283J7") _pv01 = 0.06127718;
	if (_cusip == "9128283F5") _pv01 = 0.08161449;
	if (_cusip == "912810RZ3") _pv01 = 0.15013155;
	return _pv01;
}

// Generate random ID with numbers and letters
string GenerateRandomId(long length) {
    string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    string id = "";

    for (int j = 0; j < length; ++j) {
        int randomIdx = rand() % 36;
        id += characters[randomIdx];
    }
    return id;
}

// generate random spread between 1/128 and 1/64
double genRandomSpread(std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(1.0/128.0, 1.0/64.0);
    return dist(gen);
}


#endif