/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <string>
#include <map>
#include "soa.hpp"
#include "functions.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template<typename T>
class Price
{

public:

  // ctor for a price
  Price() = default;
  Price(const T& _product, double _mid, double _bidOfferSpread):
    product(_product), mid(_mid), bidOfferSpread(_bidOfferSpread) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the mid price
  double GetMid() const { return mid; };

  // Get the bid/offer spread around the mid
  double GetBidOfferSpread() const { return bidOfferSpread; };

  // reload printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const Price<U>& price) 
  {
    T product = price.GetProduct();
    string productId = product.GetProductId();
    double mid = price.GetMid();
    double bidOfferSpread = price.GetBidOfferSpread();

    string midStr = convertPrice(mid);
    string spreadStr = convertPrice(bidOfferSpread);

    vector<string> components = {productId, midStr, spreadStr};
    string combinedStr = join(components, ",");

    // Output the combined string to the stream
    os << combinedStr;

    return os;
  };

private:
  T product;
  double mid;
  double bidOfferSpread;

};


// forward declaration of connector
template<typename T>
class PricingConnector;

/**
 * Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class PricingService : public Service<string,Price <T> >
{

public:
  // ctor and dtor
  PricingService() {
    connector = new PricingConnector<T>(this);
  };
  ~PricingService() = default;

  // Get data from service
  Price<T>& GetData(string _key) override { return prices[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(Price<T>& _data) override 
  {
    string _key = _data.GetProduct().GetProductId();
    // update prices
    if (prices.find(_key) != prices.end()) {
      prices.erase(_key);
    }
    prices.insert(pair<string, Price<Bond>> (_key, _data));
    // flow data to listeners
    for (auto& listener : listeners) {
        listener -> ProcessAdd(_data);
    }
  };

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service.
  void AddListener(ServiceListener<Price<T>>* _listener) override { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<Price<T>>* >& GetListeners() const override { return listeners; };

  // Get the connector
  PricingConnector<T>* GetConnector() { return connector; };

private:
  map<string, Price<T>> prices;
  vector<ServiceListener<Price<T>>*> listeners;
  PricingConnector<T>* connector;

};


/**
* Pricing Connector subscribing data to Pricing Service.
* Type T is the product type.
*/
template<typename T>
class PricingConnector : public Connector<Price<T>>
{
public:
  // ctor and dtor
  PricingConnector(PricingService<T>* _service) : service(_service) {};
  ~PricingConnector() = default;

  // publish data to Connector
  void Publish(Price<T>& _data) override {};

  // subscribe data from Connector
  void Subscribe(ifstream& _data) override 
  {
    string _line;
    // skip the first line of tickers
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
      double bid = convertPrice(lineVec[2]);
      double ask = convertPrice(lineVec[3]);
      double spread = ask - bid;
      double mid = (bid + ask) / 2.0;
      T product = getProductObject<T>(productId);
      
      // create Price object
      Price<T> price(product, mid, spread);

      // flow data to pricing service
      service -> OnMessage(price);
    }
  };

private:
  PricingService<T>* service; 
};


#endif
