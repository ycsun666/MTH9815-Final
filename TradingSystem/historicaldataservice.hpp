/**
 * historicaldataservice.hpp
 *
 * @author Breman Thuraisingham & Yicheng Sun
 * Defines the data types and Service for historical data.
 */

#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include "soa.hpp"
#include "streamingservice.hpp"
#include "riskservice.hpp"
#include "executionservice.hpp"
#include "inquiryservice.hpp"
#include "positionservice.hpp"
#include "functions.hpp"

enum ServiceType {POSITION, RISK, EXECUTION, STREAMING, INQUIRY};


// forward declaration of connector and historicaldata listener
template<typename T>
class HistoricalDataConnector;
template<typename T>
class HistoricalDataServiceListener;


/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataService : Service<string,T>
{

public:
  // ctor and dtor
  HistoricalDataService(ServiceType _type) : type(_type) 
  {
    historicalservicelistener = new HistoricalDataServiceListener<T>(this);
    connector = new HistoricalDataConnector<T>(this);
  };  
  ~HistoricalDataService() = default;

  // Get data on our service given a key
  T& GetData(string _key) override { return histdatas[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(T& _data) override {};

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
  void AddListener(ServiceListener<T>* _listener) override { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<T>* >& GetListeners() const override { return listeners; };

  // Get the special listener for historical data service
  HistoricalDataServiceListener<T>* GetHistoricalDataServiceListener() { return historicalservicelistener; };

  // Get the connector
  HistoricalDataConnector<T>* GetConnector() { return connector; };

  // Get the type of the service
	ServiceType GetServiceType() const { return type; };

  // Persist data to a store
  void PersistData(string persistKey, T& data) {
    if (histdatas.find(persistKey) == histdatas.end()) {
      histdatas.insert(pair<string, T>(persistKey, data));
    }
    else {
      histdatas[persistKey] = data;
    }

    connector -> Publish(data);
  };

private:
  map<string, T> histdatas;
  vector<ServiceListener<T>*> listeners;
  HistoricalDataConnector<T>* connector;
  ServiceType type;
  HistoricalDataServiceListener<T>* historicalservicelistener;

};


/**
* Historical Data Connector publishing data from Historical Data Service.
* Type T is the data type to persist.
 */
template<typename T>
class HistoricalDataConnector : public Connector<T>
{

public:
  // ctor
  HistoricalDataConnector(HistoricalDataService<T>* _service) : service(_service) {
    fileNames[POSITION] = "../data/positions.txt";
    fileNames[RISK] = "../data/risk.txt";
    fileNames[EXECUTION] = "../data/executions.txt";
    fileNames[STREAMING] = "../data/streaming.txt";
    fileNames[INQUIRY] = "../data/aggregatedinquiries.txt";
  };

  // Publish-only connector, publish to external source
  void Publish(T& data) {
      ServiceType type = service -> GetServiceType();
      ofstream outFile;

      // Open file based on service type
      auto it = fileNames.find(type);
      outFile.open(it->second, ios::app);
      if (outFile.is_open()) {
          outFile << getTimeStamp() << "," << data << endl;
      }
      outFile.close();
  };

  void Subscribe(ifstream& _data) override {};

private:
  HistoricalDataService<T>* service;
  map<ServiceType, string> fileNames;

};


/**
* Historical Data Service Listener subscribing data to Historical Data.
* Type T is the data type to persist.
*/
template<typename T>
class HistoricalDataServiceListener : public ServiceListener<T>
{
private:
  HistoricalDataService<T>* service;

public:
  // ctor
  HistoricalDataServiceListener(HistoricalDataService<T>* _service) : service(_service) {};
  // Listener callback to process an add event to the Service
  void ProcessAdd(Position<Bond>& data) {
    string persistKey = data.GetProduct().GetProductId();
    service->PersistData(persistKey, data);
  };
  void ProcessAdd(PV01<Bond>& data) {
    string persistKey = data.GetProduct().GetProductId();
    service->PersistData(persistKey, data);
  };
  void ProcessAdd(PriceStream<Bond>& data) {
    string persistKey = data.GetProduct().GetProductId();
    service->PersistData(persistKey, data);
  };
  void ProcessAdd(ExecutionOrder<Bond>& data) {
    string persistKey = data.GetProduct().GetProductId();
    service->PersistData(persistKey, data);
  };
  void ProcessAdd(Inquiry<Bond>& data) {
    string persistKey = data.GetProduct().GetProductId();
    service->PersistData(persistKey, data);
  };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(T& data) override {};
  // Listener callback to process an update event to the Service
  void ProcessUpdate(T& data) override {};
};


#endif
