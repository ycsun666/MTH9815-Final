/**
 * main.cpp
 * Main function to run the trading system
 *
 * @author Yicheng Sun
 */

#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>

#include "soa.hpp"
#include "products.hpp"
#include "marketdataservice.hpp"
#include "pricingservice.hpp"
#include "riskservice.hpp"
#include "executionservice.hpp"
#include "positionservice.hpp"
#include "inquiryservice.hpp"
#include "historicaldataservice.hpp"
#include "streamingservice.hpp"
#include "algostreamingservice.hpp"
#include "tradebookingservice.hpp"
#include "algoexecutionservice.hpp"
#include "guiservice.hpp"
#include "datagen.hpp"
#include "functions.hpp"

using namespace std;

int main(){

	// 1. generate data files for tradingsystem
	string dataDir = "../data";
	if (filesystem::exists(dataDir)) {
		filesystem::remove_all(dataDir);
	}
	filesystem::create_directory(dataDir);

	const string pricePath = dataDir + "/prices.txt";
	const string marketdataPath = dataDir + "/marketdata.txt";
	const string tradePath = dataDir + "/trades.txt";
	const string inquiryPath = dataDir + "/inquiries.txt";
	
    // bonds tickers
    vector<string> bonds = {"9128283H1", "9128283L2", "912828M80", "9128283J7", "9128283F5", "912810TW8", "912810RZ3"};
	
	logger(LogType::INFO, "Generating price data...");
    genPrices(bonds, pricePath, 42, 1000);
	logger(LogType::INFO, "Generating orderbook data...");
	genOrderBooks(bonds, marketdataPath, 42, 10000);
    logger(LogType::INFO, "Generating trade data...");
	genTrades(bonds, tradePath, 42);
    logger(LogType::INFO, "Generating inquiry data...");
	genInquiries(bonds, inquiryPath, 42);
    logger(LogType::INFO, "All data generated.");


    // 2. start trading service
    logger(LogType::INFO, "Initializing trading system services...");
	PricingService<Bond> pricingService;
	AlgoStreamingService<Bond> algoStreamingService;
	StreamingService<Bond> streamingService;
	MarketDataService<Bond> marketDataService;
	AlgoExecutionService<Bond> algoExecutionService;
	ExecutionService<Bond> executionService;
	TradeBookingService<Bond> tradeBookingService;
	PositionService<Bond> positionService;
	RiskService<Bond> riskService;
	GUIService<Bond> guiService;
	InquiryService<Bond> inquiryService;

	HistoricalDataService<Position<Bond>> historicalPositionService(POSITION);
	HistoricalDataService<PV01<Bond>> historicalRiskService(RISK);
	HistoricalDataService<ExecutionOrder<Bond>> historicalExecutionService(EXECUTION);
	HistoricalDataService<PriceStream<Bond>> historicalStreamingService(STREAMING);
	HistoricalDataService<Inquiry<Bond>> historicalInquiryService(INQUIRY);
	logger(LogType::INFO, "Trading system services initialized.");

	logger(LogType::INFO, "Linking service listeners...");
	pricingService.AddListener(algoStreamingService.GetAlgoStreamingListener());
	pricingService.AddListener(guiService.GetGUIServiceListener());
	algoStreamingService.AddListener(streamingService.GetStreamingServiceListener());
	marketDataService.AddListener(algoExecutionService.GetAlgoExecutionServiceListener());
	algoExecutionService.AddListener(executionService.GetExecutionServiceListener());
	executionService.AddListener(tradeBookingService.GetTradeBookingServiceListener());
	tradeBookingService.AddListener(positionService.GetPositionListener());
	positionService.AddListener(riskService.GetRiskServiceListener());
	// link to historicaldata service
	positionService.AddListener(historicalPositionService.GetHistoricalDataServiceListener());
	executionService.AddListener(historicalExecutionService.GetHistoricalDataServiceListener());
	streamingService.AddListener(historicalStreamingService.GetHistoricalDataServiceListener());
	riskService.AddListener(historicalRiskService.GetHistoricalDataServiceListener());
	inquiryService.AddListener(historicalInquiryService.GetHistoricalDataServiceListener());
	logger(LogType::INFO, "Service listeners linked.");


	// 3. start trading system data flows
	cout << fixed << setprecision(6);
    logger(LogType::INFO, "Processing price data...");
	ifstream priceData(pricePath);
	pricingService.GetConnector() -> Subscribe(priceData);
	logger(LogType::INFO, "Price data completed.");

	logger(LogType::INFO, "Processing market data...");
	ifstream marketData(marketdataPath);
	marketDataService.GetConnector() -> Subscribe(marketData);
	logger(LogType::INFO, "Market data completed.");

	logger(LogType::INFO, "Processing trade data...");
	ifstream tradeData(tradePath);
	tradeBookingService.GetConnector() -> Subscribe(tradeData);
	logger(LogType::INFO, "Trade data completed.");

	logger(LogType::INFO, "Processing inquiry data...");
	ifstream inquiryData(inquiryPath);
	inquiryService.GetConnector() -> Subscribe(inquiryData);
	logger(LogType::INFO, "Inquiry data completed.");

	logger(LogType::INFO, "All data flow completed.");
	logger(LogType::INFO, "Trading system ended.");

	return 0;
}
