/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham & Yicheng Sun
 */

#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include "soa.hpp"
#include "algostreamingservice.hpp"

// forward declaration of connector and streamingservice listener
template<typename T>
class StreamingServiceConnector;
template<typename T>
class StreamingServiceListener;

/**
 * Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template<typename T>
class StreamingService : public Service<string,PriceStream <T> >
{

public:
  // ctor and dtor
  StreamingService() 
  {
    streamingservicelistener = new StreamingServiceListener<T>(this);
  };
  ~StreamingService() = default;

  // Get data on our service given a key
  PriceStream<T>& GetData(string _key) override { return priceStreams[_key]; };

  // The callback that a Connector should invoke for any new or updated data
  void OnMessage(PriceStream<T>& _data) override {};

  // Add a listener to the Service for callbacks on add, remove, and update events for data to the Service
  void AddListener(ServiceListener<PriceStream<T>>* _listener) override { listeners.push_back(_listener); };

  // Get all listeners on the Service.
  const vector< ServiceListener<PriceStream<T>>* >& GetListeners() const override { return listeners; };

  // Get the special listener for streaming service
  StreamingServiceListener<T>* GetStreamingServiceListener() { return streamingservicelistener; };

  // Get the connector
  StreamingServiceConnector<T>* GetConnector() { return connector; };

  // Publish two-way prices
  void PublishPrice(const PriceStream<T>& priceStream) { connector -> Publish(priceStream); };

  // called by streaming service listener to subscribe data from algo streaming service
  void AddPriceStream(const AlgoStream<T>& _algoStream) {
    PriceStream<T> priceStream = _algoStream.GetPriceStream();
    string key = priceStream.GetProduct().GetProductId();
    
    // update the pricestream map, create if key not already exist
    if (priceStreams.find(key) != priceStreams.end()) {
        priceStreams.erase(key);
    }
    priceStreams.insert(pair<string, PriceStream<T>>(key, priceStream));

    // flow the data to listeners
    for (auto& listener : listeners) {
        listener -> ProcessAdd(priceStream);
    }
  };

private:
  map<string, PriceStream<T>> priceStreams;
  vector<ServiceListener<PriceStream<T>>*> listeners;
  StreamingServiceConnector<T>* connector;
  StreamingServiceListener<T>* streamingservicelistener;
};


/**
 * StreamingServiceConnector: publish data to streaming service.
 * Type T is the product type.
 */
template<typename T>
class StreamingServiceConnector : public Connector<PriceStream<T>>
{
private:
  StreamingService<T>* service;

public:
  // ctor and dtor
  StreamingServiceConnector(StreamingService<T>* _service) : service(_service) {};
  ~StreamingServiceConnector() = default;

  // Publish data to the Connector
  void Publish(const PriceStream<T>& _data) {
    // Print the price stream data
    T product = _data.GetProduct();
    string productId = product.GetProductId();
    PriceStreamOrder bid = _data.GetBidOrder();
    PriceStreamOrder offer = _data.GetOfferOrder();

    cout << "Price Stream (Product " << productId << "): "
         << "Bid Price: " << bid.GetPrice() << ", VisibleQuantity: " << bid.GetVisibleQuantity()
         << ", HiddenQuantity: " << bid.GetHiddenQuantity()
         << ", Ask Price: " << offer.GetPrice() << ", VisibleQuantity: " << offer.GetVisibleQuantity()
         << ", HiddenQuantity: " << offer.GetHiddenQuantity() << endl;
  }
};


/**
* Streaming Service Listener subscribing data from Algo Streaming Service to Streaming Service.
* Type T is the product type.
*/
template<typename T>
class StreamingServiceListener : public ServiceListener<AlgoStream<T>>
{

public:
  // ctor
  StreamingServiceListener(StreamingService<T>* _streamingService) : streamingService(_streamingService) {};

  // Listener callback to process an add event to the Service
  void ProcessAdd(AlgoStream<T>& _data) override {
    streamingService -> AddPriceStream(_data);
    PriceStream<T> priceStream = _data.GetPriceStream();
    // flow data to the service
    streamingService -> PublishPrice(priceStream);
  };

  // Listener callback to process a remove event to the Service
  void ProcessRemove(AlgoStream<T>& _data) override {};

  // Listener callback to process an update event to the Service
  void ProcessUpdate(AlgoStream<T>& _data) override {};

private:
  StreamingService<T>* streamingService;

};

#endif
