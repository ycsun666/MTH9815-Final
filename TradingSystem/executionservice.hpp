/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <string>
#include "soa.hpp"
#include "algoexecutionservice.hpp"

// forward declaration of connector and executionservice listener
template<typename T>
class ExecutionServiceConnector;
template<typename T>
class ExecutionServiceListener;

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class ExecutionService : public Service<string,ExecutionOrder <T> >
{

public:
  // ctor and dtor
  ExecutionService()
  {
    executionservicelistener = new ExecutionServiceListener<T>(this);
  };
  ~ExecutionService() = default;

  // Get data on our service given a key
  ExecutionOrder<T>& GetData(string _key) { return executionOrders[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(ExecutionOrder<T>& data) override {};

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
  void AddListener(ServiceListener<ExecutionOrder<T>>* _listener) override { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<ExecutionOrder<T>>* >& GetListeners() const override { return listeners; };

  // Get the special listener for execution service
  ExecutionServiceListener<T>* GetExecutionServiceListener() { return executionservicelistener; };

  // Get the connector
  ExecutionServiceConnector<T>* GetConnector() { return connector; };

  // Execute an order on a market
  void ExecuteOrder(const ExecutionOrder<T>& order, Market market) { connector -> Publish(order, market); };

  // called by ExecutionServiceListener to subscribe data from Algo Execution Service to Execution Service
  void AddExecutionOrder(const AlgoExecution<T>& _algoExecution)
  {
    ExecutionOrder<T> executionOrder = _algoExecution.GetExecutionOrder();
    string orderId = executionOrder.GetOrderId();
    // delete orderId if already exist
    if (executionOrders.find(orderId) != executionOrders.end()) 
    {
      executionOrders.erase(orderId);
    }
    executionOrders.insert(pair<string, ExecutionOrder<T>> (orderId, executionOrder));
    
    // flow data to the service
    for (auto& listener : listeners) {
      listener -> ProcessAdd(executionOrder);
    }
  };

private:
  map<string, ExecutionOrder<T>> executionOrders;
  vector<ServiceListener<ExecutionOrder<T>>*> listeners;
  ExecutionServiceConnector<T>* connector;
  ExecutionServiceListener<T>* executionservicelistener;

};


/**
 * ExecutionServiceConnector: publish data to execution service.
 * Type T is the product type.
 */
template<typename T>
class ExecutionServiceConnector : public Connector<ExecutionOrder<T>>
{
private:
  ExecutionService<T>* service; // execution service related to this connector

public:
  // ctor and dtor
  ExecutionServiceConnector(ExecutionService<T>* _service) : service(_service) {};
  ~ExecutionServiceConnector() = default;

  // Publish data to the Connector and print
  void Publish(const ExecutionOrder<T>& _order, Market& _market)
  {
    auto product = _order.GetProduct();
    string orderType;

    OrderType ot = _order.GetOrderType();
    if (ot == FOK) {
        orderType = "FOK";
    } else if (ot == MARKET) {
        orderType = "MARKET";
    } else if (ot == LIMIT) {
        orderType = "LIMIT";
    } else if (ot == STOP) {
        orderType = "STOP";
    } else if (ot == IOC) {
        orderType = "IOC";
    }

    string tradeMarket;
    if (_market == BROKERTEC) {
        tradeMarket = "BROKERTEC";
    } else if (_market == ESPEED) {
        tradeMarket = "ESPEED";
    } else if (_market == CME) {
        tradeMarket = "CME";
    }

    cout << "ExecutionOrder: "
         << "Product: " << product.GetProductId() << ", OrderId: " << _order.GetOrderId() << ", Trade Market: " << tradeMarket
         << ", PricingSide: " << (_order.GetSide() == BID ? "Bid" : "Offer")
         << ", OrderType: " << orderType << ", IsChildOrder: " << (_order.IsChildOrder() ? "True" : "False")
         << ", Price: " << _order.GetPrice() << ", VisibleQuantity: " << _order.GetVisibleQuantity()
         << ", HiddenQuantity: " << _order.GetHiddenQuantity() << endl << endl;
  };

  void Subscribe(ifstream& _data) {};
};


/**
* Execution Service Listener subscribes data from Algo Execution Service to Execution Service.
* Type T is the product type. Inner data type is AlgoExecution<T>.
*/
template<typename T>
class ExecutionServiceListener : public ServiceListener<AlgoExecution<T>>
{
private:
  ExecutionService<T>* executionService;

public:
  // ctor and dtor
  ExecutionServiceListener(ExecutionService<T>* _executionService) : executionService(_executionService) {};
  ~ExecutionServiceListener() = default;

  // Listener callback to process an add event to the Service
  void ProcessAdd(AlgoExecution<T>& _data) override 
  {
    executionService -> AddExecutionOrder(_data);
    ExecutionOrder<T> executionOrder = _data.GetExecutionOrder();
    Market market = _data.GetMarket();
    executionService -> ExecuteOrder(executionOrder, market);
  };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(AlgoExecution<T>& _data) override {};

  // Listener callback to process an update event to the Service
  void ProcessUpdate(AlgoExecution<T>& _data) override {};

};  



#endif
