/**
 * datagen.hpp
 * Generate data .txt files for trading system.
 *
 * @author Yicheng Sun
 */

#ifndef DATAGEN_HPP
#define DATAGEN_HPP

#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <random>
#include "functions.hpp"

// generate prices data
void genPrices(const vector<string>& products, const string& priceFile, long long seed, const int numDataPoints) {
    ofstream outFile(priceFile);
    mt19937 gen(seed);
    std::uniform_int_distribution<> ms_dist(1, 20);

    // Price file format: Timestamp, CUSIP, Bid, Ask
    outFile << "Timestamp,CUSIP,Bid,Ask" << endl;

    for (const auto& product : products) {
        double midPrice = 99.00;
        bool priceIncreasing = true;
        auto curTime = chrono::system_clock::now();
        string timestamp;

        for (int i = 0; i < numDataPoints; ++i) {
            double randomSpread = genRandomSpread(gen);
            curTime += chrono::milliseconds(ms_dist(gen));
            timestamp = getTimeStamp(curTime);

            double randomBid = midPrice - randomSpread / 2.0;
            double randomAsk = midPrice + randomSpread / 2.0;
            outFile << timestamp << "," << product << "," << convertPrice(randomBid) << "," << convertPrice(randomAsk) << endl;

            // Oscillate mid price
            midPrice = priceIncreasing ? midPrice + 1.0 / 256.0 : midPrice - 1.0 / 256.0;
            priceIncreasing = (randomAsk >= 101.0) ? false : (randomBid <= 99.0) ? true : priceIncreasing;
        }
    }
    outFile.close();
}

// generate orderbooks data
void genOrderBooks(const vector<string>& products, const string& orderbookFile, long long seed, const int numDataPoints) {
    ofstream outFile(orderbookFile);
    mt19937 gen(seed);
    std::uniform_int_distribution<> ms_dist(1, 20);

    // Orderbook file format: Timestamp, CUSIP, Bid1, BidSize1, Ask1, AskSize1, ..., Bid5, BidSize5, Ask5, AskSize5
    outFile << "Timestamp,CUSIP,Bid1,BidSize1,Ask1,AskSize1,Bid2,BidSize2,Ask2,AskSize2,Bid3,BidSize3,Ask3,AskSize3,Bid4,BidSize4,Ask4,AskSize4,Bid5,BidSize5,Ask5,AskSize5" << endl;

    for (const auto& product : products) {
        double midPrice = 99.00;
        bool spreadIncreasing = true;
        double fixSpread = 1.0 / 128.0;
        auto curTime = chrono::system_clock::now();
        string timestamp;

        for (int i = 0; i < numDataPoints; ++i) {
            curTime += chrono::milliseconds(ms_dist(gen));
            timestamp = getTimeStamp(curTime);

            outFile << timestamp << "," << product;
            for (int level = 1; level <= 5; ++level) {
                double fixBid = midPrice - fixSpread * level / 2.0;
                double fixAsk = midPrice + fixSpread * level / 2.0;
                int size = level * 1'000'000;
                outFile << "," << convertPrice(fixBid) << "," << size << "," << convertPrice(fixAsk) << "," << size;
            }
            outFile << endl;

            // Oscillate spread
            fixSpread = spreadIncreasing ? fixSpread + 1.0 / 128.0 : fixSpread - 1.0 / 128.0;
            spreadIncreasing = (fixSpread >= 1.0 / 32.0) ? false : (fixSpread <= 1.0 / 128.0) ? true : spreadIncreasing;
        }
    }
    outFile.close();
}


// generate trades data
void genTrades(const vector<string>& products, const string& tradeFile, long long seed) {
    ofstream outFile(tradeFile);
    mt19937 gen(seed);
    vector<string> books = {"TRSY1", "TRSY2", "TRSY3"};
    vector<long> quantities = {1000000, 2000000, 3000000, 4000000, 5000000};

    // Iterate over products and generate trade data
    for (const auto& product : products) {
        for (int i = 0; i < 10; ++i) {
            string side = (i % 2 == 0) ? "BUY" : "SELL";
            string tradeId = GenerateRandomId(12);

            // Generate random price based on the side (BUY or SELL)
            uniform_real_distribution<double> priceDist(side == "BUY" ? 99.0 : 100.0, side == "BUY" ? 100.0 : 101.0);
            double price = priceDist(gen);

            long quantity = quantities[i % quantities.size()];
            string book = books[i % books.size()];

            // Write the generated trade data to file
            outFile << product << "," << tradeId << "," << convertPrice(price) << "," 
                  << book << "," << quantity << "," << side << endl;
        }
    }

    outFile.close();
}

// generate inquiries data
void genInquiries(const vector<string>& products, const string& inquiryFile, long long seed) {
    ofstream outFile(inquiryFile);
    mt19937 gen(seed);
    vector<long> quantities = {1000000, 2000000, 3000000, 4000000, 5000000};

    // Iterate over products and generate inquiries
    for (const auto& product : products) {
        for (int i = 0; i < 10; ++i) {
            string side = (i % 2 == 0) ? "BUY" : "SELL";
            string inquiryId = GenerateRandomId(12);

            // Generate random price based on the side (BUY or SELL)
            uniform_real_distribution<double> priceDist(side == "BUY" ? 99.0 : 100.0, side == "BUY" ? 100.0 : 101.0);
            double price = priceDist(gen);

            long quantity = quantities[i % quantities.size()];
            string status = "RECEIVED";

            // Write the generated inquiry data to file
            outFile << inquiryId << "," << product << "," << side << "," 
                  << quantity << "," << convertPrice(price) << "," << status << endl;
        }
    }

    outFile.close();
}


#endif // DATAGEN_HPP
