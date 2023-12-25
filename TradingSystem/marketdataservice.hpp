/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>
#include <algorithm>
#include "soa.hpp"
#include "functions.hpp"

using namespace std;

// Side for market data
enum PricingSide { BID, OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order
{

public:

  // ctor for an order
  Order() = default;
  Order(double _price, long _quantity, PricingSide _side) : price(_price), quantity(_quantity), side(_side) {};

  // Get the price on the order
  double GetPrice() const { return price; };

  // Get the quantity on the order
  long GetQuantity() const { return quantity; };

  // Get the side on the order
  PricingSide GetSide() const { return side; };

private:
  double price;
  long quantity;
  PricingSide side;

};


/**
 * Class representing a bid and offer order
 */
class BidOffer
{

public:

  // ctor for bid/offer
  BidOffer(const Order& _bidOrder, const Order& _offerOrder) : bidOrder(_bidOrder), offerOrder(_offerOrder) {};

  // Get the bid order
  const Order& GetBidOrder() const { return bidOrder; };

  // Get the offer order
  const Order& GetOfferOrder() const { return offerOrder; };

private:
  Order bidOrder;
  Order offerOrder;

};


/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template<typename T>
class OrderBook
{

public:
  // ctor for the order book
  OrderBook() = default;
  OrderBook(const string& productId) : 
    product(getProductObject<T>(productId)), bidStack(vector<Order>()), offerStack(vector<Order>()) {};
  OrderBook(const T &_product, const vector<Order> &_bidStack, const vector<Order> &_offerStack) :
    product(_product), bidStack(_bidStack), offerStack(_offerStack) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the bid stack
  vector<Order>& GetBidStack() { return bidStack; };

  // Get the offer stack
  vector<Order>& GetOfferStack() { return offerStack; };

  // Get the best bid/offer order
  BidOffer GetBestBidOffer() const 
  {
    // return the best bid/offer (max for bid and min for offer)
    auto bestBid = std::max_element(bidStack.begin(), bidStack.end(), [](const Order& a, const Order& b) {return a.GetPrice() < b.GetPrice(); });
    auto bestOffer = std::min_element(offerStack.begin(), offerStack.end(), [](const Order& a, const Order& b) {return a.GetPrice() < b.GetPrice(); });
    return BidOffer(*bestBid, *bestOffer);
  };


private:
  T product;
  vector<Order> bidStack;
  vector<Order> offerStack;

};


// forward declaration of MarketDataConnector
template<typename T>
class MarketDataConnector;

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class MarketDataService : public Service<string,OrderBook <T> >
{

public:
  // ctor and dtor
  MarketDataService()
  { 
    connector = new MarketDataConnector<T>(this); 
    bookDepth = 5;
  };
  ~MarketDataService() = default;

  // Get data on our service given a key
  OrderBook<T>& GetData(string _key) override 
  {
  // initialize with _key if not exist
  if (orderBooks.find(_key) == orderBooks.end()) {
    orderBooks.insert(pair<string, OrderBook<T>>(_key, OrderBook<T>(_key)));
  }
    return orderBooks[_key];
  }
  
  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(OrderBook<T>& _data) override 
  {
    string key = _data.GetProduct().GetProductId();
    if (orderBooks.find(key) != orderBooks.end())
    { 
      orderBooks.erase(key); 
    }
    orderBooks.insert(pair<string, OrderBook<T>>(key, _data));

    // flow data to listeners
    for (auto& listener : listeners)
    {
      listener->ProcessAdd(_data);
    }
  };

  // Add a listener to the Service for callbacks on add, remove, and update events
  void AddListener(ServiceListener<OrderBook<T>>* listener) override { listeners.push_back(listener); };

  // Get all listeners on the Service
  const vector<ServiceListener<OrderBook<T>>*>& GetListeners() const override { return listeners; };

  // Get the connector
  MarketDataConnector<T>* GetConnector() { return connector; };

  // Get the book depth
  int GetBookDepth() const { return bookDepth; };

  // Get the best bid/offer order
  const BidOffer& GetBestBidOffer(const string& _productId) { return orderBooks[_productId].GetBestBidOffer(); };

  // Aggregate the order book
  const OrderBook<T>& AggregateDepth(const string& _productId)
  {
    OrderBook<T>& orderBook = orderBooks[_productId];

    // Aggregate bid stack
    vector<Order>& bidStack = orderBook.GetBidStack();
    unordered_map<double, long> bidAggMap;
    for (auto& order : bidStack) {
        double price = order.GetPrice();
        bidAggMap[price] += order.GetQuantity();
    }
    vector<Order> aggregatedBids;
    for (auto& item : bidAggMap) {
        aggregatedBids.push_back(Order(item.first, item.second, BID));
    }

    // Aggregate offer stack
    vector<Order>& offerStack = orderBook.GetOfferStack();
    unordered_map<double, long> offerAggMap;
    for (auto& order : offerStack) {
        double price = order.GetPrice();
        offerAggMap[price] += order.GetQuantity();
    }
    vector<Order> aggregatedOffers;
    for (auto& item : offerAggMap) {
        aggregatedOffers.push_back(Order(item.first, item.second, OFFER));
    }

    // Update and return the aggregated order book
    orderBook = OrderBook<T>(orderBook.GetProduct(), aggregatedBids, aggregatedOffers);
    return orderBook;
}



private:
  MarketDataConnector<T>* connector;
  map<string, OrderBook<T>> orderBooks;
  vector<ServiceListener<OrderBook<T>>*> listeners;
  int bookDepth;

};


/**
* Market Data Connector subscribing data to Market Data Service.
* Type T is the product type.
*/
template<typename T>
class MarketDataConnector : public Connector<OrderBook<T>>
{
private:
  MarketDataService<T>* service;

public:
  // ctor and dtor
  MarketDataConnector(MarketDataService<T>* _service) : service(_service) {};
  ~MarketDataConnector() = default;

  // Publish data to the Connector
  void Publish(OrderBook<T>& _data) override {};

  // Subscribe data
  void Subscribe(ifstream& _data) override;

};


template<typename T>
void MarketDataConnector<T>::Subscribe(ifstream& _data)
{
  string _line;
  getline(_data, _line);

  while (getline(_data, _line))
  {
    vector<string> lineVec;
    stringstream ss(_line); 
    string attribute;
    while (getline(ss, attribute, ','))
    {
      lineVec.push_back(attribute);
    }
    string timestamp = lineVec[0];
    string productId = lineVec[1];
    OrderBook<T>& orderBook = service -> GetData(productId);

    string bidPrice, bidQuantity, offerPrice, offerQuantity;
    Order bidOrder, askOrder;
    for (int order = 0; order < service -> GetBookDepth(); order++)
    {
      string bidPrice = lineVec[4 * order + 2];
      string bidQuantity = lineVec[4 * order + 3];
      string offerPrice = lineVec[4 * order + 4];
      string offerQuantity = lineVec[4 * order + 5];

      Order bidOrder(convertPrice(bidPrice), stol(bidQuantity), BID);
      Order askOrder(convertPrice(offerPrice), stol(offerQuantity), OFFER);

      orderBook.GetBidStack().push_back(bidOrder);
      orderBook.GetOfferStack().push_back(askOrder);
    }

    // aggregate the order book and match orders
    OrderBook<T> aggOrderBook = service -> AggregateDepth(productId);
    // flow data to the service
    service -> OnMessage(aggOrderBook);

  }
}

#endif
