/**
 * algoexecution.hpp
 * Defines the data types and Service for algo execution.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */
#ifndef ALGOEXECUTION_SERVICE_HPP
#define ALGOEXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"  
#include "marketdataservice.hpp"
#include "functions.hpp"

enum OrderType { FOK, IOC, MARKET, LIMIT, STOP };

enum Market { BROKERTEC, ESPEED, CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template<typename T>
class ExecutionOrder
{

public:

  // ctor for an order
  ExecutionOrder() = default;
  ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) :
    product(_product), side(_side), orderId(_orderId), orderType(_orderType), price(_price), 
    visibleQuantity(_visibleQuantity), hiddenQuantity(_hiddenQuantity), parentOrderId(_parentOrderId), isChildOrder(_isChildOrder) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the pricing side
  PricingSide GetSide() const { return side; };

  // Get the order ID
  const string& GetOrderId() const { return orderId; };

  // Get the order type on this order
  OrderType GetOrderType() const { return orderType; };

  // Get the price on this order
  double GetPrice() const { return price; };

  // Get the visible quantity on this order
  long GetVisibleQuantity() const { return visibleQuantity; };

  // Get the hidden quantity
  long GetHiddenQuantity() const { return hiddenQuantity; };

  // Get the parent order ID
  const string& GetParentOrderId() const { return parentOrderId; };

  // Is child order?
  bool IsChildOrder() const { return isChildOrder; };

  // object printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const ExecutionOrder<U>& order) {
    T product = order.GetProduct();
    string _product = product.GetProductId();
    string _orderId = order.GetOrderId();
    string _side = (order.GetSide() == BID ? "Bid" : "Ask");
    string _orderType;

    OrderType ot = order.GetOrderType();
    if (ot == FOK) {
        _orderType = "FOK";
    } else if (ot == MARKET) {
        _orderType = "MARKET";
    } else if (ot == LIMIT) {
        _orderType = "LIMIT";
    } else if (ot == STOP) {
        _orderType = "STOP";
    } else if (ot == IOC) {
        _orderType = "IOC";
    }

    string _price = convertPrice(order.GetPrice());
    string _visibleQuantity = to_string(order.GetVisibleQuantity());
    string _hiddenQuantity = to_string(order.GetHiddenQuantity());
    string _parentOrderId = order.GetParentOrderId();
    string _isChildOrder = (order.IsChildOrder() ? "True" : "False");

    vector<string> components = {_product, _orderId, _side, _orderType, _price, _visibleQuantity, _hiddenQuantity, _parentOrderId, _isChildOrder};
    string combinedStr = join(components, ",");
    os << combinedStr;
    return os;
  };

private:
  T product;
  PricingSide side;
  string orderId;
  OrderType orderType;
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  string parentOrderId;
  bool isChildOrder;

};


/**
 * Algo Execution Service to execute orders on market.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoExecution
{

public:
    // ctor for an order
    AlgoExecution() = default;
    AlgoExecution(const ExecutionOrder<T> &_executionOrder, Market _market) :
      executionOrder(_executionOrder), market(_market) {};

    // Get the execution order
    const ExecutionOrder<T>& GetExecutionOrder() const { return executionOrder; };

    // Get the market
    Market GetMarket() const { return market; };

private:
    ExecutionOrder<T> executionOrder;
    Market market;

};    


// forward declaration of AlgoExecutionServiceListener
template<typename T>
class AlgoExecutionServiceListener;

/**
 * Algo Execution Service to execute orders on market.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoExecutionService : public Service<string, AlgoExecution<T>>
{

public:
    // ctor and dtor
    AlgoExecutionService() 
    {
      algoexecservicelistener = new AlgoExecutionServiceListener<T>(this);
      count = 0;
    };
    ~AlgoExecutionService() = default;
    
    // Get data on our service given a key
    AlgoExecution<T>& GetData(string _key) { return algoExecutions[_key]; };
    
    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(AlgoExecution<T>& _data) override {};
    
    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<AlgoExecution<T>>* _listener) override { listeners.push_back(_listener); };
    
    // Get all listeners on the Service.
    const vector< ServiceListener<AlgoExecution<T>>* >& GetListeners() const override { return listeners; };
    
    // Get the special listener for algo execution service
    AlgoExecutionServiceListener<T>* GetAlgoExecutionServiceListener() { return algoexecservicelistener; };

    // Execute an algo order on a market, called by AlgoExecutionServiceListener to subscribe data from Algo Market Data Service to Algo Execution Service
    void AlgoExecuteOrder(OrderBook<T>& _orderBook) {
      // Initialize order data
      T product = _orderBook.GetProduct();
      string key = product.GetProductId();
      string orderId = "A" + GenerateRandomId(11);
      string parentOrderId = "AP" + GenerateRandomId(10);

      // Retrieve best bid and offer
      BidOffer bidOffer = _orderBook.GetBestBidOffer();
      Order bid = bidOffer.GetBidOrder();
      Order offer = bidOffer.GetOfferOrder();
      double bidPrice = bid.GetPrice();
      double offerPrice = offer.GetPrice();
      long bidQuantity = bid.GetQuantity();
      long offerQuantity = offer.GetQuantity();

      // Determine trading side and quantities based on price spread
      PricingSide side;
      double price;
      long quantity;
      if (offerPrice - bidPrice <= 1.0 / 128.0) {
          side = (count % 2 == 0) ? BID : OFFER;
          price = (side == BID) ? offerPrice : bidPrice;
          quantity = (side == BID) ? bidQuantity : offerQuantity;
      }
      count++;

      // Construct execution order
      long visibleQuantity = quantity;
      long hiddenQuantity = 0;
      bool isChildOrder = false;
      ExecutionOrder<T> executionOrder(product, side, orderId, MARKET, price, visibleQuantity, hiddenQuantity, parentOrderId, isChildOrder);
      AlgoExecution<T> algoExecution(executionOrder, BROKERTEC);

      // Update algo execution map
      if (algoExecutions.find(key) != algoExecutions.end()) {
          algoExecutions.erase(key);
      }
      algoExecutions.insert({key, algoExecution});

      // Notify listeners
      for (auto& listener : listeners) {
          listener->ProcessAdd(algoExecution);
      }
    };

private:
    map<string, AlgoExecution<T>> algoExecutions;
    vector<ServiceListener<AlgoExecution<T>>*> listeners;
    AlgoExecutionServiceListener<T>* algoexecservicelistener;
    double spread;
    long count;
    
};


/**
* AlgoExecutionService Listener subscribe data from MarketData Service
* Type T is the product type.
*/
template<typename T>
class AlgoExecutionServiceListener : public ServiceListener<OrderBook<T>>
{
private:
  AlgoExecutionService<T>* service;

public:
  // ctor and dtor
  AlgoExecutionServiceListener(AlgoExecutionService<T>* _service) : service(_service) {};
  ~AlgoExecutionServiceListener() = default;
  
  // Listener callback to process an add event to the Service
  void ProcessAdd(OrderBook<T>& _data) override {
    service -> AlgoExecuteOrder(_data);
  };
  
  // Listener callback to process a remove event to the Service
  void ProcessRemove(OrderBook<T>& _data) override {};
  
  // Listener callback to process an update event to the Service
  void ProcessUpdate(OrderBook<T>& _data) override {};
  
};


#endif