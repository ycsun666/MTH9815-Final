/**
 * algostreamingservice.hpp
 * Defines the data types and Service for algo streams.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef ALGOSTREAMING_SERVICE_HPP
#define ALGOSTREAMING_SERVICE_HPP

#include "soa.hpp"
#include "pricingservice.hpp"
#include "marketdataservice.hpp"
#include "functions.hpp"

/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder
{

public:
  // ctor for an order
  PriceStreamOrder() = default;
  PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side) :
    price(_price), visibleQuantity(_visibleQuantity), hiddenQuantity(_hiddenQuantity), side(_side) {};

  // The side on this order
  PricingSide GetSide() const { return side; };

  // Get the price on this order
  double GetPrice() const { return price; };

  // Get the visible quantity on this order
  long GetVisibleQuantity() const { return visibleQuantity; };

  // Get the hidden quantity on this order
  long GetHiddenQuantity() const { return hiddenQuantity; };

  // object printer
  friend ostream& operator<<(ostream& os, const PriceStreamOrder& order) {
    string priceStr = convertPrice(order.GetPrice());
    string visibleQuantityStr = to_string(order.GetVisibleQuantity());
    string hiddenQuantityStr = to_string(order.GetHiddenQuantity());
    string sideStr = (order.GetSide() == BID) ? "BID" : "OFFER";

    vector<string> components = {priceStr, visibleQuantityStr, hiddenQuantityStr, sideStr};
    string combinedStr = join(components, ",");
    os << combinedStr;
    return os;
  };

private:
  double price;
  long visibleQuantity;
  long hiddenQuantity;
  PricingSide side;

};


/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template<typename T>
class PriceStream
{

public:

  // ctor
  PriceStream() = default; // needed for map data structure later
  PriceStream(const T &_product, const PriceStreamOrder &_bidOrder, const PriceStreamOrder &_offerOrder) :
    product(_product), bidOrder(_bidOrder), offerOrder(_offerOrder) {};

  // Get the product
  const T& GetProduct() const { return product; };

  // Get the bid order
  const PriceStreamOrder& GetBidOrder() const { return bidOrder; };

  // Get the offer order
  const PriceStreamOrder& GetOfferOrder() const { return offerOrder; };

  // object printer
  template<typename U>
  friend ostream& operator<<(ostream& os, const PriceStream<U>& priceStream) {
    T product = priceStream.GetProduct();
    string productId = product.GetProductId();
    PriceStreamOrder bidOrder = priceStream.GetBidOrder();
    PriceStreamOrder offerOrder = priceStream.GetOfferOrder();
    os << productId << "," << bidOrder << "," << offerOrder;
    return os;
  };

private:
  T product;
  PriceStreamOrder bidOrder;
  PriceStreamOrder offerOrder;

};


/**
* An algo streaming that process algo streaming.
* Type T is the product type.
*/
template<typename T>
class AlgoStream
{
private:
    PriceStream<T> priceStream;

public:
    // ctor for an order
    AlgoStream() = default; // needed for map data structure later
    AlgoStream(const PriceStream<T> &_priceStream) :
      priceStream(_priceStream) {};

    // Get the price stream
    const PriceStream<T>& GetPriceStream() const { return priceStream; };

};


// forward declaration of AlgoStreaming listener
template<typename T>
class AlgoStreamingServiceListener;


/**
 * Algo Streaming Service to publish algo streams.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class AlgoStreamingService : public Service<string,AlgoStream <T> >
{

public:
    // ctor and dtor
    AlgoStreamingService() {
      algostreamlistener = new AlgoStreamingServiceListener<T>(this);
    };
    ~AlgoStreamingService() = default;
    
    // Get data on our service given a key
    AlgoStream<T>& GetData(string _key) override { return algoStreams[_key]; };
    
    // The callback that a Connector should invoke for any new or updated data
    void OnMessage(AlgoStream<T>& _data) override {};
    
    // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
    void AddListener(ServiceListener<AlgoStream<T>>* _listener) override { listeners.push_back(_listener); };
    
    // Get all listeners on the Service.
    const vector< ServiceListener<AlgoStream<T>>* >& GetListeners() const override { return listeners; };
    
    // Get the special listener for algo streaming service
    AlgoStreamingServiceListener<T>* GetAlgoStreamingListener() { return algostreamlistener; };

    // Publish algo streams (called by algo streaming service listener to subscribe data from pricing service)
    void PublishAlgoStream(const Price<T>& price) {
      // Retrieve necessary data from price and initialize order parameters
      T product = price.GetProduct();
      string key = product.GetProductId();
      double mid = price.GetMid();
      double spread = price.GetBidOfferSpread();
      double bidPrice = mid - spread / 2;
      double offerPrice = mid + spread / 2;
      
      // Set quantities based on count
      long visibleQuantity = (count % 2 == 0) ? 1000000 : 2000000;
      long hiddenQuantity = visibleQuantity * 2;
      count++;

      // Create orders and stream objects
      PriceStreamOrder bidOrder(bidPrice, visibleQuantity, hiddenQuantity, BID);
      PriceStreamOrder offerOrder(offerPrice, visibleQuantity, hiddenQuantity, OFFER);
      PriceStream<T> priceStream(product, bidOrder, offerOrder);
      AlgoStream<T> algoStream(priceStream);

      // Update and notify
      if (algoStreams.find(key) != algoStreams.end()) {
          algoStreams.erase(key);
      }
      algoStreams.insert(pair<string, AlgoStream<T>>(key, algoStream));
      for (auto& listener : listeners) {
          listener->ProcessAdd(algoStream);
      }
    };
    
private:
    map<string, AlgoStream<T>> algoStreams;
    vector<ServiceListener<AlgoStream<T>>*> listeners;
    AlgoStreamingServiceListener<T>* algostreamlistener;
    long count;

};


/**
 * Algo Streaming Service Listener to subscribe data from pricing service.
 * Type T is the product type.
 */
template<typename T>
class AlgoStreamingServiceListener : public ServiceListener<Price<T>>
{

public:
    // ctor
    AlgoStreamingServiceListener(AlgoStreamingService<T>* _algoStreamingService) : algoStreamingService(_algoStreamingService) {};
    
    // Listener callback to process an add event to the Service
    void ProcessAdd(Price<T>& _price) override { algoStreamingService -> PublishAlgoStream(_price); };
    
    // Listener callback to process a remove event to the Service
    void ProcessRemove(Price<T>& _price) override {};
    
    // Listener callback to process an update event to the Service
    void ProcessUpdate(Price<T>& _price) override {};

private:
    AlgoStreamingService<T>* algoStreamingService;

};


#endif