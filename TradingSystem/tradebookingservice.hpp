/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <string>
#include <vector>
#include "soa.hpp"
#include "executionservice.hpp"

// Trade sides
enum Side { BUY, SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template<typename T>
class Trade
{

public:

  // ctor for a trade
  Trade() = default;
  Trade(const T &_product, string _tradeId, double _price, string _book, long _quantity, Side _side) :
    product(_product), tradeId(_tradeId), price(_price), book(_book), quantity(_quantity), side(_side) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the trade ID
  const string& GetTradeId() const { return tradeId; };

  // Get the mid price
  double GetPrice() const { return price; };

  // Get the book
  const string& GetBook() const { return book; };

  // Get the quantity
  long GetQuantity() const { return quantity; };

  // Get the side
  Side GetSide() const { return side; };

private:
  T product;
  string tradeId;
  double price;
  string book;
  long quantity;
  Side side;

};


// forward declaration of connector and a tradebooking listener
template<typename T>
class TradeBookingConnector;
template<typename T>
class TradeBookingServiceListener;

/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingService : public Service<string,Trade <T>>
{

public:
  // ctor and dtor
  TradeBookingService()
  {
    connector = new TradeBookingConnector<T>(this);
    tradebookinglistener = new TradeBookingServiceListener<T>(this);
  };
  ~TradeBookingService() = default;

  // Get data from service
  Trade<T>& GetData(string _key) { return trades[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(Trade<T>& _data)
  {
    string _key = _data.GetTradeId();
    // create _key if not already exist
    if (trades.find(_key) != trades.end()) 
    {
      trades[_key] = _data;
    }
    else 
    {
      trades.insert(pair<string, Trade<T>>(_key, _data));
    }
      
    // flow data to the service
    for(auto& listener : listeners) 
    {
      listener -> ProcessAdd(_data);
    }
  };

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service.
  void AddListener(ServiceListener<Trade<T>>* _listener) { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector<ServiceListener<Trade<T>>*>& GetListeners() const { return listeners; };

  // Get the connector
  TradeBookingConnector<T>* GetConnector() { return connector; };

  // Get associated trade book listener
  TradeBookingServiceListener<T>* GetTradeBookingServiceListener() { return tradebookinglistener; };

private:
  map<string, Trade<T>> trades;
  vector<ServiceListener<Trade<T>>*> listeners;
  TradeBookingConnector<T>* connector;
  TradeBookingServiceListener<T>* tradebookinglistener;

};


/**
* Connector that subscribes data from socket to trade booking service.
* Type T is the product type.
*/
template<typename T>
class TradeBookingConnector : public Connector<Trade<T>>
{
private:
  TradeBookingService<T>* service;

public:
  // ctor and dtor
  TradeBookingConnector(TradeBookingService<T>* _service) : service(_service) {};
  ~TradeBookingConnector() = default;

  // Publish data to the Connector
  void Publish(Trade<T>& _data) override {};

  // Subscribe data from the Connector
  void Subscribe(ifstream& _data)
  {
    string _line;
    while (getline(_data, _line))
    {
      vector<string> lineVec;
      stringstream ss(_line);
      string attribute;
      while (getline(ss, attribute, ',')) {
        lineVec.push_back(attribute);
      }

      string productId = lineVec[0];
      T product = getProductObject<T>(productId);
      string tradeId = lineVec[1];
      double price = convertPrice(lineVec[2]);
      string book = lineVec[3];
      long quantity = stol(lineVec[4]);
      Side side = lineVec[5] == "BUY" ? BUY : SELL;

      // create Trade object
      Trade<T> trade(product, tradeId, price, book, quantity, side);

      // flows data to tradebooking service
      service -> OnMessage(trade);
    }
  };
};


/**
 * Trade Booking Execution Listener subscribing from execution service.
 * Type T is the product type.
 */
template<typename T>
class TradeBookingServiceListener : public ServiceListener<ExecutionOrder<T>>
{

public:
  // ctor and dtor
  TradeBookingServiceListener(TradeBookingService<T>* _service) : service(_service), count(0) {};
  ~TradeBookingServiceListener() = default;

  // Listener callback to process an add event to the Service
  void ProcessAdd(ExecutionOrder<T>& _data) override
  {
    T product = _data.GetProduct();
    string orderId = _data.GetOrderId();
    double price = _data.GetPrice();
    long quantity = _data.GetVisibleQuantity() + _data.GetHiddenQuantity();
    Side side = (_data.GetSide() == BID) ? BUY : SELL;
    
    // cycle through 3 books
    string book;
    count++;
    if (count % 3 == 0) {
        book = "TRSY1";
    } else if (count % 3 == 1) {
        book = "TRSY2";
    } else if (count % 3 == 2) {
        book = "TRSY3";
    }

    Trade<T> trade(product, orderId, price, book, quantity, side);
    // flow data to the execution service
    service -> OnMessage(trade);
  };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(ExecutionOrder<T>& _data) override {};

  // Listener callback to process an update event to the Service
  void ProcessUpdate(ExecutionOrder<T>& _data) override {};

private:
  TradeBookingService<T>* service;
  long count;

};


#endif
